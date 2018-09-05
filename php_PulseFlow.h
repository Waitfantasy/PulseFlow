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
  | Author: Corerman                                                     |
  +----------------------------------------------------------------------+
*/

#include "common.h"

#ifndef PHP_PULSEFLOW_H
#define PHP_PULSEFLOW_H

extern zend_module_entry PulseFlow_module_entry;
#define phpext_PulseFlow_ptr &PulseFlow_module_entry

#define PHP_PULSEFLOW_VERSION "1.0.0"

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

typedef struct Function_Prof_Struct {

    char className[CLASS_NAME_MAX_SIZE];

    char functionName[FUNC_NAME_MAX_SIZE];

    unsigned int memoryUse;

    float cpuTimeUse;

    unsigned int refcount;

    unsigned long funcNameHash;

    unsigned long classNameHash;

} Function_Prof_Data;

typedef struct SVIPC_Func_Struct {

    long message_type;

    char opts[OPTS_STR_MAX_SIZE];

    int size;

    Function_Prof_Data Function_Prof_List[FUNCTION_PROF_LIST_SIZE];

} SVIPC_Func_Prof_Message;

ZEND_BEGIN_MODULE_GLOBALS(PulseFlow)

    zend_bool enabled;

    char *disable_trace_functions; // No tracking : func list
    char *disable_trace_class;  //No tracking : class name list

    char *svipc_name; //system V message queue file name
    long svipc_gj_id; //system V message project id

    long max_package_size; //message queue max size : one package max size

    int func_chunk_size; // if current func list size bigger than this , then send package

    char *log_dir;

    long sampling_rate; //采样率

    long request_sampling_rate; //每次请求的采样随机数

    int url_enable_flag; //url enable fiter switch

    int log_enable;

    unsigned long classDisableHashList[CLASS_DISABLED_HASH_LIST_SIZE]; //class name hash list
    unsigned long FuncDisableHashList[FUNC_DISABLED_HASH_LIST_SIZE];  // func name hash list

    unsigned int FuncDisableHashListSize;
    unsigned int classDisableHashListSize;

    int Function_Prof_List_current_Size;

    SVIPC_Func_Prof_Message Func_Prof_Data;

ZEND_END_MODULE_GLOBALS(PulseFlow)


#define PULSEFLOW_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(PulseFlow, v)

#if defined(ZTS) && defined(COMPILE_DL_PULSEFLOW)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif


#define PulseFLow_LOGO_URI "data:image/jpeg;base64,/9j/4AAQSkZJRgABAQAAAQABAAD/2wBDAA0JCgsKCA0LCgsODg0PEyAVExISEyccHhcgLikxMC4pLSwzOko+MzZGNywtQFdBRkxOUlNSMj5aYVpQYEpRUk//2wBDAQ4ODhMREyYVFSZPNS01T09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT0//wAARCABvAH8DASIAAhEBAxEB/8QAGwAAAQUBAQAAAAAAAAAAAAAABgACAwQFBwH/xAA5EAACAQMDAQUGBgEDBAMAAAABAgMABBEFEiExEyJBUXEGFCNhgZEyQqGxwdHhFVJiJDNy8IKD8f/EABoBAQACAwEAAAAAAAAAAAAAAAAEBQECAwb/xAAqEQACAgIBAgUDBQEAAAAAAAAAAQIDBBEhBRITMVFxkSIysUFhocHRgf/aAAwDAQACEQMRAD8AlmnhkicCQgyiMEbT3No5Pzp0s1u8swE2FlRRu2Hgrjj9K8mhhjhc9lkxCM7tx7+4civZIIY57hhEGWNEKoScc4+tAeSTwSreEybDMw2gqT0/upRewLKZNxPaupI2/gwpH161DPBCiXiqnehYbWyehPSnXEEMaviHJidF/EfiZGf/AHFAITwdituZSAip8TaeSGJIx18a9kvIXDSZYNtkUJjruPBzWzY+zHa3Esl0nZwZHZpnkjx9K29O07SrU7bRIWkXq24O338KAFbaznm92MEM8gSNo2YREAZzzk+tXItA1BpVzGFVIOyBLDvEg84z86J5b+zhOJLiMHy3ZNRDWLA5xP0/4N/Vc5W1x+6SX/TZQk/JA02hajHbcQb2WExbVYc97OetVLm1mtTH20U6qIDEzmJsA885oxGrWDNj3hR6ggfcinS6lZRY3XCHdyNve/ani1633LXuOyW9aAMT27SuTNtXsBCDtJzx1pJdwoivkswjWPZt8mznNGL2Gj6uGYRRs46sncYev+awtS9m2tLaR4U7dEXdvGd+c8gjyx5Vummtow1rhma1zBseESEiTtDv2nu7sY/bmne+27SKxZgInDDu/jAXH06VE0MImLCLurbdrsycE0pYIVe5QRcrEJFO490nHH61kwPS8hTbJyWIjUpt/DtPJzXi3Nuo7HtWK7W+JtPBLA9PpTktIHCxlcELE3aZOW3HkeVJbeBgJuwxhW+G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"
