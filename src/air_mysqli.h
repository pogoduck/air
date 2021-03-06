/*
  +----------------------------------------------------------------------+
  | air framework                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) wukezhan<wukezhan@gmail.com>                           |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: wukezhan<wukezhan@gmail.com>                                 |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_AIR_MYSQLI_H
#define PHP_AIR_MYSQLI_H

#include "ext/mysqlnd/mysqlnd.h"
#include "ext/mysqli/mysqli_mysqlnd.h"
#include "ext/mysqli/php_mysqli_structs.h"

#define MYSQLI_ASSOC 1
#define MYSQLI_FOUND_ROWS 2
#define MYSQLI_ASYNC 8

#define AIR_ADD 1
#define AIR_DEL 2
#define AIR_SET 3
#define AIR_GET 4

#define _MYSQLI_FETCH_RESOURCE_CONN(__ptr, __id, __check) do{ \
	MY_MYSQL *mysql; \
	MYSQLI_RESOURCE *my_res; \
	mysqli_object *intern = Z_MYSQLI_P(__id); \
	if (!(my_res = (MYSQLI_RESOURCE *)intern->ptr)) {\
  		php_error_docref(NULL, E_WARNING, "Couldn't fetch %s", ZSTR_VAL(intern->zo.ce->name));\
  	}else{\
		__ptr = (MY_MYSQL *)my_res->ptr; \
		if (__check && my_res->status < __check) { \
			php_error_docref(NULL, E_WARNING, "invalid object or resource %s\n", ZSTR_VAL(intern->zo.ce->name)); \
		}else{\
			if (!(__ptr)->mysql) { \
				mysqli_object *intern = Z_MYSQLI_P(__id); \
				php_error_docref(NULL, E_WARNING, "invalid object or resource %s\n", ZSTR_VAL(intern->zo.ce->name)); \
			}else{

#define AIR_MYSQL_BEGIN(mysql_link) _MYSQLI_FETCH_RESOURCE_CONN(mysql, mysql_link, MYSQLI_STATUS_VALID)

#define AIR_MYSQL_END() \
			} \
		} \
	} \
}while(0);

static inline ulong air_mysqli_get_id(zval *mysql_link){
	AIR_MYSQL_BEGIN(mysql_link){
		return (ulong)mysql_thread_id(mysql->mysql);
	}AIR_MYSQL_END();
}

static inline ulong air_mysqli_get_errno(zval *mysql_link){
	AIR_MYSQL_BEGIN(mysql_link){
		return (ulong)mysql_errno(mysql->mysql);
	}AIR_MYSQL_END();
}

static inline char *air_mysqli_get_error(zval *mysql_link){
	AIR_MYSQL_BEGIN(mysql_link){
		return (char *)mysql_error(mysql->mysql);
	}AIR_MYSQL_END();
}

static inline void air_mysqli_get_insert_id(zval *mysql_link, zval *retval){
	AIR_MYSQL_BEGIN(mysql_link){
		my_longlong id = mysql_insert_id(mysql->mysql);
		if (id < ZEND_LONG_MAX) {
			ZVAL_LONG(retval, (zend_long) id);
		} else {
			zend_string *num_str = strpprintf(0, MYSQLI_LLU_SPEC, id);
			ZVAL_STR(retval, num_str);
			zend_string_release(num_str);
		}
	}AIR_MYSQL_END();
}

static inline void air_mysqli_get_total_rows(zval *mysql_link, zval *retval){
	//refer to ext/mysqli/mysqli_api.c: 157
	AIR_MYSQL_BEGIN(mysql_link){
		my_longlong ar = mysql_affected_rows(mysql->mysql);
		if (ar == (my_longlong) -1) {
			ZVAL_LONG(retval, -1);
		}
		if (ar < ZEND_LONG_MAX) {
			ZVAL_LONG(retval, (zend_long) ar);
		} else {
			zend_string *num_str = strpprintf(0, MYSQLI_LLU_SPEC, ar);
			ZVAL_STR(retval, num_str);
			zend_string_release(num_str);
		}
	}AIR_MYSQL_END();
}

#endif

