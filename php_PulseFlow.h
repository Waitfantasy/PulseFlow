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

#ifndef PHP_PULSEFLOW_H
#define PHP_PULSEFLOW_H

extern zend_module_entry PulseFlow_module_entry;
#define phpext_PulseFlow_ptr &PulseFlow_module_entry

#define PHP_PULSEFLOW_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_PULSEFLOW_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_PULSEFLOW_API __attribute__ ((visibility("default")))
#else
#	define PHP_PULSEFLOW_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/*
  	Declare any global variables you may need between the BEGIN
	and END macros here:
*/


typedef struct Class_Trace_Struct Class_Trace_Data;
typedef struct Func_Trace_Struct Func_Trace_Data;

#define CLASS_TRACE_RESIZE_STEP 5

#define FUNC_TRACE_RESIZE_STEP 10

struct Class_Trace_Struct {
    int refCount;
    int funcCount; //当前使用的单元数
    int funcMemoryCount;  //为FuncList分配的总单元数
    Func_Trace_Data **FuncList;
    size_t memoryUse;
    float CpuTimeUse;
    char *className;
};

struct Func_Trace_Struct {
    struct timeval CpuTimeStart;
    float useCpuTime;

    size_t useMemoryStart;
    size_t useMemory;

    size_t useMemoryPeakStart;
    size_t useMemoryPeak;

    unsigned int classFuncId;

    int refCount;

    Class_Trace_Data *ClassAddr;

    char *funcName;
};

ZEND_BEGIN_MODULE_GLOBALS(PulseFlow)
    zend_bool enabled;
    zend_bool debug;

    char *disable_trace_functions;
    HashTable *disable_trace_functions_hash;

    char *disable_trace_class;
    HashTable *disable_trace_class_hash;

    Class_Trace_Data *Class_Trace_List;
    int Class_Trace_Current_Size;
    int Class_Trace_Total_Size;

    Func_Trace_Data *Func_Trace_List;
    int Func_Trace_Current_Size;
    int Func_Trace_Total_Size;
ZEND_END_MODULE_GLOBALS(PulseFlow)


/* Always refer to the globals in your function as PULSEFLOW_G(variable).
   You are encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/
#define PULSEFLOW_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(PulseFlow, v)

#if defined(ZTS) && defined(COMPILE_DL_PULSEFLOW)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif    /* PHP_PULSEFLOW_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

