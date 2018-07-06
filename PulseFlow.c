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
#include "common.c"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_PulseFlow.h"
#include "time.h"

struct timeval startTime;
static zend_op_array* (*old_compile_file)(zend_file_handle*, int TSRMLS_DC);

float timedifference_msec(struct timeval t0, struct timeval t1) {
	return (t1.tv_sec - t0.tv_sec) * 1000.0f
			+ (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

zend_op_array *
cgi_compile_file(zend_file_handle *h, int type TSRMLS_DC) {
	printf("%s\n", h->filename);
	//printf("%s\n",h->opened_path->val);

	zend_string *opened_path;
	FILE *fp = NULL;
	fp = zend_fopen(h->filename, &opened_path TSRMLS_CC);

	int fd;
	if (fp != NULL) {
		fd = fileno(fp);
	}

	struct stat stat_ssb;

	if (fstat(fd, &stat_ssb) == -1) {
		printf("%s no perssion\n", h->opened_path->val);
		goto DEFAULT;
	} else {
		int fileSize = stat_ssb.st_size;
//		printf("File size:%d\n", stat_ssb.st_size);
		char* filecontent = malloc(fileSize);
		int nread = read(fd, filecontent, fileSize);
		fclose(fp);
	}

	DEFAULT: return old_compile_file(h, type TSRMLS_CC);
}

void timeBegin() {
	gettimeofday(&startTime, 0);
}

double timeEnd(struct timeval startTime) {
	struct timeval end;
	gettimeofday(&end, 0);
	return timedifference_msec(startTime, end);
}

/* If you declare any globals in php_PulseFlow.h uncomment this:
 ZEND_DECLARE_MODULE_GLOBALS(PulseFlow)
 */

/* True global resources - no need for thread safety here */
static int le_PulseFlow;
/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
 PHP_INI_BEGIN()
 STD_PHP_INI_ENTRY("PulseFlow.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_PulseFlow_globals, PulseFlow_globals)
 STD_PHP_INI_ENTRY("PulseFlow.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_PulseFlow_globals, PulseFlow_globals)
 PHP_INI_END()
 */
/* }}} */

/* Remove the following function when you have successfully modified config.m4
 so that your module can be compiled into PHP, it exists only for testing
 purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_PulseFlow_compiled(string arg)
 Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_PulseFlow_compiled) {
	char *arg = NULL;
	size_t arg_len, len;
	zend_string *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len)
			== FAILURE) {
		return;
	}

	strg =
			strpprintf(0,
					"Congrsatulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.",
					"PulseFlow", arg);

	RETURN_STR(strg);
}

PHP_FUNCTION(libiaofunc2) {
	int c = add(100, 200);
	RETURN_LONG(c);

}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
 unfold functions in source code. See the corresponding marks just before
 function definition, where the functions purpose is also documented. Please
 follow this convention for the convenience of others editing your code.
 */

/* {{{ php_PulseFlow_init_globals
 */
/* Uncomment this function if you have INI entries
 static void php_PulseFlow_init_globals(zend_PulseFlow_globals *PulseFlow_globals)
 {
 PulseFlow_globals->global_value = 0;
 PulseFlow_globals->global_string = NULL;
 }
 */
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(PulseFlow) {
	/* If you have INI entries, uncomment these lines
	 REGISTER_INI_ENTRIES();
	 */
	old_compile_file = zend_compile_file;
	zend_compile_file = cgi_compile_file;
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(PulseFlow) {
	/* uncomment this line if you have INI entries
	 UNREGISTER_INI_ENTRIES();
	 */
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(PulseFlow) {
	timeBegin();
#if defined(COMPILE_DL_PULSEFLOW) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(PulseFlow) {
	double delay = timeEnd(startTime);
//	printf("cpu  : %lf millisecond\n", delay);
	php_printf("cpu:%f millisecond\n", delay);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(PulseFlow) {
	php_info_print_table_start();
	php_info_print_table_header(2, "PulseFlow support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	 DISPLAY_INI_ENTRIES();
	 */
}
/* }}} */

/* {{{ PulseFlow_functions[]
 *
 * Every user visible function must have an entry in PulseFlow_functions[].
 */
const zend_function_entry PulseFlow_functions[] = {
//PHP_FE(libiaofunc2, NULL)
//PHP_FE(confirm_PulseFlow_compiled,	NULL)		/* For testing, remove later. */
		PHP_FE_END /* Must be the last line in PulseFlow_functions[] */
		};
/* }}} */

/* {{{ PulseFlow_module_entry
 */
zend_module_entry PulseFlow_module_entry = {
STANDARD_MODULE_HEADER, "PulseFlow", PulseFlow_functions,
PHP_MINIT(PulseFlow),
PHP_MSHUTDOWN(PulseFlow),
PHP_RINIT(PulseFlow), /* Replace with NULL if there's nothing to do at request start */
PHP_RSHUTDOWN(PulseFlow), /* Replace with NULL if there's nothing to do at request end */
PHP_MINFO(PulseFlow),
PHP_PULSEFLOW_VERSION,
STANDARD_MODULE_PROPERTIES };
/* }}} */

#ifdef COMPILE_DL_PULSEFLOW
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(PulseFlow)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
