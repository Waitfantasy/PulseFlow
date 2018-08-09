#include "string_hash.h"
#include "sds.c"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <string.h>
#include <mqueue.h>

//#define SVIPC_PAK_MAX_SIZE 2000

struct message_text {
    int qid;
    key_t Shm_Trace_Id;
};

struct message {
    long message_type;
    struct message_text message_text;
};

ZEND_DECLARE_MODULE_GLOBALS(PulseFlow)

static zend_always_inline zend_string *tracing_get_class_name(zend_execute_data *data TSRMLS_DC) {

    if (!data) {

        return NULL;

    }


    if (data->func->common.scope != NULL) {

        return data->func->common.scope->name;

    }

    return NULL;
}


static zend_always_inline zend_string *tracing_get_function_name(zend_execute_data *data TSRMLS_DC) {

    if (!data) {

        return NULL;

    }

    if (!data->func->common.function_name) {

        return NULL;

    }

    return data->func->common.function_name;

}

static zend_always_inline float timedifference_msec(struct timeval *t0, struct timeval *t1 TSRMLS_DC) {

    return ((*t1).tv_sec - (*t0).tv_sec) * 1000.0f + ((*t1).tv_usec - (*t0).tv_usec) / 1000.0f;

}

static zend_always_inline void getlinuxTime(struct timeval *t TSRMLS_DC) {

    gettimeofday(t TSRMLS_CC, 0);

}

static zend_always_inline float getLinuxTimeUse(struct timeval *begin TSRMLS_DC) {

    struct timeval end;

    getlinuxTime(&end TSRMLS_CC);

    return timedifference_msec(begin, &end TSRMLS_CC);

}

static zend_always_inline void INIT_disable_trace_functions_hash(TSRMLS_D) {

    ALLOC_HASHTABLE(PULSEFLOW_G(disable_trace_functions_hash));

    zend_hash_init(PULSEFLOW_G(disable_trace_functions_hash), 0, NULL, NULL, 0);

    if (strlen(PULSEFLOW_G(disable_trace_functions))) {

        char *blockFunctionList = strtok(PULSEFLOW_G(disable_trace_functions), ",");

        while (blockFunctionList != NULL) {

            zval zv;
            ZVAL_TRUE(&zv);

            zend_string *hash_str = zend_string_init(blockFunctionList, strlen(blockFunctionList), 0);

            if (!zend_hash_exists(PULSEFLOW_G(disable_trace_functions_hash), hash_str)) {

                zend_hash_add(PULSEFLOW_G(disable_trace_functions_hash), hash_str,
                              &zv /*ZEND_FILE_LINE_CC */); //修改点1： ZEND_FILE_LINE_CC

            }

            zend_string_release(hash_str);

            blockFunctionList = strtok(NULL, ",");
        }

    }

}

static zend_always_inline void FREE_disable_trace_functions_hash(TSRMLS_D) {

    zend_hash_destroy(PULSEFLOW_G(disable_trace_functions_hash));

    FREE_HASHTABLE(PULSEFLOW_G(disable_trace_functions_hash));

}


static zend_always_inline void INIT_disable_trace_class_hash(TSRMLS_D) {

    ALLOC_HASHTABLE(PULSEFLOW_G(disable_trace_class_hash));

    zend_hash_init(PULSEFLOW_G(disable_trace_class_hash), 0, NULL, NULL, 0);

    if (strlen(PULSEFLOW_G(disable_trace_class))) {

        char *blockFunctionList = strtok(PULSEFLOW_G(disable_trace_class), ",");

        while (blockFunctionList != NULL) {

            zval zv;
            ZVAL_TRUE(&zv);

            zend_string *hash_str = zend_string_init(blockFunctionList, strlen(blockFunctionList), 0);

            if (!zend_hash_exists(PULSEFLOW_G(disable_trace_class_hash), hash_str)) {

                zend_hash_add(PULSEFLOW_G(disable_trace_class_hash), hash_str,
                              &zv /*ZEND_FILE_LINE_CC */); //修改点2：ZEND_FILE_LINE_CC

            }

            zend_string_release(hash_str);

            blockFunctionList = strtok(NULL, ",");

        }

    }

}


static zend_always_inline void FREE_disable_trace_class_hash(TSRMLS_D) {

    zend_hash_destroy(PULSEFLOW_G(disable_trace_class_hash));

    FREE_HASHTABLE(PULSEFLOW_G(disable_trace_class_hash));

}

static zend_always_inline void INIT_Class_Trace_Struct(TSRMLS_D) {

    //  PULSEFLOW_G(Class_Trace_List) = NULL;

    // PULSEFLOW_G(Class_Trace_Current_Size) = 0;

    // PULSEFLOW_G(Class_Trace_Total_Size) = 0;

    // PULSEFLOW_G(Func_Trace_List) = NULL;

    // PULSEFLOW_G(Func_Trace_Current_Size) = 0;

    // PULSEFLOW_G(Func_Trace_Total_Size) = 0;

    PULSEFLOW_G(Trace_Shm_Id) = 0; //初始化共享内存初始化 ID

}

static zend_always_inline key_t Create_New_Class_Shm_Id(zend_string *className TSRMLS_DC) {
    int Class_Shm_Id;

    int classNameLen = className->len;

    Class_Shm_Id = shmget(IPC_PRIVATE, sizeof(Class_Trace_Data), IPC_CREAT | 0777);

    if (Class_Shm_Id == -1)
        return -1;

    Class_Trace_Data *Class_Trace = (Class_Trace_Data *) shmat(Class_Shm_Id, NULL, 0);

    if (Class_Trace == (void *) -1)
        return -1;

    unsigned long classHash = BKDRHash(className->val, classNameLen);

    Class_Trace->classHash = classHash;

    Class_Trace->CpuTimeUse = 0;
    Class_Trace->funcCount = 0;
    Class_Trace->memoryUse = 0;
    Class_Trace->FuncList = 0;
    Class_Trace->refCount = 0;
    Class_Trace->nextClassShmId = 0; //初始下下一个元素的共享内存ID
    Class_Trace->classNameLen = 0;

    int classNameShmId = shmget(IPC_PRIVATE, sizeof(char) * classNameLen, IPC_CREAT | 0777);

    if (classNameShmId == -1) {

        shmdt((void *) Class_Trace);

        return -1;
    }

    char *classNametemp = (char *) shmat(classNameShmId, NULL, 0);

    if (classNametemp == (void *) -1) {

        shmdt((void *) Class_Trace);

        return -1;
    }

    strncpy(classNametemp, className->val, classNameLen); //使用strcpy安全函数

    classNametemp[classNameLen] = '\0';

    Class_Trace->className = classNameShmId; //成功分配并填充内容后，把名称ID进行赋值
    Class_Trace->classNameLen = classNameLen;


    shmdt((void *) classNametemp);

    shmdt((void *) Class_Trace);  //必须要移除引用，类似与初步的垃圾回收

    return Class_Shm_Id;
}

static zend_always_inline key_t Check_Class_Name_In_ShmId(key_t ClassShmId, zend_string *className TSRMLS_DC) {


    unsigned long classHash = BKDRHash(className->val, className->len);

    Class_Trace_Data *Class_Trace = (Class_Trace_Data *) shmat(ClassShmId, NULL, 0);

    if (Class_Trace == (void *) -1) {
        return -1;
    }

    if (Class_Trace->classHash == classHash) {
        //找到了 hash相同 则去除字符串并比较
        int classNameShmId = Class_Trace->className;

        char *classNametemp = (char *) shmat(classNameShmId, NULL, 0);

        if (classNametemp == (void *) -1) {
            shmdt((void *) Class_Trace);
            return -1;
        }


        if (strcmp(classNametemp, className->val) == 0) {

            shmdt((void *) classNametemp);
            shmdt((void *) Class_Trace);

            return 1;

        } else {

            key_t temp = Class_Trace->nextClassShmId;

            shmdt((void *) classNametemp);
            shmdt((void *) Class_Trace);
            return temp;

        }
    }

    key_t temp = Class_Trace->nextClassShmId;
    shmdt((void *) Class_Trace);

    return temp;

}

static zend_always_inline key_t Trace_Class_Shm_Id(zend_string *className TSRMLS_DC) {

    key_t Class_Shm_Id;

    Class_Shm_Id = PULSEFLOW_G(Trace_Shm_Id);  //获取全局存储类共享内存起始ID的内存

    Class_Trace_Data *Class_Trace;

    int classNameLen = className->len;

    if (Class_Shm_Id == 0) {


        Class_Shm_Id = Create_New_Class_Shm_Id(className TSRMLS_CC);


        if (Class_Shm_Id == -1) {
            return -1;
        }

        PULSEFLOW_G(Trace_Shm_Id) = Class_Shm_Id;

        return Class_Shm_Id;
    }

    int currentShmId;

    for (currentShmId = Class_Shm_Id; currentShmId;) {

        key_t ret = Check_Class_Name_In_ShmId(currentShmId, className);

        if (ret == 1) {
            //正确匹配
            return currentShmId;

        } else if (ret > 0) {
            //获取到下一个地址
            currentShmId = ret;

        } else if (ret == 0) {
            //代表遍历结束 为了保持currentShmId为上一个有效元素的ID,所以必须break
            break;

        } else if (ret == -1) {

            return -1;

        }
    }

    //没有找到 则创建一个新的类空间，并 放在队列上，进行拼接操作
    if (currentShmId) {

        Class_Trace_Data *Class_Trace = (Class_Trace_Data *) shmat(currentShmId, NULL, 0);

        if (Class_Trace == (void *) -1)
            return -1;

        if (Class_Trace->nextClassShmId == 0) {

            key_t Class_Shm_Id = Create_New_Class_Shm_Id(className TSRMLS_CC);

            if (Class_Shm_Id == -1) {
                shmdt((void *) Class_Trace);
                return -1;
            }


            Class_Trace->nextClassShmId = Class_Shm_Id;
            shmdt((void *) Class_Trace);
            return Class_Shm_Id;

        } else {

            shmdt((void *) Class_Trace);
            return -1;

        }

    }

}


static zend_always_inline key_t Create_New_Func_Shm_Id(key_t ClassShmId, zend_string *funcName TSRMLS_DC) {
    int Func_Shm_Id;

    int funcNameLen = funcName->len;

    Func_Shm_Id = shmget(IPC_PRIVATE, sizeof(Func_Trace_Data), IPC_CREAT | 0777);

    if (Func_Shm_Id == -1)
        return -1;

    Func_Trace_Data *Func_Trace = (Func_Trace_Data *) shmat(Func_Shm_Id, NULL, 0);

    if (Func_Trace == (void *) -1)
        return -1;

    unsigned long funcHash = BKDRHash(funcName->val, funcNameLen);


    Func_Trace->funcHash = funcHash;

    Func_Trace->useCpuTime = 0;
    Func_Trace->refCount = 0;
    Func_Trace->ClassAddr = ClassShmId;
    Func_Trace->useMemory = 0;
    Func_Trace->useMemoryPeak = 0;
    Func_Trace->nextFuncShmId = 0;
    Func_Trace->funcNameLen = 0;


    int funcNameShmId = shmget(IPC_PRIVATE, sizeof(char) * funcNameLen, IPC_CREAT | 0777);

    if (funcNameShmId == -1) {

        shmdt((void *) Func_Trace);

        return -1;
    }

    char *funcNametemp = (char *) shmat(funcNameShmId, NULL, 0);

    if (funcNametemp == (void *) -1) {

        shmdt((void *) Func_Trace);

        return -1;
    }

    strncpy(funcNametemp, funcName->val, funcNameLen); //使用strcpy安全函数

    funcNametemp[funcNameLen] = '\0';

    Func_Trace->funcName = funcNameShmId; //成功分配并填充内容后，把名称ID进行赋值
    Func_Trace->funcNameLen = funcNameLen;

    shmdt((void *) Func_Trace);  //必须要移除引用，类似与初步的垃圾回收

    shmdt((void *) funcNametemp);

    return Func_Shm_Id;
}

static zend_always_inline key_t
Check_Func_Name_In_ShmId(key_t ClassShmId, key_t FuncShmId, zend_string *funcName TSRMLS_DC) {
    unsigned long classHash = BKDRHash(funcName->val, funcName->len);

    Func_Trace_Data *Func_Trace = (Func_Trace_Data *) shmat(FuncShmId, NULL, 0);

    if (Func_Trace == (void *) -1) {
        return -1;
    }

    if (Func_Trace->funcHash == classHash && Func_Trace->ClassAddr == ClassShmId) {
        //找到了 hash相同 则去除字符串并比较
        int funcNameShmId = Func_Trace->funcName;

        char *funcNametemp = (char *) shmat(funcNameShmId, NULL, 0);

        if (funcNametemp == (void *) -1) {
            shmdt((void *) Func_Trace);
            return -1;
        }

        if (strcmp(funcNametemp, funcName->val) == 0) {

            shmdt((void *) Func_Trace);
            shmdt((void *) funcNametemp);

            return 1;

        } else {

            key_t temp = Func_Trace->nextFuncShmId;

            shmdt((void *) Func_Trace);
            shmdt((void *) funcNametemp);
            return temp;

        }
    } else {

        key_t temp = Func_Trace->nextFuncShmId;
        shmdt((void *) Func_Trace);

        return temp;
    }
}

static zend_always_inline key_t Trace_Func_Shm_Id(key_t Class_Shm_Id, zend_string *funcName TSRMLS_DC) {

    Class_Trace_Data *Class_Trace = (Class_Trace_Data *) shmat(Class_Shm_Id, NULL, 0);

    if (Class_Trace == (void *) -1)
        return -1;


    key_t FuncShmId = Class_Trace->FuncList;

    if (FuncShmId == 0) {
        //代表全新的class 没有函数列表
        key_t newFuncShmId = Create_New_Func_Shm_Id(Class_Shm_Id, funcName);

        if (newFuncShmId > 0) {
            //创建函数共享内存成功
            Class_Trace->FuncList = newFuncShmId;
            shmdt((void *) Class_Trace);
            return newFuncShmId;

        } else {

            shmdt((void *) Class_Trace);
            return -1;

        }

    }

    if (FuncShmId > 0) {
        //类中包含相应的函数共享内存ID，进行迭代查询
        int currentFuncShmId;

        //检测函数名是否已经存在
        for (currentFuncShmId = FuncShmId; currentFuncShmId;) {

            key_t ret = Check_Func_Name_In_ShmId(Class_Shm_Id, currentFuncShmId, funcName);

            if (ret == 1) {
                //正确匹配
                shmdt((void *) Class_Trace);
                return currentFuncShmId;

            } else if (ret > 0) {
                //获取到下一个地址
                currentFuncShmId = ret;

            } else if (ret == 0) {
                //代表遍历结束 为了保持currentShmId为上一个有效元素的ID,所以必须break
                break;

            } else if (ret == -1) {
                shmdt((void *) Class_Trace);
                return -1;

            }
        }

        //没有找到 则创建一个新的类空间，并 放在队列上，进行拼接操作
        if (currentFuncShmId) {

            Func_Trace_Data *Func_Trace = (Func_Trace_Data *) shmat(currentFuncShmId, NULL, 0);

            if (Func_Trace == (void *) -1){
                shmdt((void *) Class_Trace);
                return -1;
            }


            if (Func_Trace->nextFuncShmId == 0) {

                key_t Func_Shm_Id = Create_New_Func_Shm_Id(Class_Shm_Id, funcName TSRMLS_CC);

                if (Func_Shm_Id == -1) {
                    shmdt((void *) Func_Trace);
                    shmdt((void *) Class_Trace);
                    return -1;
                }


                Func_Trace->nextFuncShmId = Func_Shm_Id;
                shmdt((void *) Func_Trace);
                shmdt((void *) Class_Trace);
                return Func_Shm_Id;

            } else {

                shmdt((void *) Func_Trace);
                shmdt((void *) Class_Trace);
                return -1;

            }

        }

    }


}

//static zend_always_inline void Trace_Clean_Class_Struct(TSRMLS_D) {
//
//    int current_Count = PULSEFLOW_G(Class_Trace_Current_Size);
//
//    Class_Trace_Data *Class_Trace_List_Poniter = PULSEFLOW_G(Class_Trace_List);
//
//    if (Class_Trace_List_Poniter != NULL) {
//        int i;
//        for (i = 0; i < current_Count; ++i) {
//            //className Malloc free
//            if (Class_Trace_List_Poniter[i].className != NULL) {
//
//                free(Class_Trace_List_Poniter[i].className);
//                Class_Trace_List_Poniter[i].className = NULL;
//
//            }
//
//            //FuncList Malloc free
//            if (Class_Trace_List_Poniter[i].FuncList != NULL) {
//
//                free(Class_Trace_List_Poniter[i].FuncList);
//                Class_Trace_List_Poniter[i].FuncList = NULL;
//
//            }
//        }
//
//        free(PULSEFLOW_G(Class_Trace_List));
//        PULSEFLOW_G(Class_Trace_List) = NULL;
//
//        //这句代码默认是不需要的，为了避免zend引擎内部可能存在的未知错误 如果裸露运行Linux 内部 则不需要
//        PULSEFLOW_G(Class_Trace_Current_Size) = 0;
//        PULSEFLOW_G(Class_Trace_Total_Size) = 0;
//    }
//}


//static zend_always_inline Func_Trace_Data *
//Trace_Class_Function_Pointer(Class_Trace_Data *classPointer, zend_string *funcName TSRMLS_DC) {
//
//    Func_Trace_Data *retPoint = NULL;
//
//    int current_Count = PULSEFLOW_G(Func_Trace_Current_Size);
//    int total_Count = PULSEFLOW_G(Func_Trace_Total_Size);
//
//    Func_Trace_Data *Func_Trace_List_Poniter = PULSEFLOW_G(Func_Trace_List);
//
//    int funcNameLen = funcName->len;
//
//    unsigned long funcHash = BKDRHash(funcName->val, funcNameLen);
//
//    int i;
//    for (i = 0; i < current_Count; ++i) {
//        //从第一个FUNC全局数组开始找FUNC名相同的指针结构体元素
//
//        if ((funcHash == Func_Trace_List_Poniter[i].funcHash) &&
//            (strcmp(Func_Trace_List_Poniter[i].funcName, funcName->val) == 0) &&
//            (Func_Trace_List_Poniter[i].ClassAddr == classPointer)) {
//            //在全局函数指针列表中找到了这个函数，则进行指针赋值
//            retPoint = Func_Trace_List_Poniter + i;
//            break;
//        }
//
//    }
//
//    if (retPoint == NULL) {
//        //没有在全局函数列表中找到，则进行内存分配 并 进行全局 函数列表 类函数结构体中的函数指针列表赋值
//        if (current_Count == total_Count) {
//
//            total_Count += FUNC_TRACE_RESIZE_STEP;
//
//            Func_Trace_Data *New_Func_Trace_List = (Func_Trace_Data *) realloc(Func_Trace_List_Poniter,
//                                                                               sizeof(Func_Trace_Data) *
//                                                                               total_Count);
//            if (New_Func_Trace_List != NULL) {
//
//                PULSEFLOW_G(Func_Trace_List) = New_Func_Trace_List;
//                PULSEFLOW_G(Func_Trace_Total_Size) = total_Count;
//                Func_Trace_List_Poniter = New_Func_Trace_List;
//
//                // 修改点2  :如果函数内存被重新分配，则需要通知所有函数所属的类指针中的 函数指针数组进行更新数据 否则会造成指向错误
//                int i;
//                for (i = 0; i < current_Count; ++i) {
//                    int funcId = Func_Trace_List_Poniter[i].classFuncId;
//                    Func_Trace_List_Poniter[i].ClassAddr->FuncList[funcId] = Func_Trace_List_Poniter + i;
//                }
//
//            } else {
//
//                total_Count -= FUNC_TRACE_RESIZE_STEP;
//
//            }
//
//        }
//
//        if (current_Count < total_Count) {
//            //当前结构体内存大小足够使用，进行填充
//            // Func_Trace_List_Poniter[current_Count].t = {0,0};
//            Func_Trace_List_Poniter[current_Count].useCpuTime = 0;
//            Func_Trace_List_Poniter[current_Count].useMemory = 0;
//            Func_Trace_List_Poniter[current_Count].useMemoryPeak = 0;
//            Func_Trace_List_Poniter[current_Count].refCount = 0;
//            Func_Trace_List_Poniter[current_Count].ClassAddr = classPointer;
//            Func_Trace_List_Poniter[current_Count].classFuncId = classPointer->funcCount;
//
//            Func_Trace_List_Poniter[current_Count].funcName = malloc(sizeof(char) * (funcNameLen + 1));
//            strncpy(Func_Trace_List_Poniter[current_Count].funcName, funcName->val, funcNameLen); //使用strcpy安全函数
//            Func_Trace_List_Poniter[current_Count].funcName[funcNameLen] = '\0';
//            Func_Trace_List_Poniter[current_Count].funcHash = funcHash;
//
//            //至此 已经完成函数结构体内存全部分配 ，但是没有完全构造链接化
//
//            retPoint = Func_Trace_List_Poniter + current_Count;
//
//            //和类 struct进行结合结合
//            Func_Trace_Data **Class_Func_Trace_List = classPointer->FuncList;
//
//            int Class_Func_Current_Count = classPointer->funcCount;
//
//            int Class_Func_Total_Count = classPointer->funcMemoryCount;
//
//            if (Class_Func_Current_Count == Class_Func_Total_Count) {
//                //如果当前数组空间不够用 内存空间扩充
//
//                Class_Func_Total_Count += FUNC_TRACE_RESIZE_STEP;
//
//                Func_Trace_Data **New_Class_Func_Trace_List = (Func_Trace_Data **) realloc(Class_Func_Trace_List,
//                                                                                           sizeof(Func_Trace_Data *) *
//                                                                                           Class_Func_Total_Count);
//
//                if (New_Class_Func_Trace_List != NULL) {
//
//                    classPointer->FuncList = New_Class_Func_Trace_List;
//                    classPointer->funcMemoryCount = Class_Func_Total_Count;
//                    Class_Func_Trace_List = New_Class_Func_Trace_List;
//
//                } else {
//
//                    Class_Func_Total_Count -= FUNC_TRACE_RESIZE_STEP;
//
//                }
//
//            }
//
//            if (Class_Func_Current_Count < Class_Func_Total_Count) {
//                //空间足够 可以进行元素添加
//
//                Class_Func_Trace_List[Class_Func_Current_Count] = retPoint;
//
//                Class_Func_Current_Count++;
//
//                classPointer->funcCount = Class_Func_Current_Count;
//            }
//
//
//
//
//            //成功初始化一个类结构体元素 所以current_Count 自 加 1， 并对全局变量Class_Trace_Current_Size 进行重新赋值
//            current_Count++;
//            PULSEFLOW_G(Func_Trace_Current_Size) = current_Count;
//        }
//
//
//    }
//
//
//    return retPoint;
//}
//
//static zend_always_inline void Trace_Clean_Func_Struct(TSRMLS_D) {
//
//    int current_Count = PULSEFLOW_G(Func_Trace_Current_Size);
//
//    Func_Trace_Data *Func_Trace_List_Poniter = PULSEFLOW_G(Func_Trace_List);
//
//    if (Func_Trace_List_Poniter != NULL) {
//        int i;
//        for (i = 0; i < current_Count; ++i) {
//            //funcName Malloc free
//            Func_Trace_Data *loopPoint = Func_Trace_List_Poniter + i;
//
//            if (loopPoint == NULL)
//                continue;
//
//            if (loopPoint->funcName == NULL)
//                continue;
//
//            free(loopPoint->funcName);
//
//            loopPoint->funcName = NULL;
//
//        }
//
//        //这句代码默认是不需要的，为了避免zend引擎内部可能存在的未知错误 如果裸露运行Linux 内部 则不需要
//        PULSEFLOW_G(Func_Trace_Current_Size) = 0;
//        PULSEFLOW_G(Func_Trace_Total_Size) = 0;
//
//        free(PULSEFLOW_G(Func_Trace_List));
//        PULSEFLOW_G(Func_Trace_List) = NULL;
//    }
//}
//

static zend_always_inline int
Trace_Performance_Begin(key_t Class_Shm_Id, key_t func_shm_Id  TSRMLS_DC) {

    Class_Trace_Data *Class_Trace = (Class_Trace_Data *) shmat(Class_Shm_Id, NULL, 0);

    if (Class_Trace == (void *) -1) {
        return -1;
    }

    Func_Trace_Data *Func_Trace = (Func_Trace_Data *) shmat(func_shm_Id, NULL, 0);

    if (Func_Trace == (void *) -1 ) {
        shmdt((void *) Class_Trace);
        return -1;
    }

    getlinuxTime(&Func_Trace->CpuTimeStart TSRMLS_CC);

    Func_Trace->useMemoryStart = zend_memory_usage(0 TSRMLS_CC);

    //  funcPointer->useMemoryPeakStart = zend_memory_peak_usage(0 TSRMLS_CC);

    Func_Trace->refCount++;

    Class_Trace->refCount++;

    shmdt((void *) Func_Trace);
    shmdt((void *) Class_Trace);

    return 1;
}

static zend_always_inline int
Trace_Performance_End(key_t Class_Shm_Id, key_t func_shm_Id  TSRMLS_DC) {

    Class_Trace_Data *Class_Trace = (Class_Trace_Data *) shmat(Class_Shm_Id, NULL, 0);

    if (Class_Trace == (void *) -1) {
        return -1;
    }

    Func_Trace_Data *Func_Trace = (Func_Trace_Data *) shmat(func_shm_Id, NULL, 0);

    if (Func_Trace == (void *) -1 ) {
        shmdt((void *) Class_Trace);
        return -1;
    }


    Func_Trace->useCpuTime += (getLinuxTimeUse(&Func_Trace->CpuTimeStart TSRMLS_CC));

    Func_Trace->useMemory += (zend_memory_usage(0 TSRMLS_CC) - Func_Trace->useMemoryStart);

    //  funcPointer->useMemoryPeak += (zend_memory_peak_usage(0 TSRMLS_CC) - funcPointer->useMemoryPeakStart);

    Class_Trace->CpuTimeUse += Func_Trace->useCpuTime;

    Class_Trace->memoryUse += Func_Trace->useMemory;

    shmdt((void *) Func_Trace);
    shmdt((void *) Class_Trace);

    return 1;

}

//static zend_always_inline void PrintClassStruct(TSRMLS_D) {
//
//    int current_Count = PULSEFLOW_G(Class_Trace_Current_Size);
//
//    Class_Trace_Data *Class_Trace_List_Poniter = PULSEFLOW_G(Class_Trace_List);
//    int i1;
//    for (i1 = 0; i1 < current_Count; ++i1) {
//        if (Class_Trace_List_Poniter[i1].className != NULL) {
//            php_printf(
//                    "Class Name %s have %d functions &nbsp; CPU Time : %f ms &nbsp; Called Total %d Memory use: %d byte<br />\n",
//                    Class_Trace_List_Poniter[i1].className,
//                    Class_Trace_List_Poniter[i1].funcCount,
//                    Class_Trace_List_Poniter[i1].CpuTimeUse,
//                    Class_Trace_List_Poniter[i1].refCount,
//                    Class_Trace_List_Poniter[i1].memoryUse);
//
//            int i2;
//            int funclen = Class_Trace_List_Poniter[i1].funcCount;
//            for (i2 = 0; i2 < funclen; ++i2) {
//                if (Class_Trace_List_Poniter[i1].FuncList[i2] != NULL) {
//                    php_printf(
//                            "&nbsp; &nbsp; Function Name %s  &nbsp; Called Total :%d &nbsp; CPU Time : %f ms &nbsp; Memory : %d Byte &nbsp; Peak Memory: %d Byte <br />\n",
//                            Class_Trace_List_Poniter[i1].FuncList[i2]->funcName,
//                            Class_Trace_List_Poniter[i1].FuncList[i2]->refCount,
//                            Class_Trace_List_Poniter[i1].FuncList[i2]->useCpuTime,
//                            Class_Trace_List_Poniter[i1].FuncList[i2]->useMemory,
//                            Class_Trace_List_Poniter[i1].FuncList[i2]->useMemoryPeak
//                    );
//                    // printf("%x\n", Class_Trace_List_Poniter[i1].FuncList[i2]->classFuncId);
//                }
//
//            }
//
//        }
//    }
//}

//static zend_always_inline sds EncodeRetsultJson(sds dataPak TSRMLS_DC) {
//    int current_Count = PULSEFLOW_G(Class_Trace_Current_Size);
//    Class_Trace_Data *Class_Trace_List_Poniter = PULSEFLOW_G(Class_Trace_List);
//
//    dataPak = sdscat(dataPak, "[");
//
//    int i1;
//    for (i1 = 0; i1 < current_Count; ++i1) {
//        if (Class_Trace_List_Poniter[i1].className != NULL) {
//            dataPak = sdscat(dataPak, "{");
//            dataPak = sdscatprintf(dataPak, "\"cn\":\"%s\",\"fc\":%d,\"c\":%f,\"rc\":%d,\"m\":%ld,",
//                                   Class_Trace_List_Poniter[i1].className,
//                                   Class_Trace_List_Poniter[i1].funcCount,
//                                   Class_Trace_List_Poniter[i1].CpuTimeUse,
//                                   Class_Trace_List_Poniter[i1].refCount,
//                                   Class_Trace_List_Poniter[i1].memoryUse);
//            dataPak = sdscat(dataPak, "\"l\":[");  //function list begin
//
//            int i2;
//            int funclen = Class_Trace_List_Poniter[i1].funcCount;
//            for (i2 = 0; i2 < funclen; ++i2) {
//                if (Class_Trace_List_Poniter[i1].FuncList[i2] != NULL) {
//
//                    if (funclen - i2 == 1) {
//                        dataPak = sdscatprintf(dataPak, "{\"n\":\"%s\",\"rc\":%d,\"c\":%f,\"m\":%ld}", //,"pm":%ld
//                                               Class_Trace_List_Poniter[i1].FuncList[i2]->funcName,
//                                               Class_Trace_List_Poniter[i1].FuncList[i2]->refCount,
//                                               Class_Trace_List_Poniter[i1].FuncList[i2]->useCpuTime,
//                                               Class_Trace_List_Poniter[i1].FuncList[i2]->useMemory /*,
//                                        Class_Trace_List_Poniter[i1].FuncList[i2]->useMemoryPeak */);
//                    } else {
//                        dataPak = sdscatprintf(dataPak, "{\"n\":\"%s\",\"rc\":%d,\"c\":%f,\"m\":%ld},", //,"pm":%ld
//                                               Class_Trace_List_Poniter[i1].FuncList[i2]->funcName,
//                                               Class_Trace_List_Poniter[i1].FuncList[i2]->refCount,
//                                               Class_Trace_List_Poniter[i1].FuncList[i2]->useCpuTime,
//                                               Class_Trace_List_Poniter[i1].FuncList[i2]->useMemory /*,
//                                        Class_Trace_List_Poniter[i1].FuncList[i2]->useMemoryPeak */);
//                    }
//
//                }
//
//            }
//            dataPak = sdscat(dataPak, "]");  //function list end
//
//            if (current_Count - i1 == 1) {
//                dataPak = sdscat(dataPak, "}");
//            } else {
//                dataPak = sdscat(dataPak, "},");
//            }
//
//        }
//    }
//
//    dataPak = sdscat(dataPak, "]");
//
//    return dataPak;
//}

static zend_always_inline int SendDataToSVIPC(key_t Class_Shm_Id TSRMLS_DC) {

    key_t server_queue_key, server_qid;

    struct message my_message;

    int ret = 1;

    if ((server_queue_key = ftok(PULSEFLOW_G(svipc_name), PULSEFLOW_G(svipc_gj_id))) != -1) {

        if ((server_qid = msgget(server_queue_key, 0)) != -1) {

            my_message.message_type = 1;

            // strcpy(my_message.message_text.buf, dataPak);
            my_message.message_text.Shm_Trace_Id = Class_Shm_Id;
            msgsnd(server_qid, &my_message, sizeof(struct message_text), IPC_NOWAIT);

        } else {

            ret = 0;

        }

    } else {

        ret = 0;

    }

    return ret;
}

static zend_always_inline int SendDataToPosixIPC(key_t *Class_Shm_Id TSRMLS_DC) {

    int sendlen, ret = 1;

    mqd_t mqd = mq_open(PULSEFLOW_G(posix_name), O_WRONLY | O_NONBLOCK);

    if (mqd == -1) {

        ret = 0;

    }

    sendlen = mq_send(mqd, (char *) Class_Shm_Id, sizeof(key_t), 0);

    if (sendlen == -1) {
        ret = 0;

    }

    mq_close(mqd);

    return ret;
}


static zend_always_inline sds EncodeData(sds dataPak TSRMLS_DC) {

    if (strcmp(PULSEFLOW_G(encode_type), "json") == 0) {

//        return EncodeRetsultJson(dataPak TSRMLS_CC);
        return NULL;
    }
}


static zend_always_inline int SendData(key_t Class_Shm_Id TSRMLS_DC) {

    int ret = 0;

    if (Class_Shm_Id > 0) {

        if (strcmp(PULSEFLOW_G(send_type), "posix") == 0) {


            ret = SendDataToPosixIPC(&Class_Shm_Id TSRMLS_CC);

        }

        if (strcmp(PULSEFLOW_G(send_type), "svipc") == 0) {
            ret = SendDataToSVIPC(Class_Shm_Id TSRMLS_CC);

        }

    }

    return ret;

}

#define FTOK_FILE "/tmp/shm_debug.file"
#define PJ_ID 4

static zend_always_inline int debug_save_shm() {
    struct student {
        int age;
        int sex;
    };


    key_t fkey = ftok(FTOK_FILE, PJ_ID);
    int key = shmget(fkey, sizeof(struct student), IPC_CREAT | 0777);


    if (key == -1 && (errno != EEXIST)) {
        perror("shmget");
    } else {
        struct student *student1 = (struct student *) shmat(key, NULL, 0);

        if (student1 == (void *) -1) {
            perror("shmat:");
        } else {
            student1->age = 2000;
            student1->sex = 1;

            if (shmdt((void *) student1) == -1) {
                perror("shmat:");
            }

        }

    }
}