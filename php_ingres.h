/*
   +----------------------------------------------------------------------+
   | PECL :: ingres                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2007 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Contributed by ECL IP'S Software & Services                          |
   |                http://www.eclips-software.com                        |
   |                mailto:idev@eclips-software.com                       |
   |                Ingres Corporation, http://ingres.com                 |
   | Authors: David Hénot <henot@php.net>                                 |
   |          Grant Croker <grantc@php.net>                               |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifndef PHP_INGRES_H
#define PHP_INGRES_H

#if HAVE_INGRES
#include <iiapi.h>
extern zend_module_entry ingres_module_entry;
#define phpext_ingres_ptr &ingres_module_entry

#define PHP_INGRES_VERSION "2.2.2-dev"

#ifdef PHP_WIN32
#define PHP_INGRES_API __declspec(dllexport)
#else
#define PHP_INGRES_API
#endif

PHP_MINIT_FUNCTION(ingres);
PHP_MSHUTDOWN_FUNCTION(ingres);
PHP_RINIT_FUNCTION(ingres);
PHP_RSHUTDOWN_FUNCTION(ingres);
PHP_MINFO_FUNCTION(ingres);

#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_connect);
PHP_FUNCTION(ingres2_pconnect);
PHP_FUNCTION(ingres2_close);
PHP_FUNCTION(ingres2_query);
PHP_FUNCTION(ingres2_num_rows);
PHP_FUNCTION(ingres2_num_fields);
PHP_FUNCTION(ingres2_field_name);
PHP_FUNCTION(ingres2_field_type);
PHP_FUNCTION(ingres2_field_nullable);
PHP_FUNCTION(ingres2_field_length);
PHP_FUNCTION(ingres2_field_precision);
PHP_FUNCTION(ingres2_field_scale);
PHP_FUNCTION(ingres2_fetch_array);
PHP_FUNCTION(ingres2_fetch_row);
PHP_FUNCTION(ingres2_fetch_object);
PHP_FUNCTION(ingres2_rollback);
PHP_FUNCTION(ingres2_commit);
PHP_FUNCTION(ingres2_autocommit);
PHP_FUNCTION(ingres2_conn_errno);
PHP_FUNCTION(ingres2_conn_error);
PHP_FUNCTION(ingres2_conn_errsqlstate);
PHP_FUNCTION(ingres2_stmt_errno);
PHP_FUNCTION(ingres2_stmt_error);
PHP_FUNCTION(ingres2_stmt_errsqlstate);
PHP_FUNCTION(ingres2_error);
PHP_FUNCTION(ingres2_errno);
PHP_FUNCTION(ingres2_errsqlstate);
PHP_FUNCTION(ingres2_prepare);
PHP_FUNCTION(ingres2_execute);
PHP_FUNCTION(ingres2_cursor);
PHP_FUNCTION(ingres2_set_environment);
PHP_FUNCTION(ingres2_fetch_proc_return);
PHP_FUNCTION(ingres2_free_result);
PHP_FUNCTION(ingres2_autocommit_state);
PHP_FUNCTION(ingres2_next_error);
PHP_FUNCTION(ingres2_result_seek);
PHP_FUNCTION(ingres2_escape_string);
PHP_FUNCTION(ingres2_charset);
PHP_FUNCTION(ingres2_unbuffered_query);
#else
PHP_FUNCTION(ingres_connect);
PHP_FUNCTION(ingres_pconnect);
PHP_FUNCTION(ingres_close);
PHP_FUNCTION(ingres_query);
PHP_FUNCTION(ingres_num_rows);
PHP_FUNCTION(ingres_num_fields);
PHP_FUNCTION(ingres_field_name);
PHP_FUNCTION(ingres_field_type);
PHP_FUNCTION(ingres_field_nullable);
PHP_FUNCTION(ingres_field_length);
PHP_FUNCTION(ingres_field_precision);
PHP_FUNCTION(ingres_field_scale);
PHP_FUNCTION(ingres_fetch_array);
PHP_FUNCTION(ingres_fetch_row);
PHP_FUNCTION(ingres_fetch_object);
PHP_FUNCTION(ingres_rollback);
PHP_FUNCTION(ingres_commit);
PHP_FUNCTION(ingres_autocommit);
PHP_FUNCTION(ingres_conn_errno);
PHP_FUNCTION(ingres_conn_error);
PHP_FUNCTION(ingres_conn_errsqlstate);
PHP_FUNCTION(ingres_stmt_errno);
PHP_FUNCTION(ingres_stmt_error);
PHP_FUNCTION(ingres_stmt_errsqlstate);
PHP_FUNCTION(ingres_error);
PHP_FUNCTION(ingres_errno);
PHP_FUNCTION(ingres_errsqlstate);
PHP_FUNCTION(ingres_prepare);
PHP_FUNCTION(ingres_execute);
PHP_FUNCTION(ingres_cursor);
PHP_FUNCTION(ingres_set_environment);
PHP_FUNCTION(ingres_fetch_proc_return);
PHP_FUNCTION(ingres_free_result);
PHP_FUNCTION(ingres_autocommit_state);
PHP_FUNCTION(ingres_next_error);
PHP_FUNCTION(ingres_result_seek);
PHP_FUNCTION(ingres_escape_string);
PHP_FUNCTION(ingres_charset);
PHP_FUNCTION(ingres_unbuffered_query);
#endif

/* PHP INI modification handlers */
ZEND_INI_MH(php_ii_modify_array_index_start);
ZEND_INI_MH(php_ii_modify_fetch_buffer_size);

ZEND_BEGIN_MODULE_GLOBALS(ingres)
	long allow_persistent;
	long max_persistent;
	long max_links;
	char *default_database;
	char *default_user;
	char *default_password;

	long num_persistent;
	long num_links;
	long default_link;

	II_PTR *errorHandle;

    char *error_text;
    char error_sqlstate[ II_SQLSTATE_LEN + 1 ];

    long error_number;

	long cursor_no;
	int cursor_mode;

	II_PTR *envHandle; /* environment handle */

	long blob_segment_length; /* size of memory to read when putting/fetching */
								/* a blob */
	long trace_connect; /* enable / disable tracing of php_ii_do_connect */
	long array_index_start; /* start value for x in  array[x], default 1*/
    
#if defined (IIAPI_VERSION_3)
    short utf8;   /* convert unicode data to / from UTF-8 */
#endif
    short auto_multi;   /* Enable multiple cursors when auto commit is enabled */
    short reuse_connection;   /* should ingres_connect() reuse existing connections? */
    short ingres_trace; /* enable E_NOTICE tracing - not suitable for production usage */
    short scroll; /* enable/disable scrollable cursors */
    short describe; /* enable/disable describe input support */
    long fetch_buffer_size; /* number of rows to attempt in a fetch */

ZEND_END_MODULE_GLOBALS(ingres)

#define II_ASSOC (1<<0)
#define II_NUM   (1<<1)
#define II_BOTH  (II_ASSOC|II_NUM)

#define II_CURSOR_UPDATE 0    
#define II_CURSOR_READONLY 1 /* default */

#ifdef ZTS
#define INGRESG(v) TSRMG(ingres_globals_id, zend_ingres_globals *, v)
#define II_THREAD_ID (unsigned long)tsrm_thread_id()
#else
#define INGRESG(v) (ingres_globals.v)
#define II_THREAD_ID 0
#endif

#else

#define phpext_ingres_ptr NULL

#endif


#ifdef HAVE_INGRES2

#define INGRES_INI_ALLOW_PERSISTENT "ingres2.allow_persistent"
#define INGRES_INI_MAX_PERSISTENT "ingres2.max_persistent"
#define INGRES_INI_MAX_LINKS "ingres2.max_links"
#define INGRES_INI_DEFAULT_DATABASE "ingres2.default_database"
#define INGRES_INI_DEFAULT_USER "ingres2.default_user"
#define INGRES_INI_DEFAULT_PASSWORD "ingres2.default_password"
#define INGRES_INI_REPORT_DB_WARNINGS "ingres2.report_db_warnings"
#define INGRES_INI_CURSOR_MODE "ingres2.cursor_mode"
#define INGRES_INI_BLOB_SEGMENT_LENGTH "ingres2.blob_segment_length"
#define INGRES_INI_TRACE_CONNECT "ingres2.trace_connect"
#define INGRES_INI_TIMEOUT "ingres2.timeout"
#define INGRES_INI_ARRAY_INDEX_START "ingres2.array_index_start"
#define INGRES_INI_AUTO "ingres2.auto"
#define INGRES_INI_UTF8 "ingres2.utf8"
#define INGRES_INI_REUSE_CONNECTION "ingres2.reuse_connection"
#define INGRES_INI_TRACE "ingres2.trace"
#define INGRES_INI_SCROLL "ingres2.scrollable"
#define INGRES_INI_DESCRIBE "ingres2.describe"
#define INGRES_INI_FETCH_BUFFER_SIZE "ingres2.fetch_buffer_size"

#define INGRES_EXT_NAME "ingres2"

#else

#define INGRES_INI_ALLOW_PERSISTENT "ingres.allow_persistent"
#define INGRES_INI_MAX_PERSISTENT "ingres.max_persistent"
#define INGRES_INI_MAX_LINKS "ingres.max_links"
#define INGRES_INI_DEFAULT_DATABASE "ingres.default_database"
#define INGRES_INI_DEFAULT_USER "ingres.default_user"
#define INGRES_INI_DEFAULT_PASSWORD "ingres.default_password"
#define INGRES_INI_REPORT_DB_WARNINGS "ingres.report_db_warnings"
#define INGRES_INI_CURSOR_MODE "ingres.cursor_mode"
#define INGRES_INI_BLOB_SEGMENT_LENGTH "ingres.blob_segment_length"
#define INGRES_INI_TRACE_CONNECT "ingres.trace_connect"
#define INGRES_INI_TIMEOUT "ingres.timeout"
#define INGRES_INI_ARRAY_INDEX_START "ingres.array_index_start"
#define INGRES_INI_AUTO "ingres.auto"
#define INGRES_INI_UTF8 "ingres.utf8"
#define INGRES_INI_REUSE_CONNECTION "ingres.reuse_connection"
#define INGRES_INI_TRACE "ingres.trace"
#define INGRES_INI_SCROLL "ingres.scrollable"
#define INGRES_INI_DESCRIBE "ingres.describe"
#define INGRES_INI_FETCH_BUFFER_SIZE "ingres.fetch_buffer_size"

#define INGRES_EXT_NAME "ingres"

#endif /* HAVE_INGRES2 */


#endif	/* PHP_INGRES_H */

/*
 * Local variables:
 * tab-width: 4
 * shift-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker ff=unix expandtab
 * vim<600: sw=4 ts=4
 */
