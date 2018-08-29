
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <zend_exceptions.h>
#include "string_hash.h"


ZEND_DECLARE_MODULE_GLOBALS(PulseFlow)

// init class name hash lists
static zend_always_inline void Init_Class_Disable_Hash_List(TSRMLS_D) {

    unsigned long strhash = 0;
    int i = 0;

    if (strlen(PULSEFLOW_G(disable_trace_class))) {

        char* pSave = NULL;
        char *blockClass = strtok_r(PULSEFLOW_G(disable_trace_class), "," ,&pSave); // strtok_r thread safe

        while (blockClass != NULL) {

            strhash = BKDRHash(blockClass, strlen(blockClass));

            if (i < CLASS_DISABLED_HASH_LIST_SIZE) {

                PULSEFLOW_G(classDisableHashList)[i] = strhash;

            } else {

                break;

            }

            i++;

            blockClass = strtok_r(NULL, "," , &pSave);

        }
    }

    PULSEFLOW_G(classDisableHashListSize) = i;

}


static zend_always_inline void Init_Func_Disable_Hash_List(TSRMLS_D) {

    unsigned long strhash = 0;
    int i = 0;

    if (strlen(PULSEFLOW_G(disable_trace_functions))) {

        char* pSave = NULL;
        char *blockfunc = strtok_r(PULSEFLOW_G(disable_trace_functions), "," , &pSave);

        while (blockfunc != NULL) {

            strhash = BKDRHash(blockfunc, strlen(blockfunc));

            if (i < FUNC_DISABLED_HASH_LIST_SIZE) {  //此处需要修改为宏定义

                PULSEFLOW_G(FuncDisableHashList)[i] = strhash;

            } else {
                break;
            }

            i++;

            blockfunc = strtok_r(NULL, "," , &pSave);

        }
    }

    PULSEFLOW_G(FuncDisableHashListSize) = i;

}


static zend_always_inline int
Exist_In_Hash_List(unsigned long strhash, unsigned long *hashList, int hashListLen TSRMLS_DC) {  //1:存在 0：不存在

    int i = 0;

    for (i = 0; i < hashListLen; i++) {

        if (hashList[i] == strhash) {
            return 1;
        }

    }

    return 0;

}

static zend_always_inline void
Simple_Trace_Performance_Begin(struct timeval *CpuTimeStart, size_t *useMemoryStart, unsigned int funcArrayPointer
                               TSRMLS_DC) {

    gettimeofday(CpuTimeStart TSRMLS_CC, 0);

    *useMemoryStart = zend_memory_usage(0 TSRMLS_CC);

    PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcArrayPointer].refcount++;
}

static zend_always_inline void
Simple_Trace_Performance_End(struct timeval *CpuTimeStart, size_t *useMemoryStart, unsigned int funcArrayPointer
                             TSRMLS_DC) {

    struct timeval endTime;

    gettimeofday(&endTime, 0);

    PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcArrayPointer].cpuTimeUse +=
            ((endTime).tv_sec - (*CpuTimeStart).tv_sec) * 1000 + ((endTime).tv_usec - (*CpuTimeStart).tv_usec) / 1000;

    PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcArrayPointer].memoryUse += (zend_memory_usage(0 TSRMLS_CC) -
                                                                                   (*useMemoryStart));

}

static zend_always_inline int
getFuncArrayId(zend_string *funcName, zend_string *className, unsigned long funcNameHash, unsigned long classNameHash TSRMLS_DC) {

    int funcCurrentPointer = PULSEFLOW_G(Function_Prof_List_current_Size);

    int funcArrayId = -1;

    for (int i = 0; i < funcCurrentPointer; ++i) {

        if ((PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[i].funcNameHash == funcNameHash) &&
            (PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[i].classNameHash == classNameHash)) {

            funcArrayId = i;

            return funcArrayId;

        }

    }

    if ((funcArrayId == -1) && (funcCurrentPointer < FUNCTION_PROF_LIST_SIZE)) { //空间未满 未找到Hash所在位置

        PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcCurrentPointer].funcNameHash = funcNameHash;
        PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcCurrentPointer].classNameHash = classNameHash;

        strncpy(PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcCurrentPointer].functionName,
                funcName->val,
                FUNC_NAME_MAX_SIZE - 1);

        //add funcname zero flag
        int funNameLen = funcName->len;
        if (funNameLen >= FUNC_NAME_MAX_SIZE) {

            PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcCurrentPointer].functionName[FUNC_NAME_MAX_SIZE -
                                                                                            1] = '\0';

        } else {

            PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcCurrentPointer].functionName[funNameLen] = '\0';

        }

        strncpy(PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcCurrentPointer].className, className->val,
                CLASS_NAME_MAX_SIZE - 1);

        int classNameLen = className->len;
        if (classNameLen >= CLASS_NAME_MAX_SIZE) {

            PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcCurrentPointer].className[CLASS_NAME_MAX_SIZE -
                                                                                         1] = '\0';

        } else {

            PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcCurrentPointer].className[classNameLen] = '\0';

        }

        PULSEFLOW_G(Func_Prof_Data).Function_Prof_List[funcCurrentPointer].refcount = 0;
        funcArrayId = funcCurrentPointer;
        PULSEFLOW_G(Function_Prof_List_current_Size)++;

        return funcArrayId;
    }

    return funcArrayId;

}

static zend_always_inline int SendDataToSVIPC(TSRMLS_D) {

    key_t server_queue_key, server_qid;

    int ret = -1;

    if ((server_queue_key = ftok(PULSEFLOW_G(svipc_name), PULSEFLOW_G(svipc_gj_id))) != -1) {

        if ((server_qid = msgget(server_queue_key, 0)) != -1) {

            PULSEFLOW_G(Func_Prof_Data).message_type = 1;

            PULSEFLOW_G(Func_Prof_Data).size = PULSEFLOW_G(Function_Prof_List_current_Size);

            unsigned int msgsize = sizeof(PULSEFLOW_G(Func_Prof_Data).size) +
                                   sizeof(PULSEFLOW_G(Func_Prof_Data).opts) +
                    (sizeof(Function_Prof_Data) * PULSEFLOW_G(Function_Prof_List_current_Size));

            ret = msgsnd(server_qid, &PULSEFLOW_G(Func_Prof_Data), msgsize, IPC_NOWAIT);

            if (ret == -1) {

                ret = 0; //发送失败

            }
        }

    }

    return ret;

}

static zend_always_inline int FirewallCheck(TSRMLS_D) {

    int ret = -1;

    //根据URL 参数判断是否需要进行监控
    char *uri_query_string = SG(request_info).query_string;

    if (!((uri_query_string == NULL) || (uri_query_string != NULL && uri_query_string[0] == '\0'))) {

        if (strstr(uri_query_string, URI_REQUEST_ENABLED_PARM_ON)) {  //如果找到请求监控开启参数

            ret = 1;

        }

        if (strstr(uri_query_string, URI_REQUEST_ENABLED_PARM_OFF)) {  //如果找到请求监控开启参数

            ret = 0;

        }

    }

    return ret;
}


//check uri request has pulseflowswitch
//
//    zend_llist_position pos;
//    sapi_header_struct* h;
//    h = zend_llist_get_first_ex(&SG(sapi_headers).headers, &pos);
//    for (; h;h= (sapi_header_struct*)zend_llist_get_next_ex(&SG(sapi_headers).headers, &pos))
//    {
//              php_printf("SAPI! %d, %s <br/>", h->header_len, h->header);
//    }