ZEND_DECLARE_MODULE_GLOBALS(PulseFlow)

static zend_always_inline zend_string *tracing_get_class_name(zend_execute_data *data TSRMLS_DC) {
    zend_function *curr_func;

    if (!data) {
        return NULL;
    }

    curr_func = data->func;

    if (curr_func->common.scope != NULL) {
        zend_string_addref(curr_func->common.scope->name);

        return curr_func->common.scope->name;
    }

    return NULL;
}


static zend_always_inline zend_string *tracing_get_function_name(zend_execute_data *data TSRMLS_DC) {
    zend_function *curr_func;

    if (!data) {
        return NULL;
    }

    curr_func = data->func;

    if (!curr_func->common.function_name) {
        return NULL;
    }

    zend_string_addref(curr_func->common.function_name);

    return curr_func->common.function_name;
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

//static zend_always_inline int getlinuxMemory(TSRMLS_D) {
//
//    return zend_memory_usage(0 TSRMLS_CC);
//
//}
//
//static zend_always_inline int getLinuxMemoryUse(int beginMemory TSRMLS_DC) {
//
//    return zend_memory_usage(0 TSRMLS_CC) - beginMemory;
//
//}

static zend_always_inline void INIT_disable_trace_functions_hash(TSRMLS_D) {

    ALLOC_HASHTABLE(PULSEFLOW_G(disable_trace_functions_hash));
    zend_hash_init(PULSEFLOW_G(disable_trace_functions_hash), 0, NULL, NULL, 0);

    if (strlen(PULSEFLOW_G(disable_trace_functions))) {

        char *blockFunctionList = strtok(PULSEFLOW_G(disable_trace_functions), ",");

        while (blockFunctionList != NULL) {

            zval zv;
            ZVAL_BOOL(&zv, IS_TRUE);

            zend_string *hash_str = zend_string_init(blockFunctionList, strlen(blockFunctionList), 0);

            if (!zend_hash_exists(PULSEFLOW_G(disable_trace_functions_hash), hash_str)) {

                zend_hash_add(PULSEFLOW_G(disable_trace_functions_hash), hash_str, &zv
                        ZEND_FILE_LINE_CC);
            }

            blockFunctionList = strtok(NULL, ",");

        }

    }

}

static zend_always_inline void FREE_disable_trace_functions_hash(TSRMLS_D) {

    FREE_HASHTABLE(PULSEFLOW_G(disable_trace_functions_hash));

}


static zend_always_inline void INIT_disable_trace_class_hash(TSRMLS_D) {

    ALLOC_HASHTABLE(PULSEFLOW_G(disable_trace_class_hash));
    zend_hash_init(PULSEFLOW_G(disable_trace_class_hash), 0, NULL, NULL, 0);

    if (strlen(PULSEFLOW_G(disable_trace_class))) {

        char *blockFunctionList = strtok(PULSEFLOW_G(disable_trace_class), ",");

        while (blockFunctionList != NULL) {

            zval zv;
            ZVAL_BOOL(&zv, IS_TRUE);

            zend_string *hash_str = zend_string_init(blockFunctionList, strlen(blockFunctionList), 0);

            if (!zend_hash_exists(PULSEFLOW_G(disable_trace_class_hash), hash_str)) {

                zend_hash_add(PULSEFLOW_G(disable_trace_class_hash), hash_str, &zv
                        ZEND_FILE_LINE_CC);
            }

            blockFunctionList = strtok(NULL, ",");

        }

    }

}


static zend_always_inline void FREE_disable_trace_class_hash(TSRMLS_D) {

    FREE_HASHTABLE(PULSEFLOW_G(disable_trace_class_hash));

}

static zend_always_inline void INIT_Class_Trace_Struct(TSRMLS_D) {

    PULSEFLOW_G(Class_Trace_List) = NULL;

    PULSEFLOW_G(Class_Trace_Current_Size) = 0;

    PULSEFLOW_G(Class_Trace_Total_Size) = 0;

    PULSEFLOW_G(Func_Trace_List) = NULL;

    PULSEFLOW_G(Func_Trace_Current_Size) = 0;

    PULSEFLOW_G(Func_Trace_Total_Size) = 0;

}

static zend_always_inline Class_Trace_Data *Trace_Class_Pointer(zend_string *className TSRMLS_DC) {

    Class_Trace_Data *retPoint = NULL;

    int current_Count = PULSEFLOW_G(Class_Trace_Current_Size);
    int total_Count = PULSEFLOW_G(Class_Trace_Total_Size);

    Class_Trace_Data *Class_Trace_List_Poniter = PULSEFLOW_G(Class_Trace_List);

    int classNameLen = className->len;

    int i;
    for (i = 0; i < current_Count; ++i) {
        //从第一个CLASS全局数组开始找Class名相同的指针结构体元素

        if (strcmp(Class_Trace_List_Poniter[i].className, className->val) == 0) {
            retPoint = Class_Trace_List_Poniter + i;
            break;
        }

    }

    if (retPoint == NULL) {

        //没有找到对应名称的类结构体指针，进行current_class_count 与 Total_class_count 判断
        if (current_Count == total_Count) {
            //结构体数组空间已经不够使用 进行空间扩容
            total_Count += CLASS_TRACE_RESIZE_STEP;

            //严格注意内存重新分配可能造成的内存泄露 此处为了避免 relloc造成的内存泄露
            Class_Trace_Data *New_Class_Trace_List = (Class_Trace_Data *) realloc(Class_Trace_List_Poniter,
                                                                                  sizeof(Class_Trace_Data) *
                                                                                  total_Count);
            if (New_Class_Trace_List != NULL) {

                PULSEFLOW_G(Class_Trace_List) = New_Class_Trace_List;
                PULSEFLOW_G(Class_Trace_Total_Size) = total_Count;
                Class_Trace_List_Poniter = New_Class_Trace_List;

                // 重新分配内存后 循环更新 类 内部的所有函数的 指针体内的 classAddr  超级重点之一
                int i;
                for (i = 0; i < current_Count; ++i) {
                    int funcloop = Class_Trace_List_Poniter[i].funcCount;
                    int i2;
                    for (i2 = 0; i2 < funcloop; ++i2) {
                        Class_Trace_List_Poniter[i].FuncList[i2]->ClassAddr = Class_Trace_List_Poniter + i;
                    }
                }


            } else {

                total_Count -= CLASS_TRACE_RESIZE_STEP;

            }

        }

        if (current_Count < total_Count) {

            Class_Trace_List_Poniter[current_Count].refCount = 0;
            Class_Trace_List_Poniter[current_Count].funcCount = 0;
            Class_Trace_List_Poniter[current_Count].funcMemoryCount = 0;
            Class_Trace_List_Poniter[current_Count].memoryUse =0;
            Class_Trace_List_Poniter[current_Count].CpuTimeUse = 0;
            Class_Trace_List_Poniter[current_Count].FuncList = NULL;

            //当前结构体内存大小足够使用，进行填充
            // free(Class_Trace_List[current_Count].className);  //释放内存 避免内存区异常数据区 此处代码存在破坏内存区数据的风险
            Class_Trace_List_Poniter[current_Count].className = malloc(sizeof(char) * (classNameLen + 1));
            strncpy(Class_Trace_List_Poniter[current_Count].className, className->val, classNameLen); //使用strcpy安全函数
            Class_Trace_List_Poniter[current_Count].className[classNameLen] = '\0';

            retPoint = Class_Trace_List_Poniter + current_Count;

            //成功初始化一个类结构体元素 所以current_Count 自 加 1， 并对全局变量Class_Trace_Current_Size 进行重新赋值
            current_Count++;
            PULSEFLOW_G(Class_Trace_Current_Size) = current_Count;
        }

    }

    return retPoint;
}

static zend_always_inline Class_Trace_Data *Trace_Clean_Class_Struct(TSRMLS_D) {
    int current_Count = PULSEFLOW_G(Class_Trace_Current_Size);

    Class_Trace_Data *Class_Trace_List_Poniter = PULSEFLOW_G(Class_Trace_List);

    if (Class_Trace_List_Poniter != NULL) {
        int i;
        for (i = 0; i < current_Count; ++i) {
            //className Malloc free
            if (Class_Trace_List_Poniter[i].className != NULL) {
                free(Class_Trace_List_Poniter[i].className);
            }

            //FuncList Malloc free
            if (Class_Trace_List_Poniter[i].FuncList != NULL) {
                free(Class_Trace_List_Poniter[i].FuncList);
            }
        }

        free(Class_Trace_List_Poniter);

        //这句代码默认是不需要的，为了避免zend引擎内部可能存在的未知错误 如果裸露运行Linux 内部 则不需要
        PULSEFLOW_G(Class_Trace_Current_Size) = 0;
        PULSEFLOW_G(Class_Trace_Total_Size) = 0;
    }
}


static zend_always_inline Func_Trace_Data *
Trace_Class_Function_Pointer(Class_Trace_Data *classPointer, zend_string *funcName TSRMLS_DC) {

    Func_Trace_Data *retPoint = NULL;

    int current_Count = PULSEFLOW_G(Func_Trace_Current_Size);
    int total_Count = PULSEFLOW_G(Func_Trace_Total_Size);

    Func_Trace_Data *Func_Trace_List_Poniter = PULSEFLOW_G(Func_Trace_List);

    int funcNameLen = funcName->len;

    int i;
    for (i = 0; i < current_Count; ++i) {
        //从第一个FUNC全局数组开始找FUNC名相同的指针结构体元素

        if ((strcmp(Func_Trace_List_Poniter[i].funcName, funcName->val) == 0) &&
            (Func_Trace_List_Poniter[i].ClassAddr == classPointer)) {
            //在全局函数指针列表中找到了这个函数，则进行指针赋值
            retPoint = Func_Trace_List_Poniter + i;
            break;
        }

    }

    if (retPoint == NULL) {
        //没有在全局函数列表中找到，则进行内存分配 并 进行全局 函数列表 类函数结构体中的函数指针列表赋值
        if (current_Count == total_Count) {

            total_Count += FUNC_TRACE_RESIZE_STEP;

            Func_Trace_Data *New_Func_Trace_List = (Func_Trace_Data *) realloc(Func_Trace_List_Poniter,
                                                                               sizeof(Func_Trace_Data) *
                                                                               total_Count);
            if (New_Func_Trace_List != NULL) {

                PULSEFLOW_G(Func_Trace_List) = New_Func_Trace_List;
                PULSEFLOW_G(Func_Trace_Total_Size) = total_Count;
                Func_Trace_List_Poniter = New_Func_Trace_List;

                // 修改点2  :如果函数内存被重新分配，则需要通知所有函数所属的类指针中的 函数指针数组进行更新数据 否则会造成指向错误
                int i;
                for (i = 0; i < current_Count; ++i) {
                    int funcId = Func_Trace_List_Poniter[i].classFuncId;
                    Func_Trace_List_Poniter[i].ClassAddr->FuncList[funcId] = Func_Trace_List_Poniter + i;
                }

            } else {

                total_Count -= FUNC_TRACE_RESIZE_STEP;

            }

        }

        if (current_Count < total_Count) {
            //当前结构体内存大小足够使用，进行填充
            // Func_Trace_List_Poniter[current_Count].t = {0,0};
            Func_Trace_List_Poniter[current_Count].useCpuTime = 0;
            Func_Trace_List_Poniter[current_Count].useMemory = 0;
            Func_Trace_List_Poniter[current_Count].useMemoryPeak = 0;
            Func_Trace_List_Poniter[current_Count].refCount = 0;
            Func_Trace_List_Poniter[current_Count].ClassAddr = classPointer;
            Func_Trace_List_Poniter[current_Count].classFuncId = classPointer->funcCount;

            Func_Trace_List_Poniter[current_Count].funcName = malloc(sizeof(char) * (funcNameLen + 1));
            strncpy(Func_Trace_List_Poniter[current_Count].funcName, funcName->val, funcNameLen); //使用strcpy安全函数
            Func_Trace_List_Poniter[current_Count].funcName[funcNameLen] = '\0';

            //至此 已经完成函数结构体内存全部分配 ，但是没有完全构造链接化

            retPoint = Func_Trace_List_Poniter + current_Count;

            //和类 struct进行结合结合
            Func_Trace_Data **Class_Func_Trace_List = classPointer->FuncList;

            int Class_Func_Current_Count = classPointer->funcCount;

            int Class_Func_Total_Count = classPointer->funcMemoryCount;

            if (Class_Func_Current_Count == Class_Func_Total_Count) {
                //如果当前数组空间不够用 内存空间扩充

                Class_Func_Total_Count += FUNC_TRACE_RESIZE_STEP;

                Func_Trace_Data **New_Class_Func_Trace_List = (Func_Trace_Data **) realloc(Class_Func_Trace_List,
                                                                                           sizeof(Func_Trace_Data *) *
                                                                                           Class_Func_Total_Count);

                if (New_Class_Func_Trace_List != NULL) {

                    classPointer->FuncList = New_Class_Func_Trace_List;
                    classPointer->funcMemoryCount = Class_Func_Total_Count;
                    Class_Func_Trace_List = New_Class_Func_Trace_List;

                } else {

                    Class_Func_Total_Count -= FUNC_TRACE_RESIZE_STEP;

                }

            }

            if (Class_Func_Current_Count < Class_Func_Total_Count) {
                //空间足够 可以进行元素添加

                Class_Func_Trace_List[Class_Func_Current_Count] = retPoint;

                Class_Func_Current_Count++;

                classPointer->funcCount = Class_Func_Current_Count;
            }




            //成功初始化一个类结构体元素 所以current_Count 自 加 1， 并对全局变量Class_Trace_Current_Size 进行重新赋值
            current_Count++;
            PULSEFLOW_G(Func_Trace_Current_Size) = current_Count;
        }


    }


    return retPoint;
}

static zend_always_inline Class_Trace_Data *Trace_Clean_Func_Struct(TSRMLS_D) {
    int current_Count = PULSEFLOW_G(Func_Trace_Current_Size);

    Func_Trace_Data *Func_Trace_List_Poniter = PULSEFLOW_G(Func_Trace_List);

    if (Func_Trace_List_Poniter != NULL) {
        int i;
        for (i = 0; i < current_Count; ++i) {
            //funcName Malloc free
            if (Func_Trace_List_Poniter[i].funcName != NULL) {
                free(Func_Trace_List_Poniter[i].funcName);
            }

        }

        free(Func_Trace_List_Poniter);

        //这句代码默认是不需要的，为了避免zend引擎内部可能存在的未知错误 如果裸露运行Linux 内部 则不需要
        PULSEFLOW_G(Func_Trace_Current_Size) = 0;
        PULSEFLOW_G(Func_Trace_Total_Size) = 0;
    }
}

static zend_always_inline void
Trace_Performance_Begin(Class_Trace_Data *classPointer, Func_Trace_Data *funcPointer  TSRMLS_DC) {
    getlinuxTime(&funcPointer->CpuTimeStart TSRMLS_CC);
    funcPointer->useMemoryStart = zend_memory_usage(0 TSRMLS_CC);
    funcPointer->useMemoryPeakStart = zend_memory_peak_usage(0 TSRMLS_CC);
    funcPointer->refCount++;
    classPointer->refCount++;
}

static zend_always_inline void
Trace_Performance_End(Class_Trace_Data *classPointer, Func_Trace_Data *funcPointer  TSRMLS_DC) {
    funcPointer->useCpuTime += (getLinuxTimeUse(&funcPointer->CpuTimeStart TSRMLS_CC));
    funcPointer->useMemory += (zend_memory_usage(0 TSRMLS_CC) - funcPointer->useMemoryStart);
    funcPointer->useMemoryPeak += (zend_memory_peak_usage(0 TSRMLS_CC) - funcPointer->useMemoryPeakStart);

    classPointer->CpuTimeUse+= funcPointer->useCpuTime;
    classPointer->memoryUse += funcPointer->useMemory;
}

static zend_always_inline void PrintClassStruct(TSRMLS_D) {
    int current_Count = PULSEFLOW_G(Class_Trace_Current_Size);

    Class_Trace_Data *Class_Trace_List_Poniter = PULSEFLOW_G(Class_Trace_List);
    int i1;
    for (i1 = 0; i1 < current_Count; ++i1) {
        if (Class_Trace_List_Poniter[i1].className != NULL) {
            php_printf("Class Name %s have %d functions &nbsp; CPU Time : %f ms &nbsp; Called Total %d Memory use: %d byte<br />\n",
                       Class_Trace_List_Poniter[i1].className,
                       Class_Trace_List_Poniter[i1].funcCount,
                       Class_Trace_List_Poniter[i1].CpuTimeUse,
                       Class_Trace_List_Poniter[i1].refCount,
                       Class_Trace_List_Poniter[i1].memoryUse);

            int i2;
            int funclen = Class_Trace_List_Poniter[i1].funcCount;
            for (i2 = 0; i2 < funclen; ++i2) {
                if (Class_Trace_List_Poniter[i1].FuncList[i2] != NULL) {
                    php_printf(
                            "&nbsp; &nbsp; Function Name %s  &nbsp; Called Total :%d &nbsp; CPU Time : %f ms &nbsp; Memory : %d Byte &nbsp; Peak Memory: %d Byte <br />\n",
                            Class_Trace_List_Poniter[i1].FuncList[i2]->funcName,
                            Class_Trace_List_Poniter[i1].FuncList[i2]->refCount,
                            Class_Trace_List_Poniter[i1].FuncList[i2]->useCpuTime,
                            Class_Trace_List_Poniter[i1].FuncList[i2]->useMemory,
                            Class_Trace_List_Poniter[i1].FuncList[i2]->useMemoryPeak
                    );
                    // printf("%x\n", Class_Trace_List_Poniter[i1].FuncList[i2]->classFuncId);
                }

            }

        }
    }
}
