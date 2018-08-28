/*
 +----------------------------------------------------------------------+
 | PHP Version 7                                                        |
 +----------------------------------------------------------------------+
 | Copyright (c) 1997-2018 The PHP Group                                |
 +----------------------------------------------------------------------+
 | This source file is subject to version 3.01 of the PHP license,      |
 | that is bundled with this package in the file LICENSE, and is        |
 | available through the world-wide-web at the following url:           |
 | http://www.php.net/license/3_01.txt                                  |
 | If you did not receive a copy of the PHP license and are unable to   |
 | obtain it through the world-wide-web, please send a note to          |
 | license@php.net so we can mail you a copy immediately.               |
 +----------------------------------------------------------------------+
 | Author:                                                              |
 +----------------------------------------------------------------------+
 */
/* $Id$ */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zend_compile.h>
#include <SAPI.h>
#include "php.h"
#include "SAPI.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_PulseFlow.h"
#include "tracing.h"

ZEND_DECLARE_MODULE_GLOBALS(PulseFlow)

PHP_INI_BEGIN()
                STD_PHP_INI_ENTRY
                ("PulseFlow.enabled", "0", PHP_INI_ALL, OnUpdateBool, enabled,
                 zend_PulseFlow_globals, PulseFlow_globals)

                STD_PHP_INI_ENTRY
                ("PulseFlow.disable_trace_functions", "", PHP_INI_ALL, OnUpdateString, disable_trace_functions,
                 zend_PulseFlow_globals, PulseFlow_globals)

                STD_PHP_INI_ENTRY
                ("PulseFlow.disable_trace_class", "", PHP_INI_ALL, OnUpdateString, disable_trace_class,
                 zend_PulseFlow_globals, PulseFlow_globals)

                STD_PHP_INI_ENTRY
                ("PulseFlow.svipc_name", "/dev/shm/PulseFlow_sv_ipc", PHP_INI_ALL, OnUpdateString, svipc_name,
                 zend_PulseFlow_globals, PulseFlow_globals)

                STD_PHP_INI_ENTRY
                ("PulseFlow.svipc_pj_id", "1000", PHP_INI_ALL, OnUpdateLong, svipc_gj_id,
                 zend_PulseFlow_globals, PulseFlow_globals)

                STD_PHP_INI_ENTRY
                ("PulseFlow.max_package_size", "0", PHP_INI_ALL, OnUpdateLong, max_package_size,
                 zend_PulseFlow_globals, PulseFlow_globals)

//                STD_PHP_INI_ENTRY
//                ("PulseFlow.encode_type", "json", PHP_INI_ALL, OnUpdateString, encode_type,
//                 zend_PulseFlow_globals, PulseFlow_globals)

//                STD_PHP_INI_ENTRY
//                ("PulseFlow.send_type", "posix", PHP_INI_ALL, OnUpdateString, send_type,
//                 zend_PulseFlow_globals, PulseFlow_globals)

//                STD_PHP_INI_ENTRY
//                ("PulseFlow.posix_name", "/PulseFlow_posix_ipc", PHP_INI_ALL, OnUpdateString, posix_name,
//                 zend_PulseFlow_globals, PulseFlow_globals)

//                STD_PHP_INI_ENTRY
//                ("PulseFlow.debug", "0", PHP_INI_ALL, OnUpdateBool, debug,
//                 zend_PulseFlow_globals, PulseFlow_globals)

PHP_INI_END()

static void (*_zend_execute_ex)(zend_execute_data *execute_data);

//static void (*_zend_execute_internal)(zend_execute_data *execute_data, zval *return_value);

//ZEND_DLEXPORT void PulseFlow_xhprof_execute_internal(zend_execute_data *execute_data, zval *return_value);

ZEND_DLEXPORT void PulseFlow_xhprof_execute_ex(zend_execute_data *execute_data);

PHP_MINIT_FUNCTION (PulseFlow) {
    REGISTER_INI_ENTRIES();

    Init_Class_Disable_Hash_List();
    Init_Func_Disable_Hash_List();

    memset(&PULSEFLOW_G(Func_Prof_Data), 0, sizeof(SVIPC_Func_Prof_Message));

    //模拟分块发送开始：根据参数进行数据分块，因为修改消息队列大小 对于不同系统可能需要重启操作，所以可以进行模拟分块发送
    //如果消息队列分块大小值 大于 SVIPC_Func_Prof_Message 体积大小，则禁止分块，设置为0
    PULSEFLOW_G(max_package_size) =
            PULSEFLOW_G(max_package_size) > sizeof(SVIPC_Func_Prof_Message) ? 0 : PULSEFLOW_G(max_package_size);

    //获取还有多少空间可以分配函数
    int funcBlockSize = PULSEFLOW_G(max_package_size) -
                        (sizeof(PULSEFLOW_G(Func_Prof_Data)) - sizeof(PULSEFLOW_G(Func_Prof_Data).Function_Prof_List));

    if (funcBlockSize > 0) {

        //设置可用的函数分块大小
        PULSEFLOW_G(func_chunk_size) = funcBlockSize / sizeof(Function_Prof_Data);

    } else {

        PULSEFLOW_G(func_chunk_size) = 0; //没有可用空间进行函数分块 设置为0 禁止分块

    }

    //模拟分块发送结束

    _zend_execute_ex = zend_execute_ex;
    zend_execute_ex = PulseFlow_xhprof_execute_ex;

    return SUCCESS;
}

ZEND_DLEXPORT void PulseFlow_xhprof_execute_ex(zend_execute_data *execute_data) {
    if (!PULSEFLOW_G(enabled)) {

        _zend_execute_ex(execute_data);

    } else {

        unsigned long classNameHash = 0;
        unsigned long funcNameHash = 0;

        zend_string *className = NULL, *funcName = NULL;

        if (execute_data->func->common.scope != NULL) {
            className = execute_data->func->common.scope->name;
            classNameHash = BKDRHash(className->val, className->len);
        }

        if (execute_data->func->common.function_name) {
            funcName = execute_data->func->common.function_name;
            funcNameHash = BKDRHash(funcName->val, funcName->len);
        }

        if (funcName == NULL || className == NULL || classNameHash == 0 || funcNameHash == 0) {

            _zend_execute_ex(execute_data TSRMLS_CC);

        } else if (Exist_In_Hash_List(funcNameHash, PULSEFLOW_G(FuncDisableHashList),
                                      PULSEFLOW_G(FuncDisableHashListSize))) {

            _zend_execute_ex(execute_data TSRMLS_CC);

        } else if (Exist_In_Hash_List(classNameHash, PULSEFLOW_G(classDisableHashList),
                                      PULSEFLOW_G(classDisableHashListSize))) {

            _zend_execute_ex(execute_data TSRMLS_CC);

        } else {

            int currentFuncSize = PULSEFLOW_G(Function_Prof_List_current_Size);

            int func_chunk_size = PULSEFLOW_G(func_chunk_size);
            //如果模拟分块大小大于0，并且 当前函数总量大于分块大小
            if (func_chunk_size && (currentFuncSize >= func_chunk_size)) {

                SendDataToSVIPC(TSRMLS_C);
                //发送状态不进行监控 如果发送失败也丢弃
                PULSEFLOW_G(Function_Prof_List_current_Size) = 0;

            }

            int funcArrayPointer = getFuncArrayId(funcName, className, funcNameHash, classNameHash);
            if (funcArrayPointer != -1) {
                struct timeval CpuTimeStart;

                size_t useMemoryStart;

                Simple_Trace_Performance_Begin(&CpuTimeStart, &useMemoryStart, funcArrayPointer TSRMLS_CC);

                _zend_execute_ex(execute_data TSRMLS_CC);

                Simple_Trace_Performance_End(&CpuTimeStart, &useMemoryStart, funcArrayPointer TSRMLS_CC);

            } else {

                _zend_execute_ex(execute_data TSRMLS_CC);
            }

        }

    }
}


PHP_MSHUTDOWN_FUNCTION (PulseFlow) {

    return SUCCESS;

}


PHP_RINIT_FUNCTION (PulseFlow) {
#if defined(COMPILE_DL_PULSEFLOW) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif
    int ret = FirewallCheck(TSRMLS_C);
    if (ret == 1) {  //如果通过检测

        PULSEFLOW_G(enabled) = 1;

    } else if (ret == 0) {

        PULSEFLOW_G(enabled) = 0;

    }

    PULSEFLOW_G(Function_Prof_List_current_Size) = 0;

    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION (PulseFlow) {

    if (PULSEFLOW_G(Function_Prof_List_current_Size) > 0) {

        SendDataToSVIPC(TSRMLS_C);

    }
    return SUCCESS;

}

static zend_always_inline int PulseFlow_info_print(const char *str) {

    return php_output_write(str, strlen(str));

}


PHP_MINFO_FUNCTION (PulseFlow) {

    php_info_print_table_start();

    if (PULSEFLOW_G(enabled)) {

        php_info_print_table_header(2, "PulseFlow support", "enabled");

    } else {

        php_info_print_table_header(2, "PulseFlow support", "disabled");

    }

    php_info_print_table_end();

    php_info_print_box_start(0);

    if (!sapi_module.phpinfo_as_text) {

        PulseFlow_info_print("<a href=\"https://github.com/gitsrc/PulseFlow\"><img border=0 src=\"");

        PulseFlow_info_print(PulseFLow_LOGO_URI "\" alt=\"PulseFlow logo\" /></a>\n");

    }

    PulseFlow_info_print("PulseFlow is a PHP Profiler, Monitoring and Trace PHP .");

    PulseFlow_info_print(!sapi_module.phpinfo_as_text ? "<br /><br />" : "\n\n");

    PulseFlow_info_print(
            "The 'PulseFlow' extension optimized fork of the XHProf extension from tideways_xhprof and Facebook as open-source. <br /><br />&nbsp;&nbsp;(c) Tideways GmbH 2014-2017 <br /> &nbsp;&nbsp;(c) Facebook 2009");

    if (!sapi_module.phpinfo_as_text) {
        PulseFlow_info_print(
                "<br /><br /><strong>Source Code : https://github.com/gitsrc/PulseFlow  </strong>");

    }

    php_info_print_box_end();

}

PHP_FUNCTION (pulseflow_enable) {

    PULSEFLOW_G(enabled) = 1;

}

PHP_FUNCTION (pulseflow_disable) {

    PULSEFLOW_G(enabled) = 0;

}

PHP_FUNCTION (pulseflow_set_options) {

    zval *val;

    ZEND_PARSE_PARAMETERS_START(1, 1);  //解析参数
            Z_PARAM_ARRAY(val);
    ZEND_PARSE_PARAMETERS_END();

    zend_string *key;
    zval *key_val;
    HashTable *arr_hash;

    arr_hash = Z_ARRVAL_P(val); //把zval数据结构转换为 hashtable

    int currentPointer = 0;

    const int OPTS_STR_MEM_SIZE = sizeof(PULSEFLOW_G(Func_Prof_Data).opts); //获取opts实际内存长度

    char tempstr[OPTS_STR_MEM_SIZE];

    memset(PULSEFLOW_G(Func_Prof_Data).opts, 0, OPTS_STR_MEM_SIZE); //初始化opts内存 保障 每次使用函数内存干净

    ZEND_HASH_FOREACH_STR_KEY_VAL(arr_hash, key, key_val)
            {
                if (Z_TYPE_P(key_val) == IS_STRING && key && key_val) {

                    int len = snprintf(tempstr, OPTS_STR_MEM_SIZE, "%s=%s&", key->val, key_val->value.str->val);

                    if ((len < OPTS_STR_MEM_SIZE - 1) && (len + currentPointer < OPTS_STR_MEM_SIZE - 1)) {  //长度安全

                        if (strncpy(&PULSEFLOW_G(Func_Prof_Data).opts[currentPointer], tempstr, len)) {

                            currentPointer += len;

                        }

                    }
                }
            }
    ZEND_HASH_FOREACH_END();

//    if (currentPointer) {
//
//        PULSEFLOW_G(Func_Prof_Data).opts[currentPointer - 1] = '\0';
//
//    }

}

const zend_function_entry PulseFlow_functions[] = {

        PHP_FE(pulseflow_enable, NULL)

        PHP_FE(pulseflow_disable, NULL)

        PHP_FE(pulseflow_set_options, NULL)


        PHP_FE_END /* Must be the last line in PulseFlow_functions[] */

};

zend_module_entry PulseFlow_module_entry = {
        STANDARD_MODULE_HEADER, "PulseFlow", PulseFlow_functions,
        PHP_MINIT(PulseFlow),
        PHP_MSHUTDOWN(PulseFlow),
        PHP_RINIT(PulseFlow), /* Replace with NULL if there's nothing to do at request start */
        PHP_RSHUTDOWN(PulseFlow), /* Replace with NULL if there's nothing to do at request end */
        PHP_MINFO(PulseFlow),
        PHP_PULSEFLOW_VERSION,
        STANDARD_MODULE_PROPERTIES};

#ifdef COMPILE_DL_PULSEFLOW
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(PulseFlow)
#endif