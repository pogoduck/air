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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_hash.h"
#include "php_air.h"

#include "src/air_async_service.h"
#include "src/air_async_waiter.h"
#include "src/air_exception.h"
#include "src/air_mysqli.h"
#include "src/air_mysql.h"
#include "src/air_mysql_keeper.h"
#include "src/air_mysql_waiter.h"

zend_class_entry *air_mysql_waiter_ce;

PHP_METHOD(air_mysql_waiter, step_0) {
	AIR_INIT_THIS;
	zval *services = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("_services"), 0 TSRMLS_CC);
	zval *responses = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("_responses"), 0 TSRMLS_CC);
	zval *context = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("_context"), 0 TSRMLS_CC);
	zend_class_entry *mysqli_ce = air_get_ce(ZEND_STRL("mysqli") TSRMLS_CC);
	zval *async;
	MAKE_STD_ZVAL(async);
	ZVAL_LONG(async, MYSQLI_ASYNC);

	zval *wait_pool, *m2s;
	MAKE_STD_ZVAL(wait_pool);
	array_init(wait_pool);
	MAKE_STD_ZVAL(m2s);
	array_init(m2s);

	zval *service;
	ulong idx;
	uint _idx, key_len;
	char *key;
	zval *mysqli = NULL;
	HashTable *ah = Z_ARRVAL_P(services);
	for(zend_hash_internal_pointer_reset(ah);
			zend_hash_has_more_elements(ah) == SUCCESS;){
		zval **___tmp;
		if (zend_hash_get_current_data(ah, (void**)&___tmp) == FAILURE) {
			continue;
		}
		if(zend_hash_get_current_key_ex(ah, &key, &key_len, &idx, 0, NULL) != HASH_KEY_IS_STRING) {
			key = NULL; key_len = 0;
		}
		service = *___tmp;
		if(Z_TYPE_P(service) == IS_NULL){
			zend_hash_index_del(ah, idx);
			continue;
		}
		zval *service_id = zend_read_property(air_async_service_ce, service, ZEND_STRL("_id"), 1 TSRMLS_CC);
		zval *mysql = zend_read_property(air_async_service_ce, service, ZEND_STRL("_request"), 1 TSRMLS_CC);
		zval *status = zend_read_property(air_mysql_ce, mysql, ZEND_STRL("_status"), 0 TSRMLS_CC);
		if(Z_LVAL_P(status)){
			//ignore if the mysql's been executed
			zend_hash_index_del(ah, idx);
			continue;
		}
		zval *mysql_config = zend_read_property(air_mysql_ce, mysql, ZEND_STRL("_config"), 1 TSRMLS_CC);
		zval *mode = zend_read_property(air_mysql_ce, mysql, ZEND_STRL("_mode"), 1 TSRMLS_CC);
		if(Z_TYPE_P(mode) == IS_NULL){
			air_mysql_auto_mode(mysql);
			mode = zend_read_property(air_mysql_ce, mysql, ZEND_STRL("_mode"), 1 TSRMLS_CC);
		}
		zval **acquire_params[2] = {&mysql_config, &mode};
		mysqli = NULL;
		air_call_static_method(air_mysql_keeper_ce, "acquire", &mysqli, 2, acquire_params);
		if(Z_TYPE_P(mysqli) != IS_NULL){
			zval **build_params[1] = {&mysqli};
			zval *sql = NULL;
			air_call_method(&mysql, air_mysql_ce, NULL, ZEND_STRL("build"), &sql, 1, build_params);
			if(sql){
				zval **query_params[2] = {&sql, &async};
				air_call_method(&mysqli, mysqli_ce, NULL, ZEND_STRL("query"), NULL, 2, query_params);
				add_next_index_zval(wait_pool, mysqli);
				Z_ADDREF_P(service_id);
				add_index_zval(m2s, air_mysqli_get_id(mysqli), service_id);
				zval_ptr_dtor(&sql);
			}else{
				//should not happen
				php_error(E_ERROR, "sql not found");
				zval_ptr_dtor(&mysqli);
			}
		}else{
			zval_ptr_dtor(&mysqli);
		}
		zend_hash_move_forward(ah);
	}
	zval_ptr_dtor(&async);
	add_assoc_zval(context, "pool", wait_pool);
	add_assoc_zval(context, "m2s", m2s);
	add_assoc_long(context, "waited", zend_hash_num_elements(Z_ARRVAL_P(wait_pool)));
	add_assoc_long(context, "processed", 0);
	add_assoc_long(context, "step", 1);
}

PHP_METHOD(air_mysql_waiter, step_1) {
    AIR_INIT_THIS;
    zval *context = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("_context"), 0 TSRMLS_CC);
    zval *wait_pool = air_arr_find(context, ZEND_STRS("pool"));;

	zval *timeout;
	MAKE_STD_ZVAL(timeout);
	ZVAL_LONG(timeout, 0);
	zend_class_entry *mysqli_ce = air_get_ce(ZEND_STRL("mysqli") TSRMLS_CC);
	zval *mysqli;
	zval *errors, *reads, *rejects;
	MAKE_STD_ZVAL(errors);
	ZVAL_ZVAL(errors, wait_pool, 1, 0);
	MAKE_STD_ZVAL(reads);
	ZVAL_ZVAL(reads, wait_pool, 1, 0);
	MAKE_STD_ZVAL(rejects);
	ZVAL_ZVAL(rejects, wait_pool, 1, 0);
	zval **poll_params[4] = {&reads, &errors, &rejects, &timeout};
	zval *count;
	air_call_static_method(mysqli_ce, "poll", &count, 4, poll_params);
	if(Z_LVAL_P(count)){
		add_assoc_long(context, "step", 2);
	}
    add_assoc_zval(context, "reads", reads);
    zval_ptr_dtor(&errors);
    zval_ptr_dtor(&rejects);
    zval_ptr_dtor(&timeout);
    zval_ptr_dtor(&count);
}

PHP_METHOD(air_mysql_waiter, step_2) {
	AIR_INIT_THIS;
	zval *services = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("_services"), 0 TSRMLS_CC);
	zval *responses = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("_responses"), 0 TSRMLS_CC);
	zval *context = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("_context"), 0 TSRMLS_CC);
	zval *m2s = air_arr_find(context, ZEND_STRS("m2s"));
	zval *reads = air_arr_find(context, ZEND_STRS("reads"));
	zval *waited = air_arr_find(context, ZEND_STRS("waited"));
	zval *processed = air_arr_find(context, ZEND_STRS("processed"));
	zval *step = air_arr_find(context, ZEND_STRS("step"));
	zend_class_entry *mysqli_ce = air_get_ce(ZEND_STRL("mysqli") TSRMLS_CC);

	ulong idx;
	char *key;
	int key_len;
	zval *mysqli;
	AIR_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(reads), idx, key, key_len, mysqli){
		zval *service_id = air_arr_idx_find(m2s, air_mysqli_get_id(mysqli));
		zval *service = air_arr_idx_find(services, Z_LVAL_P(service_id));
		zval *mysql = zend_read_property(air_async_service_ce, service, ZEND_STRL("_request"), 0 TSRMLS_CC);
		zval **trigger_params[2];
		zval *event;
		MAKE_STD_ZVAL(event);
		zval *event_params;
		MAKE_STD_ZVAL(event_params);
		array_init(event_params);
		Z_ADDREF_P(mysqli);
		add_next_index_zval(event_params, mysqli);
		zval *mysqli_result = NULL;
		if(air_mysqli_get_errno(mysqli)){
			ZVAL_STRING(event, "error", 1);
		}else{
			ZVAL_STRING(event, "success", 1);
			air_call_method(&mysqli, mysqli_ce, NULL, ZEND_STRL("reap_async_query"), &mysqli_result, 0, NULL TSRMLS_CC);
			Z_ADDREF_P(mysqli_result);
			add_next_index_zval(event_params, mysqli_result);
		}
		trigger_params[0] = &event;
		trigger_params[1] = &event_params;
		zval *results = NULL;
		air_call_method(&mysql, air_mysql_ce, NULL, ZEND_STRL("trigger"), &results, 2, trigger_params TSRMLS_CC);
		if(results){
			add_index_zval(responses, Z_LVAL_P(service_id), results);
		}else{
			php_error(E_WARNING, "error on trigger event %s with no results", Z_STRVAL_P(event));
		}
		zend_hash_index_del(Z_ARRVAL_P(services), Z_LVAL_P(service_id));
		zval_ptr_dtor(&event);
		zval_ptr_dtor(&event_params);
		if(mysqli_result){
			zval_ptr_dtor(&mysqli_result);
		}
		zval *mysql_config = zend_read_property(air_mysql_ce, mysql, ZEND_STRL("_config"), 0 TSRMLS_CC);
		zval *mode = zend_read_property(air_mysql_ce, mysql, ZEND_STRL("_mode"), 0 TSRMLS_CC);
		zval **release_params[3] = {&mysqli, &mysql_config, &mode};
		air_call_static_method(air_mysql_keeper_ce, "release", NULL, 3, release_params);
	}AIR_HASH_FOREACH_END();
    Z_LVAL_P(processed) += zend_hash_num_elements(Z_ARRVAL_P(reads));
    if(Z_LVAL_P(processed) < Z_LVAL_P(waited)){
        Z_LVAL_P(step) = 1;
    }else if(zend_hash_num_elements(Z_ARRVAL_P(services))){
        Z_LVAL_P(step) = 0;
    }else{
        Z_LVAL_P(step) = -1;
    }
}
/* }}} */

/* {{{ air_mysql_waiter_methods */
zend_function_entry air_mysql_waiter_methods[] = {
	PHP_ME(air_mysql_waiter, step_0, NULL,  ZEND_ACC_PUBLIC)
	PHP_ME(air_mysql_waiter, step_1, NULL,  ZEND_ACC_PUBLIC)
	PHP_ME(air_mysql_waiter, step_2, NULL,  ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AIR_MINIT_FUNCTION */
AIR_MINIT_FUNCTION(air_mysql_waiter) {
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "air\\mysql\\waiter", air_mysql_waiter_methods);

	air_mysql_waiter_ce = zend_register_internal_class_ex(&ce, air_async_waiter_ce, NULL TSRMLS_CC);
	return SUCCESS;
}
/* }}} */

