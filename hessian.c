/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
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

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_hessian.h"
#define DEBUG

/* If you declare any globals in php_hessian.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(hessian)
*/

/* True global resources - no need for thread safety here */
static int le_hessian;
static zend_class_entry *mcphessian_service_ce_ptr;
static zend_class_entry *mcphessian_client_ce_ptr;
/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("hessian.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_hessian_globals, hessian_globals)
    STD_PHP_INI_ENTRY("hessian.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_hessian_globals, hessian_globals)
PHP_INI_END()
*/
/* }}} */

static zval *mcpack2array(zval *data) {
    zval *params[] = { data };
    zval *function_name, *retval_ptr;

    MAKE_STD_ZVAL(retval_ptr);
    MAKE_STD_ZVAL(function_name);
    ZVAL_STRING(function_name, "mc_pack_pack2array", 0);
    if (call_user_function(CG(function_table), NULL, function_name, 
        retval_ptr, 1, params TSRMLS_CC) == SUCCESS) {
    } else {
        php_error(E_WARNING, "call function mc_pack_pack2array fail.");
    }
    efree(function_name);//堆上分配的 需要及时释放
    return retval_ptr;
}

static zval *array2mcpack(zval *data) {
    if (Z_TYPE_P(data) != IS_ARRAY) {
        php_error(E_WARNING, "parameter type should be array.");
    }
    
    zval *mc_pack_v;
    MAKE_STD_ZVAL(mc_pack_v);
    ZVAL_STRING(mc_pack_v, "PHP_MC_PACK_V2", 0);
    zval *params[] = { data, mc_pack_v };
    zval *function_name, *retval_ptr;
    MAKE_STD_ZVAL(function_name);
    ZVAL_STRING(function_name, "mc_pack_array2pack", 0);
    MAKE_STD_ZVAL(retval_ptr);

    if (call_user_function(CG(function_table), NULL, function_name, 
        retval_ptr, 1, params TSRMLS_CC) == SUCCESS) {
    } else {
        php_error(E_WARNING, "call function mc_pack_array2pack fail.");
    }
    efree(function_name);
    return retval_ptr;
}

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_hessian_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_hessian_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "hessian", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_hessian_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_hessian_init_globals(zend_hessian_globals *hessian_globals)
{
	hessian_globals->global_value = 0;
	hessian_globals->global_string = NULL;
}
*/
/* }}} */


//Hessian client定义
/*
* {{{ constructor, create one new instance like this:
 * $client = new McpackHessianClient('http://xxxx', array('xxx' => 'xxx')),
 * method's declaration is McpackHessianClient($strUrl, $arrOptions).
 **/
PHP_METHOD(McpackHessianClient, __construct) {
    zend_class_entry *ce;
    zval *url, *options, *p_this;

    MAKE_STD_ZVAL(options);
    array_init(options);
    p_this = getThis();

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, NULL,
            "z|z", &url, &options) == FAILURE) {
        zend_error(E_WARNING, "parse parameters error.");
        return;
    }
    // set the properties of instance
    zend_update_property(mcphessian_client_ce_ptr, p_this, ZEND_STRL("url"), url TSRMLS_CC);
    zend_update_property(mcphessian_client_ce_ptr, p_this, ZEND_STRL("options"), options TSRMLS_CC);
}
/* }}} */

PHP_METHOD(McpackHessianClient, getUrl) {
    zval *url, *p_this;
    
    p_this = getThis();
    url = zend_read_property(mcphessian_client_ce_ptr, p_this, ZEND_STRL("url"), 1 TSRMLS_CC);
    RETURN_ZVAL(url, 1, 0);
}
/**
 * override __call()
 * it require two parameters, func_name and args
 **/
PHP_METHOD(McpackHessianClient, __call) {
    zend_class_entry *ce;
    zval *p_this, *args, *params, *result, *method, *tmp;
    char *func_name, *ret_str = NULL;
    int func_name_len = 0;
    size_t return_len = 0, max_len = 1024 * 1024 * 1024;

    p_this = getThis();
    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, NULL, 
        "sz",  &func_name, &func_name_len, &args) == FAILURE) {
        php_error(E_WARNING, "parse parameters error.");
        RETURN_NULL();
    }
    // init params
    MAKE_STD_ZVAL(params);
    array_init(params);
    add_assoc_string(params, "jsonrpc", "2.0", 0);
    add_assoc_string(params, "method", func_name, 0);
    add_assoc_zval(params, "params", args);
    add_assoc_string(params, "id", "123456", 0);
    #ifdef DEBUG
    zend_error(E_NOTICE, "In client before pack.");
    #endif
    zval *pack = array2mcpack(params);
    #ifdef DEBUG
    zend_error(E_NOTICE, "In client after pack.");
    #endif
    // post data
    zval *z_url = zend_read_property(mcphessian_client_ce_ptr, p_this, ZEND_STRL("url"), 1 TSRMLS_CC);
    convert_to_string(z_url);
    char *url = Z_STRVAL_P(z_url);
    php_stream_context *context = php_stream_context_alloc();
    MAKE_STD_ZVAL(method);
    ZVAL_STRING(method, "POST", 0);
    php_stream_context_set_option(context, "http", "method", method);
    php_stream_context_set_option(context, "http", "content", pack);

    // read data from stream
    php_stream *stream = php_stream_open_wrapper_ex(url, "rb", REPORT_ERRORS, NULL, context);
    if (stream) {
        ret_str = php_stream_get_line(stream, NULL, max_len, &return_len);
    } else {
        php_error(E_WARNING, "failed to open stream %s.", url);
        RETURN_NULL();
    }
    zend_error(E_NOTICE, "in client after call ...%s", ret_str);
    
    MAKE_STD_ZVAL(tmp);
    ZVAL_STRINGL(tmp, ret_str, return_len, 1);
    result = mcpack2array(tmp);
    php_stream_close(stream);
    efree(tmp);
    
    // get result from array
    zval **ret_val = NULL;
    if (zend_hash_exists(Z_ARRVAL_P(result), "result", 7)) {
        zend_hash_find(Z_ARRVAL_P(result), "result", 7, (void **)&ret_val);
        RETURN_ZVAL(*ret_val, 1, 0);
    } else {
        php_error(E_WARNING, "return value illegal.");
        RETURN_NULL();
    }
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_McpackHessianClient___call, 0, 0, 2)
    ZEND_ARG_INFO(0, func_name)
    ZEND_ARG_ARRAY_INFO(0, args, 0)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(arginfo_McpackHessianClient___construct, 0)
    ZEND_ARG_INFO(0, url)
    ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()
zend_function_entry mcphessian_client_methods[] = {
    PHP_ME(McpackHessianClient, __construct, arginfo_McpackHessianClient___construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(McpackHessianClient, getUrl, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(McpackHessianClient, __call, arginfo_McpackHessianClient___call, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

/**
 * 服务端逻辑定义
 **/
PHP_METHOD(McpackHessianService, __construct) 
{
    zval *obj_ptr = NULL;
    zval *this_obj_ptr = NULL;
    zend_class_entry *ce_ptr;
    //zend_parse_method_parameters 并非我们表面理解的意思 第一个参数必须是O 且最好当前类型 this_obj_ptr返回getThis指针
    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|o", &this_obj_ptr, mcphessian_service_ce_ptr, &obj_ptr)) {
        zend_error(E_WARNING, "parse parameters error.");
        return;
    }
    if (obj_ptr && IS_OBJECT == Z_TYPE_P(obj_ptr)) {
        zend_update_property(mcphessian_service_ce_ptr, this_obj_ptr, ZEND_STRL("service"), obj_ptr TSRMLS_CC);  
    }
    
}
PHP_METHOD(McpackHessianService, getService)
{
	zval *service = zend_read_property(Z_OBJCE_P(getThis()), getThis(), ZEND_STRL("service"), 1 TSRMLS_CC);
	if (!service) {
		RETURN_NULL();
	}
	RETURN_ZVAL(service, 1, 0);
}
PHP_METHOD(McpackHessianService, registerObject)
{
    zval * obj_ptr = NULL;
    //第二种参数解析方式
    if (FAILURE == zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, NULL, "o", &obj_ptr)) {
        zend_error(E_WARNING, "parse parameters error.");
        return;
    }
    zend_update_property(Z_OBJCE_P(getThis()), getThis(), ZEND_STRL("service"), obj_ptr TSRMLS_CC);
    RETURN_ZVAL(obj_ptr, 1, 0);
}
PHP_METHOD(McpackHessianService, service)
{
    //TODO 内存泄漏检查 varglid   
    zval *response = NULL;
    zval *result = NULL;
    zval *error = NULL;
    zval *tmp = NULL;
    zval *request = NULL;
    zval *service_ptr = NULL;
    
    zval *retval_ptr = NULL;
	zend_class_entry *service_ce_ptr = NULL;

	char *output = "php://output";
    char *input = "php://input";
    size_t max_len = 1024 * 1024 * 1024;

    char *str_request = NULL;
    size_t str_request_len = 0;

    char *method_name = NULL;
    zval **method = NULL;
    zval **params = NULL;
  
 	MAKE_STD_ZVAL(response);
    array_init(response);
    add_assoc_string(response, "jsonrpc", JSONRPC_VERSION, 1);
    add_assoc_string(response, "id", "123456", 1);

    do {
    	php_stream_context *context = php_stream_context_alloc();
	    php_stream *stream_in = php_stream_open_wrapper_ex(input, "rb", REPORT_ERRORS, NULL, context);
	    if (stream_in) {
	        str_request = php_stream_get_line(stream_in, NULL, max_len, &str_request_len);
	    } else {
	        php_error(E_WARNING, "failed to open stream %s.", input);
	        MAKE_STD_ZVAL(error);
	        array_init(error);//code  messsage data(可忽略)
	        add_assoc_long(error, "code", JSONRPC_INVALID_REQUEST);
	        add_assoc_string(error, "message", "Invalid Request.", 1);
	        add_assoc_zval(response, "error", error);
	        break;

	    }

	    // #ifdef DEBUG
	    // 	php_error(E_NOTICE, "In service before parse.")
	    // #endif
	    #ifdef DEBUG
	      php_error(E_NOTICE, "In service before parse.");
	    #endif
		MAKE_STD_ZVAL(tmp);
    	ZVAL_STRINGL(tmp, str_request, str_request_len, 0);
    	zval *request = mcpack2array(tmp);//string ---> array
    	efree(tmp);//及时释放资源
    	//efree(str_request);

    	if(!request) {
    		php_error(E_WARNING, "parse request fail.");
    		MAKE_STD_ZVAL(error);
	        array_init(error);//code  messsage data(可忽略)
	        add_assoc_long(error, "code", JSONRPC_PARSE_ERROR);
	        add_assoc_string(error, "message", "Parse Error.", 1);
	        add_assoc_zval(response, "error", error);
	        break;
    	}
    	#ifdef DEBUG
    		php_error(E_NOTICE, "In service after parse.");
    	#endif
    	if (IS_ARRAY != Z_TYPE_P(request)) {
    		//php_error(E_NOTICE, "the request Invalid.");
    		MAKE_STD_ZVAL(error);
	        array_init(error);//code  messsage data(可忽略)
	        add_assoc_long(error, "code", JSONRPC_INVALID_REQUEST);
	        add_assoc_string(error, "message", "Invalid Request.", 1);
	        add_assoc_zval(response, "error", error);
	        break;
    	}
	   //request: {"jsonrpc":"2.0". "method":"", "params":[], "id":123456}
		if (FAILURE == zend_hash_find(Z_ARRVAL_P(request), ZEND_STRS("method"), (void **)&method)) {
        	zend_error(E_WARNING, "request param error,the method not found in request.");
        	MAKE_STD_ZVAL(error);
	        array_init(error);//code  messsage data(可忽略)
	        add_assoc_long(error, "code", JSONRPC_INVALID_REQUEST);
	        add_assoc_string(error, "message", "Invalid Request.", 1);
	        add_assoc_zval(response, "error", error);
	        break;
    	}

		if (FAILURE == zend_hash_find(Z_ARRVAL_P(request), ZEND_STRS("params"), (void **)&params)
			|| IS_ARRAY != Z_TYPE_PP(params)) {
        	zend_error(E_WARNING, "request param error,the params not found in request.");
        	MAKE_STD_ZVAL(error);
	        array_init(error);//code  messsage data(可忽略)
	        add_assoc_long(error, "code", JSONRPC_INVALID_REQUEST);
	        add_assoc_string(error, "message", "Invalid Request.", 1);
	        add_assoc_zval(response, "error", error);
	        break;
   		}
 		if (FAILURE == zend_hash_exists(Z_ARRVAL_P(request), ZEND_STRS("jsonrpc"))) {
	        zend_error(E_WARNING, "request param error,the jsonrpc not found in request.");
	        MAKE_STD_ZVAL(error);
	        array_init(error);//code  messsage data(可忽略)
	        add_assoc_long(error, "code", JSONRPC_INVALID_REQUEST);
	        add_assoc_string(error, "message", "Invalid Request.", 1);
	        add_assoc_zval(response, "error", error);
	        break;
    	}
	    if (FAILURE == zend_hash_exists(Z_ARRVAL_P(request), ZEND_STRS("id"))) {
	        zend_error(E_WARNING, "request param error,the id not found in request.");
	        MAKE_STD_ZVAL(error);
	        array_init(error);//code  messsage data(可忽略)
	        add_assoc_long(error, "code", JSONRPC_INVALID_REQUEST);
	        add_assoc_string(error, "message", "Invalid Request.", 1);
	        add_assoc_zval(response, "error", error);
	        break;
	    }
	    #ifdef DEBUG
	    	php_error(E_NOTICE, "the params num:%d", zend_hash_num_elements(Z_ARRVAL_PP(params)));
	    #endif
		service_ptr = zend_read_property(Z_OBJCE_P(getThis()), getThis(), ZEND_STRL("service"), 1 TSRMLS_CC);
    	service_ce_ptr = Z_OBJCE_P(service_ptr);

    	//检查对象方法是否存在
	    if (FAILURE == zend_hash_exists(&(service_ce_ptr->function_table), Z_STRVAL_PP(method), Z_STRLEN_PP(method)+1)) {
	        zend_error(E_WARNING, "the service:%s not have method:%s", service_ce_ptr->name, Z_STRVAL_PP(method));
	        MAKE_STD_ZVAL(error);
	        array_init(error);//code  messsage data(可忽略)
	        add_assoc_long(error, "code", JSONRPC_METHOD_NOT_FOUND);
	        add_assoc_string(error, "message", "Method not found.", 1);
	        add_assoc_zval(response, "error", error);
	        break;
	    }

	    //查看php is_callable方法源码
	    if (!zend_is_callable_ex(*method, service_ptr, 0, &method_name, NULL, NULL, NULL TSRMLS_CC)) {
	        zend_error(E_WARNING, "the method %s is not callable.", method_name);
	        MAKE_STD_ZVAL(error);
	        array_init(error);//code  messsage data(可忽略)
	        add_assoc_long(error, "code", JSONRPC_METHOD_NOT_CALLABLE);
	        add_assoc_string(error, "message", "Method not callable.", 1);
	        add_assoc_zval(response, "error", error);
	        break;
	    }
	 
	    #ifdef DEBUG
	    	php_error(E_NOTICE, "In service before actual call.");
	    #endif
    	zend_fcall_info fci;
	    fci.size = sizeof(fci);
	    fci.function_table = &(Z_OBJCE_P(service_ptr)->function_table);
	    fci.object_ptr = service_ptr;
	    fci.function_name = *method;
	    fci.retval_ptr_ptr = &retval_ptr;


	    zend_fcall_info_args(&fci, *params TSRMLS_CC);
	    fci.no_separation = 0;  
	    fci.symbol_table = NULL;
	    #ifdef DEBUG
	    	php_error(E_NOTICE, "zend_fcall_info_args num:%u", fci.param_count);
	    #endif
	    //问题无法获取数组参数 ...?
	    if (FAILURE == zend_call_function(&fci, NULL TSRMLS_CC)) {
	        zend_error(E_WARNING, "call function fail.");
	        RETURN_FALSE;
	    }

	    zend_fcall_info_args_clear(&fci, 1);

		add_assoc_zval(response, "result", retval_ptr);
 		

    } while(0);

    zval *pack = array2mcpack(response);
    // //PHPWRITE(Z_STRVAL_P(pack), Z_STRLEN_P(pack));
    //输出结果
    php_stream *stream_out = php_stream_open_wrapper(output, "wb", REPORT_ERRORS, NULL);
    if (stream_out == NULL) {
        zend_error(E_ERROR, "open output stream failed.");
        RETURN_NULL();
    }
    php_stream_write(stream_out, Z_STRVAL_P(pack), Z_STRLEN_P(pack));
    RETURN_ZVAL(retval_ptr, 1, 0)

}

PHP_METHOD(McpackHessianService, __destruct)
{
}

/* {{{ php_hession_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_hession_init_globals(zend_hession_globals *hession_globals)
{
    hession_globals->global_value = 0;
    hession_globals->global_string = NULL;
}
*/
/* }}} */


ZEND_BEGIN_ARG_INFO(arginfo_McpackHessianService___construct, 0)
    ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(arginfo_McpackHessianService_service, 0)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(arginfo_McpackHessianService_getService, 0)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(arginfo_McpackHessianService_registerObject, 0)
    ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

zend_function_entry mcphessian_service_methods[] = {
    PHP_ME(McpackHessianService, __construct, arginfo_McpackHessianService___construct, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
    PHP_ME(McpackHessianService, __destruct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
    PHP_ME(McpackHessianService, service, arginfo_McpackHessianService_service, ZEND_ACC_PUBLIC)
    PHP_ME(McpackHessianService, getService, arginfo_McpackHessianService_getService, ZEND_ACC_PUBLIC)
    PHP_ME(McpackHessianService, registerObject, arginfo_McpackHessianService_registerObject, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};



/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(hessian)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	//注册服务类
	zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "McpackHessianService", mcphessian_service_methods);
    mcphessian_service_ce_ptr = zend_register_internal_class(&ce TSRMLS_CC);//线程安全的注册函数
	zend_declare_property_null(mcphessian_service_ce_ptr, ZEND_STRL("service"), ZEND_ACC_PROTECTED TSRMLS_CC);
	//zend_declare_property_null(mcphessian_service_ce_ptr, ZEND_STRL("info"), ZEND_ACC_PROTECTED TSRMLS_CC);//实际对象实例属性

	//class client
	//注册客户类
	zend_class_entry ce2;
	INIT_CLASS_ENTRY(ce2, "McpackHessianClient", mcphessian_client_methods);
	mcphessian_client_ce_ptr = zend_register_internal_class(&ce2);
	zend_declare_property_null(mcphessian_client_ce_ptr, ZEND_STRL("url"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(mcphessian_client_ce_ptr, ZEND_STRL("options"), ZEND_ACC_PROTECTED TSRMLS_CC);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(hessian)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(hessian)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(hessian)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(hessian)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "hessian support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ hessian_functions[]
 *
 * Every user visible function must have an entry in hessian_functions[].
 */
const zend_function_entry hessian_functions[] = {
	PHP_FE(confirm_hessian_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in hessian_functions[] */
};
/* }}} */

/* {{{ hessian_module_entry
 */
zend_module_entry hessian_module_entry = {
	STANDARD_MODULE_HEADER,
	"hessian",
	hessian_functions,
	PHP_MINIT(hessian),
	PHP_MSHUTDOWN(hessian),
	PHP_RINIT(hessian),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(hessian),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(hessian),
	PHP_HESSIAN_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_HESSIAN
ZEND_GET_MODULE(hessian)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
