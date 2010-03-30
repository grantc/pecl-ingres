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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_globals.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_ingres.h"
#include "ingres.h"
#include "ext/standard/php_string.h"
#if defined (IIAPI_VERSION_3)
#include "convertUTF.h"
#endif
#ifndef ZEND_MM_ALIGNMENT
#include "php_config.h"
#endif

#if HAVE_INGRES

ZEND_DECLARE_MODULE_GLOBALS(ingres)
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 1) || (PHP_MAJOR_VERSION > 5)
static PHP_GINIT_FUNCTION(ingres);
#endif

/* True globals, no need for thread safety */
static int le_ii_result;
static int le_ii_link, le_ii_plink;

#define SAFE_STRING(s) ((s)?(s):"")

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_connect, 0, 0, 1)
   ZEND_ARG_INFO(0, database)
   ZEND_ARG_INFO(0, username)
   ZEND_ARG_INFO(0, password)
   ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_pconnect, 0, 0, 1)
   ZEND_ARG_INFO(0, database)
   ZEND_ARG_INFO(0, username)
   ZEND_ARG_INFO(0, password)
   ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_close, 0, 0, 1)
   ZEND_ARG_INFO(0, link)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_query, 0, 0, 2)
   ZEND_ARG_INFO(0, link)
   ZEND_ARG_INFO(0, query)
   ZEND_ARG_ARRAY_INFO(0, params, 0)
   ZEND_ARG_INFO(0, types)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_unbuffered_query, 0, 0, 2)
   ZEND_ARG_INFO(0, link)
   ZEND_ARG_INFO(0, query)
   ZEND_ARG_ARRAY_INFO(0, params, 0)
   ZEND_ARG_INFO(0, types)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_prepare, 0, 0, 2)
   ZEND_ARG_INFO(0, link)
   ZEND_ARG_INFO(0, query)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_execute, 0, 0, 1)
   ZEND_ARG_INFO(0, result)
   ZEND_ARG_INFO(0, params)
   ZEND_ARG_INFO(0, types)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_cursor, 0, 0, 1)
   ZEND_ARG_INFO(0, result)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_num_rows, 0, 0, 1)
   ZEND_ARG_INFO(0, result)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_num_fields, 0, 0, 1)
   ZEND_ARG_INFO(0, result)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_field_name, 0, 0, 2)
   ZEND_ARG_INFO(0, result)
   ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_field_type, 0, 0, 2)
   ZEND_ARG_INFO(0, result)
   ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_field_nullable, 0, 0, 2)
   ZEND_ARG_INFO(0, result)
   ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_field_length, 0, 0, 2)
   ZEND_ARG_INFO(0, result)
   ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_field_precision, 0, 0, 2)
   ZEND_ARG_INFO(0, result)
   ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_field_scale, 0, 0, 2)
   ZEND_ARG_INFO(0, result)
   ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_free_result, 0, 0, 1)
   ZEND_ARG_INFO(0, result)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_result_seek, 0, 0, 2)
   ZEND_ARG_INFO(0, link)
   ZEND_ARG_INFO(0, position)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_fetch_array, 0, 0, 2)
   ZEND_ARG_INFO(0, result)
   ZEND_ARG_INFO(0, result_type)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_fetch_assoc, 0, 0, 1)
   ZEND_ARG_INFO(0, result)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_fetch_row, 0, 0, 1)
   ZEND_ARG_INFO(0, result)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_fetch_object, 0, 0, 2)
   ZEND_ARG_INFO(0, result)
   ZEND_ARG_INFO(0, result_type)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_fetch_proc_return, 0, 0, 1)
   ZEND_ARG_INFO(0, result)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_rollback, 0, 0, 1)
   ZEND_ARG_INFO(0, link)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_commit, 0, 0, 1)
   ZEND_ARG_INFO(0, link)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_autocommit, 0, 0, 1)
   ZEND_ARG_INFO(0, link)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_autocommit_state, 0, 0, 1)
   ZEND_ARG_INFO(0, link)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_next_error, 0, 0, 0)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_errno, 0, 0, 0)
   ZEND_ARG_INFO(0, link)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_error, 0, 0, 0)
   ZEND_ARG_INFO(0, link)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_errsqlstate, 0, 0, 0)
   ZEND_ARG_INFO(0, link)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_set_environment, 0, 0, 2)
   ZEND_ARG_INFO(0, link)
   ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_escape_string, 0, 0, 2)
   ZEND_ARG_INFO(0, link)
   ZEND_ARG_INFO(0, source_string)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_ingres_charset, 0, 0, 1)
   ZEND_ARG_INFO(0, link)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Ingres module function list
 * Every user visible function must have an entry in ingres_functions[].
*/
function_entry ingres_functions[] = {
#if defined(HAVE_INGRES2)
    PHP_FE(ingres2_connect,		        arginfo_ingres_connect)
    PHP_FE(ingres2_pconnect,		    arginfo_ingres_pconnect)
    PHP_FE(ingres2_close,		        arginfo_ingres_close)
    PHP_FE(ingres2_query,		        arginfo_ingres_query)
    PHP_FE(ingres2_num_rows,		    arginfo_ingres_num_rows)
    PHP_FE(ingres2_num_fields,		    arginfo_ingres_num_fields)
    PHP_FE(ingres2_field_name,		    arginfo_ingres_field_name)
    PHP_FE(ingres2_field_type,		    arginfo_ingres_field_type)
    PHP_FE(ingres2_field_nullable,		arginfo_ingres_field_nullable)
    PHP_FE(ingres2_field_length,		arginfo_ingres_field_length)
    PHP_FE(ingres2_field_precision,		arginfo_ingres_field_precision)
    PHP_FE(ingres2_field_scale,		    arginfo_ingres_field_scale)
    PHP_FE(ingres2_fetch_array,		    arginfo_ingres_fetch_array)
    PHP_FE(ingres2_fetch_assoc,		    arginfo_ingres_fetch_assoc)
    PHP_FE(ingres2_fetch_row,		    arginfo_ingres_fetch_row)
    PHP_FE(ingres2_fetch_object,		arginfo_ingres_fetch_object)
    PHP_FE(ingres2_rollback,		    arginfo_ingres_rollback)
    PHP_FE(ingres2_commit,		        arginfo_ingres_commit)
    PHP_FE(ingres2_autocommit,		    arginfo_ingres_autocommit)
    PHP_FE(ingres2_prepare,		        arginfo_ingres_prepare)
    PHP_FE(ingres2_execute,		        arginfo_ingres_execute)
    PHP_FE(ingres2_cursor,		        arginfo_ingres_cursor)
    PHP_FE(ingres2_set_environment,		arginfo_ingres_set_environment)
    PHP_FE(ingres2_fetch_proc_return,	arginfo_ingres_fetch_proc_return)
    PHP_FE(ingres2_free_result,		    arginfo_ingres_free_result)
    PHP_FE(ingres2_autocommit_state,	arginfo_ingres_autocommit_state)
    PHP_FE(ingres2_errno,		        arginfo_ingres_errno)
    PHP_FE(ingres2_error,		        arginfo_ingres_error)
    PHP_FE(ingres2_errsqlstate,		    arginfo_ingres_errsqlstate)
    PHP_FE(ingres2_next_error,		    arginfo_ingres_next_error)
    PHP_FE(ingres2_result_seek,		    arginfo_ingres_result_seek)
    PHP_FE(ingres2_escape_string,		arginfo_ingres_escape_string)
    PHP_FE(ingres2_charset,		        arginfo_ingres_charset)
    PHP_FE(ingres2_unbuffered_query,	arginfo_ingres_unbuffered_query)
    PHP_FALIAS(ingres2_conn_errno,       ingres2_errno, arginfo_ingres_errno)
    PHP_FALIAS(ingres2_stmt_errno,       ingres2_errno, arginfo_ingres_errno)
    PHP_FALIAS(ingres2_conn_error,       ingres2_error, arginfo_ingres_error)
    PHP_FALIAS(ingres2_stmt_error,       ingres2_error, arginfo_ingres_error)
    PHP_FALIAS(ingres2_stmt_errsqlstate,  ingres2_errsqlstate, arginfo_ingres_errsqlstate)
    PHP_FALIAS(ingres2_conn_errsqlstate,  ingres2_errsqlstate, arginfo_ingres_errsqlstate)
#else
    PHP_FE(ingres_connect,		        arginfo_ingres_connect)
    PHP_FE(ingres_pconnect,		        arginfo_ingres_pconnect)
    PHP_FE(ingres_close,		        arginfo_ingres_close)
    PHP_FE(ingres_query,		        arginfo_ingres_query)
    PHP_FE(ingres_num_rows,		        arginfo_ingres_num_rows)
    PHP_FE(ingres_num_fields,		    arginfo_ingres_num_fields)
    PHP_FE(ingres_field_name,		    arginfo_ingres_field_name)
    PHP_FE(ingres_field_type,		    arginfo_ingres_field_type)
    PHP_FE(ingres_field_nullable,		arginfo_ingres_field_nullable)
    PHP_FE(ingres_field_length,		    arginfo_ingres_field_length)
    PHP_FE(ingres_field_precision,		arginfo_ingres_field_precision)
    PHP_FE(ingres_field_scale,		    arginfo_ingres_field_scale)
    PHP_FE(ingres_fetch_array,		    arginfo_ingres_fetch_array)
    PHP_FE(ingres_fetch_assoc,		    arginfo_ingres_fetch_assoc)
    PHP_FE(ingres_fetch_row,		    arginfo_ingres_fetch_row)
    PHP_FE(ingres_fetch_object,		    arginfo_ingres_fetch_object)
    PHP_FE(ingres_rollback,		        arginfo_ingres_rollback)
    PHP_FE(ingres_commit,		        arginfo_ingres_commit)
    PHP_FE(ingres_autocommit,		    arginfo_ingres_autocommit)
    PHP_FE(ingres_prepare,		        arginfo_ingres_prepare)
    PHP_FE(ingres_execute,		        arginfo_ingres_execute)
    PHP_FE(ingres_cursor,		        arginfo_ingres_cursor)
    PHP_FE(ingres_set_environment,		arginfo_ingres_set_environment)
    PHP_FE(ingres_fetch_proc_return,	arginfo_ingres_fetch_proc_return)
    PHP_FE(ingres_free_result,		    arginfo_ingres_free_result)
    PHP_FE(ingres_autocommit_state,		arginfo_ingres_autocommit_state)
    PHP_FE(ingres_errno,		        arginfo_ingres_errno)
    PHP_FE(ingres_error,		        arginfo_ingres_error)
    PHP_FE(ingres_errsqlstate,		    arginfo_ingres_errsqlstate)
    PHP_FE(ingres_next_error,		    arginfo_ingres_next_error)
    PHP_FE(ingres_result_seek,		    arginfo_ingres_result_seek)
    PHP_FE(ingres_escape_string,		arginfo_ingres_escape_string)
    PHP_FE(ingres_charset,		        arginfo_ingres_charset)
    PHP_FE(ingres_unbuffered_query,		arginfo_ingres_unbuffered_query)
    PHP_FALIAS(ingres_conn_errno,       ingres_errno, arginfo_ingres_errno)
    PHP_FALIAS(ingres_stmt_errno,       ingres_errno, arginfo_ingres_errno)
    PHP_FALIAS(ingres_conn_error,       ingres_error, arginfo_ingres_error)
    PHP_FALIAS(ingres_stmt_error,       ingres_error, arginfo_ingres_error)
    PHP_FALIAS(ingres_stmt_errsqlstate,  ingres_errsqlstate, arginfo_ingres_errsqlstate)
    PHP_FALIAS(ingres_conn_errsqlstate,  ingres_errsqlstate, arginfo_ingres_errsqlstate)

#endif

    {NULL, NULL, NULL}    /* Must be the last line in ingres_functions[] */
};
/* }}} */

zend_module_entry ingres_module_entry = {
    STANDARD_MODULE_HEADER,
#ifdef HAVE_INGRES2
    "ingres2",
#else
    "ingres",
#endif
    ingres_functions,
    PHP_MINIT(ingres),
    PHP_MSHUTDOWN(ingres),
    PHP_RINIT(ingres),
    PHP_RSHUTDOWN(ingres),
    PHP_MINFO(ingres),
    PHP_INGRES_VERSION,
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 1) || (PHP_MAJOR_VERSION > 5)
    PHP_MODULE_GLOBALS(ingres),
    PHP_GINIT(ingres),
    NULL,
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
#else
    STANDARD_MODULE_PROPERTIES
#endif
};

#ifdef HAVE_INGRES2

#ifdef COMPILE_DL_INGRES2
ZEND_GET_MODULE(ingres)
#endif

#else

#ifdef COMPILE_DL_INGRES
ZEND_GET_MODULE(ingres)
#endif

#endif



#ifndef ZEND_ENGINE_2
#    define OnUpdateLong OnUpdateInt
#endif

/* {{{ php.ini entries */
PHP_INI_BEGIN()
    STD_PHP_INI_BOOLEAN(INGRES_INI_ALLOW_PERSISTENT, "1", PHP_INI_SYSTEM,    OnUpdateLong, allow_persistent, zend_ingres_globals, ingres_globals)
    STD_PHP_INI_ENTRY_EX(INGRES_INI_MAX_PERSISTENT, "-1", PHP_INI_SYSTEM, OnUpdateLong, max_persistent, zend_ingres_globals, ingres_globals, display_link_numbers)
    STD_PHP_INI_ENTRY_EX(INGRES_INI_MAX_LINKS, "-1", PHP_INI_SYSTEM, OnUpdateLong, max_links, zend_ingres_globals, ingres_globals, display_link_numbers)
    STD_PHP_INI_ENTRY(INGRES_INI_DEFAULT_DATABASE, NULL, PHP_INI_ALL, OnUpdateString, default_database, zend_ingres_globals, ingres_globals)
    STD_PHP_INI_ENTRY(INGRES_INI_DEFAULT_USER, NULL, PHP_INI_ALL, OnUpdateString, default_user, zend_ingres_globals, ingres_globals)
    STD_PHP_INI_ENTRY(INGRES_INI_DEFAULT_PASSWORD, NULL, PHP_INI_ALL, OnUpdateString, default_password, zend_ingres_globals, ingres_globals)
    STD_PHP_INI_ENTRY(INGRES_INI_CURSOR_MODE, "1", PHP_INI_ALL, OnUpdateLong, cursor_mode, zend_ingres_globals, ingres_globals)
    STD_PHP_INI_ENTRY(INGRES_INI_BLOB_SEGMENT_LENGTH, "4096", PHP_INI_ALL, OnUpdateLong, blob_segment_length, zend_ingres_globals, ingres_globals)
    STD_PHP_INI_BOOLEAN(INGRES_INI_TRACE_CONNECT, "0", PHP_INI_ALL, OnUpdateBool, trace_connect, zend_ingres_globals, ingres_globals)
    PHP_INI_ENTRY(INGRES_INI_ARRAY_INDEX_START,"1", PHP_INI_ALL, php_ii_modify_array_index_start)
    STD_PHP_INI_BOOLEAN(INGRES_INI_AUTO, "1", PHP_INI_ALL, OnUpdateBool, auto_multi, zend_ingres_globals, ingres_globals)
#if defined (IIAPI_VERSION_3)
    STD_PHP_INI_BOOLEAN(INGRES_INI_UTF8, "1", PHP_INI_ALL, OnUpdateBool, utf8, zend_ingres_globals, ingres_globals)
#endif
    STD_PHP_INI_BOOLEAN(INGRES_INI_REUSE_CONNECTION, "1", PHP_INI_ALL, OnUpdateBool, reuse_connection, zend_ingres_globals, ingres_globals)
    STD_PHP_INI_BOOLEAN(INGRES_INI_TRACE, "0", PHP_INI_ALL, OnUpdateBool, ingres_trace, zend_ingres_globals, ingres_globals)
#if defined (IIAPI_VERSION_6)
    STD_PHP_INI_BOOLEAN(INGRES_INI_SCROLL, "1", PHP_INI_ALL, OnUpdateBool, scroll, zend_ingres_globals, ingres_globals)
#endif
#if defined (IIAPI_VERSION_5)
    STD_PHP_INI_BOOLEAN(INGRES_INI_DESCRIBE, "1", PHP_INI_ALL, OnUpdateBool, describe, zend_ingres_globals, ingres_globals)
#endif
    PHP_INI_ENTRY(INGRES_INI_FETCH_BUFFER_SIZE, "100", PHP_INI_ALL, php_ii_modify_fetch_buffer_size)
PHP_INI_END()
/* }}} */

/* PHP INI modification handlers */

/* {{{ ZEND_INI_MH(php_ii_modify_array_index_start) */
/* Make sure array_index_start can only be set to 0 or 1 */
ZEND_INI_MH(php_ii_modify_array_index_start)
{
    long new_value_long = 0;
    new_value_long = atoi(new_value);

    if ((new_value_long < 0) || (new_value_long > 1))
    {
        return FAILURE;
    }
    INGRESG(array_index_start) = new_value_long;
    return SUCCESS;
}
/* }}} */

/* {{{ ZEND_INI_MH(php_ii_modify_fetch_buffer_size) */
/* Make sure fetch_buffer_size is greater than 0 */
ZEND_INI_MH(php_ii_modify_fetch_buffer_size)
{
    long new_value_long = 0;
    new_value_long = atoi(new_value);

    if (new_value_long < 1)
    {
        return FAILURE;
    }
    INGRESG(fetch_buffer_size) = new_value_long;
    return SUCCESS;
}
/* }}} */

/* {{{ static int _close_statement(II_LINK *ii_link TSRMLS_DC) */
/* closes statement in given link */
static int _close_statement(II_RESULT *ii_result TSRMLS_DC)
{
    IIAPI_CANCELPARM   cancelParm;
    IIAPI_CLOSEPARM    closeParm;

    if (ii_result->stmtHandle)
    {
        /* Release any resultset data */
        _free_resultdata (ii_result);

        /* see if we can close the query without cancelling it */
        /* Free query resources. */
        closeParm.cl_genParm.gp_callback = NULL;
        closeParm.cl_genParm.gp_closure = NULL;
        closeParm.cl_stmtHandle = ii_result->stmtHandle;

        IIapi_close(&closeParm);
        ii_sync(&(closeParm.cl_genParm));

        if (ii_success(&(closeParm.cl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
        {
            /* unable to close */
            /* Cancel query processing. */
            cancelParm.cn_genParm.gp_callback = NULL;
            cancelParm.cn_genParm.gp_closure = NULL;
            cancelParm.cn_stmtHandle = ii_result->stmtHandle;

            IIapi_cancel(&cancelParm );

            ii_sync(&(cancelParm.cn_genParm));

            /* Free query resources. */
            closeParm.cl_genParm.gp_callback = NULL;
            closeParm.cl_genParm.gp_closure = NULL;
            closeParm.cl_stmtHandle = ii_result->stmtHandle;

            IIapi_close( &closeParm );

            ii_sync(&(closeParm.cl_genParm));

            if (ii_success(&(closeParm.cl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "_close_statement : failed ");
                return II_FAIL;
            }
        }
    }

    ii_result->stmtHandle = NULL;
    ii_result->fieldCount = 0;
    ii_result->descriptor = NULL;
    if ( ii_result->procname != NULL )
    {
        free(ii_result->procname);
        ii_result->procname = NULL;
    }
    if ( ii_result->descriptor != NULL )
    {
        free(ii_result->descriptor);
        ii_result->descriptor = NULL;
    }
    if ( ii_result->inputDescr != NULL )
    {
        efree(ii_result->inputDescr);
        ii_result->inputDescr = NULL;
    }
    if ( ii_result->cursor_id != NULL )
    {
        efree(ii_result->cursor_id);
        ii_result->cursor_id = NULL;
    }

    ii_result->paramCount = 0;

    return II_OK;
}
/* }}} */

/* {{{ static int _rollback_transaction(II_LINK *ii_link  TSRMLS_DC) */
/* rolls back transaction in given link after closing the active transaction (if any) */
static int _rollback_transaction(II_LINK *ii_link  TSRMLS_DC)
{
    IIAPI_ROLLBACKPARM rollbackParm;
    IIAPI_GETEINFOPARM error_info;

    /* clean up any un-freed statements/results */
    _free_ii_link_result_list(ii_link TSRMLS_CC);

    rollbackParm.rb_genParm.gp_callback = NULL;
    rollbackParm.rb_genParm.gp_closure = NULL;
    rollbackParm.rb_tranHandle = ii_link->tranHandle;
    rollbackParm.rb_savePointHandle = NULL;

    IIapi_rollback(&rollbackParm);
    ii_sync(&(rollbackParm.rb_genParm));

    switch((rollbackParm.rb_genParm).gp_status)
    {
        case IIAPI_ST_SUCCESS:
            if (INGRESG(ingres_trace))
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_rollback_transaction: IIAPI_ST_SUCCESS");
            }
            break;
        case IIAPI_ST_MESSAGE:
            if (INGRESG(ingres_trace))
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_rollback_transaction: IIAPI_ST_MESSAGE");
            }
            break;
        case IIAPI_ST_WARNING:
            if (INGRESG(ingres_trace))
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_rollback_transaction: IIAPI_ST_WARNING");
            }
            break;
        case IIAPI_ST_NO_DATA:
            if (INGRESG(ingres_trace))
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_rollback_transaction: IIAPI_ST_NO_DATA");
            }
            break;
        case IIAPI_ST_FAILURE:
            if (INGRESG(ingres_trace))
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_rollback_transaction: IIAPI_ST_FAILURE");
            }
            error_info.ge_errorHandle = (rollbackParm.rb_genParm).gp_errorHandle;
            IIapi_getErrorInfo(&error_info);
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_rollback_transaction : IIAPI_ST_FAILURE: %d - %s", error_info.ge_errorCode, error_info.ge_message);
            break;
        default:
            error_info.ge_errorHandle = (rollbackParm.rb_genParm).gp_errorHandle;
            IIapi_getErrorInfo(&error_info);
            if (error_info.ge_status == IIAPI_ST_SUCCESS)
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_rollback_transaction : %d - %s", error_info.ge_errorCode, error_info.ge_message);
            }
            else
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_rollback_transaction : Unable to perform internal rollback");
            }

            php_error_docref(NULL TSRMLS_CC, E_WARNING, "_rollback_transaction : Unable to rollback transaction");
    }
                
    ii_link->tranHandle = NULL;

    return 0;
}
/* }}} */

/* {{{ static int _commit_transaction(II_LINK *ii_link  TSRMLS_DC) */
/* commit transaction in given link after closing the active transaction (if any) */
static int _commit_transaction(II_LINK *ii_link  TSRMLS_DC)
{
    IIAPI_COMMITPARM commitParm;
    if (INGRESG(ingres_trace))
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_commit_transaction: started");
    }

    if ( ii_link->tranHandle != NULL )
    {
        commitParm.cm_genParm.gp_callback = NULL;
        commitParm.cm_genParm.gp_closure = NULL;
        commitParm.cm_tranHandle = ii_link->tranHandle;

        IIapi_commit(&commitParm);
        ii_sync(&(commitParm.cm_genParm));

        if (ii_success(&(commitParm.cm_genParm), &ii_link->errorHandle TSRMLS_CC) == II_FAIL)
        {
            if (INGRESG(ingres_trace))
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_commit_transaction: no success");
            }
            return II_FAIL;
        }

        ii_link->tranHandle = NULL;
    }

    return II_OK;
}
/* }}} */

/* {{{ static int _autocommit_transaction(II_LINK *ii_link  TSRMLS_DC) */
/* rolls back transaction in given link after closing the active transaction (if any) */
static int _autocommit_transaction(II_LINK *ii_link  TSRMLS_DC)
{
    IIAPI_AUTOPARM autoParm;

    if ( !ii_link->result_list_ptr )
    {

        autoParm.ac_genParm.gp_callback = NULL;
        autoParm.ac_genParm.gp_closure = NULL;
        autoParm.ac_connHandle = ii_link->connHandle;
        autoParm.ac_tranHandle = (ii_link->autocommit ? ii_link->tranHandle : NULL );

        IIapi_autocommit(&autoParm);
        ii_sync(&(autoParm.ac_genParm));

        if (ii_success(&(autoParm.ac_genParm), &ii_link->errorHandle TSRMLS_CC) == II_FAIL)
        {
            if ( ii_link->autocommit )
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_autocommit_transaction : Unable to disable autocommit");
            }
            else
            {  
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_autocommit_transaction : Unable to enable autocommit");
            }
            return II_FAIL;
        }

        if ( ii_link->autocommit )
        {
            ii_link->autocommit = 0;
            ii_link->tranHandle = autoParm.ac_tranHandle;
        }
        else
        {  
            ii_link->autocommit = 1;
            ii_link->tranHandle = autoParm.ac_tranHandle;
        }

        return II_OK;
    }
    else
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Unable to change the auto-commit state with active result-sets.");
        return II_FAIL;
    }
}
/* }}} */

/* {{{ static void _close_ii_link(II_LINK *ii_link TSRMLS_DC) */
static void _close_ii_link(II_LINK *ii_link TSRMLS_DC)
{
    IIAPI_DISCONNPARM disconnParm;
    IIAPI_GETEINFOPARM error_info;
    IIAPI_ABORTPARM abortParm;

    if (INGRESG(ingres_trace))
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_close_ii_link: started");
    }
    /* clean up any un-freed statements/results */
    _free_ii_link_result_list(ii_link TSRMLS_CC);

    /* if in auto-commit emulation mode we should commit any outstanding transactions */
    if (ii_link->auto_multi)
    {
        /* commit the previous statement before changing the auto-commit state */
        if (_commit_transaction(ii_link TSRMLS_CC) == II_FAIL)
        {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "An error occur when issuing an internal commit");
        }
    }

    /* Disable auto-commit if enabled */
    if ( ii_link->autocommit ) 
    {
        if (_autocommit_transaction(ii_link TSRMLS_CC) == II_FAIL)
        {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "_close_ii_link : An error occurred when changing the auto-commit state");
        }
    }

    if (ii_link->tranHandle && _rollback_transaction(ii_link TSRMLS_CC))
    {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "_close_ii_link : Unable to rollback transaction");
    }

    disconnParm.dc_genParm.gp_callback = NULL;
    disconnParm.dc_genParm.gp_closure = NULL;
    disconnParm.dc_connHandle = ii_link->connHandle;

    IIapi_disconnect(&disconnParm);

    ii_sync(&(disconnParm.dc_genParm));

    switch((disconnParm.dc_genParm).gp_status)
    {
        case IIAPI_ST_SUCCESS:
        case IIAPI_ST_NO_DATA:
            ii_link->stmtHandle = NULL;
            break;
        default:
            error_info.ge_errorHandle = (disconnParm.dc_genParm).gp_errorHandle;
            IIapi_getErrorInfo(&error_info);
            if (error_info.ge_status == IIAPI_ST_SUCCESS)
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_close_ii_link : %d - %s", error_info.ge_errorCode, error_info.ge_message);
            }
            else
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_close_ii_link : Unable to close link");
            }
            abortParm.ab_genParm.gp_callback = NULL;
            abortParm.ab_genParm.gp_closure = NULL;
            abortParm.ab_connHandle = ii_link->connHandle;
            IIapi_abort(&abortParm);

            ii_sync(&(abortParm.ab_genParm));
            switch((abortParm.ab_genParm).gp_status)
            {
                case IIAPI_ST_SUCCESS:
                case IIAPI_ST_NO_DATA:
                    ii_link->stmtHandle = NULL;
                    break;
                default:
                    error_info.ge_errorHandle = (abortParm.ab_genParm).gp_errorHandle;
                    IIapi_getErrorInfo(&error_info);
                    if (error_info.ge_status == IIAPI_ST_SUCCESS)
                    {
                        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_close_ii_link : %d - %s", error_info.ge_errorCode, error_info.ge_message);
                    }
                    else
                    {
                        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_close_ii_link : Unable to close link");
                    }
            }
    }

    if (ii_link->charset)
    {
        efree(ii_link->charset);
    }

    free(ii_link);

    INGRESG(num_links)--;
    if (INGRESG(ingres_trace))
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "_close_ii_link: done");
    }
}
/*  }}} */

/* {{{ static void _free_ii_link_result_list (II_LINK *ii_link TSRMLS_DC) */
static void _free_ii_link_result_list (II_LINK *ii_link TSRMLS_DC)
{
    IIAPI_CANCELPARM   cancelParm;
    IIAPI_CLOSEPARM       closeParm;

    II_RESULT *ii_result = NULL;
    ii_result_entry *result_entry = NULL;
    int type;

    /* clean up any un-freed statements/results */

    while ( ii_link->result_list_ptr ) 
    {
        ii_result = (II_RESULT *)zend_list_find(ii_link->result_list_ptr->result_id, &type);

        if ( ii_result )
        {
            /* if the result and link statement handles are the same set the latter to NULL since it will be destroyed */
            /* _close_statement().                                                                                     */
            if (ii_result->stmtHandle == ii_link->stmtHandle)
            {
                ii_link->stmtHandle = NULL;
            }
            _close_statement (ii_result TSRMLS_CC);
        }
        if (ii_link->result_list_ptr->result_id)
        {
            zend_list_delete(ii_link->result_list_ptr->result_id);
            // set the result_id 0 to indicate it has been released
            ii_link->result_list_ptr->result_id = 0;
        }

        result_entry =  ii_link->result_list_ptr;
        ii_link->result_list_ptr = (ii_result_entry *)ii_link->result_list_ptr->next_result_ptr;

        /* free memory associated with the top entry in the list */
        free(result_entry);
    }

    /* if there are no result resources created it is possible the last query generated an error
       for which there will still be an active transaction that needs to be closed */

    if ( ii_link->stmtHandle != NULL )
    { 
        /* Cancel query processing. */
        cancelParm.cn_genParm.gp_callback = NULL;
        cancelParm.cn_genParm.gp_closure = NULL;
        cancelParm.cn_stmtHandle = ii_link->stmtHandle;

        IIapi_cancel(&cancelParm );

        ii_sync(&(cancelParm.cn_genParm));

        /* Free query resources. */
        closeParm.cl_genParm.gp_callback = NULL;
        closeParm.cl_genParm.gp_closure = NULL;
        closeParm.cl_stmtHandle = ii_link->stmtHandle;

        IIapi_close( &closeParm );

        ii_sync(&(closeParm.cl_genParm));

        switch((closeParm.cl_genParm).gp_status)
        {
            case IIAPI_ST_SUCCESS:
            case IIAPI_ST_NO_DATA:
                ii_link->stmtHandle = NULL;
                break;
            default:
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "_free_ii_result_link_list : Unable to close statement");
        }
    }
}
/*  }}} */

/* {{{ static void php_close_ii_link(zend_rsrc_list_entry *rsrc TSRMLS_DC)
 * closes the given link, actually disconnecting from server and releasing associated resources 
 * after rolling back the active transaction (if any)
*/
static void php_close_ii_link(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    II_LINK *ii_link = (II_LINK *) rsrc->ptr;

    _close_ii_link(ii_link TSRMLS_CC);
}
/*  }}} */

/* {{{ static void php_close_ii_result(zend_rsrc_list_entry *rsrc TSRMLS_DC)
 * closes the given result
*/
static void php_close_ii_result(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    II_RESULT *ii_result = (II_RESULT *) rsrc->ptr;

    _close_statement(ii_result TSRMLS_CC);
}
/*  }}} */

/* {{{ static void _close_ii_plink(zend_rsrc_list_entry *rsrc TSRMLS_DC) */
/* closes the given persistent link, see _close_ii_link */
static void _close_ii_plink(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    II_LINK *ii_link = (II_LINK *) rsrc->ptr;

    _close_ii_link(ii_link TSRMLS_CC);
    INGRESG(num_persistent)--;
}
/*  }}} */

/* {{{ static void _ai_clean_ii_plink(II_LINK *ii_link TSRMLS_DC) */
/* cleans up the given persistent link.  used when the request ends to 'refresh' the link for use */
/* by the next request */
static void _ai_clean_ii_plink(II_LINK *ii_link TSRMLS_DC)
{
    int ai_error = 0;
    IIAPI_DISCONNPARM disconnParm;


    /* if link as always been marked as broken do nothing */
    /* This because we call this function directly from close function */
    /* And it's called in the end of request */
    if (ii_link->connHandle == NULL)
    {
        return;
    }

    /* clean up any un-freed statements/results */
    _free_ii_link_result_list(ii_link TSRMLS_CC);

    /* if in auto-commit emulation mode we should commit any outstanding transactions */
    if (ii_link->auto_multi)
    {
        /* commit the previous statement before changing the auto-commit state */
        if (_commit_transaction(ii_link TSRMLS_CC) == II_FAIL)
        {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "An error occur when issuing an internal commit");
        }
    }
    /* auto-commit must be disabled before before disconnection */
    if (ii_link->autocommit)
    {
        if (_autocommit_transaction(ii_link TSRMLS_CC) == II_FAIL)
        {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "_ai_clean_ii_plink : An error occur when changing the auto-commit state");
        }
    }

    if (ii_link->tranHandle && _rollback_transaction(ii_link TSRMLS_CC))
    {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "_ai_clean_ii_plink : Unable to rollback transaction");
    }

    /* Assume link is broken, close it, and mark it as broken with conn Handle NULL */
    if (ai_error)
    {
        disconnParm.dc_genParm.gp_callback = NULL;
        disconnParm.dc_genParm.gp_closure = NULL;
        disconnParm.dc_connHandle = ii_link->connHandle;
        
        IIapi_disconnect(&disconnParm);
        ii_sync(&(disconnParm.dc_genParm));
        ii_link->connHandle = NULL;
    }

    if (INGRESG(error_text) != NULL )
    {
        efree(INGRESG(error_text));
        INGRESG(error_text) = NULL;
    }

}
/*  }}} */

/* {{{ static void _clean_ii_plink(zend_rsrc_list_entry *rsrc TSRMLS_DC) */
static void _clean_ii_plink(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    II_LINK *ii_link;
    ii_link = (II_LINK *)rsrc->ptr;
    _ai_clean_ii_plink(ii_link TSRMLS_CC);
}
/* }}} */

/* {{{ PHP_GINIT_FUNCTION */
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 1) || (PHP_MAJOR_VERSION > 5)
static PHP_GINIT_FUNCTION(ingres)
#else
static void php_ingres_globals_init(zend_ingres_globals *ingres_globals)
#endif
{
    IIAPI_INITPARM initParm;

    /* Ingres api initialization */
    /* timeout in ms, -1, (default) = no timeout */
    initParm.in_timeout = -1;

#if defined(IIAPI_VERSION_6) 
    initParm.in_version = IIAPI_VERSION_6;
#elif defined(IIAPI_VERSION_5) 
    initParm.in_version = IIAPI_VERSION_5;
#elif defined(IIAPI_VERSION_4) 
    initParm.in_version = IIAPI_VERSION_4;
#elif defined(IIAPI_VERSION_3)
    initParm.in_version = IIAPI_VERSION_3;
#elif defined(IIAPI_VERSION_2) 
    initParm.in_version = IIAPI_VERSION_2;
#else
    initParm.in_version = IIAPI_VERSION_1;
#endif

    IIapi_initialize(&initParm);

#if defined(IIAPI_VERSION_2)
    if ( initParm.in_envHandle != NULL )
    {    
        ingres_globals->envHandle = initParm.in_envHandle; 
    }
#else
    ingres_globals->envHandle = NULL;
#endif

    /* Defaults for the ingres.xxxxxx ini settings */

    ingres_globals->num_persistent = 0;
    ingres_globals->auto_multi = 1;
    ingres_globals->trace_connect = 0;
    ingres_globals->ingres_trace = 0;
    ingres_globals->array_index_start = 1;
    ingres_globals->error_text = NULL;
    ingres_globals->error_number = 0;
    ingres_globals->reuse_connection = 1;
    ingres_globals->scroll = 1;
    ingres_globals->describe = 1;
#if defined (IIAPI_VERSION_3)
    ingres_globals->utf8 = 1;
#endif
    ingres_globals->fetch_buffer_size = II_BUFFER_SIZE;
    ingres_globals->cursor_mode = II_CURSOR_READONLY;
    ingres_globals->default_database = NULL;
    ingres_globals->default_user = NULL;
    ingres_globals->default_password = NULL;

}
/* }}} */

/* {{{ Module initialization
*/
PHP_MINIT_FUNCTION(ingres)
{

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 1) || (PHP_MAJOR_VERSION > 5)
    /* Handled by PHP_GINIT_FUNCTION(ingres) */
#else
    ZEND_INIT_MODULE_GLOBALS(ingres, php_ingres_globals_init, NULL)
#endif
    REGISTER_INI_ENTRIES();

    le_ii_result = zend_register_list_destructors_ex(php_close_ii_result,     NULL, "ingres result", module_number);
    le_ii_link = zend_register_list_destructors_ex(php_close_ii_link,     NULL, "ingres connection", module_number);
    le_ii_plink = zend_register_list_destructors_ex(_clean_ii_plink, _close_ii_plink, "ingres persistent connection", module_number);

    /* Constants registration */
#ifndef HAVE_INGRES2
    REGISTER_LONG_CONSTANT("INGRES_ASSOC",              II_ASSOC,               CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_NUM",                II_NUM,                 CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_BOTH",               II_BOTH,                CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES_EXT_VERSION",      PHP_INGRES_VERSION,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_API_VERSION",        IIAPI_VERSION,          CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_CURSOR_READONLY",    II_CURSOR_READONLY,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_CURSOR_UPDATE",      II_CURSOR_UPDATE,       CONST_CS | CONST_PERSISTENT);
#if defined (IIAPI_VERSION_2)
    REGISTER_LONG_CONSTANT("INGRES_DATE_US",            II_DATE_US,             CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_DATE_MULTINATIONAL", II_DATE_MULTINATIONAL,  CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_DATE_MULTINATIONAL4",II_DATE_MULTINATIONAL4, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_DATE_FINNISH",       II_DATE_FINNISH,        CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_DATE_ISO",           II_DATE_ISO,            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_DATE_ISO4",          II_DATE_ISO4,           CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_DATE_GERMAN",        II_DATE_GERMAN,         CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_DATE_MDY",           II_DATE_MDY,            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_DATE_DMY",           II_DATE_DMY,            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_DATE_YMD",           II_DATE_YMD,            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_MONEY_LEADING",      II_MONEY_LEAD_SIGN,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES_MONEY_TRAILING",     II_MONEY_TRAIL_SIGN,    CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES_STRUCTURE_ISAM",   II_STRUCTURE_ISAM,      CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES_STRUCTURE_CISAM",  II_STRUCTURE_CISAM,     CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES_STRUCTURE_BTREE",  II_STRUCTURE_BTREE,     CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES_STRUCTURE_CTREE",  II_STRUCTURE_CBTREE,    CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES_STRUCTURE_HASH",   II_STRUCTURE_HASH,      CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES_STRUCTURE_CHASH",  II_STRUCTURE_CHASH,     CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES_STRUCTURE_HEAP",   II_STRUCTURE_HEAP,      CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES_STRUCTURE_CHEAP",  II_STRUCTURE_CHEAP,     CONST_CS | CONST_PERSISTENT);
#endif /* IIAPI_VERSION_2 */

#else
    REGISTER_LONG_CONSTANT("INGRES2_ASSOC",              II_ASSOC,               CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_NUM",                II_NUM,                 CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_BOTH",               II_BOTH,                CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES2_EXT_VERSION",      PHP_INGRES_VERSION,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_API_VERSION",        IIAPI_VERSION,          CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_CURSOR_READONLY",    II_CURSOR_READONLY,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_CURSOR_UPDATE",      II_CURSOR_UPDATE,       CONST_CS | CONST_PERSISTENT);
#if defined (IIAPI_VERSION_2)
    REGISTER_LONG_CONSTANT("INGRES2_DATE_US",            II_DATE_US,             CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_DATE_MULTINATIONAL", II_DATE_MULTINATIONAL,  CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_DATE_MULTINATIONAL4",II_DATE_MULTINATIONAL4, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_DATE_FINNISH",       II_DATE_FINNISH,        CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_DATE_ISO",           II_DATE_ISO,            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_DATE_ISO4",          II_DATE_ISO4,           CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_DATE_GERMAN",        II_DATE_GERMAN,         CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_DATE_MDY",           II_DATE_MDY,            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_DATE_DMY",           II_DATE_DMY,            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_DATE_YMD",           II_DATE_YMD,            CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_MONEY_LEADING",      II_MONEY_LEAD_SIGN,     CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INGRES2_MONEY_TRAILING",     II_MONEY_TRAIL_SIGN,    CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES2_STRUCTURE_ISAM",   II_STRUCTURE_ISAM,      CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES2_STRUCTURE_CISAM",  II_STRUCTURE_CISAM,     CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES2_STRUCTURE_BTREE",  II_STRUCTURE_BTREE,     CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES2_STRUCTURE_CTREE",  II_STRUCTURE_CBTREE,    CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES2_STRUCTURE_HASH",   II_STRUCTURE_HASH,      CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES2_STRUCTURE_CHASH",  II_STRUCTURE_CHASH,     CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES2_STRUCTURE_HEAP",   II_STRUCTURE_HEAP,      CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("INGRES2_STRUCTURE_CHEAP",  II_STRUCTURE_CHEAP,     CONST_CS | CONST_PERSISTENT);
#endif /* IIAPI_VERSION_2 */
#endif


    return SUCCESS;
} 
/* }}} */

/* {{{ Module shutdown */
PHP_MSHUTDOWN_FUNCTION(ingres)
{
    IIAPI_TERMPARM termParm;
#if defined(IIAPI_VERSION_2)
    IIAPI_RELENVPARM   relEnvParm;

    relEnvParm.re_envHandle = INGRESG(envHandle);
    IIapi_releaseEnv(&relEnvParm);
#endif
    /* Ingres api termination */
    IIapi_terminate(&termParm);

#ifdef ZTS
    ts_free_id(ingres_globals_id);    
#endif

    UNREGISTER_INI_ENTRIES();
    if (termParm.tm_status == IIAPI_ST_SUCCESS)
    {
        return SUCCESS;
    } 
    else 
    {
        return FAILURE;
    }
}
/* }}} */

/* {{{ New request initialization */
PHP_RINIT_FUNCTION(ingres)
{
    INGRESG(num_links) = INGRESG(num_persistent);
    INGRESG(cursor_no) = 0;

    return SUCCESS;
}
/* }}} */

/* {{{ End of request */
PHP_RSHUTDOWN_FUNCTION(ingres)
{
    if (INGRESG(error_text) != NULL)
    {
        efree(INGRESG(error_text));
        INGRESG(error_text) = NULL;
    }
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION(ingres)
 * Information reported to phpinfo()
*/
PHP_MINFO_FUNCTION(ingres)
{
    char buf[32];

    php_info_print_table_start();
    php_info_print_table_header(2, "Ingres Support", "enabled");
    php_info_print_table_row(2, "Ingres Extension Version", PHP_INGRES_VERSION);
    php_info_print_table_row(2, "Revision", "$Revision$");
    sprintf(buf, "%d", IIAPI_VERSION );
    php_info_print_table_row(2, "Ingres OpenAPI Version", buf);
    sprintf(buf, "%ld", INGRESG(num_persistent));
    php_info_print_table_row(2, "Active Persistent Links", buf);
    sprintf(buf, "%ld", INGRESG(num_links));
    php_info_print_table_row(2, "Active Links", buf);
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
} 
/* }}} */

/* {{{ static int ii_sync(IIAPI_GENPARM *genParm)
 * Waits for completion of the last Ingres api call used because of the asynchronous design of this api
*/
static int ii_sync(IIAPI_GENPARM *genParm)
{
    IIAPI_WAITPARM waitParm = {
        -1,        /* no timeout, we don't want asynchronous queries */
        0        /* wt_status (output) */
    };

    while (genParm->gp_completed == FALSE)
    {
        IIapi_wait(&waitParm);
    }

    if (waitParm.wt_status != IIAPI_ST_SUCCESS)
    {
        TSRMLS_FETCH();
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unexpected failure of IIapi_wait()");
        return 0;
    }
    return 1;
}
/* }}} */

/* {{{ static int ii_success(IIAPI_GENPARM *genParm, II_PTR *errorHandle  TSRMLS_DC)
 * Handles and stores errors for later retrieval
*/
static int ii_success(IIAPI_GENPARM *genParm, II_PTR *errorHandle TSRMLS_DC)
{
    IIAPI_GETEINFOPARM error_info;

    /* Initialise global variables */
    INGRESG(errorHandle) = NULL;
    INGRESG(error_number) = 0;
    if (INGRESG(error_text) != NULL)
    {
        efree(INGRESG(error_text));
        INGRESG(error_text) = NULL;
    }

    switch (genParm->gp_status)
    {
        case IIAPI_ST_SUCCESS:
            return II_OK;
            
        case IIAPI_ST_NO_DATA:
            return II_NO_DATA;

        default:
            if (genParm->gp_errorHandle == NULL)
            {    
                /* no error message available */
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "ii_success : Server or API error - no error message available, status %d", genParm->gp_status);
            } 
            else 
            {
                error_info.ge_errorHandle = genParm->gp_errorHandle;
                IIapi_getErrorInfo(&error_info);

                switch (error_info.ge_status)
                {
                    case IIAPI_ST_SUCCESS:
                    case IIAPI_ST_NO_DATA:
                        INGRESG(error_number) = error_info.ge_errorCode;
                        INGRESG(error_text) = estrdup(error_info.ge_message);
                        memcpy(INGRESG(error_sqlstate), error_info.ge_SQLSTATE, II_SQLSTATE_LEN + 1);

                        break;
                    case IIAPI_ST_INVALID_HANDLE:
                        php_error_docref(NULL TSRMLS_CC, E_WARNING, "IIapi_getErrorInfo error, invalid error handle");
                        break;
                    default: /* An error occured with IIapi_getErrorInfo() */
                        php_error_docref(NULL TSRMLS_CC, E_WARNING, "IIapi_getErrorInfo error, status returned was : %d", error_info.ge_status );
                        break;
                }
                INGRESG(errorHandle) = genParm->gp_errorHandle;
            }
            return II_FAIL;
    }
} 
/* }}} */

/* {{{ static void _ii_init_link ( II_LINK *ii_link )
 *
 */
static void _ii_init_link (INTERNAL_FUNCTION_PARAMETERS,  II_LINK *ii_link )
{
    ii_link->envHandle = INGRESG(envHandle);
    if ( ii_link->envHandle != NULL )
    {
        ii_link->connHandle = ii_link->envHandle;
    }
    else
    {
        ii_link->connHandle = NULL;
    }
    ii_link->tranHandle = NULL;
    ii_link->stmtHandle = NULL;
    ii_link->autocommit = 0;
    ii_link->errorHandle = NULL;
    ii_link->result_list_ptr = NULL;
    ii_link->auto_multi = 0;
    ii_link->apiLevel = 0;
    ii_link->charset = NULL;
}
/* }}} */

/* {{{ static void _ii_init_result ( II_RESULT *ii_result, II_LINK *ii_link  )
 *
 */
static void _ii_init_result (INTERNAL_FUNCTION_PARAMETERS, II_RESULT *ii_result, II_LINK *ii_link )
{
    ii_result->stmtHandle = ii_link->stmtHandle;
    ii_result->connHandle = ii_link->connHandle;
    ii_result->tranHandle = ii_link->tranHandle;
    ii_result->fieldCount = 0;
    ii_result->descriptor = NULL;
    ii_result->paramCount = 0 ;
    ii_result->cursor_id = NULL;
    ii_result->procname = NULL;
    ii_result->cursor_mode = II_CURSOR_READONLY;
    ii_result->errorHandle = NULL;
#if defined(IIAPI_VERSION_6)
    ii_result->scrollable = 0;
    ii_result->rowCount = -1;  /* A call to ingres_num_rows()/ingres_data_seek() has yet to happen */
#endif
    ii_result->link_id = -1;
    ii_result->apiLevel = ii_link->apiLevel;
    ii_result->metaData = NULL;
    ii_result->dataBuffer = NULL;
    ii_result->rowsReturned = 0;
    ii_result->rowNumber = 0;
    ii_result->rowWidth = 0;
    ii_result->inputCount = 0;
    ii_result->descriptor = NULL;
    ii_result->queryType = IIAPI_QT_BASE;  /* 0 */
    ii_result->inputDescr = NULL;
    ii_result->buffered = FALSE;
    ii_result->prepared = FALSE;
    ii_result->executed = FALSE;
}
/* }}} */

/* {{{ static void php_ii_do_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent)
 * Actually handles connection creation, either persistent or not
*/
static void php_ii_do_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent)
{
    char *database = NULL;
    char *username = NULL; 
    char *password = NULL;
    int database_len = 0;
    int username_len = 0;
    int password_len = 0;
    zval *options = NULL;
    char *db = NULL;
    char *user = NULL;
    char *pass = NULL;
    char *hashed_details;
    int hashed_details_length;
    IIAPI_CONNPARM connParm;
    II_LINK *ii_link;

#if defined(IIAPI_VERSION_2)
    IIAPI_SETENVPRMPARM    setEnvPrmParm;
    long temp_long;
#endif
    if ( getenv("II_SYSTEM") == NULL )
    {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Unable to connect - II_SYSTEM is not defined, refer to http://php.net/manual/en/ingres.installation.php for information about setting up your environment." );
        RETURN_FALSE;
    }

    if (INGRESG(trace_connect)) {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Enter php_ii_do_connect");
    }

    /* Setting db, user and pass according to sql_safe_mode, parameters and/or default values */
    if (PG(sql_safe_mode))
    {    /* sql_safe_mode */

        if (INGRESG(trace_connect)) {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "SQL safe mode in effect");
        }

        if (ZEND_NUM_ARGS() > 0)
        {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "SQL safe mode in effect - ignoring host/user/password/option information");
        }

        if (INGRESG(default_database))
        {
            db = INGRESG(default_database);
        }
        else
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "SQL safe mode in effect - %s.default_database is not set, unable to continue", INGRES_EXT_NAME);
            RETURN_FALSE;
        }
        if (INGRESG(default_user))
        {
            username = INGRESG(default_user);
            user = username;

            /* Require a password if a username is defined in php.ini */
            if (INGRESG(default_password))
            {
                pass = INGRESG(default_password);
            }
            else
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "SQL safe mode in effect - %s.default_password is not set, unable to authenticate as %s", INGRES_EXT_NAME, user);
                RETURN_FALSE;
            }
        }
        else
        {
            /* if no username is supplied we need supply one for the hash */
            username = php_get_current_user();
        }
        
        hashed_details_length = strlen(username) + sizeof("ingres___") - 1;
        hashed_details = (char *) emalloc(hashed_details_length + 1);
        sprintf(hashed_details, "Ingres__%s_", username);

    } 
    else 
    {                    /* non-sql_safe_mode */
        if (INGRESG(trace_connect)) {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "SQL safe mode not in effect");
        }
        db = INGRESG(default_database);
        user = INGRESG(default_user);
        pass = INGRESG(default_password);

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"s|ssa", &database, &database_len, &username, &username_len, &password, &password_len, &options) == FAILURE) 
        {
            RETURN_NULL();
        }

        switch (ZEND_NUM_ARGS())
        {
            case 4:  /* do nothing */
            case 3:
                pass = password;
                /* Fall-through. */
        
            case 2:
                user = username;
                /* Fall-through. */
        
            case 1:
                db = database;
                /* Fall-through. */

            case 0:
                break;
        }
        
        /* Perform Sanity Check. If no database has been set then we have a problem */
        if (INGRESG(trace_connect)) {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Connecting to %s as %s", db, user);
        }

        hashed_details_length =    sizeof("ingres___") - 1 + 
                                strlen(SAFE_STRING(db)) +
                                strlen(SAFE_STRING(user)) + 
                                strlen(SAFE_STRING(pass));

        hashed_details = (char *) emalloc(hashed_details_length + 1);
        sprintf(hashed_details, "Ingres_%s_%s_%s", 
                                SAFE_STRING(db),    
                                SAFE_STRING(user), 
                                SAFE_STRING(pass));
    }

    /* if asked for an unauthorized persistent connection, issue a warning and go for a non-persistent link */
    if (persistent && !INGRESG(allow_persistent))
    {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Persistent links disabled !");
        persistent = 0;
    }

    if (persistent)
    {
        list_entry *le;

        if (INGRESG(trace_connect)) {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Enter persistent connection");
        }

        /* is this link already in the persistent list ? */
        if (zend_hash_find(&EG(persistent_list), hashed_details, hashed_details_length + 1, (void *) &le) == FAILURE)
        { /* no, new persistent connection */

            list_entry new_le;

            if (INGRESG(trace_connect)) {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "creating new persistent connection");
            }

            if (INGRESG(max_links) != -1 && INGRESG(num_links) >= INGRESG(max_links))
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Too many open links (%ld)", INGRESG(num_links));
                efree(hashed_details);
                RETURN_FALSE;
            }
            if (INGRESG(max_persistent) != -1 && INGRESG(num_persistent) >= INGRESG(max_persistent))
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Too many open persistent links (%ld)", INGRESG(num_persistent));
                efree(hashed_details);
                RETURN_FALSE;
            }

            /* setup the link */
            ii_link = (II_LINK *) malloc(sizeof(II_LINK));
            _ii_init_link(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_link);

            if ( ZEND_NUM_ARGS() == 4 ) /* set options */
            {
                if ( php_ii_set_connect_options(options, ii_link, db TSRMLS_CC ) == II_FAIL )
                {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to set options provided");
                    RETURN_FALSE;
                }    
            }

            /* create the link */
            connParm.co_genParm.gp_callback = NULL;
            connParm.co_genParm.gp_closure = NULL;
            connParm.co_target = db;
            connParm.co_username = user;
            connParm.co_password = pass;
            connParm.co_timeout = -1;    /* -1 is no timeout */
#if defined (IIAPI_VERSION_2)
            connParm.co_type = IIAPI_CT_SQL;
#endif
            if ( ii_link->connHandle == NULL ) 
            {    
                connParm.co_connHandle =  ii_link->envHandle;
            }
            else
            {
                connParm.co_connHandle =  ii_link->connHandle;
            }
            
            connParm.co_tranHandle = NULL;

            if (INGRESG(trace_connect)) {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Start IIapi_connect()");
            }

            IIapi_connect(&connParm);

            if (!ii_sync(&(connParm.co_genParm)) || ii_success(&(connParm.co_genParm), &ii_link->errorHandle TSRMLS_CC) == II_FAIL)
            {
                free(ii_link);
                efree(hashed_details);
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to connect to database (%s)", db);
                RETURN_FALSE;
            }
            if (INGRESG(trace_connect)) {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "End IIapi_connect()");
            }

            ii_link->connHandle = connParm.co_connHandle;
            ii_link->apiLevel = connParm.co_apiLevel;

            /* set environment params */

#if defined(IIAPI_VERSION_2)
            if (ii_link->apiLevel >= IIAPI_LEVEL_1)
            {
                /* first set the blob_segment_length according to the default */
                setEnvPrmParm.se_envHandle = ii_link->envHandle;
                temp_long = INGRESG(blob_segment_length);
                setEnvPrmParm.se_paramValue = (II_PTR)&temp_long;
                setEnvPrmParm.se_paramID = IIAPI_EP_MAX_SEGMENT_LEN;
                IIapi_setEnvParam( &setEnvPrmParm );

                if (setEnvPrmParm.se_status != IIAPI_ST_SUCCESS)
                {
                    efree(hashed_details);
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to set BLOB segment size");
                    RETURN_FALSE;
                }
            }
#endif

            if ( ZEND_NUM_ARGS() == 4 ) /* set options */
            {
                if ( php_ii_set_environment_options(options, ii_link TSRMLS_CC) == II_FAIL )
                {
                    efree(hashed_details);
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to set environment options provided");
                    RETURN_FALSE;
                }    
            }
            
            /* hash it up */
            Z_TYPE(new_le) = le_ii_plink;
            new_le.ptr = ii_link;
            if (zend_hash_update(&EG(persistent_list), hashed_details, hashed_details_length + 1, (void *) &new_le, sizeof(list_entry), NULL) == FAILURE)
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to hash (%s)", hashed_details);
                free(ii_link);
                efree(hashed_details);
                RETURN_FALSE;
            }
            INGRESG(num_persistent)++;
            INGRESG(num_links)++;

        } else { /* already open persistent connection */

            if (INGRESG(trace_connect)) {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "using existing persistent connection");
            }

            if (Z_TYPE_P(le) != le_ii_plink)
            {
                efree(hashed_details);
                RETURN_FALSE;
            }
            /* here we should ensure that the link did not die */
            /* unable to figure out the right way to do this   */
            /* maybe does the api handle the reconnection transparently ? */
            ii_link = (II_LINK *) le->ptr;
            
            /* Unfortunetaly NO !!!*/
            /* Ingres api doesn't reconnect */
            /* Have to reconnect if cleaning function has flagged link as broken */
            if (ii_link->connHandle == NULL)
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING,"Broken persistent link (%s),reconnect", db);
                _ii_init_link(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_link);

                if ( ZEND_NUM_ARGS() == 4 ) /* set options */
                {
                    if ( php_ii_set_connect_options(options, ii_link, db TSRMLS_CC ) == II_FAIL )
                    {
                        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to set options provided");
                        efree(hashed_details);
                        RETURN_FALSE;
                    }    
                }

                /* Recreate the link */

                connParm.co_connHandle = ii_link->connHandle;
                connParm.co_tranHandle = NULL;
                connParm.co_genParm.gp_callback = NULL;
                connParm.co_genParm.gp_closure = NULL;
                connParm.co_target = db;
                connParm.co_username = user;
                connParm.co_password = pass;
                connParm.co_timeout = -1; /* no timeout */
                connParm.co_tranHandle = NULL;
#if defined (IIAPI_VERSION_2)
                connParm.co_type = IIAPI_CT_SQL;
#endif

                if (INGRESG(trace_connect)) {
                    php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Start IIapi_connect()");
                }

                IIapi_connect(&connParm);

                if (!ii_sync(&(connParm.co_genParm)) || ii_success(&(connParm.co_genParm), &ii_link->errorHandle TSRMLS_CC) == II_FAIL)
                {
                    efree(hashed_details);
                    php_error_docref(NULL TSRMLS_CC, E_WARNING,"Unable to connect to database (%s)", db);
                    RETURN_FALSE;
                }

                if (INGRESG(trace_connect)) {
                    php_error_docref(NULL TSRMLS_CC, E_NOTICE, "End IIapi_connect()");
                }

                ii_link->connHandle = connParm.co_connHandle;
                ii_link->apiLevel = connParm.co_apiLevel;

                /* set environment params */

                if ( ZEND_NUM_ARGS() == 4 ) /* set options */
                {
                    if ( php_ii_set_environment_options(options, ii_link TSRMLS_CC) == II_FAIL )
                    {
                        efree(hashed_details);
                        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to set environment options provided");
                        RETURN_FALSE;
                    }    
                }
            }
        }
        
        ZEND_REGISTER_RESOURCE(return_value, ii_link, le_ii_plink);

        if (INGRESG(trace_connect)) {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Exit persistent connection");
        }

    } else { /* non persistent */

        list_entry *index_ptr, new_index_ptr;

        if (INGRESG(trace_connect)) {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Enter non-persistent connection");
        }

        /* first we check the hash for the hashed_details key.  if it exists,
         * it should point us to the right offset where the actual link sits.
         * if it doesn't, open a new link, add it to the resource list,
         * and add a pointer to it with hashed_details as the key.
         */

        if ((INGRESG(reuse_connection)) && (zend_hash_find(&EG(regular_list), hashed_details, hashed_details_length + 1, (void *) &index_ptr) == SUCCESS))
        {
            int type;
            void *ptr;

            if (Z_TYPE_P(index_ptr) != le_index_ptr)
            {
                RETURN_FALSE;
            }
            ii_link = (II_LINK *) index_ptr->ptr;
            ptr = zend_list_find((int) ii_link, &type);    /* check if the link is still there */
            if (ptr && (type == le_ii_link || type == le_ii_plink))
            {
                zend_list_addref((int) ii_link);
                Z_LVAL_P(return_value) = (int) ii_link;

                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Freeing hashed_details");
                Z_TYPE_P(return_value) = IS_RESOURCE;
                efree(hashed_details);
                hashed_details = NULL;
                return;
            } else {
                zend_hash_del(&EG(regular_list), hashed_details, hashed_details_length + 1);
            }
        }
        if (INGRESG(max_links) != -1 && INGRESG(num_links) >= INGRESG(max_links))
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Too many open links (%ld)", INGRESG(num_links));
            efree(hashed_details);
            RETURN_FALSE;
        }

        /* setup the link */
        ii_link = (II_LINK *) malloc(sizeof(II_LINK));
        _ii_init_link(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_link);

        if ( ZEND_NUM_ARGS() == 4 ) /* set options */
        {
            if ( php_ii_set_connect_options(options, ii_link, db TSRMLS_CC) == II_FAIL )
            {
                efree(hashed_details);
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to set options provided");
                RETURN_FALSE;
            }    
        }

        /* create the link */
        connParm.co_genParm.gp_callback = NULL;
        connParm.co_genParm.gp_closure = NULL;
        connParm.co_target = db;
        connParm.co_username = user;
        connParm.co_password = pass;
        connParm.co_timeout = -1;    /* -1 is no timeout */
#if defined (IIAPI_VERSION_2)
        connParm.co_type = IIAPI_CT_SQL;
#endif
        if ( ii_link->connHandle == NULL ) 
        {
            connParm.co_connHandle =  ii_link->envHandle;
        }
        else
        {
            connParm.co_connHandle =  ii_link->connHandle;
        }

        connParm.co_tranHandle = NULL;

        if (INGRESG(trace_connect)) {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Start IIapi_connect()");
        }

        IIapi_connect(&connParm);

        if (!ii_sync(&(connParm.co_genParm)) || ii_success(&(connParm.co_genParm), &ii_link->errorHandle TSRMLS_CC) == II_FAIL)
        {
            efree(hashed_details);
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to connect to database (%s)", db);
            RETURN_FALSE;
        }

        if (INGRESG(trace_connect)) {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "End IIapi_connect()");
        }

        ii_link->connHandle = connParm.co_connHandle;        
        ii_link->apiLevel = connParm.co_apiLevel;

        /* set environment params */

#if defined(IIAPI_VERSION_2)
            if (ii_link->apiLevel >= IIAPI_LEVEL_1)
            {
                /* first set the blob_segment_length according to the default */
                setEnvPrmParm.se_envHandle = ii_link->envHandle;
                temp_long = INGRESG(blob_segment_length);
                setEnvPrmParm.se_paramValue = (II_PTR)&temp_long;
                setEnvPrmParm.se_paramID = IIAPI_EP_MAX_SEGMENT_LEN;
                IIapi_setEnvParam( &setEnvPrmParm );

                if (setEnvPrmParm.se_status != IIAPI_ST_SUCCESS)
                {
                    efree(hashed_details);
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to set BLOB segment size");
                    RETURN_FALSE;
                }
            }
#endif

        if ( ZEND_NUM_ARGS() == 4 ) /* set options */
        {
            if ( php_ii_set_environment_options(options, ii_link TSRMLS_CC) == II_FAIL )
            {
                efree(hashed_details);
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to set environment options provided");
                RETURN_FALSE;
            }    
        }

        /* add it to the list */
        ZEND_REGISTER_RESOURCE(return_value, ii_link, le_ii_link);

        /* add it to the hash */
        new_index_ptr.ptr = (void *) Z_LVAL_P(return_value);
        Z_TYPE(new_index_ptr) = le_index_ptr;
        if (zend_hash_update(&EG(regular_list), hashed_details, hashed_details_length + 1, (void *) &new_index_ptr, sizeof(list_entry), NULL) == FAILURE)
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to hash (%s)", hashed_details);
            free(ii_link);
            efree(hashed_details);
            RETURN_FALSE;
        }
        INGRESG(num_links)++;

        if (INGRESG(trace_connect)) {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Exit non-persistent connection");
        }
    }

    if (hashed_details != NULL)
    {
        efree(hashed_details);
        hashed_details = NULL;
    }

    if (INGRESG(trace_connect)) {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Exit php_ii_do_connect");
    }
} 
/* }}} */

/* {{{ proto resource ingres_connect([string database [, string username [, string password [, options ]]]])
   Open a connection to an Ingres database the syntax of database is [node_id::]dbname[/svr_class] */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_connect)
#else
PHP_FUNCTION(ingres_connect)
#endif
{
    php_ii_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);

}
/* }}} */

/* {{{ proto resource ingres_pconnect([string database [, string username [, string password [, options ]]]])
   Open a persistent connection to an Ingres database the syntax of database is [node_id::]dbname[/svr_class] */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_pconnect)
#else
PHP_FUNCTION(ingres_pconnect)
#endif
{
    php_ii_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto bool ingres_close(resource link)
   Close an Ingres database connection */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_close)
#else
PHP_FUNCTION(ingres_close)
#endif
{
    zval *link = NULL;
    II_LINK *ii_link;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &link) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, &link, -1, "Ingres Link", le_ii_link, le_ii_plink);

    /* Call the clean function synchronously here */
    /* Otherwise we have to wait for request shutdown */
    /* This way we can reuse the link in the same script */
    _ai_clean_ii_plink(ii_link TSRMLS_CC);
  
    /* explicit resource number */
    zend_list_delete(Z_RESVAL_P(link));
    
    RETURN_TRUE;
}
/* }}} */

/* {{{ static void php_ii_query(INTERNAL_FUNCTION_PARAMETERS, int buffered) */
static void php_ii_query(INTERNAL_FUNCTION_PARAMETERS, int buffered)
{
    zval *link = NULL;
    zval *queryParams = NULL;
    char *query = NULL;
    char *paramtypes = NULL;
    int query_len=0;
    int paramtypes_len=0;
    II_LINK *ii_link = {0};
    II_RESULT *ii_result = {0};
    IIAPI_QUERYPARM     queryParm;
    IIAPI_GETDESCRPARM  getDescrParm;
    IIAPI_CANCELPARM   cancelParm;
    IIAPI_CLOSEPARM    closeParm;

    HashTable *arr_hash;
    int elementCount;
    char *query_ptr = NULL;
    char *converted_query = NULL;
    int query_type;
    int converted_query_len;
    ii_result_entry *result_ptr = NULL;
    ii_result_entry *next_ptr = NULL;
    short int result_resource = FALSE;
    int type = 0;
    int col = 0;
    short int canPrepare = 0; /* Indicates if we can issue a PREPARE against the statement */
    long pad_bytes = 0;

    ii_result_entry *result_entry;
    

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"rs|as" , &link, &query, &query_len, &queryParams, &paramtypes, &paramtypes_len) == FAILURE) 
    {
        RETURN_FALSE;
    }
    
    ZEND_FETCH_RESOURCE2(ii_link, II_LINK*, &link, -1, "Ingres Link", le_ii_link, le_ii_plink);

    /* Check to see if the last ingres function generated an error */
    /* If so close off the statement */
    if (ii_link->stmtHandle && INGRESG(errorHandle))
    {
        /* see if we can close the query without cancelling it */
        /* Free query resources. */
        closeParm.cl_genParm.gp_callback = NULL;
        closeParm.cl_genParm.gp_closure = NULL;
        closeParm.cl_stmtHandle = ii_link->stmtHandle;

        IIapi_close(&closeParm);
        ii_sync(&(closeParm.cl_genParm));

        if (ii_success(&(closeParm.cl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
        {
            /* unable to close */
            /* Cancel query processing. */
            cancelParm.cn_genParm.gp_callback = NULL;
            cancelParm.cn_genParm.gp_closure = NULL;
            cancelParm.cn_stmtHandle = ii_result->stmtHandle;

            IIapi_cancel(&cancelParm );
            ii_sync(&(cancelParm.cn_genParm));

            /* Free query resources. */
            closeParm.cl_genParm.gp_callback = NULL;
            closeParm.cl_genParm.gp_closure = NULL;
            closeParm.cl_stmtHandle = ii_result->stmtHandle;

            IIapi_close( &closeParm );
            ii_sync(&(closeParm.cl_genParm));

            if (ii_success(&(closeParm.cl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close a previously executed query");
                RETURN_FALSE;
            }
        }
        ii_link->stmtHandle = NULL;
    }

    /* Ingres only allows for a single query opened with IIAPI_QT_QUERY */
    /* Check to see any existing result-sets are unbuffered and close them */
    if (ii_link->result_list_ptr)
    {
        result_ptr = (ii_result_entry *)ii_link->result_list_ptr;
        while (result_ptr) 
        {
            ii_result = (II_RESULT *)zend_list_find(result_ptr->result_id, &type);
            next_ptr = (ii_result_entry *)result_ptr->next_result_ptr;
            if ((ii_result) && (type == le_ii_result))
            {
                if (ii_result->buffered == FALSE)
                {
                    /* Close the statement */
                    if (INGRESG(ingres_trace))
                    {
                        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Closing previous unbuffered query");
                    }
                    if (_close_statement(ii_result TSRMLS_CC) == II_FAIL)
                    {
                         php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close statement");
                         RETURN_FALSE;
                    }
                    php_ii_result_remove (ii_result, result_ptr->result_id TSRMLS_CC);
                }
            }
            /* Next */
            result_ptr = next_ptr;
        }
    }

    /* If the last statement produced no rows we should call IIapi_close() else it will be lost */
    /* Row returning statements will have a result resource that references the stmtHandle. */
    if (ii_link->stmtHandle != NULL)
    {
        /* Is this stmtHandle associated with a result resource? */
        if (ii_link->result_list_ptr)
        {
            result_ptr = (ii_result_entry *)ii_link->result_list_ptr;
            while (result_ptr) 
            {
                ii_result = (II_RESULT *)zend_list_find(result_ptr->result_id, &type);
                if ((ii_result) && (type == le_ii_result))
                {
                    if (ii_result->stmtHandle == ii_link->stmtHandle)
                    {
                        /* Found it */
                        result_resource = TRUE;
                        break;
                    }
                }
                /* Next */
                result_ptr = (ii_result_entry *)result_ptr->next_result_ptr;
            }
        }
        if (result_resource == FALSE)
        {
            /* Close the statement since it is not associated with any current resource */
            if (INGRESG(ingres_trace))
            {
                php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Closing a non-result resource statHandle");
            }

            /* Cancel query processing. */
            cancelParm.cn_genParm.gp_callback = NULL;
            cancelParm.cn_genParm.gp_closure = NULL;
            cancelParm.cn_stmtHandle = ii_link->stmtHandle;

            IIapi_cancel(&cancelParm );

            ii_sync(&(cancelParm.cn_genParm));

            /* Free query resources. */
            closeParm.cl_genParm.gp_callback = NULL;
            closeParm.cl_genParm.gp_closure = NULL;
            closeParm.cl_stmtHandle = ii_link->stmtHandle;

            IIapi_close( &closeParm );

            ii_sync(&(closeParm.cl_genParm));

            switch((closeParm.cl_genParm).gp_status)
            {
                case IIAPI_ST_SUCCESS:
                case IIAPI_ST_NO_DATA:
                    ii_link->stmtHandle = NULL;
                    break;
                default:
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error: %d - Unable to close non-result resource statement", (closeParm.cl_genParm).gp_status);
            }
        }
    }

    /* Allocate and initialize the memory for the new result resource */
    ii_result = (II_RESULT *) malloc(sizeof(II_RESULT));
    _ii_init_result(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, ii_link);

    /* determine what sort of query is being executed */
    query_type = php_ii_query_type(query TSRMLS_CC);

    switch (query_type)
    {
        case INGRES_SQL_SELECT:
            if (buffered)
            {
                ii_result->buffered = TRUE;
                ii_result->queryType = IIAPI_QT_OPEN;
            }
            else
            {
                ii_result->queryType = IIAPI_QT_QUERY;
            }
            canPrepare = 1;
            break;
        case INGRES_SQL_INSERT:
        case INGRES_SQL_UPDATE:
        case INGRES_SQL_DELETE:
            canPrepare = 1;
            ii_result->queryType = IIAPI_QT_QUERY;
            break;
        case INGRES_SQL_CREATE:
        case INGRES_SQL_ALTER:
            ii_result->queryType = IIAPI_QT_QUERY;
            break;
        case INGRES_SQL_COMMIT:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Use ingres_commit() to commit a transaction");
            RETURN_FALSE;
            break;
        case INGRES_SQL_ROLLBACK:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Use ingres_rollback() to rollback a transaction");
            RETURN_FALSE;
            break;
        case INGRES_SQL_OPEN:
        case INGRES_SQL_CLOSE:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Use ingres_prepare() and ingres_execute() for cursor operations");
            RETURN_FALSE;
            break;
        case INGRES_SQL_CONNECT:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Use ingres_connect() to connect to a database");
            RETURN_FALSE;
            break;
        case INGRES_SQL_DISCONNECT:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Use ingres_close() to disconnect from a database");
            RETURN_FALSE;
            break;
        case INGRES_SQL_GETDBEVENT:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres 'GET DBEVENT' is not supported at the current time");
            RETURN_FALSE;
            break;
        case INGRES_SQL_SAVEPOINT:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres 'SAVEPOINT' is not supported at the current time");
            RETURN_FALSE;
            break;
        case INGRES_SQL_AUTOCOMMIT:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Use ingres_autocommit() to set the auto-commit state");
            RETURN_FALSE;
            break;
        case INGRES_SQL_EXECUTE_PROCEDURE:
        case INGRES_SQL_CALL:
            ii_result->queryType = IIAPI_QT_EXEC_PROCEDURE;
            break;
        case INGRES_SQL_COPY:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres 'COPY TABLE() INTO/FROM' is not supported at the current time");
            RETURN_FALSE;
            break;
        default:
            ii_result->queryType = IIAPI_QT_QUERY;
            break;
    }

    if (INGRESG(ingres_trace))
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s", query);
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s:%d, ac-state:%d, ac-emulation:%d",INGRES_INI_AUTO, INGRESG(auto_multi), ii_link->autocommit, ii_link->auto_multi);
    }

    /* Ingres only allows for a single cursor to be open when auto-commit is enabled */
    /* To allow multiple open cursors we disable auto-commit before opening the first cursor */
    /* auto-commit is restarted when all resultsets have been freed. The last freed resultset */
    /* invokes a commit before auto-commit is re-enabled */

    /* If we are to simulate auto-commit */ 
    if ((INGRESG(auto_multi)) && ((ii_link->autocommit) || (ii_link->auto_multi)))
    {
        if (ii_link->auto_multi) /* We are in emulation mode */
        {
            /* if there are active resultsets then we don't want to commit */
            if ((query_type != INGRES_SQL_SELECT) && (!ii_link->result_list_ptr))
            {
                /* re-enable auto-commit */
                if ( ii_link->result_list_ptr )
                {
                    /* clean up any un-freed statements/results */
                    _free_ii_link_result_list(ii_link TSRMLS_CC);
                }
                if (INGRESG(ingres_trace))
                {
                    php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Issuing commit whilst in auto-commit emulation mode");
                }
                /* commit the previous statement before changing the auto-commit state */
                if (_commit_transaction(ii_link TSRMLS_CC) == II_FAIL)
                {
                    php_error_docref(NULL TSRMLS_CC, E_ERROR, "An error occur when issuing an internal commit");
                }
                if (INGRESG(ingres_trace))
                {
                    php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Re-activating auto-commit");
                }
                if (_autocommit_transaction(ii_link TSRMLS_CC) == II_FAIL)
                {
                    ii_link->auto_multi = 1;
                    php_error_docref(NULL TSRMLS_CC, E_ERROR, "An error occur when changing the auto-commit state");
                }
                /* Update the transaction handle */
                ii_result->tranHandle = ii_link->tranHandle;
                ii_link->auto_multi = 0;
            }
        }
        else /* We are not in emulation mode */
        {
            if (query_type == INGRES_SQL_SELECT)
            {
                /* with multiple cursors and auto-commit is enabled */
                if (ii_link->autocommit) 
                {
                    /* If there are any active transactions they must be freed */
                    if ( ii_link->result_list_ptr )
                    {
                        /* clean up any un-freed statements/results */
                        _free_ii_link_result_list(ii_link TSRMLS_CC);
                    }
                    if (INGRESG(ingres_trace))
                    {
                        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Disabling auto-commit");
                    }
                    if (_autocommit_transaction(ii_link TSRMLS_CC) == II_FAIL)
                    {
                        php_error_docref(NULL TSRMLS_CC, E_ERROR, "An error occur when changing the auto-commit state");
                        ii_link->auto_multi = 0;
                    }
                    /* Update the transaction handle */
                    ii_result->tranHandle = ii_link->tranHandle;
                    /* we are in emulation mode */
                    ii_link->auto_multi = 1;
                }
            }
        }
    }
    if (INGRESG(ingres_trace))
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s:%d, ac-state:%d, ac-emulation:%d",INGRES_INI_AUTO, INGRESG(auto_multi), ii_link->autocommit, ii_link->auto_multi);
    }

    /* check to see if there are any parameters to the query */
    ii_result->paramCount = php_ii_paramcount(query TSRMLS_CC);

    if (ii_result->paramCount)
    {
        if (queryParams==NULL)
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expecting a parameter array but did not get one" );
            RETURN_FALSE;
        }

        arr_hash = Z_ARRVAL_P(queryParams);

        if ((elementCount = zend_hash_num_elements(arr_hash)) != ii_result->paramCount )
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Incorrect number of parameters passed, expected %d got %d",ii_result->paramCount, elementCount );
            RETURN_FALSE;
        }
        
        zend_hash_internal_pointer_reset(Z_ARRVAL_P(queryParams));
        
        /* if we have been provided with type hints for parameters check to see the number of params */
        /* passed matches the number of type hints recieved */
        if (paramtypes)
        {
            if (paramtypes_len != ii_result->paramCount )
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Incorrect number of parameter types received, expected %d got %d", ii_result->paramCount, paramtypes_len );
                RETURN_FALSE;
            }
        }
#if defined(IIAPI_VERSION_5)
        else
        {
            if (((ii_link->apiLevel > IIAPI_LEVEL_3) && INGRESG(describe)) && (canPrepare))
            {
                /* We can use DESCRIBE INPUT to work out the types that Ingres is expecting */
                if (_ii_prepare(ii_result, query TSRMLS_CC) == II_FAIL)
                {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occurred when preparing a query for DESCRIBE INPUT");
                    RETURN_FALSE;
                }
                if (_ii_describe_input (ii_result, query TSRMLS_CC) == II_FAIL)
                {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occurred whilst trying to describe the query");
                    RETURN_FALSE;
                }
            }
        }
#endif
    }

    ii_result->procname = php_ii_check_procedure(query, ii_link TSRMLS_CC);

    /* convert ? to ~V so we don't have to prepare the query */
    if ( ii_result->paramCount > 0  && ii_result->procname == NULL )
    { 
        converted_query = emalloc(query_len + ( ii_result->paramCount*3 ) + 1);
        memcpy (converted_query,  query, query_len);
        converted_query[query_len] ='\0';
        php_ii_convert_param_markers( query, converted_query TSRMLS_CC );
    }

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = ii_result->connHandle;
    queryParm.qy_tranHandle = ii_result->tranHandle;
    queryParm.qy_stmtHandle = NULL;
#if defined(IIAPI_VERSION_6)
    queryParm.qy_flags  = 0;
#endif
    queryParm.qy_queryType  = ii_result->queryType;
    if ( ii_result->paramCount  || ii_result->queryType == IIAPI_QT_EXEC_PROCEDURE)
    {
        queryParm.qy_parameters = TRUE;
    }
    else
    {
        queryParm.qy_parameters = FALSE;
    }

    switch(ii_result->queryType)
    {
        case IIAPI_QT_OPEN:
#if defined(IIAPI_VERSION_6)
            if (INGRESG(scroll) && (ii_link->apiLevel >= IIAPI_LEVEL_5))
            {
                /* Enable scrolling cursor support */
                queryParm.qy_flags  = IIAPI_QF_SCROLL;
                ii_result->scrollable = 1;
            }
#endif
            if ( converted_query != NULL )
            {
                converted_query_len  =  strlen(converted_query);
                converted_query = erealloc(converted_query, (converted_query_len + 16));
                query_ptr = &(converted_query[converted_query_len]);
            }
            else
            {
                converted_query = emalloc(query_len + 16);
                memcpy (converted_query,  query, query_len);
                query_ptr = &(converted_query[query_len]);
            }
            sprintf (query_ptr," for readonly");
            ii_result->cursor_mode = II_CURSOR_READONLY;
            /* fall through */
        case IIAPI_QT_QUERY:
            /* query has params or is a read-only cursor or both */
            if ( converted_query != NULL )
            {
                queryParm.qy_queryText = converted_query;
            }
            else
            {
                queryParm.qy_queryText = query;
            }
            break;
        case IIAPI_QT_EXEC_PROCEDURE:
            queryParm.qy_parameters = TRUE;
            queryParm.qy_queryText  = NULL;
            break;
        default:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to execute OpenAPI query type, %d", ii_result->queryType);
            RETURN_FALSE;
            break;
    }

    IIapi_query(&queryParm);
    ii_sync(&(queryParm.qy_genParm));

    if (ii_success(&(queryParm.qy_genParm), &ii_link->errorHandle TSRMLS_CC) == II_FAIL)
    {
        RETURN_FALSE;
    } 

    /* store transaction and statement handles */
    ii_result->tranHandle = queryParm.qy_tranHandle;
    ii_result->stmtHandle = queryParm.qy_stmtHandle;
    ii_link->tranHandle = queryParm.qy_tranHandle;
    ii_link->stmtHandle = queryParm.qy_stmtHandle;

    if ( ii_result->paramCount > 0  ||  ii_result->queryType == IIAPI_QT_EXEC_PROCEDURE )
    {
        if (php_ii_bind_params (INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, queryParams, paramtypes) == II_FAIL)
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,"Error binding parameters");
            efree(converted_query);
            RETURN_FALSE;
        }
    }

    /* get description of results */
    if ( query_type == INGRES_SQL_SELECT )
    {
        getDescrParm.gd_genParm.gp_callback = NULL;
        getDescrParm.gd_genParm.gp_closure  = NULL;
        getDescrParm.gd_stmtHandle = ii_result->stmtHandle;

        IIapi_getDescriptor(&getDescrParm);
        ii_sync(&(getDescrParm.gd_genParm));

        if (ii_success(&(getDescrParm.gd_genParm), &ii_link->errorHandle TSRMLS_CC) == II_FAIL)
        {
            if (converted_query) 
            {
                efree(converted_query);
                converted_query = NULL;
            }
            if (ii_result->cursor_id)
            {
                efree(ii_result->cursor_id);
                ii_result->cursor_id = NULL;
            }
            if (ii_result->inputDescr)
            {
                efree(ii_result->inputDescr);
                ii_result->inputDescr = NULL;
            }
            if (ii_result)
            {
                free(ii_result);
                ii_result = NULL;
            }
            RETURN_FALSE;
        }

        ii_link->stmtHandle = NULL;

        /* store the results */
        ii_result->fieldCount = getDescrParm.gd_descriptorCount;
        ii_result->descriptor = getDescrParm.gd_descriptor;
        ii_result->link_id = Z_LVAL_P(link);
        ii_result->rowWidth = ii_result_row_width(ii_result);
    }

    if ( ii_result->paramCount > 0  && ii_result->procname == NULL )
    { 
        efree(converted_query);
        converted_query = NULL;
    }

    ZEND_REGISTER_RESOURCE(return_value, ii_result, le_ii_result)

    /* Add details of the II_RESULT resource to the II_LINK resource for later clean up */

    result_entry = (ii_result_entry *)malloc(sizeof(ii_result_entry));
    if (result_entry == NULL )
    {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Unable to create memory for result_entry");
        RETURN_FALSE;
    }

    if (ii_link->result_list_ptr == NULL)
    {
        ii_link->result_list_ptr = result_entry;
        result_entry->next_result_ptr = NULL;
        result_entry->result_id = Z_LVAL_P(return_value); /* resource id */
    }
    else
    {   
        /* insert the new result_entry to the head of the result_list */
        result_entry->next_result_ptr = (char *)ii_link->result_list_ptr;
        ii_link->result_list_ptr = result_entry;
        result_entry->result_id = Z_LVAL_P(return_value); /* resource id */
    }

    if (converted_query) 
    {
        efree(converted_query);
        converted_query = NULL;
    }
}
/* }}} */

/* {{{ proto mixed ingres_query(resource link, string query [, array queryParams] [, array paramtypes] ) */
/* Send a SQL query to Ingres */
/* This should go into the documentation */
/* Unsupported query types:
   - close
   - commit
   - connect
   - disconnect
   - get dbevent
   - prepare to commit
   - rollback
   - savepoint
   - set autocommit
   - <all cursor related queries>
   (look for dedicated functions instead) */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_query)
#else
PHP_FUNCTION(ingres_query)
#endif
{
    php_ii_query(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto mixed ingres_unbuffered_query(resource link, string query [, array queryParams] [, array paramtypes] ) */
/* Send an unbuffered SQL query to Ingres */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_unbuffered_query)
#else
PHP_FUNCTION(ingres_unbuffered_query)
#endif
{
    php_ii_query(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto mixed ingres_prepare(resource link, string query) */
/* Prepare SQL for later execution */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_prepare)
#else
PHP_FUNCTION(ingres_prepare)
#endif
{
    zval  *link;
    char *query;
    II_LINK *ii_link;
    II_RESULT *ii_result;

    IIAPI_QUERYPARM     queryParm;
    IIAPI_GETQINFOPARM  getQInfoParm;
    IIAPI_CANCELPARM    cancelParm;
    IIAPI_CLOSEPARM     closeParm;

    int query_len=0;
    char *statement;
    char *preparedStatement;

    int cursor_id_len;

    ii_result_entry *result_entry;
    int query_type;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"rs" , &link, &query, &query_len) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, &link, -1, "Ingres Link", le_ii_link, le_ii_plink);

    /* Check to see if the last ingres function generated an error */
    /* If so close off the statement */
    if (ii_link->stmtHandle && INGRESG(errorHandle))
    {
        /* see if we can close the query without cancelling it */
        /* Free query resources. */
        closeParm.cl_genParm.gp_callback = NULL;
        closeParm.cl_genParm.gp_closure = NULL;
        closeParm.cl_stmtHandle = ii_link->stmtHandle;

        IIapi_close(&closeParm);
        ii_sync(&(closeParm.cl_genParm));

        if (ii_success(&(closeParm.cl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
        {
            /* unable to close */
            /* Cancel query processing. */
            cancelParm.cn_genParm.gp_callback = NULL;
            cancelParm.cn_genParm.gp_closure = NULL;
            cancelParm.cn_stmtHandle = ii_result->stmtHandle;

            IIapi_cancel(&cancelParm );
            ii_sync(&(cancelParm.cn_genParm));

            /* Free query resources. */
            closeParm.cl_genParm.gp_callback = NULL;
            closeParm.cl_genParm.gp_closure = NULL;
            closeParm.cl_stmtHandle = ii_result->stmtHandle;

            IIapi_close( &closeParm );
            ii_sync(&(closeParm.cl_genParm));

            if (ii_success(&(closeParm.cl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close a previously executed query");
                RETURN_FALSE;
            }
        }
        ii_link->stmtHandle = NULL;
    }

    /* Allocate and initialize the memory for the new result resource */
    ii_result = (II_RESULT *) malloc(sizeof(II_RESULT));
    _ii_init_result(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, ii_link);

    /* determine what sort of query is being executed */
    query_type = php_ii_query_type(query TSRMLS_CC);

    switch (query_type)
    {
        case INGRES_SQL_SELECT:
            ii_result->queryType = IIAPI_QT_OPEN;
            break;
        case INGRES_SQL_INSERT:
        case INGRES_SQL_UPDATE:
        case INGRES_SQL_DELETE:
            ii_result->queryType = IIAPI_QT_EXEC;
            break;
        case INGRES_SQL_CREATE:
        case INGRES_SQL_ALTER:
        case INGRES_SQL_COMMIT:
        case INGRES_SQL_ROLLBACK:
        case INGRES_SQL_OPEN:
        case INGRES_SQL_CLOSE:
        case INGRES_SQL_CONNECT:
        case INGRES_SQL_DISCONNECT:
        case INGRES_SQL_GETDBEVENT:
        case INGRES_SQL_SAVEPOINT:
        case INGRES_SQL_AUTOCOMMIT:
        case INGRES_SQL_EXECUTE_PROCEDURE:
        case INGRES_SQL_CALL:
        default:
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to execute a %s statement using ingres_prepare() use ingres_query() instead", SQL_COMMANDS[query_type].command );
            RETURN_FALSE;
            break;
    }
    statement = query;

    /* figure how many parameters are expected */
    ii_result->paramCount = php_ii_paramcount(query TSRMLS_CC);
    ii_result->procname = NULL;
    ii_result->prepared = TRUE;

    /* check to see if this is a procedure or not
       load the procedure name into ii_link->procname.
       If ii_link->procname is NULL then there is no procedure */

    ii_result->procname = php_ii_check_procedure(query, ii_link TSRMLS_CC);

    /* Adapt the query into a prepared statement */
    php_ii_gen_cursor_id(ii_result TSRMLS_CC);
    cursor_id_len = strlen(ii_result->cursor_id);
    preparedStatement=ecalloc(query_len + 15 + cursor_id_len, 1);
    sprintf (preparedStatement,"prepare %s from %s", ii_result->cursor_id, statement);
    statement = preparedStatement;

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = ii_result->connHandle;
    queryParm.qy_tranHandle = ii_link->tranHandle;
    queryParm.qy_stmtHandle = NULL;
    queryParm.qy_queryType  = IIAPI_QT_QUERY; 
    queryParm.qy_parameters = FALSE;
    queryParm.qy_queryText  = statement;
#if defined (IIAPI_VERSION_6)
    queryParm.qy_flags = 0;
#endif

    IIapi_query(&queryParm);
    ii_sync(&(queryParm.qy_genParm));

    if (ii_success(&(queryParm.qy_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL) 
    {
        efree(preparedStatement);
        RETURN_FALSE;
    }

    ii_result->tranHandle = queryParm.qy_tranHandle;
    ii_result->stmtHandle = queryParm.qy_stmtHandle;

    /* Update ii_link so we can commit later */
    ii_link->tranHandle = ii_result->tranHandle;
    ii_link->stmtHandle = ii_result->stmtHandle;

    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = ii_result->stmtHandle;

    IIapi_getQueryInfo(&getQInfoParm);
    ii_sync(&(getQInfoParm.gq_genParm));

    if (ii_success(&(getQInfoParm.gq_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        efree(preparedStatement);
        RETURN_FALSE;
    }

    if  (_ii_close( &ii_result->stmtHandle, &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close a prepared statement - %s", statement);
        RETURN_FALSE;
    }
    ii_link->stmtHandle = NULL;

#if defined (IIAPI_VERSION_5)
    if (ii_result->paramCount && ((ii_result->apiLevel > IIAPI_LEVEL_3) && INGRESG(describe)))
    {
        /* Use the result from DESCRIBE INPUT */
        if (_ii_describe_input (ii_result, query TSRMLS_CC) == II_FAIL)
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occurred whilst trying to fetch a description of the input parameters");
            RETURN_FALSE;
        }
    }
    else
    {
        /* DESCRIBE the prepared query */
        if (_ii_describe (ii_result, query TSRMLS_CC) == II_FAIL)
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occurred whilst trying to describe the query");
            RETURN_FALSE;
        }
    }
#else
    /* DESCRIBE the prepared query */
    if (_ii_describe (ii_result, query TSRMLS_CC) == II_FAIL)
    {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occurred whilst trying to describe the query");
        RETURN_FALSE;
    }
#endif

    ZEND_REGISTER_RESOURCE(return_value, ii_result, le_ii_result)

    /* Add details of the II_RESULT resource to the II_LINK resource for later clean up */

    result_entry = (ii_result_entry *)malloc(sizeof(ii_result_entry));
    if (result_entry == NULL )
    {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Unable to create memory for result_entry");
        RETURN_FALSE;
    }

    if (ii_link->result_list_ptr == NULL)
    {
        ii_link->result_list_ptr = result_entry;
        result_entry->next_result_ptr = NULL;
        result_entry->result_id = Z_LVAL_P(return_value); /* resource id */
    }
    else
    {   
        /* insert the new result_entry to the head of the result_list */
        result_entry->next_result_ptr = (char *)ii_link->result_list_ptr;
        ii_link->result_list_ptr = result_entry;
        result_entry->result_id = Z_LVAL_P(return_value); /* resource id */
    }
    efree(preparedStatement);
}
/* }}} */

/* {{{ proto mixed ingres_execute(resource result [,array params,[string paramtypes]]) 
   execute a query prepared by ingres_prepare() */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_execute)
#else
PHP_FUNCTION(ingres_execute)
#endif
{
    zval *result, *queryParams;
    char *paramtypes = NULL;
    int paramtypes_len;
    II_RESULT *ii_result;
    IIAPI_QUERYPARM     queryParm;
    IIAPI_GETDESCRPARM  getDescrParm;
    IIAPI_CANCELPARM    cancelParm;
    IIAPI_CLOSEPARM     closeParm;

    int                 col;

    int elementCount;
    char *statement = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r|as" , &result, &queryParams, &paramtypes, &paramtypes_len) == FAILURE) 
    {
        RETURN_FALSE;
    }
        
    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres Result", le_ii_result);

    /* Bail if passed a non-prepared query */
    if (ii_result->prepared == FALSE)
    {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot execute a non-prepared query" );
        RETURN_FALSE;
    }

    /* Check if we have already executed a query against this prepared statement */
    if (ii_result->executed)
    {
        if (ii_result->stmtHandle)
        {
            /* Release any resultset data */
            _free_resultdata (ii_result);

            /* see if we can close the query without cancelling it */
            /* Free query resources. */
            closeParm.cl_genParm.gp_callback = NULL;
            closeParm.cl_genParm.gp_closure = NULL;
            closeParm.cl_stmtHandle = ii_result->stmtHandle;

            IIapi_close(&closeParm);
            ii_sync(&(closeParm.cl_genParm));

            if (ii_success(&(closeParm.cl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
            {
                /* unable to close */
                /* Cancel query processing. */
                cancelParm.cn_genParm.gp_callback = NULL;
                cancelParm.cn_genParm.gp_closure = NULL;
                cancelParm.cn_stmtHandle = ii_result->stmtHandle;

                IIapi_cancel(&cancelParm );

                ii_sync(&(cancelParm.cn_genParm));

                /* Free query resources. */
                closeParm.cl_genParm.gp_callback = NULL;
                closeParm.cl_genParm.gp_closure = NULL;
                closeParm.cl_stmtHandle = ii_result->stmtHandle;

                IIapi_close( &closeParm );

                ii_sync(&(closeParm.cl_genParm));

                if (ii_success(&(closeParm.cl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
                {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close a previously executed query");
                    RETURN_FALSE;
                }
            }
        }
    }

    if ( ii_result->paramCount > 0 )
    {
        if (Z_TYPE_P(queryParams) != IS_ARRAY )
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expecting a parameter array but did not get one" );
            RETURN_FALSE;
        }

        if ((elementCount = zend_hash_num_elements(Z_ARRVAL_P(queryParams))) != ii_result->paramCount )
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Incorrect number of parameters passed, expected %d got %d",ii_result->paramCount, elementCount );
            RETURN_FALSE;
        }
        zend_hash_internal_pointer_reset(Z_ARRVAL_P(queryParams));
    }

    statement = ecalloc(strlen(ii_result->cursor_id) + 25, 1);
    if (ii_result->queryType == IIAPI_QT_OPEN)
    {
        if (ii_result->cursor_mode == II_CURSOR_READONLY)
        {
            sprintf (statement,"%s for readonly", ii_result->cursor_id );
        }
        else
        {
            sprintf (statement,"%s", ii_result->cursor_id );
        }
    }
    else
    {
        sprintf (statement,"execute %s", ii_result->cursor_id );
    }

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = ii_result->connHandle;
    queryParm.qy_tranHandle = ii_result->tranHandle;
    queryParm.qy_stmtHandle = NULL;
    queryParm.qy_queryType  = ii_result->queryType; 
    queryParm.qy_queryText  = statement; 
    if ((ii_result->paramCount) || (ii_result->queryType == IIAPI_QT_OPEN))
    {
        queryParm.qy_parameters = TRUE;
    }
    else
    {
        queryParm.qy_parameters = FALSE;
    }
#if defined (IIAPI_VERSION_6)
    queryParm.qy_flags = 0;
    if (INGRESG(scroll) && (ii_result->apiLevel >= IIAPI_LEVEL_5))
    {
        /* Enable scrolling cursor support */
        queryParm.qy_flags  = IIAPI_QF_SCROLL;
        ii_result->scrollable = 1;
    }
        queryParm.qy_flags = 0;
#endif

    IIapi_query(&queryParm);
    ii_sync(&(queryParm.qy_genParm));

    if (ii_success(&(queryParm.qy_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        if ( ii_result->procname == NULL )
        {
            efree (statement);
        }
        RETURN_FALSE;
    }

    ii_result->tranHandle = queryParm.qy_tranHandle;
    ii_result->stmtHandle = queryParm.qy_stmtHandle;
    ii_result->executed = TRUE;

    if ((ii_result->paramCount) || (ii_result->queryType == IIAPI_QT_OPEN))
    {
        if (php_ii_bind_params (INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, queryParams, paramtypes) == II_FAIL)
        {
            if (ii_result->inputCount)
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING,"Error occurred whilst binding pre-described parameters");
            }
            else
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING,"Error binding parameters using user supplied parameter types");
            }
            if (statement)
            {
                efree (statement);
            }
            RETURN_FALSE;
        }
    } 

    /* store transaction and statement handles */
    ii_result->tranHandle = queryParm.qy_tranHandle;
    ii_result->stmtHandle = queryParm.qy_stmtHandle;
    
    /* get description of results */
    getDescrParm.gd_genParm.gp_callback = NULL;
    getDescrParm.gd_genParm.gp_closure  = NULL;
    getDescrParm.gd_stmtHandle = ii_result->stmtHandle;

    IIapi_getDescriptor(&getDescrParm);
    ii_sync(&(getDescrParm.gd_genParm));

    if (ii_success(&(getDescrParm.gd_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        if ( ii_result->procname == NULL )
        {
            efree (statement);
        }
        RETURN_FALSE;
    }

    /* store the results */
    ii_result->fieldCount = getDescrParm.gd_descriptorCount;
    ii_result->descriptor = getDescrParm.gd_descriptor;
    ii_result->rowWidth = ii_result_row_width(ii_result);

    if ( ii_result->procname == NULL )
    {
        efree (statement);
    }
    RETURN_TRUE;

}
/* }}} */

/* {{{ php_ii_gen_cursor_id(char *cursor_id TSRMLS_DC) 
   generates a cursor name */
static void  php_ii_gen_cursor_id(II_RESULT *ii_result TSRMLS_DC)
{
    char *tmp_id = '\0';
    unsigned long thread_id;

    if (ii_result->cursor_id != NULL)
    {
        efree(ii_result->cursor_id);
        ii_result->cursor_id = NULL;
    }    

    /* remove potential for a negative number */ 
    thread_id = II_THREAD_ID;

    tmp_id = ecalloc (33,1);
    INGRESG(cursor_no)++;
    sprintf (tmp_id,"php_%lu_%ld", II_THREAD_ID, INGRESG(cursor_no));
    ii_result->cursor_id = emalloc(strlen(tmp_id) + 1);
    strcpy(ii_result->cursor_id,tmp_id);
    efree(tmp_id);
}
/* }}} */

/* {{{ proto string ingres_cursor(resource result)
   Gets a cursor name for a given result resource */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_cursor)
#else
PHP_FUNCTION(ingres_cursor)
#endif
{
    zval *result; 
    II_RESULT *ii_result;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &result) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres Result", le_ii_result);

    RETURN_STRING(ii_result->cursor_id,1);
}
/* }}} */

/* {{{ proto int ingres_num_rows(resource result)
   Return the number of rows affected/returned by the last query */

/* Warning : don't call ingres_num_rows() before ingres_fetch_xx() since IIapi_getQueryInfo()
 * nullifies the data set being returned and ingres_fetch_xx() will not find any data 
 * TODO : Add some mechanism to allow a row count to be returned */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_num_rows)
#else
PHP_FUNCTION(ingres_num_rows)
#endif
{
    zval *result;
    II_RESULT *ii_result;
    IIAPI_GETQINFOPARM getQInfoParm;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &result) == FAILURE) 
    {
        RETURN_FALSE;
    }
    
    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres Result", le_ii_result);

#if defined(IIAPI_VERSION_6)


    if (ii_result->scrollable)
    {
        if (php_ii_scroll_row_count (ii_result TSRMLS_CC) == II_FAIL)
        {
            RETURN_FALSE;
        }
        RETURN_LONG(ii_result->rowCount);
    }
    else
    {
        /* FIXME - When a SELECT is opened using IIAPI_QT_OPEN and is not scrollable this does not work */
        /* get number of affected rows */
        getQInfoParm.gq_genParm.gp_callback = NULL;
        getQInfoParm.gq_genParm.gp_closure = NULL;
        getQInfoParm.gq_stmtHandle = ii_result->stmtHandle;

        IIapi_getQueryInfo(&getQInfoParm);
        ii_sync(&(getQInfoParm.gq_genParm));

        if (ii_success(&(getQInfoParm.gq_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
        {
            RETURN_FALSE;
        }
        if (getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT)
        {
            RETURN_LONG(getQInfoParm.gq_rowCount);
        } else {
            RETURN_LONG(0);
        }
    }
#else
    /* get number of affected rows */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = ii_result->stmtHandle;

    IIapi_getQueryInfo(&getQInfoParm);
    ii_sync(&(getQInfoParm.gq_genParm));

    if (ii_success(&(getQInfoParm.gq_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        RETURN_FALSE;
    }
    if (getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT)
    {
        RETURN_LONG(getQInfoParm.gq_rowCount);
    } else {
        RETURN_LONG(0);
    }
#endif
}
/* }}} */

/* {{{ proto int ingres_num_fields(resource result)
   Return the number of fields returned by the last query */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_num_fields)
#else
PHP_FUNCTION(ingres_num_fields)
#endif
{
    zval *result;
    II_RESULT *ii_result;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &result) == FAILURE) 
    {
        RETURN_FALSE;
    }
    
    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres Result", le_ii_result);

    RETURN_LONG(ii_result->fieldCount);
}
/* }}} */

/* {{{ static void php_ii_field_info(INTERNAL_FUNCTION_PARAMETERS, int info_type)
 *  Return information about a field in a query result
*/
static void php_ii_field_info(INTERNAL_FUNCTION_PARAMETERS, II_RESULT *ii_result, long index, int info_type)
{
    char *name;
    int columnNo = 0;

    /* Make adjustments to the supplied index/offset value depending on the value of array_index_start */
    if (index < INGRESG(array_index_start)  || (index > (ii_result->fieldCount + (INGRESG(array_index_start) - 1))))
    {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "%ld is an invalid offset. Offset should be between %ld and %ld", index, INGRESG(array_index_start), ii_result->fieldCount + (INGRESG(array_index_start) - 1));
        RETURN_FALSE;
    }
    columnNo = index - INGRESG(array_index_start) ;

    switch (info_type)
    {
        case II_FIELD_INFO_NAME:
            name = php_ii_field_name(ii_result, index TSRMLS_CC);
            if (name == NULL)
            {
                RETURN_FALSE;
            }
            RETURN_STRING(name, 1);
            break;

        case II_FIELD_INFO_TYPE:
            switch ((ii_result->descriptor[columnNo]).ds_dataType)
            {
                case IIAPI_BYTE_TYPE:
                    RETURN_STRING("IIAPI_BYTE_TYPE", 1);
    
                case IIAPI_CHA_TYPE:
                    RETURN_STRING("IIAPI_CHA_TYPE", 1);
    
                case IIAPI_CHR_TYPE:
                    RETURN_STRING("IIAPI_CHR_TYPE", 1);

                case IIAPI_DEC_TYPE:
                    RETURN_STRING("IIAPI_DEC_TYPE", 1);
    
                case IIAPI_DTE_TYPE:
                    RETURN_STRING("IIAPI_DTE_TYPE", 1);

#if defined(IIAPI_VERSION_5) 
                case IIAPI_ADATE_TYPE: /* SQL Standard DATE */
                    RETURN_STRING("IIAPI_ADATE_TYPE", 1);

                case IIAPI_TIME_TYPE: /* Ingres Time */
                    RETURN_STRING("IIAPI_TIME_TYPE", 1);

                case IIAPI_TMWO_TYPE: /* Time without Timezone */
                    RETURN_STRING("IIAPI_TMWO_TYPE", 1);

                case IIAPI_TMTZ_TYPE: /* Time with Timezone */
                    RETURN_STRING("IIAPI_TMTZ_TYPE", 1);

                case IIAPI_TS_TYPE: /* Ingres Timestamp */
                    RETURN_STRING("IIAPI_TS_TYPE", 1);

                case IIAPI_TSWO_TYPE: /* Timestamp without Timezone */
                    RETURN_STRING("IIAPI_TSWO_TYPE", 1);

                case IIAPI_TSTZ_TYPE: /* Timestamp with Timezone */
                    RETURN_STRING("IIAPI_TSTZ_TYPE", 1);

                case IIAPI_INTYM_TYPE: /* Interval Year to Month */
                    RETURN_STRING("IIAPI_INTYM_TYPE", 1);

                case IIAPI_INTDS_TYPE: /* Interval Day to Second */
                    RETURN_STRING("IIAPI_INTDS_TYPE", 1);

#endif
                case IIAPI_FLT_TYPE:
                    RETURN_STRING("IIAPI_FLT_TYPE", 1);
    
                case IIAPI_INT_TYPE:
                    RETURN_STRING("IIAPI_INT_TYPE", 1);
    
                case IIAPI_LOGKEY_TYPE:
                    RETURN_STRING("IIAPI_LOGKEY_TYPE", 1);
    
                case IIAPI_LBYTE_TYPE:
                    RETURN_STRING("IIAPI_LBYTE_TYPE", 1);
    
                case IIAPI_LVCH_TYPE:
                    RETURN_STRING("IIAPI_LVCH_TYPE", 1);
    
                case IIAPI_MNY_TYPE:
                    RETURN_STRING("IIAPI_MNY_TYPE", 1);
    
                case IIAPI_TABKEY_TYPE:
                    RETURN_STRING("IIAPI_TABKEY_TYPE", 1);
    
                case IIAPI_TXT_TYPE:
                    RETURN_STRING("IIAPI_TXT_TYPE", 1);
    
                case IIAPI_VBYTE_TYPE:
                    RETURN_STRING("IIAPI_VBYTE_TYPE", 1);
    
                case IIAPI_VCH_TYPE:
                    RETURN_STRING("IIAPI_VCH_TYPE", 1);
#if defined(IIAPI_VERSION_3)
                case IIAPI_NCHA_TYPE:
                    RETURN_STRING("IIAPI_NCHA_TYPE", 1);

                case IIAPI_NVCH_TYPE:
                    RETURN_STRING("IIAPI_NVCH_TYPE", 1);

                case IIAPI_LNVCH_TYPE:
                    RETURN_STRING("IIAPI_LNVCH_TYPE", 1);
#endif
        
                default:
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown Ingres data type, %d",(ii_result->descriptor[columnNo]).ds_dataType);
                    RETURN_FALSE;
                    break;
            }
            break;

        case II_FIELD_INFO_NULLABLE:
            if ((ii_result->descriptor[columnNo]).ds_nullable)
            {
                RETURN_TRUE;
            } else {
                RETURN_FALSE;
            }
            break;

        case II_FIELD_INFO_LENGTH:
            RETURN_LONG((ii_result->descriptor[columnNo]).ds_length);
            break;
    
        case II_FIELD_INFO_PRECISION:
            RETURN_LONG((ii_result->descriptor[columnNo]).ds_precision);
            break;

        case II_FIELD_INFO_SCALE:
            RETURN_LONG((ii_result->descriptor[columnNo]).ds_scale);
            break;
    
        default:
            RETURN_FALSE;
    }
}

/* }}} */

/* {{{ static char *php_ii_field_name(II_RESULT *ii_result, int index TSRMLS_DC)
  Return the name of a field in a query result */
static char *php_ii_field_name(II_RESULT *ii_result, int index TSRMLS_DC)
{
    char space;
    
    space = ' ';

    if ( (ii_result->descriptor[index - INGRESG(array_index_start)]).ds_columnName != NULL )
    {
        /* if the column name has a space */
        if ( strchr((ii_result->descriptor[index - INGRESG(array_index_start)]).ds_columnName, space ) != NULL ) 
        {
            sprintf(ii_result->descriptor[index - INGRESG(array_index_start)].ds_columnName,"col%d",index); 
        }
        /* If we have created the memory ourselves */
        if ( (ii_result->descriptor[index - INGRESG(array_index_start)]).ds_columnName[0]  == '\0' ) 
        {
            sprintf(ii_result->descriptor[index - INGRESG(array_index_start)].ds_columnName,"col%d",index); 
        }
    }

    return (ii_result->descriptor[index - INGRESG(array_index_start)]).ds_columnName;
}
/* }}} */

/* {{{ proto string ingres_field_name(resource result, int index)
   Return the name of a field in a query result index must be >0 and <= ingres_num_fields() */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_field_name)
#else
PHP_FUNCTION(ingres_field_name)
#endif
{
    zval *result = NULL;
    II_RESULT *ii_result;
    long index=INGRESG(array_index_start);

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"rl" , &result, &index) == FAILURE) 
    {
        RETURN_FALSE;
    }
    
    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres Result", le_ii_result);

    php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, index, II_FIELD_INFO_NAME);
}
/* }}} */

/* {{{ proto string ingres_field_type(resource result, int index)
   Return the type of a field in a query result index must be >0 and <= ingres_num_fields() */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_field_type)
#else
PHP_FUNCTION(ingres_field_type)
#endif
{
    zval *result = NULL;
    II_RESULT *ii_result;
    long index=INGRESG(array_index_start);

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"rl" , &result, &index) == FAILURE) 
    {
        RETURN_FALSE;
    }
    
    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres Result", le_ii_result);

    php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, index, II_FIELD_INFO_TYPE);
}
/* }}} */

/* {{{ proto string ingres_field_nullable(resource result, int index)
   Return true if the field is nullable and false otherwise index must be >0 and <= ingres_num_fields() */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_field_nullable)
#else
PHP_FUNCTION(ingres_field_nullable)
#endif
{
    zval *result = NULL;
    II_RESULT *ii_result;
    long index=INGRESG(array_index_start);

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"rl" , &result, &index) == FAILURE) 
    {
        RETURN_FALSE;
    }
    
    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres Result", le_ii_result);

    php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, index, II_FIELD_INFO_NULLABLE);
}
/* }}} */

/* {{{ proto string ingres_field_length(resource result, int index)
   Return the length of a field in a query result index must be >0 and <= ingres_num_fields() */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_field_length)
#else
PHP_FUNCTION(ingres_field_length)
#endif
{
    zval *result = NULL;
    II_RESULT *ii_result;
    long index=INGRESG(array_index_start);

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"rl" , &result, &index) == FAILURE) 
    {
        RETURN_FALSE;
    }
    
    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres Result", le_ii_result);

    php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, index, II_FIELD_INFO_LENGTH);
}
/* }}} */

/* {{{ proto string ingres_field_precision(resource result, int index)
   Return the precision of a field in a query result index must be >0 and <= ingres_num_fields() */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_field_precision)
#else
PHP_FUNCTION(ingres_field_precision)
#endif
{
    zval *result = NULL;
    II_RESULT *ii_result;
    long index=INGRESG(array_index_start);

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"rl" , &result, &index) == FAILURE) 
    {
        RETURN_FALSE;
    }
    
    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres Result", le_ii_result);

    php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, index, II_FIELD_INFO_PRECISION);
}
/* }}} */

/* {{{ proto string ingres_field_scale(resource result, int index)
   Return the scale of a field in a query result index must be >0 and <= ingres_num_fields() */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_field_scale)
#else
PHP_FUNCTION(ingres_field_scale)
#endif
{
    zval *result = NULL;
    II_RESULT *ii_result;
    long index=INGRESG(array_index_start);

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"rl" , &result, &index) == FAILURE) 
    {
        RETURN_FALSE;
    }
    
    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres Result", le_ii_result);

    php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, index, II_FIELD_INFO_SCALE);
}
/* }}} */

/* {{{ proto bool ingres_free_result(resource result)
   Free result set memory allocated by ingres_query()/ingres_execute() */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_free_result)
#else
PHP_FUNCTION(ingres_free_result)
#endif
{
    zval *result = NULL;
    II_RESULT *ii_result;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &result) == FAILURE) 
    {
        RETURN_FALSE;
    }

    if (Z_LVAL_P(result) == 0) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres result", le_ii_result);

    if (ii_result->stmtHandle && _close_statement(ii_result TSRMLS_CC) == II_FAIL)
    {
         php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close statement");
         RETURN_FALSE;
    }

    php_ii_result_remove (ii_result, Z_LVAL_P(result) TSRMLS_CC);

    zend_list_delete(Z_LVAL_P(result));

    RETURN_TRUE;
}
/* }}} */

/* {{{ static short php_ii_result_remove ( II_RESULT *ii_result, long result_id  ) 
    Remove the result information from the associated link
*/
static short php_ii_result_remove ( II_RESULT *ii_result, long result_id TSRMLS_DC)
{

    char *resource = NULL;
    char *type_name = NULL;
    int type;
    ii_result_entry *result_entry;
    char *last_ptr = NULL;
    char *next_ptr = NULL;
    char *this_ptr = NULL;

    /* Check to see we have a valid link_id */
    if ( ii_result->link_id != -1 )
    {
        resource = zend_list_find(ii_result->link_id, &type);
        type_name = zend_rsrc_list_get_rsrc_type(ii_result->link_id TSRMLS_CC);
        /* Is it an "ingres (persistent) connection" ? */
        if ((strcmp("ingres connection\0",type_name) == 0 ) || (strcmp("ingres persistent connection\0",type_name) == 0 ))
        {
            this_ptr = (char *)((II_LINK *)resource)->result_list_ptr;
            result_entry = ((II_LINK *)resource)->result_list_ptr;
            next_ptr = result_entry->next_result_ptr;
            /* scan the link's resource list */
            while (result_entry->result_id != result_id)
            {
                result_entry = (ii_result_entry *)result_entry->next_result_ptr;
                last_ptr = this_ptr;
                this_ptr = next_ptr;
                next_ptr = (char *)((ii_result_entry *)this_ptr)->next_result_ptr;
            }

            if (this_ptr == (char *)((II_LINK *)resource)->result_list_ptr) /* head of list */
            {
                if ( next_ptr != NULL ) /* another result after this one */
                {
                    ((II_LINK *)resource)->result_list_ptr = (ii_result_entry *)next_ptr;
                    free(this_ptr);
                    this_ptr = NULL;

                }
                else /* nothing else after us in the list */
                {
                    this_ptr = NULL;
                    free(((II_LINK *)resource)->result_list_ptr);
                    ((II_LINK *)resource)->result_list_ptr = NULL;
                    next_ptr = NULL;
                    last_ptr = NULL;
                }
            }
            else
            {
                if ( next_ptr != NULL ) /* another result after this one */
                {
                    ((ii_result_entry *)last_ptr)->next_result_ptr = next_ptr;
                    free(this_ptr);
                } 
                else /* nothing else after us in the list */
                {
                    ((ii_result_entry *)last_ptr)->next_result_ptr = NULL;
                    free(this_ptr);
                } /* ( next_ptr != NULL ) */
            } /* (this_ptr == (char *)((II_LINK *)resource)->result_list_ptr) */
        }
        else
        {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "php_ii_result_remove : An unknown resource type was passed");
            return II_FAIL;
        }
    }
    return II_OK;
}
/* }}} */

/* {{{static II_LONG php_ii_convert_data ( short destType, int destSize, int precision, II_RESULT *ii_result, IIAPI_DATAVALUE *columnData, IIAPI_GETCOLPARM getColParm, int field, int column TSRMLS_DC ) */
/* Convert complex Ingres data types to php-usable ones */
static II_LONG php_ii_convert_data ( short destType, int destSize, int precision, II_RESULT *ii_result, IIAPI_DATAVALUE *columnData, IIAPI_GETCOLPARM getColParm, int field, int column TSRMLS_DC )
{
    /* TODO: Add a mechanism for handling an IIAPI_LEVEL_1 client (or newer) talking to a level 0 client */
#if defined (IIAPI_VERSION_2)
    IIAPI_FORMATPARM formatParm;

    formatParm.fd_envHandle = INGRESG(envHandle);

    formatParm.fd_srcDesc.ds_dataType = (ii_result->descriptor[field+column]).ds_dataType;
    formatParm.fd_srcDesc.ds_nullable = (ii_result->descriptor[field+column]).ds_nullable;
    formatParm.fd_srcDesc.ds_length = (ii_result->descriptor[field+column]).ds_length;
    formatParm.fd_srcDesc.ds_precision = (ii_result->descriptor[field+column]).ds_precision;
    formatParm.fd_srcDesc.ds_scale = (ii_result->descriptor[field+column]).ds_scale;
    formatParm.fd_srcDesc.ds_columnType = (ii_result->descriptor[field+column]).ds_columnType;
    formatParm.fd_srcDesc.ds_columnName = (ii_result->descriptor[field+column]).ds_columnName;
    formatParm.fd_srcValue.dv_null = columnData[column].dv_null;
    formatParm.fd_srcValue.dv_length = columnData[column].dv_length;
    formatParm.fd_srcValue.dv_value = columnData[column].dv_value;

    formatParm.fd_dstDesc.ds_dataType = destType;
    formatParm.fd_dstDesc.ds_nullable = FALSE;
    formatParm.fd_dstDesc.ds_length = destSize;
    formatParm.fd_dstDesc.ds_precision = precision;
    formatParm.fd_dstDesc.ds_scale = 0;
    formatParm.fd_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
    formatParm.fd_dstDesc.ds_columnName = NULL;
    formatParm.fd_dstValue.dv_null = FALSE;
    formatParm.fd_dstValue.dv_length = formatParm.fd_dstDesc.ds_length;
    formatParm.fd_dstValue.dv_value = emalloc(formatParm.fd_dstDesc.ds_length);

    IIapi_formatData(&formatParm);

    if (formatParm.fd_status != IIAPI_ST_SUCCESS ) {
        return formatParm.fd_status;
    }

    columnData[column].dv_value = formatParm.fd_dstValue.dv_value;
    columnData[column].dv_length = formatParm.fd_dstValue.dv_length;

    if (ii_result->metaData == NULL)
    {
        efree(formatParm.fd_srcValue.dv_value);
        formatParm.fd_srcValue.dv_value = NULL;
    }

#else
    IIAPI_CONVERTPARM convertParm;
    
    convertParm.cv_srcDesc.ds_dataType = (ii_result->descriptor[field+column]).ds_dataType;
    convertParm.cv_srcDesc.ds_nullable = (ii_result->descriptor[field+column]).ds_nullable;
    convertParm.cv_srcDesc.ds_length = (ii_result->descriptor[field+column]).ds_length;
    convertParm.cv_srcDesc.ds_precision = (ii_result->descriptor[field+column]).ds_precision;
    convertParm.cv_srcDesc.ds_scale = (ii_result->descriptor[field+column]).ds_scale;
    convertParm.cv_srcDesc.ds_columnType = (ii_result->descriptor[field+column]).ds_columnType;
    convertParm.cv_srcDesc.ds_columnName = (ii_result->descriptor[field+column]).ds_columnName;
    convertParm.cv_srcValue.dv_null = columnData[column].dv_null;
    convertParm.cv_srcValue.dv_length = columnData[column].dv_length;
    convertParm.cv_srcValue.dv_value = columnData[column].dv_value;
    convertParm.cv_dstDesc.ds_dataType = destType;
    convertParm.cv_dstDesc.ds_nullable = FALSE;
    convertParm.cv_dstDesc.ds_length = destSize;
    convertParm.cv_dstDesc.ds_precision = precision;
    convertParm.cv_dstDesc.ds_scale = 0;
    convertParm.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
    convertParm.cv_dstDesc.ds_columnName = NULL;
    convertParm.cv_dstValue.dv_null = FALSE;
    convertParm.cv_dstValue.dv_length = convertParm.cv_dstDesc.ds_length;
    convertParm.cv_dstValue.dv_value = emalloc(convertParm.cv_dstDesc.ds_length);

    IIapi_convertData(&convertParm);

    if(convertParm.cv_status != IIAPI_ST_SUCCESS ) {
      return convertParm.cv_status;
    }

    columnData[column].dv_value = convertParm.cv_dstValue.dv_value;
    columnData[column].dv_length = convertParm.cv_dstValue.dv_length;

    if (ii_result->metaData == NULL)
    {
        efree(convertParm.cv_srcValue.dv_value);
        convertParm.cv_srcValue.dv_value = NULL;
    }
#endif

    return II_OK;
}
/* }}} */

/* {{{ proto bool ingres_result_seek(resource link, int position)
   Scroll to the row number specified */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_result_seek)
#else
PHP_FUNCTION(ingres_result_seek)
#endif
{
#if defined(IIAPI_VERSION_6)
    IIAPI_SCROLLPARM scrollParm;
#endif
    zval *result;
    II_RESULT *ii_result;
    int  position = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"rl" , &result, &position) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1, "Ingres Result", le_ii_result);

#if defined(IIAPI_VERSION_6)

    /* adjust position for ingres.array_index_start */
    position = INGRESG(array_index_start) ? position : position++;

    /* Check to see if the result is scrollable */
    if ( ii_result->scrollable )
    {
        if ( ii_result->rowCount == -1)
        {
            if (php_ii_scroll_row_count (ii_result TSRMLS_CC) == II_FAIL)
            {
                RETURN_FALSE;
            }
        }
        if ( position > ii_result->rowCount)
        {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Unable to reposition the result to position %d when there are only %d rows", position, ii_result->rowCount);
            RETURN_FALSE;
        }
        if ( position > -1 ) 
        {
            scrollParm.sl_genParm.gp_callback = NULL;
            scrollParm.sl_genParm.gp_closure = NULL;
            scrollParm.sl_stmtHandle = ii_result->stmtHandle;
            scrollParm.sl_orientation = IIAPI_SCROLL_ABSOLUTE;
            scrollParm.sl_offset = INGRESG(array_index_start) ? position : position++;

            IIapi_scroll(&scrollParm);
            ii_sync(&(scrollParm.sl_genParm));

            if (ii_success(&(scrollParm.sl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
            {
                RETURN_FALSE;
            }
            /* For the time being if some one has requested a specific row we do not check to see
               if we can satisfy that request from the buffer. For the time being we release 
               the metaData buffer.
               TODO : Check to see if we can satisfy the request from ii_result->metaData.  */
            if ( ii_result->metaData != NULL )
            {
                _free_resultdata(ii_result);
            }
        }
        else
        {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "Negative row positions are not allowed");
        }
    } 
    else
    {
        /* Give a nice message if the server cannot support it or if the result set is not-scrollable */
        if ( ii_result->apiLevel < IIAPI_LEVEL_5 )
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Row positioning is not available  with an API level of less than 5, you have level %d", ii_result->apiLevel);
        }
        else
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to position result on a non-scrollable resultset");
        }
        RETURN_FALSE;
    }
#else
    if ( position > 0)
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Row positioning is not supported at this release level (%d), level 5 is required", ii_result->apiLevel);
        RETURN_FALSE;
    }
#endif
    RETURN_TRUE;

}
/* }}} */

/* {{{ static void php_ii_fetch(INTERNAL_FUNCTION_PARAMETERS, II_LINK *ii_link, int result_type) */
/* Fetch a row of result */
static void php_ii_fetch(INTERNAL_FUNCTION_PARAMETERS, II_RESULT *ii_result, int result_type)
{
    IIAPI_DATAVALUE *columnData;
    IIAPI_GETQINFOPARM getQInfoParm;
    IIAPI_GETDESCRPARM getDescrParm;

    int i, j, k, l;
    double value_double = 0;
    long value_long = 0;
    ingres_int64 value_long_long = 0;
    char value_long_long_str[21];
    int value_long_long_str_len=0;
    char *value_char_p;
    int len, should_copy, correct_length=0;
    int lob_len ;
    short int lob_segment_len;
    short int null_column_name, mem;
    char *lob_segment, *lob_ptr, *lob_data;
    II_BOOL have_lob = FALSE;
    int cell, col_no;
    char *next_cell = NULL;
    long pad_bytes = 0;

#if defined(IIAPI_VERSION_3)
    UTF8 *tmp_utf8_string_ptr = NULL;
    UTF8 *tmp_utf8_string = NULL;
    UTF16 *string_start;
    ConversionResult result;
#endif

    /* array initialization */
    array_init(return_value);
    /* init first char of array used to return BIG INT values as strings */
    value_long_long_str[0] = '\0';

    if (ii_result->descriptor == NULL)
    {
        /* get description of results */
        getDescrParm.gd_genParm.gp_callback = NULL;
        getDescrParm.gd_genParm.gp_closure  = NULL;
        getDescrParm.gd_stmtHandle = ii_result->stmtHandle;

        IIapi_getDescriptor(&getDescrParm);
        ii_sync(&(getDescrParm.gd_genParm));

        if (ii_success(&(getDescrParm.gd_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
        {
            if (ii_result->cursor_id)
            {
                efree(ii_result->cursor_id);
                ii_result->cursor_id = NULL;
            }
            if (ii_result->inputDescr)
            {
                efree(ii_result->inputDescr);
                ii_result->inputDescr = NULL;
            }
            if (ii_result->metaData)
            {
                efree(ii_result->metaData);
                ii_result->metaData = NULL;
            }
            if (ii_result)
            {
                free(ii_result);
                ii_result = NULL;
            }
            RETURN_FALSE;
        }
        ii_result->fieldCount = getDescrParm.gd_descriptorCount;
        ii_result->descriptor = getDescrParm.gd_descriptor;
    }


    if ( ii_result->metaData != NULL )
    {
        /* If we have pre-fetched data available */
        ii_result->rowNumber++;
        if ( ii_result->rowNumber < ii_result->rowsReturned )
        {
            /* take from the existing buffer */
            for (col_no = 0; col_no < ii_result->fieldCount; col_no++)
            {
                php_ii_setup_return_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, &ii_result->metaData[(ii_result->rowNumber * ii_result->fieldCount) + col_no], ii_result, col_no, result_type);
            }
        }
        else
        {
            /* We have run out of rows in our buffer */
            _free_resultdata(ii_result);

            /* go fetch some more rows */

            ii_result->getColParm.gc_genParm.gp_callback = NULL;
            ii_result->getColParm.gc_genParm.gp_closure = NULL;
            ii_result->getColParm.gc_rowCount = INGRESG(fetch_buffer_size); /* 100 rows by default */
            ii_result->getColParm.gc_columnCount = ii_result->fieldCount;
            ii_result->getColParm.gc_stmtHandle = ii_result->stmtHandle;
            ii_result->getColParm.gc_moreSegments = 0;
            /* Allocate the memory needed to retrieve the data */
            ii_result->metaData = (IIAPI_DATAVALUE *) emalloc (sizeof(IIAPI_DATAVALUE) * ii_result->getColParm.gc_rowCount * ii_result->getColParm.gc_columnCount);
            ii_result->getColParm.gc_columnData = ii_result->metaData;

            /* Setup the buffer for receiving the results */
            ii_result->dataBuffer = (II_PTR) emalloc(ii_result->rowWidth * ii_result->getColParm.gc_rowCount );
            next_cell = (char *)ii_result->dataBuffer;
            for( cell=0; cell < ( ii_result->fieldCount * ii_result->getColParm.gc_rowCount ); cell++)
            {
                ii_result->metaData[cell].dv_value = next_cell;
                next_cell += (ii_result->descriptor[cell % ii_result->fieldCount]).ds_length;
            }  

            IIapi_getColumns(&ii_result->getColParm);
            ii_sync(&(ii_result->getColParm.gc_genParm));

            if (ii_success(&(ii_result->getColParm.gc_genParm), &ii_result->errorHandle TSRMLS_CC) != II_OK)
            {
                _free_resultdata(ii_result);
                zend_hash_destroy(return_value->value.ht);
                efree(return_value->value.ht);
                RETURN_FALSE;
            }

            /* Store the actual number of rows fetched from the server */
            ii_result->rowsReturned = ii_result->getColParm.gc_rowsReturned;

            /* Load the first row into the return value */
            for (col_no = 0; col_no < ii_result->fieldCount; col_no++)
            {
                /* some releases of Ingres do not set a column name for row producing procedures */
                /* set up some memory so we can roll our own column name */
                if ( ii_result->procname != NULL && ii_result->descriptor[col_no].ds_columnName == NULL )
                {
                    ii_result->descriptor[col_no].ds_columnName = emalloc(8);
                    for ( mem = 0 ; mem < 8 ; mem++ ) 
                    {
                        ii_result->descriptor[col_no].ds_columnName[mem] = '\0';
                    }
                    null_column_name = 1;
                }
                php_ii_setup_return_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, &ii_result->metaData[(ii_result->rowNumber * ii_result->fieldCount) + col_no], ii_result, col_no, result_type);

                if ( ii_result->procname != NULL && null_column_name == 1 )
                {
                    efree(ii_result->descriptor[col_no].ds_columnName);
                    ii_result->descriptor[col_no].ds_columnName = NULL;
                    null_column_name = 0;
                }
            }

        }
    }
    else
    {
        if ( ii_result->procname != NULL ) /* look to see if there is a return value*/
        {
            if ( ii_result->fieldCount == 0 ) 
            /* if > 0 then we are looking at a row producing procedure and don't want to call IIapi_getQueryInfo
             * to nullify any results */
            {
                getQInfoParm.gq_genParm.gp_callback = NULL;
                getQInfoParm.gq_genParm.gp_closure = NULL;
                getQInfoParm.gq_stmtHandle = ii_result->stmtHandle;

                IIapi_getQueryInfo( &getQInfoParm );
                ii_sync(&(getQInfoParm.gq_genParm));

                if (ii_success(&(getQInfoParm.gq_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
                {
                    RETURN_FALSE;
                } 

                /* Check that the last query was for a procedure and there is a return value */
                if ((getQInfoParm.gq_mask & IIAPI_GQ_PROCEDURE_ID) && 
                        (getQInfoParm.gq_mask & IIAPI_GQ_PROCEDURE_RET)) 
                {
                    if (result_type & II_NUM)
                    {
                        add_index_long(return_value, 0, getQInfoParm.gq_procedureReturn);
                    }
                    if (result_type & II_ASSOC)
                    {
                        add_assoc_long(return_value, "returnvalue", getQInfoParm.gq_procedureReturn);
                    }
                }
                else
                {
                    RETURN_FALSE;
                }
            }
        }

        /* Ingres OpenAPI/GCA can fetch mulitple rows at a time if and only if:   
           - There are no LONG types in the result set                         
           - If the result is from a cursor that the cursor is readonly         
           Assuming the above to be true we can batch fetch rows from the server  */

        for (col_no = 0; col_no < ii_result->fieldCount; col_no++)
        {
            if ((ii_result->descriptor[col_no]).ds_dataType == IIAPI_LBYTE_TYPE ||
                (ii_result->descriptor[col_no]).ds_dataType == IIAPI_LVCH_TYPE 
#if defined (IIAPI_VERSION_3)
                || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_LNVCH_TYPE 
#endif
               )
            {
                have_lob = TRUE;
            }
        }

        /* If we do not have a lob and the query is not an update cursor we can fetch */
        /* blocks of rows */
        if (!have_lob && (ii_result->cursor_mode = II_CURSOR_READONLY))
        {
            ii_result->getColParm.gc_genParm.gp_callback = NULL;
            ii_result->getColParm.gc_genParm.gp_closure = NULL;
            ii_result->getColParm.gc_rowCount = INGRESG(fetch_buffer_size); /* 100 rows by default */
            ii_result->getColParm.gc_columnCount = ii_result->fieldCount;
            ii_result->getColParm.gc_stmtHandle = ii_result->stmtHandle;
            ii_result->getColParm.gc_moreSegments = 0;
            /* Allocate the memory needed to retrieve the data */
            ii_result->metaData = (IIAPI_DATAVALUE *) emalloc (sizeof(IIAPI_DATAVALUE) * ii_result->getColParm.gc_rowCount * ii_result->getColParm.gc_columnCount);
            ii_result->getColParm.gc_columnData = ii_result->metaData;

            /* Setup the buffer for receiving the results */
            ii_result->dataBuffer = (II_PTR) emalloc(ii_result->rowWidth * ii_result->getColParm.gc_rowCount );
            next_cell = (char *)ii_result->dataBuffer;
#ifdef ALIGN_MEMORY
            /* Make sure that ii_result->metaData[cell].dv_value is pointing to a memory boundary */
            for( cell=0; cell < ( ii_result->fieldCount * ii_result->getColParm.gc_rowCount ); cell++)
            {
                 ii_result->metaData[cell].dv_value = next_cell;
                 if ((next_cell + (ii_result->descriptor[cell % ii_result->fieldCount]).ds_length) % ZEND_MM_ALIGNMENT != 0)
                 {
                     /* Pad the space between the end of the cell and the start of the next one */
                     pad_bytes = ZEND_MM_ALIGNMENT -
                         (next_cell + (ii_result->descriptor[cell % ii_result->fieldCount]).ds_length) % ZEND_MM_ALIGNMENT;
                     next_cell += (ii_result->descriptor[cell % ii_result->fieldCount]).ds_length + pad_bytes;

                 }
                 else
                 {
                     next_cell += (ii_result->descriptor[cell % ii_result->fieldCount]).ds_length;
                 }
            }  
#else
            for( cell=0; cell < ( ii_result->fieldCount * ii_result->getColParm.gc_rowCount ); cell++)
            {
                 ii_result->metaData[cell].dv_value = next_cell;
                 next_cell += (ii_result->descriptor[cell % ii_result->fieldCount]).ds_length;
            }  
#endif

            IIapi_getColumns(&ii_result->getColParm);
            ii_sync(&(ii_result->getColParm.gc_genParm));

            if (ii_success(&(ii_result->getColParm.gc_genParm), &ii_result->errorHandle TSRMLS_CC) != II_OK)
            {
                _free_resultdata(ii_result);
                zend_hash_destroy(return_value->value.ht);
                efree(return_value->value.ht);
                RETURN_FALSE;
            }

            /* Store the actual number of rows fetched from the server */
            ii_result->rowsReturned = ii_result->getColParm.gc_rowsReturned;

            /* Load the first row into the return value */
            for (col_no = 0; col_no < ii_result->fieldCount; col_no++)
            {
                /* some releases of Ingres do not set a column name for row producing procedures */
                /* set up some memory so we can roll our own column name */
                if ( ii_result->procname != NULL && ii_result->descriptor[col_no].ds_columnName == NULL )
                {
                    ii_result->descriptor[col_no].ds_columnName = emalloc(8);
                    for ( mem = 0 ; mem < 8 ; mem++ ) 
                    {
                        ii_result->descriptor[col_no].ds_columnName[mem] = '\0';
                    }
                    null_column_name = 1;
                }
                php_ii_setup_return_value(INTERNAL_FUNCTION_PARAM_PASSTHRU, &ii_result->metaData[(ii_result->rowNumber * ii_result->fieldCount) + col_no], ii_result, col_no, result_type);

                if ( ii_result->procname != NULL && null_column_name == 1 )
                {
                    efree(ii_result->descriptor[col_no].ds_columnName);
                    ii_result->descriptor[col_no].ds_columnName = NULL;
                    null_column_name = 0;
                }
            }


        }
        else
        {
            /* Look to see if we have a LOB ( LONG VARCHAR/BYTE/NVARCHAR ) in the resultset */
            for (i = 0; i < ii_result->fieldCount;)
            {
                j = 0;
                k = 0;
                have_lob = FALSE;

                /* as long as there are no long byte or long varchar fields, Ingres is able to fetch 
                   many fields at a time, so try to find these types and stop if they're found. variable 
                   j will get number of fields to fetch */
                while ((i + j) < ii_result->fieldCount) 
                {
                    if ((ii_result->descriptor[i + j]).ds_dataType != IIAPI_LBYTE_TYPE &&
                        (ii_result->descriptor[i + j]).ds_dataType != IIAPI_LVCH_TYPE 
#if defined (IIAPI_VERSION_3)
                        && (ii_result->descriptor[i + j]).ds_dataType != IIAPI_LNVCH_TYPE 
#endif
                       )
                    {
                        j++;
                    }
                    else
                    {
                        /* break out of loop */
                        /* a lob needs to be processed separately */
                        have_lob = 1;
                        break;
                    }
                }

                if ( j > 0 ) 
                {
                    /* process non LOB fields */

                    /* allocate memory for j fields */
                    columnData = (IIAPI_DATAVALUE *) safe_emalloc(sizeof(IIAPI_DATAVALUE), j, 0);
                    for (k = 0; k < j; k++)
                    {
                        columnData[k].dv_value = (II_PTR) emalloc((ii_result->descriptor[i + k]).ds_length);
                    }

                    ii_result->getColParm.gc_genParm.gp_callback = NULL;
                    ii_result->getColParm.gc_genParm.gp_closure = NULL;
                    ii_result->getColParm.gc_rowCount = 1;
                    ii_result->getColParm.gc_columnCount = j;
                    ii_result->getColParm.gc_columnData = columnData;
                    ii_result->getColParm.gc_stmtHandle = ii_result->stmtHandle;
                    ii_result->getColParm.gc_moreSegments = 0;

                    IIapi_getColumns(&ii_result->getColParm);
                    ii_sync(&(ii_result->getColParm.gc_genParm));

                    if (ii_success(&(ii_result->getColParm.gc_genParm), &ii_result->errorHandle TSRMLS_CC) != II_OK)
                    {
                        for (k = 0; k < j; k++)
                        {
                            efree(columnData[k].dv_value);
                        }
                        zend_hash_destroy(return_value->value.ht);
                        efree(return_value->value.ht);
                        efree(columnData);
                        RETURN_FALSE;
                    }

                    for (k = 0; k < j; k++)
                    {

                        /* some releases of Ingres do not set a column name for row producing procedures */
                        /* set up some memory so we can roll our own column name */
                        if ( ii_result->procname != NULL && ii_result->descriptor[i + k].ds_columnName == NULL )
                        {
                            ii_result->descriptor[i+k].ds_columnName = emalloc(8);
                            for ( mem = 0 ; mem < 8 ; mem++ ) 
                            {
                                ii_result->descriptor[i+k].ds_columnName[mem] = '\0';
                            }
                            null_column_name = 1;
                        }
                        if (columnData[k].dv_null)
                        {    /* NULL value ? */

                            if (result_type & II_NUM)
                            {
                                add_index_null(return_value, i + k + INGRESG(array_index_start));
                            }
                            if (result_type & II_ASSOC)
                            {
                                add_assoc_null(return_value, php_ii_field_name(ii_result, i + k + INGRESG(array_index_start) TSRMLS_CC));
                            }

                        } else {    /* non NULL value */
                            correct_length = 0;

                            switch ((ii_result->descriptor[i + k]).ds_dataType)
                            {

                                case IIAPI_DEC_TYPE:    /* decimal (fixed point number) */
                                case IIAPI_MNY_TYPE:    /* money */
                                    /* convert to floating point number */
                                    php_ii_convert_data ( IIAPI_FLT_TYPE, sizeof(II_FLOAT8), 53, ii_result, columnData, ii_result->getColParm, i, k TSRMLS_CC );
                                    /* NO break */

                                case IIAPI_FLT_TYPE:    /* floating point number */
                                    switch (columnData[k].dv_length)
                                    {

                                        case 4:
                                            value_double = (double) *((II_FLOAT4 *) columnData[k].dv_value);
                                            break;

                                        case 8:
                                            value_double = (double) *((II_FLOAT8 *) columnData[k].dv_value);
                                            break;

                                        default:
                                            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid size for IIAPI_FLT_TYPE data (%d)", columnData[k - 1].dv_length);
                                            break;
                                    }

                                    if (result_type & II_NUM)
                                    {
                                        add_index_double(return_value, i + k + INGRESG(array_index_start), value_double);
                                    }

                                    if (result_type & II_ASSOC)
                                    {
                                        add_assoc_double(return_value, php_ii_field_name(ii_result, i + k + INGRESG(array_index_start) TSRMLS_CC), value_double);

                                    }
                                    break;

                                case IIAPI_INT_TYPE:    /* integer */
                                    switch (columnData[k].dv_length)
                                    {

                                        case 1:
                                            value_long = (long) *((II_INT1 *) columnData[k].dv_value);
                                            break;

                                        case 2:
                                            value_long = (long) *((II_INT2 *) columnData[k].dv_value);
                                            break;
                
                                        case 4:
                                            value_long = (long) *((II_INT4 *) columnData[k].dv_value);
                                            break;
#if defined(IIAPI_VERSION_4)
                                        case 8:
                                            /* PHP does not support BIGINT/INTEGER8 so we have to return */
                                            /* values greater/smaller than the max/min size of a LONG value as a string */
                                            /* Anyone wanting to manipulate this value can use PECL big_int */
                                            if ((*((ingres_int64 *) columnData[k].dv_value) > LONG_MAX ) ||
                                                (*((ingres_int64 *) columnData[k].dv_value) < LONG_MIN ))
                                            {
                                                value_long_long = *((ingres_int64 *) columnData[k].dv_value);
                                                sprintf(value_long_long_str, "%lld", value_long_long);
                                                value_long_long_str_len = strlen(value_long_long_str);
                                            }
                                            else
                                            {
                                                value_long = (long) *((II_INT4 *) columnData[k].dv_value);
                                            }
                                            break;
#endif
                                        default:
                                            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid size for IIAPI_INT_TYPE data (%d)", columnData[k].dv_length);
                                            break;
                                    }

                                    if (value_long_long_str[0] != '\0')
                                    {
                                        should_copy = 1;
                                        if (result_type & II_NUM)
                                        {
                                            add_index_stringl(return_value, i + k + INGRESG(array_index_start), value_long_long_str, value_long_long_str_len, should_copy);
                                        }
                                        if (result_type & II_ASSOC)
                                        {
                                            add_assoc_stringl(return_value, php_ii_field_name(ii_result, i + k + INGRESG(array_index_start) TSRMLS_CC), value_long_long_str, value_long_long_str_len, should_copy);
                                        }
                                        /* Init the first char to '\0' for later reuse */
                                        value_long_long_str[0] = '\0';
                                    }
                                    else
                                    {
                                        if (result_type & II_NUM)
                                        {
                                            add_index_long(return_value, i + k + INGRESG(array_index_start), value_long);
                                        }
                                        if (result_type & II_ASSOC)
                                        {
                                            add_assoc_long(return_value, php_ii_field_name(ii_result, i + k + INGRESG(array_index_start) TSRMLS_CC), value_long);
                                        }
                                    }
                                    break;

#if defined(IIAPI_VERSION_3)
                                case IIAPI_NVCH_TYPE:    /* variable length unicode character string */
                                    columnData[k].dv_length = *((II_INT2 *) columnData[k].dv_value) * 2;
                                    columnData[k].dv_value = (II_CHAR *)(columnData[k]).dv_value + 2;
                                    correct_length = 1;
                                case IIAPI_NCHA_TYPE:    /* fixed length unicode character string */    
                                    /* use php_addslashes if asked to */
                                    if (PG(magic_quotes_runtime))
                                    {
                                        value_char_p = php_addslashes((char *) columnData[k].dv_value, columnData[k].dv_length, &len, 0 TSRMLS_CC);
                                        should_copy = 0;
                                    } else {
                                        value_char_p = (char *) columnData[k].dv_value;
                                        len = columnData[k].dv_length;
                                        should_copy = 1;
                                    }

                                    if (INGRESG(utf8)) {
                                        /* User has requested the output in UTF-8 */
                                        /* create a big enough buffer - each code point in UTF-16 can be upto 4 bytes in UTF-8 */
                                        tmp_utf8_string = emalloc((len * 4) + 1);
                                        for ( l = 0; l < len * 4 ; l++) {
                                            tmp_utf8_string[l] = '\0';
                                        }
                                        string_start = (UTF16 *)columnData[k].dv_value;
                                        tmp_utf8_string_ptr = tmp_utf8_string;
                                        result = ConvertUTF16toUTF8((const UTF16 **) &string_start, string_start + len/2, &tmp_utf8_string_ptr, tmp_utf8_string_ptr + (len * 4), strictConversion);
                                        len = tmp_utf8_string_ptr - tmp_utf8_string;
                                        tmp_utf8_string[len] = '\0';
                                        value_char_p = (char *)tmp_utf8_string;
                                    }

                                    if (result_type & II_NUM)
                                    {
                                        add_index_stringl(return_value, i + k + INGRESG(array_index_start), value_char_p, len, should_copy);
                                    }
                                    if (result_type & II_ASSOC)
                                    {
                                        add_assoc_stringl(return_value, php_ii_field_name(ii_result, i + k + INGRESG(array_index_start) TSRMLS_CC), value_char_p, len, should_copy);
                                    }
                                    if (tmp_utf8_string) {
                                        efree(tmp_utf8_string);
                                        tmp_utf8_string = NULL;
                                    }
                                    break;
#endif                                
                                case IIAPI_TXT_TYPE:    /* variable length character string */
                                case IIAPI_VBYTE_TYPE:    /* variable length binary string */
                                case IIAPI_VCH_TYPE:    /* variable length character string */
                                    /* real length is stored in first 2 bytes of data, so adjust
                                       length variable and data pointer */
                                    columnData[k].dv_length = *((II_INT2 *) columnData[k].dv_value);
                                    columnData[k].dv_value = (II_CHAR *)(columnData[k]).dv_value + 2;
                                    correct_length = 1;
                                    /* NO break */

                                case IIAPI_BYTE_TYPE:    /* fixed length binary string */
                                case IIAPI_CHA_TYPE:    /* fixed length character string */
                                case IIAPI_CHR_TYPE:    /* fixed length character string */
                                case IIAPI_LOGKEY_TYPE:    /* value unique to database */
                                case IIAPI_TABKEY_TYPE:    /* value unique to table */
                                case IIAPI_DTE_TYPE:    /* Ingres date */
#if defined(IIAPI_VERSION_5) 
                                case IIAPI_ADATE_TYPE:  /* SQL Date (aka ANSI DATE) */
                                case IIAPI_TIME_TYPE: /* Ingres Time */
                                case IIAPI_TMWO_TYPE: /* Time without Timezone */
                                case IIAPI_TMTZ_TYPE: /* Time with Timezone */
                                case IIAPI_TS_TYPE: /* Ingres Timestamp */
                                case IIAPI_TSWO_TYPE: /* Timestamp without Timezone */
                                case IIAPI_TSTZ_TYPE: /* Timestamp with Timezone */
                                case IIAPI_INTYM_TYPE: /* Interval Year to Month */
                                case IIAPI_INTDS_TYPE: /* Interval Day to Second */
#endif
                                    /* convert date to variable length string */
                                    if ((ii_result->descriptor[i + k]).ds_dataType == IIAPI_DTE_TYPE 
#if defined(IIAPI_VERSION_5) 
                                        || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_ADATE_TYPE
                                        || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TIME_TYPE 
                                        || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TMWO_TYPE
                                        || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TMTZ_TYPE
                                        || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TS_TYPE
                                        || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TSWO_TYPE
                                        || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TSTZ_TYPE
                                        || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_INTYM_TYPE
                                        || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_INTDS_TYPE
#endif
                                        )
                                    {
                                        php_ii_convert_data ( IIAPI_VCH_TYPE, 32, 0, ii_result, columnData, ii_result->getColParm, i, k TSRMLS_CC );
                                        columnData[k].dv_length = *((II_INT2 *) columnData[k].dv_value);
                                        columnData[k].dv_value = (II_CHAR *)(columnData[k]).dv_value + 2;
                                        correct_length = 1;
                                    }

                                    /* use php_addslashes if asked to */
                                    if (PG(magic_quotes_runtime))
                                    {
                                        value_char_p = php_addslashes((char *) columnData[k].dv_value, columnData[k].dv_length, &len, 0 TSRMLS_CC);
                                        should_copy = 0;
                                    }
                                    else 
                                    {
                                        value_char_p = (char *) columnData[k].dv_value;
                                        len = columnData[k].dv_length;
                                        should_copy = 1;
                                    }

                                    if (result_type & II_NUM)
                                    {
                                        add_index_stringl(return_value, i + k + INGRESG(array_index_start), value_char_p, len, should_copy);
                                    }

                                    if (result_type & II_ASSOC)
                                    {
                                        add_assoc_stringl(return_value, php_ii_field_name(ii_result, i + k + INGRESG(array_index_start) TSRMLS_CC), value_char_p, len, should_copy);
                                    }

                                    break;
                
                                default:
                                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid SQL data type in fetched field (%d -- length : %d)", (ii_result->descriptor[i + k]).ds_dataType, columnData[k].dv_length);
                                    break;
                            }
                            /* eventually restore data pointer state for variable length data types */
                            if (correct_length)
                            {
                                columnData[k].dv_value = (II_CHAR *)(columnData[k]).dv_value - 2;
                                if ((ii_result->descriptor[i + k]).ds_dataType == IIAPI_DTE_TYPE 
#if defined(IIAPI_VERSION_5) 
                                    || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_ADATE_TYPE
                                    || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TIME_TYPE 
                                    || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TMWO_TYPE
                                    || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TMTZ_TYPE
                                    || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TS_TYPE
                                    || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TSWO_TYPE
                                    || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_TSTZ_TYPE
                                    || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_INTYM_TYPE
                                    || (ii_result->descriptor[i + k]).ds_dataType == IIAPI_INTDS_TYPE
#endif
                                    )
                                {
                                    efree(columnData[k].dv_value);
                                    columnData[k].dv_value = NULL;
                                }
                            }
                            else if (((ii_result->descriptor[i + k]).ds_dataType == IIAPI_MNY_TYPE) ||
                                    ((ii_result->descriptor[i + k]).ds_dataType == IIAPI_DEC_TYPE))
                            {
                                efree(columnData[k].dv_value);
                                columnData[k].dv_value = NULL;
                            }
                        }
                        if ( ii_result->procname != NULL && null_column_name == 1 )
                        {
                            efree(ii_result->descriptor[i+k].ds_columnName);
                            ii_result->descriptor[i+k].ds_columnName = NULL;
                            null_column_name = 0;
                        }

                    }

                    /* free the memory buffers */
                    for (k = 0; k < j; k++)
                    {
                        efree(columnData[k].dv_value);
                    }
                    efree(columnData);
                    columnData = NULL;
                }

                if (have_lob) 
                {

                    /* alloc memory for the size of the segment we need */

                    columnData = (IIAPI_DATAVALUE *) safe_emalloc(sizeof(IIAPI_DATAVALUE), 1, 0);
                    lob_segment = (char *) emalloc (INGRESG(blob_segment_length));
                    lob_len = 0;
                    do
                    {
                        for (cell = 0; cell< INGRESG(blob_segment_length); cell++)
                        {
                            lob_segment[cell] = '\0';
                        }
                        ii_result->getColParm.gc_genParm.gp_callback = NULL;
                        ii_result->getColParm.gc_genParm.gp_closure = NULL;
                        ii_result->getColParm.gc_rowCount = 1;
                        ii_result->getColParm.gc_columnCount = 1; /* just the lob */
                        ii_result->getColParm.gc_columnData = columnData;
                        ii_result->getColParm.gc_columnData[0].dv_value = lob_segment;
                        ii_result->getColParm.gc_stmtHandle = ii_result->stmtHandle;

                        IIapi_getColumns(&ii_result->getColParm);
                        ii_sync(&(ii_result->getColParm.gc_genParm));

                        switch (ii_success(&(ii_result->getColParm.gc_genParm), &ii_result->errorHandle TSRMLS_CC)) 
                        {
                            case II_OK:
                                break;
                            case II_NO_DATA:
                                efree(lob_segment);
                                efree(columnData);
                                RETURN_FALSE;
                                break;
                            default:
                                efree(lob_segment);
                                efree(columnData);
                                php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occurred whilst fetching a BLOB");
                                RETURN_FALSE;
                                break;
                        }

                        if (columnData[0].dv_null)
                        {
                            break;
                        }

                        /* length of the segment fetched is in the first 2 bytes of
                           lob_segment */
                        memcpy( (char *)&lob_segment_len, lob_segment, 2 );
                        if ((ii_result->descriptor[i + j]).ds_dataType == IIAPI_LNVCH_TYPE )
                        {
                            lob_segment_len = lob_segment_len * 2;
                        }

                        if ( lob_len == 0 )
                        {
                            lob_data = (char *) emalloc (lob_segment_len + 1);
                        }
                        else
                        {
                            /* extend existing lob buffer by the size of the new segment */
                            lob_data = erealloc (lob_data, lob_len + lob_segment_len + 16);
                        }

                        /* copy segement in to buffer. First two bytes contain the length of the segment. */


                        lob_segment += 2;
                        lob_ptr = lob_data + lob_len;
                        memcpy(lob_ptr, lob_segment, lob_segment_len);

                        /* move on dv_length and add to the total length of the lob fetched 
                        so far */
                        lob_segment -= 2;
                        lob_len += lob_segment_len;

                    } 
                    while ( ii_result->getColParm.gc_moreSegments );

                    /* If this is an LONG NVARCHAR and conversion to UTF-8 has been requested */
                    if (((ii_result->descriptor[i + j]).ds_dataType == IIAPI_LNVCH_TYPE) && (INGRESG(utf8))) {
                        /* User has requested the output in UTF-8 */
                        /* create a big enough buffer - each code point in UTF-16 can be upto 4 bytes in UTF-8 */
                        tmp_utf8_string = emalloc((lob_len * 4) + 1);
                        string_start = (UTF16 *)lob_data;
                        tmp_utf8_string_ptr = tmp_utf8_string;
                        result = ConvertUTF16toUTF8((const UTF16 **) &string_start, string_start + lob_len/2, &tmp_utf8_string_ptr, tmp_utf8_string_ptr + (lob_len * 4), strictConversion);
                        len = tmp_utf8_string_ptr - tmp_utf8_string;
                        tmp_utf8_string[len] = '\0';
                        value_char_p = (char *)tmp_utf8_string;

                        if (columnData[0].dv_null)
                        {    /* NULL value ? */

                            if (result_type & II_NUM)
                            {
                                add_index_null(return_value, i + k + INGRESG(array_index_start));
                            }
                            if (result_type & II_ASSOC)
                            {
                                add_assoc_null(return_value, php_ii_field_name(ii_result, i + k + INGRESG(array_index_start) TSRMLS_CC));
                            }
                        }
                        else
                        {
                            if (result_type & II_NUM)
                            {
                                add_index_stringl(return_value, i + k + INGRESG(array_index_start), tmp_utf8_string, len, 1);
                            }

                            if (result_type & II_ASSOC)
                            {
                                add_assoc_stringl(return_value, php_ii_field_name(ii_result, i + k + INGRESG(array_index_start) TSRMLS_CC), tmp_utf8_string, len, 1);
                            }
                        }
                    }
                    else
                    {
                        if (columnData[0].dv_null)
                        {    /* NULL value ? */

                            if (result_type & II_NUM)
                            {
                                add_index_null(return_value, i + k + INGRESG(array_index_start));
                            }
                            if (result_type & II_ASSOC)
                            {
                                add_assoc_null(return_value, php_ii_field_name(ii_result, i + k + INGRESG(array_index_start) TSRMLS_CC));
                            }
                        }
                        else
                        {
                            if (result_type & II_NUM)
                            {
                                add_index_stringl(return_value, i + k + INGRESG(array_index_start), lob_data, lob_len, 1);
                            }

                            if (result_type & II_ASSOC)
                            {
                                add_assoc_stringl(return_value, php_ii_field_name(ii_result, i + k + INGRESG(array_index_start) TSRMLS_CC), lob_data, lob_len, 1);
                            }
                        }
                    }

                    efree(lob_segment);
                    efree(columnData);
                    if (lob_len != 0)
                    {
                        efree(lob_data);
                    }
                    if (tmp_utf8_string)
                    {
                        efree(tmp_utf8_string);
                    }
                } 
            
                    
                /* increase field pointer by number of fetched fields */
                /* include any LOB data fetched */
                i += j + have_lob;
            }
        }
    }
}
/* }}} */

/* {{{ proto array ingres_fetch_array(resource result [,int result_type])
   Fetch a row of result into an array result_type can be 
   II_NUM for enumerated array, 
   II_ASSOC for associative array, or 
   II_BOTH (default) */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_fetch_array)
#else
PHP_FUNCTION(ingres_fetch_array)
#endif
{
    long result_type=II_BOTH; 
    zval *result;
    II_RESULT *ii_result;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r|l" , &result, &result_type) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1 , "Ingres Result", le_ii_result);

    php_ii_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, (ZEND_NUM_ARGS() == 1 ? II_BOTH : result_type));

}
/* }}} */

/* {{{ proto array ingres_fetch_assoc(resource result)
   Fetch a row of result into an associative array */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_fetch_assoc)
#else
PHP_FUNCTION(ingres_fetch_assoc)
#endif
{
    long result_type=II_BOTH; 
    zval *result;
    II_RESULT *ii_result;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r|l" , &result, &result_type) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1 , "Ingres Result", le_ii_result);

    php_ii_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, II_ASSOC);

}
/* }}} */

/* {{{ proto array ingres_fetch_row(resource result)
   Fetch a row of result into an enumerated array */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_fetch_row)
#else
PHP_FUNCTION(ingres_fetch_row)
#endif
{
    zval *result;
    II_RESULT *ii_result;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &result) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1 , "Ingres Result", le_ii_result);

    php_ii_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, II_NUM);
}
/* }}} */

/* {{{ proto array ingres_fetch_object(resource result [,int result_type])
   Fetch a row of result into an object result_type can be II_NUM for enumerated object, II_ASSOC for associative object, or II_BOTH (default) */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_fetch_object)
#else
PHP_FUNCTION(ingres_fetch_object)
#endif
{

    long result_type = II_BOTH; 
    zval *result = NULL;
    II_RESULT *ii_result;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r|l" , &result, &result_type) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1 , "Ingres Result", le_ii_result);

    php_ii_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_result, (ZEND_NUM_ARGS() == 1 ? II_ASSOC : result_type));

    if (Z_TYPE_P(return_value) == IS_ARRAY)
    {
        convert_to_object(return_value);
    }
}
/* }}} */

/* {{{ proto array ingres_fetch_proc_return(resource result)
   Fetch the return code from a procedure call. Calling this with rows to fetch from a row producing procedure
   will destroy any rows left to fetch */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_fetch_proc_return)
#else
PHP_FUNCTION(ingres_fetch_proc_return)
#endif
{

    zval  *result;
    II_RESULT *ii_result;
    IIAPI_GETQINFOPARM getQInfoParm;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &result) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(ii_result, II_RESULT *, &result, -1 , "Ingres Result", le_ii_result);

    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = ii_result->stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );
    ii_sync(&(getQInfoParm.gq_genParm));

    if (ii_success(&(getQInfoParm.gq_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        RETURN_FALSE;
    } 

    /* Check that the last query was for a procedure and there is a return value */
    if ((getQInfoParm.gq_mask & IIAPI_GQ_PROCEDURE_ID) && 
            (getQInfoParm.gq_mask & IIAPI_GQ_PROCEDURE_RET)) 
    {
        RETURN_LONG(getQInfoParm.gq_procedureReturn);
    }
    RETURN_NULL();
}
/* }}} */

/* {{{ proto bool ingres_rollback(resource link)
   Roll back a transaction */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_rollback)
#else
PHP_FUNCTION(ingres_rollback)
#endif
{
    zval *link;
    II_LINK *ii_link;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &link) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, &link, -1, "Ingres Link", le_ii_link, le_ii_plink);

    if (_rollback_transaction(ii_link TSRMLS_CC))
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ingres_commit(resource link)
   Commit a transaction */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_commit)
#else
PHP_FUNCTION(ingres_commit)
#endif
{
    zval *link;
    II_LINK *ii_link;
    IIAPI_CANCELPARM   cancelParm;
    IIAPI_CLOSEPARM    closeParm;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &link) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, &link, -1, "Ingres Link", le_ii_link, le_ii_plink);

    /* clean up any un-freed statements/results */
    _free_ii_link_result_list(ii_link TSRMLS_CC);

    if ( ii_link->tranHandle != NULL )
    {
        if ( _commit_transaction(ii_link TSRMLS_CC) == II_FAIL )
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occurred when committing the transaction");
            RETURN_FALSE;
        }
    }
    else
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "There appears to be no active transaction to commit");
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ingres_autocommit(resource link)
   Switch auto-commit on or off */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_autocommit)
#else
PHP_FUNCTION(ingres_autocommit)
#endif
{
    zval *link;
    II_LINK *ii_link;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &link) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, &link, -1, "Ingres Link", le_ii_link, le_ii_plink);


    /* If we are emulating auto commit any active result-sets need to be freed and committed to change the */
    /* auto-commit state */
    if ((ii_link->auto_multi))
    {
        /* if there are active result-sets */
        if (ii_link->result_list_ptr)
        {
            /* clean up any un-freed statements/results */
            _free_ii_link_result_list(ii_link TSRMLS_CC);
        }
        /* commit the previous statement before changing the auto-commit state */
        if (_commit_transaction(ii_link TSRMLS_CC) == II_FAIL)
        {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "An error occur when issuing an internal commit");
        }
        ii_link->auto_multi = 0;
    }

    if (_autocommit_transaction(ii_link TSRMLS_CC) == II_FAIL)
    {
        if (ii_link->result_list_ptr)
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to change the auto-commit state due to open transactions");
        }
        else
        {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "An error occur when changing the auto-commit state");
        }
        RETURN_FALSE;
    };

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ingres_autocommit_state(resource link)
   Return the current auto-commit state TRUE=ON, FALSE=OFF*/
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_autocommit_state)
#else
PHP_FUNCTION(ingres_autocommit_state)
#endif
{
    zval *link;
    II_LINK *ii_link;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &link) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, &link, -1, "Ingres Link", le_ii_link, le_ii_plink);

    /* Return TRUE if auto-commit is on or we are emulating auto-commit) */ 
    if ((ii_link->auto_multi) || (ii_link->autocommit))
    {
        RETURN_TRUE;
    }
    else
    {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto long ingres_next_error()
   Gets the last ingres error code generated */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_next_error)
#else
PHP_FUNCTION(ingres_next_error)
#endif
{
    short int i;

    IIAPI_GETEINFOPARM error_info;


    if (INGRESG(errorHandle) != NULL)
    {
        INGRESG(error_number) = 0;
        if (INGRESG(error_text) != NULL )
        {
            efree(INGRESG(error_text));
            INGRESG(error_text) = NULL;
        }
        for ( i = 0; i < (II_SQLSTATE_LEN + 1); i++ ) 
        {
            INGRESG(error_sqlstate)[i] = '\0';
        }
        error_info.ge_errorHandle = INGRESG(errorHandle);
        IIapi_getErrorInfo(&error_info);

        switch (error_info.ge_status)
        {
            case IIAPI_ST_SUCCESS:
                INGRESG(error_number) = error_info.ge_errorCode;
                INGRESG(error_text) = estrdup(error_info.ge_message);
                memcpy(INGRESG(error_sqlstate),error_info.ge_SQLSTATE, II_SQLSTATE_LEN + 1);
                RETURN_TRUE;
                break;
            case IIAPI_ST_NO_DATA:
                RETURN_FALSE;
                break;
            default: /* An error occured with IIapi_getErrorInfo() */
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "IIapi_getErrorInfo error, status returned was : %d", error_info.ge_status );
                RETURN_FALSE;
                break;
        }
    }
    else
    {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto long ingres_errno([resource link])
   Gets the last ingres error code generated */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_errno)
#else
PHP_FUNCTION(ingres_errno)
#endif
{
    php_ii_error(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto long ingres_error([resource link])
   Gets the last ingres error message generated */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_error)
#else
PHP_FUNCTION(ingres_error)
#endif
{
    php_ii_error(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto long ingres_errsqlstate([resource link])
   Gets the last ingres error sqlstate code generated */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_errsqlstate)
#else
PHP_FUNCTION(ingres_errsqlstate)
#endif
{
    php_ii_error(INTERNAL_FUNCTION_PARAM_PASSTHRU, 2);
}
/* }}} */

/* {{{ php_ii_error (INTERNAL_FUNCTION_PARAMETERS, int mode) */
static void php_ii_error(INTERNAL_FUNCTION_PARAMETERS, int mode)
{
    zval *value = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"|z" , &value) == FAILURE) 
    {
        RETURN_FALSE;
    }

    switch (mode)
    {
        case 0:
            RETVAL_LONG(INGRESG(error_number));
            break;
        case 1:
            if ( INGRESG(error_text) != NULL )
            {
                RETVAL_STRING(INGRESG(error_text),1);
            }
            else
            {
                RETVAL_NULL();
            }
            break;
        case 2:
            RETVAL_STRING(INGRESG(error_sqlstate),1);
            break;
        default:
            break;
    }
}
/* }}} */

/* {{{ static long php_ii_paramcount(char *statement TSRMLS_DC) */
/* ----------------------------------------------------------------------
 * int php_ii_paramcount(char *statement TSRMLS_DC)
 *
 * Count the placeholders (?) parameters in the statement
 * return -1 for error. 0 or number of question marks
 *
 * Thanks to ext/informix (based on php_intifx_preparse).
 *
 * ----------------------------------------------------------------------
*/
static long php_ii_paramcount(char *statement TSRMLS_DC)
{
    char *src;
    char *dst;
    int   idx = 0;
    int   style = 0;
    int   laststyle = 0;
    char  ch;
    char  end_quote;
    
    end_quote = '\0';
    src = statement;
    dst = statement;

    while ((ch = *src++) != '\0')
    {
        if (ch == end_quote)
        {
            end_quote = '\0';
        } else if (end_quote != '\0')
        {
            *dst++ = ch;
            continue;
        } else if (ch == '\'' || ch == '\"')
        {
            end_quote = ch;
        } else if (ch == '{')
        {
            end_quote = '}';
        }
        if (ch == '?')
        {
            /* X/Open standard       */
            *dst++ = '?';
            idx++;
            style = 3;
        } 
        else {
            *dst++ = ch;
            continue;
        }

        if (laststyle && style != laststyle)
        {
            return -1;
        }
        laststyle = style;
    }

    *dst = '\0';
    return(idx);
}
/* }}} */

/* {{{ static void php_ii_check_procedure(char *statement, II_LINK *ii_link TSRMLS_DC) */
/* check to see if the query is for a procedure or not, if it is return the procedure name 
 * 
 * Procedure calls come in two forms either:
 * execute procedure procname
 * call procedure 
 *
 */
static char *php_ii_check_procedure(char *statement, II_LINK *ii_link TSRMLS_DC)
{
    char *src;
    char *end_space;
    char *end_term;
    char *end_bracket;
    char *end_addr;
    char exec_proc[19];
    char call_proc[6];
    char *tmp_procname;
    int  style = 0;
    int  start;
    int  proc_len;
    int  pos;

    sprintf(exec_proc,"execute procedure ");
    sprintf(call_proc,"call ");

    if (strncmp(statement,exec_proc,18) == 0 ) 
    {
        style = 1;
    } 
    else if (strncmp(statement,call_proc,5) == 0 )
    {
        style = 2;
    }
    
    if ( style != 0 )
    {
        switch (style)
        {
            case 1:
                start = 18;
                break;
            case 2:
                start = 5;
                break;
            default:
                /* should not be here */
                break;
        }
        src = statement;
        src = src + start;

        /* look for a space, bracket or null terminator to determine end of */ 
        /* the procedure name, end_term should never be NULL */
        end_term = strchr(src,'\0');
        end_space = strchr(src,' ');
        end_bracket = strchr(src,'(');

        if ( end_space == NULL && end_bracket == NULL ) 
        {
            proc_len = end_term - src;
            end_addr = end_term;
        } 
        else if ( end_space != NULL && end_bracket == NULL )    
        {
            proc_len = end_space - src;
            end_addr = end_space;
        }
        else if ( end_space == NULL && end_bracket != NULL )    
        {
            proc_len = end_bracket - src;
            end_addr = end_bracket;
        }
        else if ( end_space != NULL && end_bracket != NULL )    
        {
            if ( end_space > end_bracket) 
            {
                proc_len = end_bracket - src;
                end_addr = end_bracket;
            }
            else
            {
                proc_len = end_space - src;
                end_addr = end_space;
            }
        }

        tmp_procname = malloc(proc_len + 1);
        for ( pos = 0; pos <= proc_len; pos++)
        {
            tmp_procname[pos] = src[pos];
        }
        tmp_procname[proc_len]='\0';
    }
    else
    {
        tmp_procname= NULL;
    }

    return tmp_procname;
}
/* }}} */

/* {{{ static int php_ii_query_type(char *statement TSRMLS_DC) */
/* Returns the type of query being called in order to better handle certain types of query. */
static int php_ii_query_type(char *statement TSRMLS_DC)
{
    int count = 0;
    char *statement_ptr = NULL;

    statement_ptr = statement;
    /* Look for some white space */
    while (isspace(*statement_ptr))
    {
        statement_ptr++;
    }
    for ( count = 0; count < INGRES_NO_OF_COMMANDS; count++ )
    {
        if (strncasecmp(SQL_COMMANDS[count].command, statement_ptr, strlen(SQL_COMMANDS[count].command)) == 0 )
        {
            return SQL_COMMANDS[count].code;
        }
    }
    
    return -1;
}
/* }}} */

/* {{{ static short int php_ii_set_environment_options(zval *options, II_LINK *ii_link) */
/*     Sets up options provided to ingres_connect() via a parameter array */
static short int php_ii_set_environment_options (zval *options, II_LINK *ii_link TSRMLS_DC)
{

#if defined(IIAPI_VERSION_2)
    II_LONG parameter_id;
    IIAPI_SETENVPRMPARM    setEnvPrmParm;
    zval **data;
    char *key;
    unsigned long index;
    unsigned int key_len;
    char *temp_string;
    long temp_long;
    II_BOOL ignore;
    HashTable *arr_hash;

    arr_hash = Z_ARRVAL_P(options);

    setEnvPrmParm.se_envHandle = ii_link->envHandle;

#ifndef __VMS
    for ( zend_hash_internal_pointer_reset(arr_hash);
          zend_hash_has_more_elements(arr_hash) == SUCCESS; 
          zend_hash_move_forward(arr_hash))
#else
    zend_hash_internal_pointer_reset(arr_hash);
    while (zend_hash_move_forward(arr_hash) == SUCCESS)
#endif
    {
        ignore = FALSE;

        if (zend_hash_get_current_key_ex(arr_hash, &key, &key_len, &index, 0, NULL) == HASH_KEY_IS_STRING)
        {
            zend_hash_get_current_data_ex(arr_hash, (void**)&data, NULL);

            if ( strcmp("role", key) == 0 ) 
            {
                    ignore = TRUE;
            }
            else if ( strcmp("group", key) == 0 )
            {
                    ignore = TRUE;
            }
            else if ( strcmp("effective_user", key) == 0 )
            {
                    ignore = TRUE;
            }
            else if ( strcmp("dbms_password", key) == 0 )
            {
                    ignore = TRUE;
            }
            else if ( strcmp("table_structure", key) == 0 )
            {
                ignore = TRUE;
            }
            else if ( strcmp("index_structure", key) == 0 )
            {
                ignore = TRUE;
            }
            else if ( strcmp("login_local", key) == 0 )
            {
                ignore = TRUE;
            }
            else if ( strcmp("timezone", key) == 0 )
            {
                    parameter_id = IIAPI_EP_TIMEZONE;
            }
            else if (strcmp( "date_format", key) == 0 )
            {
                    parameter_id = IIAPI_EP_DATE_FORMAT;
            }
            else if ( strcmp("decimal_separator", key) == 0 ) 
            {
                    parameter_id = IIAPI_EP_DECIMAL_CHAR;
            }
            else if ( strcmp("date_century_boundary", key) == 0 )
            {
                    parameter_id = IIAPI_EP_CENTURY_BOUNDARY;
            }
            else if ( strcmp("money_lort", key) == 0 ) /* leading or trailing money sign, default is leading*/
            {
                    parameter_id = IIAPI_EP_MONEY_LORT;
            }
            else if ( strcmp("money_sign", key) == 0 ) /* defaults to the ingres variable II_MONEY_FORMAT or "$" */
            {
                    parameter_id = IIAPI_EP_MONEY_SIGN;
            }
            else if ( strcmp("money_precision", key) == 0 ) /* defaults to 2 if not set */
            {
                    parameter_id = IIAPI_EP_MONEY_PRECISION;
            }
            else if ( strcmp("float4_precision", key) == 0 )
            {
                    parameter_id = IIAPI_EP_FLOAT4_PRECISION;
            }
            else if ( strcmp("float8_precision", key) == 0 ) 
            {
                    parameter_id = IIAPI_EP_FLOAT8_PRECISION;
            }
            else if ( strcmp("blob_segment_length", key) == 0 ) 
            {
                    parameter_id = IIAPI_EP_MAX_SEGMENT_LEN;
                    convert_to_long_ex(data);
                    INGRESG(blob_segment_length) = Z_LVAL_PP(data);
            }
            else 
            {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown connection option '%s'",key );
                    return II_FAIL;
            }
        }
        else
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unexpected index in evironment options array.");
            return II_FAIL;
        }

        if (ignore != TRUE) {

            setEnvPrmParm.se_paramID = parameter_id;

            switch (Z_TYPE_PP(data))
            {
                case IS_STRING:
                    convert_to_string_ex(data);
                    temp_string = Z_STRVAL_PP(data);
                    setEnvPrmParm.se_paramValue = (II_PTR)temp_string;
                    break;
                case IS_LONG:
                case IS_BOOL:
                    convert_to_long_ex(data);
                    temp_long = Z_LVAL_PP(data);
                    setEnvPrmParm.se_paramValue = (II_PTR)&temp_long;
                    break;
                default:
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown option type, %d, in environment options", Z_TYPE_PP(data));
                    return II_FAIL;
            }

            IIapi_setEnvParam( &setEnvPrmParm );

            if (setEnvPrmParm.se_status != IIAPI_ST_SUCCESS)
            {
                if ( Z_TYPE_PP(data) == IS_STRING )
                {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to set option, %s, with value, %s", key, Z_STRVAL_PP(data));
                    return II_FAIL;
                }
                else
                {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to set option, %s, with value, %ld. Error code %d.", key, Z_LVAL_PP(data), setEnvPrmParm.se_status );
                    return II_FAIL;
                }
            }
        }
    }

    return II_OK;
#else
    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Setting environment options requires Ingres II 2.5 or newer");
    return II_FAIL;
#endif

}
/* }}} */

/* {{{ static short int php_ii_set_connect_options(zval *options, II_LINK *ii_link, char *database TSRMLS_DC) */
/*     Sets up options provided to ingres_connect() via a parameter array */
static short int php_ii_set_connect_options(zval *options, II_LINK *ii_link, char *database TSRMLS_DC)
{
    II_LONG parameter_id;
    IIAPI_SETCONPRMPARM    setConPrmParm;
    zval **data;
    char *key;
    unsigned long index;
    unsigned int key_len;
    char *temp_string;
    long temp_long;
    II_BOOL ignore;
    HashTable *arr_hash;

    arr_hash = Z_ARRVAL_P(options);

#ifndef __VMS
    for ( zend_hash_internal_pointer_reset(arr_hash);
          zend_hash_has_more_elements(arr_hash) == SUCCESS; 
          zend_hash_move_forward(arr_hash))
#else
    zend_hash_internal_pointer_reset(arr_hash);
    while (zend_hash_move_forward(arr_hash) == SUCCESS)
#endif
    {
        ignore = FALSE;

        if (zend_hash_get_current_key_ex(arr_hash, &key, &key_len, &index, 0, NULL) == HASH_KEY_IS_STRING)
        {
            zend_hash_get_current_data_ex(arr_hash, (void**)&data, NULL);

            if ( strcmp("role", key) == 0 ) 
            {
                parameter_id = IIAPI_CP_APP_ID;
            }
            else if ( strcmp("group", key) == 0 )
            {
                parameter_id = IIAPI_CP_GROUP_ID;
            }
            else if ( strcmp("effective_user", key) == 0 )
            {
                parameter_id = IIAPI_CP_EFFECTIVE_USER;
            }
            else if ( strcmp("dbms_password", key) == 0 )
            {
                parameter_id = IIAPI_CP_DBMS_PASSWORD;
            }
            else if ( strcmp("table_structure", key) == 0 )
            {
                parameter_id = IIAPI_CP_RESULT_TBL;
            }
            else if ( strcmp("index_structure", key) == 0 )
            {
                parameter_id = IIAPI_CP_SECONDARY_INX;
            }
#if defined(IIAPI_VERSION_3)
            else if ( strcmp("login_local", key) == 0 )
            {
                if (ii_link->apiLevel >= IIAPI_LEVEL_2)
                {
                    parameter_id = IIAPI_CP_LOGIN_LOCAL;
                }
            }
#endif
            else if ( strcmp("timezone", key) == 0 )
            {
                ignore = TRUE;
            }
            else if (strcmp( "date_format", key) == 0 )
            {
                ignore = TRUE;
            }
            else if ( strcmp("decimal_separator", key) == 0 ) 
            {
                ignore = TRUE;
            }
            else if ( strcmp("date_century_boundary", key) == 0 )
            {
                ignore = TRUE;
            }
            else if ( strcmp("money_lort", key) == 0 ) /* leading or trailing money sign, default is leading*/
            {
                ignore = TRUE;
            }
            else if ( strcmp("money_sign", key) == 0 ) /* defaults to the ingres variable II_MONEY_FORMAT or "$" */
            {
                ignore = TRUE;
            }
            else if ( strcmp("money_precision", key) == 0 ) /* defaults to 2 if not set */
            {
                ignore = TRUE;
            }
            else if ( strcmp("float4_precision", key) == 0 )
            {
                ignore = TRUE;
            }
            else if ( strcmp("float8_precision", key) == 0 ) 
            {
                ignore = TRUE;
            }
            else if ( strcmp("blob_segment_length", key) == 0 ) 
            {
                ignore = TRUE;
            }

            else 
            {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown connection option '%s'",key );
                    return II_FAIL;
            }
        }
        else
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unexpected index in connection options array.");
            return II_FAIL;
        }

        if ( ignore != TRUE )
        {

            setConPrmParm.sc_genParm.gp_callback = NULL;
#if defined(IIAPI_VERSION_2)
            setConPrmParm.sc_connHandle = ii_link->envHandle;
#else
            setConPrmParm.sc_connHandle = NULL;
#endif
            setConPrmParm.sc_paramID = parameter_id;

            switch (Z_TYPE_PP(data))
            {
                case IS_STRING:
                    convert_to_string_ex(data);
                    temp_string = Z_STRVAL_PP(data);
                    setConPrmParm.sc_paramValue = temp_string;
                    break;
                case IS_LONG:
                case IS_BOOL:
                    convert_to_long_ex(data);
                    temp_long = Z_LVAL_PP(data);
                    setConPrmParm.sc_paramValue = (II_PTR)&temp_long;
                    break;
                default:
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown option type, %d, in connection options", Z_TYPE_PP(data));
                    return II_FAIL;
            }

            IIapi_setConnectParam( &setConPrmParm );

            if (!ii_sync(&(setConPrmParm.sc_genParm)) || ii_success(&(setConPrmParm.sc_genParm), &ii_link->errorHandle TSRMLS_CC) == II_FAIL)
            {
                if ( Z_TYPE_PP(data) == IS_STRING )
                {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to set option, %s, with value, %s", key, Z_STRVAL_PP(data));
                    return II_FAIL;
                }
                else
                {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to set option, %s, with value, %ld", key, Z_LVAL_PP(data));
                    return II_FAIL;
                }
            }

            ii_link->connHandle = setConPrmParm.sc_connHandle;
        }
    }
    return II_OK;
}
/* }}} */

/* {{{ static char *php_ii_convert_param_markers (char *statement TSRMLS_DC) */
/* takes a statement with ? param markers and converts them to ~V */
static void php_ii_convert_param_markers (char *query, char *converted_query TSRMLS_DC)
{
    char ch, tmp_ch;
    char *p, *tmp_p;
    int j;

    sprintf(converted_query,"\0");

    j = 0;

    p = query;
    tmp_p = converted_query;

    while ( (ch = *p++) != '\0') 
    {
        if ( ch == '?' )
        {
            if ( *(p-2) != ' ') /* check for space before '?' */
                                /* if there is no space we add '~V' */
                                /* ingres will error with "Invalid operator '~V'" */
            {
                *tmp_p = ' ';
                *tmp_p++;
            }

            *tmp_p = '~';
            *tmp_p++;
            *tmp_p = 'V';
            
            if ( *p != ' ') /* check for space after '?' */
                            /* if there is no space we add '~V' */
                            /* ingres will error with "Invalid operator '~V'" */
            {
                *tmp_p++;
                *tmp_p = ' ';
            }
        }
        else
        {
            *tmp_p = ch;
        }
        tmp_p++;

        tmp_ch = *p; 

    }
    *tmp_p = '\0'; /* terminate the new query */
}
/* }}} */

/* {{{ proto string ingres_set_environment(resource link, array options)
   Sets various environment options for Ingres */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_set_environment)
#else
PHP_FUNCTION(ingres_set_environment)
#endif
{
    zval *link = NULL;
    zval *options = NULL;
    II_LINK *ii_link;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"ra" , &link, &options) == FAILURE) 
    {
        RETURN_NULL();
    }

    ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, &link, -1, "Ingres Link", le_ii_link, le_ii_plink);

    if ( php_ii_set_environment_options(options, ii_link TSRMLS_CC) == II_FAIL )
    {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to set environment options provided");
        RETURN_FALSE;
    }    
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string ingres_escape_string(resource link, string string_to_escape)
   Escape special characters for use in Ingres queries */
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_escape_string)
#else
PHP_FUNCTION(ingres_escape_string)
#endif
{
    zval *link; 
    II_LINK *ii_link;
    char *str_in = NULL;
    int str_in_len = 0;
    char *str_out = NULL;
    int str_out_len = 0;
    char * str_out_ptr = NULL;
    int pos = 0;

    /* At the moment the link resource is not actually required but it may be needed in the future to */
    /* Implement character escaping for different versions of the DBMS server */
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"rs" , &link, &str_in, &str_in_len) == FAILURE) 
    {
        RETURN_FALSE;
    }

    /* Allocate memory for return value */
    /* We are allocating 9 times the space should the string be full of \0 values */
    /* TODO - allocate double the size of str_in_len, if we run out of space double that, and so on */
    str_out = safe_emalloc(str_in_len, 9, 1);

    str_out_ptr = str_out;

    for ( pos = 0; pos < str_in_len; pos++ )
    {
        switch (str_in[pos])
        {
            case '\'': /* ' becomes '' */
                memcpy(str_out_ptr,"''",2);
                str_out_ptr +=2;
                break;
            case '\0': /* \0 becomes x'00' */
                if (str_in_len == 1) /* a single \0 */
                {
                    memcpy(str_out_ptr,"x'00'",5);
                    str_out_ptr +=5;
                }
                else if (pos == 0) /* At the start of str_in */
                {
                    memcpy(str_out_ptr,"x'00'+'",7);
                    str_out_ptr +=7;
                }
                else if (pos == (str_in_len - 1)) /* At the end of str_in */
                {
                    memcpy(str_out_ptr,"'+x'00'",7);
                    str_out_ptr +=7;
                }
                else /* Some where in the middle */
                {
                    memcpy(str_out_ptr,"'+x'00'+'",9);
                    str_out_ptr +=9;
                }
                break;
            default: /* No "conversion", just copy and move on... */
                *str_out_ptr = str_in[pos];
                str_out_ptr++;
                break;

        }
    }

    *str_out_ptr = '\0'; /* terminate the new string */
    str_out_len = str_out_ptr - str_out;
    /* reduce the buffer down to the required size */
    str_out = erealloc(str_out, str_out_len + 1);

    RETURN_STRINGL(str_out, str_out_len, 0);
}
/* }}} */

/* {{{ static short php_ii_bind_params (II_RESULT *ii_result, zval *queryParams) */
/* Binds and sends data for parameters passed via queryParams */
static short php_ii_bind_params (INTERNAL_FUNCTION_PARAMETERS, II_RESULT *ii_result, zval *queryParams, char *paramtypes)
{
    zval **val = NULL;
    IIAPI_SETDESCRPARM    setDescrParm;
    IIAPI_PUTPARMPARM    putParmParm;
    IIAPI_DESCRIPTOR    *descriptorInfo;
    IIAPI_DATAVALUE        *columnData;
    int param;
    II_INT2                columnType;
    HashTable *arr_hash;
    HashPosition pointer;
    
    double tmp_double;
    long tmp_long;
    char *tmp_string = NULL;
    char *tmp_lob = NULL;
    char *tmp_lob_ptr;
    char *key;
    unsigned int key_len;
    unsigned long index;
    long lob_len;
    long segment_length;
    short param_offset = 0;
    char *types;
    short unicode_lob = 0;

#if defined (IIAPI_VERSION_3)
    UTF8 *string_start = NULL;
    UTF16 *tmp_utf16_string = NULL; 
    UTF16 *tmp_utf16_string_ptr;
    II_INT2  utf16_string_len = 0;
    ConversionResult result = conversionOK;
#endif

    if ( ii_result->paramCount > 0 )
    {
        arr_hash = Z_ARRVAL_P(queryParams);
        zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
    }

    /* if we are sending params then we need to describe them into to Ingres */
    /* if no parameters have been provided to a procedure call there is always 1 */
    /* parameter, the procedure name */

    setDescrParm.sd_genParm.gp_callback = NULL;
    setDescrParm.sd_genParm.gp_closure = NULL;
    setDescrParm.sd_stmtHandle = ii_result->stmtHandle;

    /* If we are executing a procedure or a cursor that has been prepared then we need to bind an additional */
    /* parameter */
    if ((ii_result->queryType == IIAPI_QT_EXEC_PROCEDURE) || 
        ((ii_result->queryType == IIAPI_QT_OPEN)  && (ii_result->prepared)))
    {
        /* bump descriptorCount to allow for procedure name */
        setDescrParm.sd_descriptorCount = ii_result->paramCount + 1;
        param_offset = 1;
    }
    else
    {
        setDescrParm.sd_descriptorCount = ii_result->paramCount;
    }

    descriptorInfo = (IIAPI_DESCRIPTOR *) safe_emalloc(sizeof(IIAPI_DESCRIPTOR),setDescrParm.sd_descriptorCount, 0);
    setDescrParm.sd_descriptor = descriptorInfo;

    if (ii_result->queryType != IIAPI_QT_EXEC_PROCEDURE)
    {
        columnType = IIAPI_COL_QPARM;
    }
    else
    {
        columnType = IIAPI_COL_PROCPARM;
    }

    /* extract the paramtypes */

    if ( paramtypes != NULL )
    {
        types = emalloc(strlen(paramtypes));
        memcpy(types,paramtypes,strlen(paramtypes));
    }

    for ( param = 0 ; param < setDescrParm.sd_descriptorCount ; param++)
    {

        if (((ii_result->queryType == IIAPI_QT_EXEC_PROCEDURE) || 
            ((ii_result->queryType == IIAPI_QT_OPEN) && (ii_result->prepared))) && (param == 0)) 
        { 
            if (ii_result->queryType == IIAPI_QT_EXEC_PROCEDURE)
            {
                /* setup the first parameter as the procedure name */
                setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_CHA_TYPE;
                setDescrParm.sd_descriptor[param].ds_length = strlen(ii_result->procname);
                setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                setDescrParm.sd_descriptor[param].ds_precision = 0;
                setDescrParm.sd_descriptor[param].ds_scale = 0;
                setDescrParm.sd_descriptor[param].ds_columnType = IIAPI_COL_SVCPARM;
                setDescrParm.sd_descriptor[param].ds_columnName = NULL;
            }
            else /* ii_result->queryType == IIAPI_QT_OPEN */
            {
                setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_CHA_TYPE;
                setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                //setDescrParm.sd_descriptor[param].ds_length = 2;
                setDescrParm.sd_descriptor[param].ds_length = strlen(ii_result->cursor_id);
                setDescrParm.sd_descriptor[param].ds_precision = 0;
                setDescrParm.sd_descriptor[param].ds_scale = 0;
                setDescrParm.sd_descriptor[param].ds_columnType = IIAPI_COL_SVCPARM;
                setDescrParm.sd_descriptor[param].ds_columnName = NULL;
            }
        } 
        else 
        {

            if (zend_hash_get_current_data_ex(arr_hash, (void **)&val, &pointer) == FAILURE)
            {
                efree(descriptorInfo);
                return II_FAIL;
            }

            if ((ii_result->procname != NULL) && (zend_hash_get_current_key_ex(arr_hash, &key, &key_len, &index, 0, &pointer) == FAILURE))
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING,"Error getting parameter key");
                efree(descriptorInfo);
                return II_FAIL;
            }
            if ((ii_result->procname) && (PZVAL_IS_REF(*val))) {
                php_error_docref(NULL TSRMLS_CC, E_ERROR,"Byref parameters can only be used against procedures");
                return II_FAIL;
            }

            if ((ii_result->procname != NULL) && (PZVAL_IS_REF(*val))) {
                columnType = IIAPI_COL_PROCBYREFPARM;
            }

            if (paramtypes)
            {
                /* bind parameters using the types indicated */
                if ( ii_result->procname == NULL )
                {
                    setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                }
                else
                {
                    setDescrParm.sd_descriptor[param].ds_columnName = key;
                }
                switch (types[param - param_offset])
                {
                    case 'B': /* long byte */
                        convert_to_string_ex(val);
                        setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_LBYTE_TYPE;
                        setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                        setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val);
                        setDescrParm.sd_descriptor[param].ds_precision = 0;
                        setDescrParm.sd_descriptor[param].ds_scale = 0;
                        setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                        break;
                    case 'b': /* byte */
                        convert_to_string_ex(val);
                        setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_CHA_TYPE;
                        setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                        setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val);
                        setDescrParm.sd_descriptor[param].ds_precision = 0;
                        setDescrParm.sd_descriptor[param].ds_scale = 0;
                        setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                        break;
                    case 'f': /* float */
                    case 'm': /* money */
                        convert_to_double_ex(val);
                        setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_FLT_TYPE;
                        setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                        setDescrParm.sd_descriptor[param].ds_length = sizeof(Z_DVAL_PP(val));
                        setDescrParm.sd_descriptor[param].ds_precision = 31;
                        setDescrParm.sd_descriptor[param].ds_scale = 15;
                        setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                        break;
                    case 'T': /* long text */
                    case 'V': /* long varchar */
                        convert_to_string_ex(val);
                        setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_LVCH_TYPE;
                        setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                        setDescrParm.sd_descriptor[param].ds_length =  Z_STRLEN_PP(val);
                        setDescrParm.sd_descriptor[param].ds_precision = 0;
                        setDescrParm.sd_descriptor[param].ds_scale = 0;
                        setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                        setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                        break;
#if defined (IIAPI_VERSION_3)
                    case 'M': /* long nvarchar */
                        if (ii_result->apiLevel >= IIAPI_LEVEL_2)
                        {
                            convert_to_string_ex(val);
                            setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_LNVCH_TYPE;
                            setDescrParm.sd_descriptor[param].ds_precision = 0;
                            setDescrParm.sd_descriptor[param].ds_scale = 0;
                            setDescrParm.sd_descriptor[param].ds_columnType = columnType;

                            /* If this is a NULL value being passed we should pick it up */
                            if (Z_TYPE_PP(val) == IS_NULL)
                            {
                                setDescrParm.sd_descriptor[param].ds_nullable = TRUE;
                                setDescrParm.sd_descriptor[param].ds_length = 32;
                            }
                            else
                            {
                                convert_to_string_ex(val);
                                if (INGRESG(utf8)) {
                                    tmp_utf16_string = emalloc((Z_STRLEN_PP(val) * 4));
                                    string_start = (UTF8 *)Z_STRVAL_PP(val);
                                    tmp_utf16_string_ptr = tmp_utf16_string;
                                    result = ConvertUTF8toUTF16((const UTF8 **) &string_start,  string_start + Z_STRLEN_PP(val) + 1, &tmp_utf16_string_ptr, tmp_utf16_string_ptr + (Z_STRLEN_PP(val) * 4) , strictConversion);
                                    utf16_string_len = ((tmp_utf16_string_ptr - 1) - (tmp_utf16_string)) * 2;
                                    setDescrParm.sd_descriptor[param].ds_length = utf16_string_len; 
                                    efree(tmp_utf16_string);
                                    tmp_utf16_string = NULL;
                                }
                                else
                                {
                                    setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val);
                                }
                                setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                            }


                            if ( ii_result->procname == NULL )
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                            }
                            else
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = key;
                            }
                        }
                        else
                        {
                            php_error_docref(NULL TSRMLS_CC, E_ERROR, "This DBMS server does not support the LONG NVARCHAR type" );
                        }
                        break;                        
#endif /* IIAPI_VERSION_3 */
                    case 'i': /* integer */
                        setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_INT_TYPE;
                        setDescrParm.sd_descriptor[param].ds_precision = 0;
                        setDescrParm.sd_descriptor[param].ds_scale = 0;
                        setDescrParm.sd_descriptor[param].ds_columnType = columnType;

                        if (Z_TYPE_PP(val) == IS_NULL)
                        {
                            setDescrParm.sd_descriptor[param].ds_nullable = TRUE;
                            setDescrParm.sd_descriptor[param].ds_length = sizeof(II_INT4);
                        }
                        else
                        {
                            convert_to_long_ex(val);
                            setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                            setDescrParm.sd_descriptor[param].ds_length = sizeof(Z_LVAL_PP(val));
                        }
                        break;
#if defined (IIAPI_VERSION_3)
                    case 'n': /* nchar NFC/NFD UTF-16*/
                        if (ii_result->apiLevel >= IIAPI_LEVEL_2)
                        {
                            setDescrParm.sd_descriptor[param].ds_precision = 0;
                            setDescrParm.sd_descriptor[param].ds_scale = 0;
                            setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                            setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_NCHA_TYPE;

                            /* If this is a NULL value being passed we should pick it up */
                            if (Z_TYPE_PP(val) == IS_NULL)
                            {
                                setDescrParm.sd_descriptor[param].ds_nullable = TRUE;
                                setDescrParm.sd_descriptor[param].ds_length = 32;
                            }
                            else
                            {
                                convert_to_string_ex(val);
                                if (INGRESG(utf8)) {
                                    tmp_utf16_string = emalloc((Z_STRLEN_PP(val) * 4));
                                    string_start = (UTF8 *)Z_STRVAL_PP(val);
                                    tmp_utf16_string_ptr = tmp_utf16_string;
                                    result = ConvertUTF8toUTF16((const UTF8 **) &string_start,  string_start + Z_STRLEN_PP(val) + 1, &tmp_utf16_string_ptr, tmp_utf16_string_ptr + (Z_STRLEN_PP(val) * 4) , strictConversion);
                                    utf16_string_len = ((tmp_utf16_string_ptr - 1) - (tmp_utf16_string)) * 2;
                                    setDescrParm.sd_descriptor[param].ds_length = utf16_string_len; 
                                    efree(tmp_utf16_string);
                                    tmp_utf16_string = NULL;
                                }
                                else
                                {
                                    setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val);
                                }
                                setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                            }

                            if ( ii_result->procname == NULL )
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                            }
                            else
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = key;
                            }
                        }
                        else
                        {
                            php_error_docref(NULL TSRMLS_CC, E_ERROR, "This DBMS server does not support the NCHAR type" );
                        }
                        break;
                    case 'N': /* nvarchar NFC/NFD UTF-16*/ 
                        if (ii_result->apiLevel >= IIAPI_LEVEL_2)
                        {
                            setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_NVCH_TYPE;
                            setDescrParm.sd_descriptor[param].ds_precision = 0;
                            setDescrParm.sd_descriptor[param].ds_scale = 0;
                            setDescrParm.sd_descriptor[param].ds_columnType = columnType;

                            if (Z_TYPE_PP(val) == IS_NULL)
                            {
                                setDescrParm.sd_descriptor[param].ds_length = 32;
                                setDescrParm.sd_descriptor[param].ds_nullable = TRUE;
                            }
                            else
                            {
                                convert_to_string_ex(val);
                                if (INGRESG(utf8)) {
                                    /* Convert the UTF-8 data we have to UTF-16 so Ingres will store it */
                                    tmp_utf16_string = emalloc((Z_STRLEN_PP(val) * 4) + 2);
                                    string_start = (UTF8 *)Z_STRVAL_PP(val);
                                    tmp_utf16_string_ptr = tmp_utf16_string + 1;
                                    result = ConvertUTF8toUTF16((const UTF8 **) &string_start,  string_start + Z_STRLEN_PP(val) + 1, &tmp_utf16_string_ptr, tmp_utf16_string_ptr + (Z_STRLEN_PP(val) * 4) , strictConversion);
                                    utf16_string_len = ((tmp_utf16_string_ptr - 1) - (tmp_utf16_string)) * 2;
                                    *((II_INT2*)(tmp_utf16_string)) = utf16_string_len/2;
                                    efree(tmp_utf16_string);
                                    tmp_utf16_string = NULL;

                                    setDescrParm.sd_descriptor[param].ds_length = utf16_string_len;
                                }
                                else /* assume UTF-16 */
                                {
                                    setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val) + 2;
                                }
                                setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                            }

                            if ( ii_result->procname == NULL )
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                            }
                            else
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = key;
                            }
                        }
                        else
                        {
                            php_error_docref(NULL TSRMLS_CC, E_ERROR, "This DBMS server does not support the NVARCHAR type" );
                        }
                        break;
#endif
                    case 'c': /* char */
                    case 'd': /* date */
                    case 't': /* text */
                    case 'D': /* decimal - treat a string since we want the exact */
                    case 'v': /* varchar */ 
                        convert_to_string_ex(val);
                        setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_VCH_TYPE;
                        setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                        setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val) + 2;
                        setDescrParm.sd_descriptor[param].ds_precision = 0;
                        setDescrParm.sd_descriptor[param].ds_scale = 0;
                        setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                        if ( ii_result->procname == NULL )
                        {
                            setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                        }
                        else
                        {
                            setDescrParm.sd_descriptor[param].ds_columnName = key;
                        }
                        break;
                    default:
                        break;
                }
            }
            else
            {
                /* Check to see if we have descriptors from DESCRIBE INPUT */
                if (ii_result->inputCount > 0)
                {
                    switch ((ii_result->inputDescr[param - param_offset]).ds_dataType)
                    {
                        case IIAPI_LBYTE_TYPE: /* long byte */
                        case IIAPI_BYTE_TYPE: /* byte */
                        case IIAPI_VBYTE_TYPE: /* byte varying */
                        case IIAPI_CHA_TYPE: /* char */
                        case IIAPI_CHR_TYPE: /* text fixed */
                        case IIAPI_DTE_TYPE: /* date */
#if defined(IIAPI_VERSION_5) 
                        case IIAPI_ADATE_TYPE:  /* SQL Date (aka ANSI DATE) */
                        case IIAPI_TIME_TYPE: /* Ingres Time */
                        case IIAPI_TMWO_TYPE: /* Time without Timezone */
                        case IIAPI_TMTZ_TYPE: /* Time with Timezone */
                        case IIAPI_TS_TYPE: /* Ingres Timestamp */
                        case IIAPI_TSWO_TYPE: /* Timestamp without Timezone */
                        case IIAPI_TSTZ_TYPE: /* Timestamp with Timezone */
                        case IIAPI_INTYM_TYPE: /* Interval Year to Month */
                        case IIAPI_INTDS_TYPE: /* Interval Day to Second */
#endif
                        case IIAPI_TXT_TYPE: /* text */
                        case IIAPI_DEC_TYPE: /* decimal - we need an exact value for decimal storage */
                        case IIAPI_VCH_TYPE: /* varchar */ 
                            convert_to_string_ex(val);
                            if ((ii_result->inputDescr[param - param_offset]).ds_dataType != IIAPI_LBYTE_TYPE) 
                            {
                                setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_VCH_TYPE;
                                setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val) + 2;
                            }
                            else
                            {
                                setDescrParm.sd_descriptor[param].ds_dataType = (ii_result->inputDescr[param - param_offset]).ds_dataType;
                                setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val);
                            }
                            setDescrParm.sd_descriptor[param].ds_precision = (ii_result->inputDescr[param - param_offset]).ds_precision;
                            setDescrParm.sd_descriptor[param].ds_scale = (ii_result->inputDescr[param - param_offset]).ds_scale;
                            setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                            setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                            break;
                        case IIAPI_MNY_TYPE: /* money */
                        case IIAPI_FLT_TYPE: /* float */
                            convert_to_double_ex(val);
                            setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_FLT_TYPE;
                            setDescrParm.sd_descriptor[param].ds_length = sizeof(Z_DVAL_PP(val));
                            setDescrParm.sd_descriptor[param].ds_precision = 31;
                            setDescrParm.sd_descriptor[param].ds_scale = 15;
                            setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                            setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                            break;
                        case IIAPI_LTXT_TYPE: /* long text */
                        case IIAPI_LVCH_TYPE: /* long varchar */
                            convert_to_string_ex(val);
                            setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_LVCH_TYPE;
                            setDescrParm.sd_descriptor[param].ds_length =  Z_STRLEN_PP(val);
                            setDescrParm.sd_descriptor[param].ds_precision = 0;
                            setDescrParm.sd_descriptor[param].ds_scale = 0;
                            setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                            setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                            setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                            break;
                        case IIAPI_INT_TYPE: /* INTEGER */
                            setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_INT_TYPE;
                            setDescrParm.sd_descriptor[param].ds_precision = 0;
                            setDescrParm.sd_descriptor[param].ds_scale = 0;
                            setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                            setDescrParm.sd_descriptor[param].ds_dataType = (ii_result->inputDescr[param - param_offset]).ds_dataType;
                            setDescrParm.sd_descriptor[param].ds_length = sizeof(Z_LVAL_PP(val));
                            setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                            break;
#if defined (IIAPI_VERSION_3)
                        case IIAPI_NCHA_TYPE: /* NCHAR */
                        case IIAPI_NVCH_TYPE: /* NVARCHAR */
                        case IIAPI_LNVCH_TYPE:/* LONG NVARCHAR */
                            if (ii_result->apiLevel >= IIAPI_LEVEL_2)
                            {
                                if (Z_TYPE_PP(val) == IS_NULL)
                                {
                                    setDescrParm.sd_descriptor[param].ds_length = 32;
                                    setDescrParm.sd_descriptor[param].ds_nullable = TRUE;
                                }
                                else
                                {
                                    convert_to_string_ex(val);
                                    if (INGRESG(utf8)) {
                                        /* Convert the UTF-8 data we have to UTF-16 so Ingres will store it */
                                        tmp_utf16_string = emalloc((Z_STRLEN_PP(val) * 4) + 2);
                                        string_start = (UTF8 *)Z_STRVAL_PP(val);
                                        tmp_utf16_string_ptr = tmp_utf16_string + 1;
                                        result = ConvertUTF8toUTF16((const UTF8 **) &string_start,  string_start + Z_STRLEN_PP(val) + 1, &tmp_utf16_string_ptr, tmp_utf16_string_ptr + (Z_STRLEN_PP(val) * 4) , strictConversion);
                                        utf16_string_len = ((tmp_utf16_string_ptr - 1) - (tmp_utf16_string)) * 2;
                                        efree(tmp_utf16_string);
                                        tmp_utf16_string = NULL;

                                        setDescrParm.sd_descriptor[param].ds_length = utf16_string_len;
                                    }
                                    else /* assume UTF-16 */
                                    {
                                        setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val) + 2;
                                    }
                                    setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                                }
                                setDescrParm.sd_descriptor[param].ds_dataType = (ii_result->inputDescr[param - param_offset]).ds_dataType;
                                setDescrParm.sd_descriptor[param].ds_nullable = (ii_result->inputDescr[param - param_offset]).ds_nullable;
                                setDescrParm.sd_descriptor[param].ds_precision = (ii_result->inputDescr[param - param_offset]).ds_precision;
                                setDescrParm.sd_descriptor[param].ds_scale = (ii_result->inputDescr[param - param_offset]).ds_scale;
                                setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                            }
                            else
                            {
                                php_error_docref(NULL TSRMLS_CC, E_WARNING, "This DBMS server does not support National Character types" );
                            }
                            break;
#endif
                        default:
                            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unsupported type, %d", (ii_result->inputDescr[param - param_offset]).ds_dataType);
                            break;
                    }
                    if ( ii_result->procname == NULL )
                    {
                        setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                    }
                    else
                    {
                        setDescrParm.sd_descriptor[param].ds_columnName = key;
                    }
                }
                else
                {
                    /* bind parameters based on what PHP thinks it has */
                    /* Process each parameter into our descriptor buffer */
                    switch (Z_TYPE_PP(val))
                    {
                        case IS_LONG:
                            /* TODO: does not handle int8 yet */
                            convert_to_long_ex(val);
                            setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_INT_TYPE;
                            setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                            setDescrParm.sd_descriptor[param].ds_length = sizeof(Z_LVAL_PP(val));
                            setDescrParm.sd_descriptor[param].ds_precision = 0;
                            setDescrParm.sd_descriptor[param].ds_scale = 0;
                            setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                            if ( ii_result->procname == NULL )
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                            }
                            else
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = key;
                            }
                            break;
                        case IS_DOUBLE:
                            convert_to_double_ex(val);
                            setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_FLT_TYPE;
                            setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                            setDescrParm.sd_descriptor[param].ds_length = sizeof(Z_DVAL_PP(val));
                            setDescrParm.sd_descriptor[param].ds_precision = 0;
                            setDescrParm.sd_descriptor[param].ds_scale = 0;
                            setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                            if ( ii_result->procname == NULL )
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                            }
                            else
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = key;
                            }
                            break;
                        case IS_STRING:
                            convert_to_string_ex(val);
                            setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_VCH_TYPE;
                            setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
                            setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val) + 2;
                            setDescrParm.sd_descriptor[param].ds_precision = 0;
                            setDescrParm.sd_descriptor[param].ds_scale = 0;
                            setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                            if ( ii_result->procname == NULL )
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                            }
                            else
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = key;
                            }
                            break;
                        case IS_NULL:
                            setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_CHA_TYPE;
                            setDescrParm.sd_descriptor[param].ds_nullable = TRUE;
                            setDescrParm.sd_descriptor[param].ds_length = 0;
                            setDescrParm.sd_descriptor[param].ds_precision = 0;
                            setDescrParm.sd_descriptor[param].ds_scale = 0;
                            setDescrParm.sd_descriptor[param].ds_columnType = columnType;
                            if ( ii_result->procname == NULL )
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = NULL;
                            }
                            else
                            {
                                setDescrParm.sd_descriptor[param].ds_columnName = key;
                            }
                            break;
                        default:
                            php_error_docref(NULL TSRMLS_CC, E_WARNING, "A parameter has been passed of unknown type" );
                            if ( ii_result->procname == NULL )
                            efree(descriptorInfo);
                            return II_FAIL;
                        }
                    }
                }

            if (((param_offset) && (param > 0)) || (!param_offset))
            {
                zend_hash_move_forward_ex(arr_hash, &pointer);
            }
            
        }

    } /* param=0; param < setDescrParm.sd_descriptorCount; param++ */

    if (((param_offset) && (setDescrParm.sd_descriptorCount > 1)) || ((!param_offset) && (ii_result->paramCount > 0)))
    {
        zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
    }

    IIapi_setDescriptor( &setDescrParm );

    if (ii_success(&(setDescrParm.sd_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        efree(descriptorInfo);
        return II_FAIL;
    }

    /*  Put query parameter values.  */
    putParmParm.pp_genParm.gp_callback = NULL;
    putParmParm.pp_genParm.gp_closure = NULL;
    putParmParm.pp_stmtHandle = ii_result->stmtHandle;
    putParmParm.pp_parmCount = 0; /* we will work this out as we go along */

    columnData = (IIAPI_DATAVALUE *) safe_emalloc(sizeof(IIAPI_DATAVALUE),setDescrParm.sd_descriptorCount, 0);
    putParmParm.pp_parmData = columnData;

    for ( param = 0 ; param < setDescrParm.sd_descriptorCount ; param++)
    {
        putParmParm.pp_parmCount=1; /* New parameter */
        
        if (((ii_result->queryType == IIAPI_QT_EXEC_PROCEDURE) || 
            ((ii_result->queryType == IIAPI_QT_OPEN) && (ii_result->prepared))) && (param == 0)) 
        {
            /* place the procedure name / cursor_id as the first parameter */
            putParmParm.pp_parmData[0].dv_null = FALSE;
            putParmParm.pp_parmData[0].dv_length = ii_result->queryType == IIAPI_QT_EXEC_PROCEDURE ? strlen(ii_result->procname) :  strlen(ii_result->cursor_id);
            putParmParm.pp_parmData[0].dv_value = ii_result->queryType == IIAPI_QT_EXEC_PROCEDURE ? ii_result->procname : ii_result->cursor_id;
        }
        else
        {

            if (zend_hash_get_current_data_ex(arr_hash, (void **)&val, &pointer) == FAILURE)
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING,"Error getting parameter from array");
                efree(columnData);
                efree(descriptorInfo);
                return II_FAIL;
            }

            switch (Z_TYPE_PP(val))
            {
                case IS_LONG:
                    convert_to_long_ex(val);
                    putParmParm.pp_parmData[0].dv_null = FALSE;
                    putParmParm.pp_parmData[0].dv_length = sizeof(Z_LVAL_PP(val));
                    tmp_long = Z_LVAL_PP(val);
                    putParmParm.pp_parmData[0].dv_value = &tmp_long;
                    break;
                case IS_DOUBLE:
                    convert_to_double_ex(val);
                    putParmParm.pp_parmData[0].dv_null = FALSE;
                    putParmParm.pp_parmData[0].dv_length = sizeof(Z_DVAL_PP(val)); 
                    tmp_double = Z_DVAL_PP(val);
                    putParmParm.pp_parmData[0].dv_value = &tmp_double;
                    break;
                case IS_STRING:
                    convert_to_string_ex(val);
                    if ( paramtypes != NULL ) 
                    {
                        switch (types[param - param_offset])
                        {
#if defined (IIAPI_VERSION_3)
                            case 'N': /* NVARCHAR */
                                if (INGRESG(utf8)) {
                                    /* Convert the UTF-8 data we have to UTF-16 so Ingres will store it */
                                    tmp_utf16_string = emalloc((Z_STRLEN_PP(val) * 4) + 2);
                                    string_start = (UTF8 *)Z_STRVAL_PP(val);
                                    tmp_utf16_string_ptr = tmp_utf16_string + 1;
                                    result = ConvertUTF8toUTF16((const UTF8 **) &string_start,  string_start + Z_STRLEN_PP(val) + 1, &tmp_utf16_string_ptr, tmp_utf16_string_ptr + (Z_STRLEN_PP(val) * 4) , strictConversion);
                                    utf16_string_len = ((tmp_utf16_string_ptr - 1) - (tmp_utf16_string + 1)) * 2;
                                    *((II_INT2*)(tmp_utf16_string)) = utf16_string_len/2;
                                    putParmParm.pp_parmData[0].dv_value = tmp_utf16_string;
                                    putParmParm.pp_parmData[0].dv_length = utf16_string_len + 2; 
                                }
                                else /* assume UTF-16 */
                                {
                                    /* copy the data to a new buffer then set the size  */
                                    /* of the string in chars at the beggining of the buffer */
                                    tmp_string = emalloc(Z_STRLEN_PP(val) + 2);
                                    memcpy(tmp_string + 2, Z_STRVAL_PP(val), Z_STRLEN_PP(val));
                                    /* set the 1st 2 bytes as the length of the string in chars */
                                    *((II_INT2*)(tmp_string)) = Z_STRLEN_PP(val)/2; 
                                    putParmParm.pp_parmData[0].dv_value = tmp_string;
                                    putParmParm.pp_parmData[0].dv_length = Z_STRLEN_PP(val) + 2; 
                                }
                                
                                break;
                            case 'n': /* NCHAR - for UTF-8 source data only */
                                if (INGRESG(utf8)) {
                                    tmp_utf16_string = emalloc((Z_STRLEN_PP(val) * 4));
                                    string_start = (UTF8 *)Z_STRVAL_PP(val);
                                    tmp_utf16_string_ptr = tmp_utf16_string;
                                    result = ConvertUTF8toUTF16((const UTF8 **) &string_start,  string_start + Z_STRLEN_PP(val) + 1, &tmp_utf16_string_ptr, tmp_utf16_string_ptr + (Z_STRLEN_PP(val) * 4) , strictConversion);
                                    utf16_string_len = ((tmp_utf16_string_ptr - 1) - (tmp_utf16_string)) * 2;
                                    putParmParm.pp_parmData[0].dv_value = (II_PTR *)tmp_utf16_string;
                                    putParmParm.pp_parmData[0].dv_length = utf16_string_len; 
                                }
                                else
                                {
                                    putParmParm.pp_parmData[0].dv_value = Z_STRVAL_PP(val);
                                    putParmParm.pp_parmData[0].dv_length = Z_STRLEN_PP(val); 
                                }
                                break;
#endif
                            case 'c': /* char */
                            case 'd': /* date */
                            case 't': /* text */
                            case 'D': /* decimal - treat a string since we want the exact */
                            case 'v': /* VARCHAR */
                                /* copy the data to a new buffer then set the size  */
                                /* of the string at the begining of the buffer */
                                tmp_string = emalloc(Z_STRLEN_PP(val) + 2);
                                memcpy(tmp_string + 2, Z_STRVAL_PP(val), Z_STRLEN_PP(val));
                                /* set the 1st 2 bytes as the length of the string */
                                *((II_INT2*)(tmp_string)) = Z_STRLEN_PP(val) ; 
                                putParmParm.pp_parmData[0].dv_value = tmp_string;
                                putParmParm.pp_parmData[0].dv_length = Z_STRLEN_PP(val) + 2; 
                                break;
                            case 'M': /* LONG NVARCHAR */
                                unicode_lob = 1;
                            case 'B': /* LONG BYTE */
                            case 'L': /* LONG TEXT */
                            case 'V': /* LONG VARCHAR */
                                putParmParm.pp_parmCount--; /* decrement parameter count */
                                
                                /* If any non LOB columns have been prepared we need to flush before "put"ing LOB data */
                                if ( putParmParm.pp_parmCount )
                                {
                                    putParmParm.pp_moreSegments = 0;

                                    IIapi_putParms( &putParmParm );
                                    ii_sync(&(putParmParm.pp_genParm));

                                    if (ii_success(&(putParmParm.pp_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
                                    {
                                        efree(descriptorInfo);
                                        efree(columnData);
                                        if ( paramtypes != NULL )
                                        {
                                            efree (types);
                                            if ( tmp_string != NULL)
                                            {
                                                efree (tmp_string);
                                            }
                                        }

                                        return II_FAIL;
                                    }
                                }

                                putParmParm.pp_parmCount  = 1;   /* soon to be "put" LOB */
                                putParmParm.pp_moreSegments = 1; 

                                if ((unicode_lob) && (INGRESG(utf8))) {
                                    /* Convert the UTF-8 data we have to UTF-16 so Ingres will store it */
                                    tmp_utf16_string = emalloc((Z_STRLEN_PP(val) * 4) + 2);
                                    string_start = (UTF8 *)Z_STRVAL_PP(val);
                                    tmp_utf16_string_ptr = tmp_utf16_string;
                                    result = ConvertUTF8toUTF16((const UTF8 **) &string_start,  string_start + Z_STRLEN_PP(val) + 1, &tmp_utf16_string_ptr, tmp_utf16_string_ptr + (Z_STRLEN_PP(val) * 4) , strictConversion);
                                    utf16_string_len = ((tmp_utf16_string_ptr - 1) - (tmp_utf16_string)) * 2;
                                    tmp_lob_ptr = (char *)tmp_utf16_string;
                                    /* Get the length of the new LOB */
                                    lob_len = utf16_string_len;
                                }
                                else
                                {
                                    tmp_lob_ptr = Z_STRVAL_PP(val);
                                    /* Get the length of the new LOB */
                                    lob_len = Z_STRLEN_PP(val);
                                }
                                /* setup a buffer to stream the data in */
                                tmp_lob = emalloc(INGRESG(blob_segment_length) + 2 );
                                
                                while (putParmParm.pp_moreSegments) 
                                {
                                    if ( lob_len <= INGRESG(blob_segment_length) )
                                    {
                                        putParmParm.pp_moreSegments = 0;
                                        segment_length = lob_len; 
                                    }
                                    else
                                    {
                                        putParmParm.pp_moreSegments = 1; 
                                        lob_len -= INGRESG(blob_segment_length);
                                        segment_length = INGRESG(blob_segment_length); 
                                    }

                                    memcpy(tmp_lob + 2, tmp_lob_ptr, segment_length);
                                    if (unicode_lob) 
                                    {
                                        /* Pass the length in terms of UTF-16 code-points */
                                        *((II_UINT2*)tmp_lob) = (II_UINT2)segment_length/2;
                                    }
                                    else
                                    {
                                        *((II_UINT2*)tmp_lob) = (II_UINT2)segment_length;
                                    }

                                    putParmParm.pp_parmData[0].dv_length = segment_length + 2;
                                    putParmParm.pp_parmData[0].dv_null = FALSE;
                                    putParmParm.pp_parmData[0].dv_value = tmp_lob;

                                    IIapi_putParms( &putParmParm );
                                    ii_sync(&(putParmParm.pp_genParm));

                                    if (ii_success(&(putParmParm.pp_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
                                    {
                                        efree(descriptorInfo);
                                        efree(columnData);
                                        efree(tmp_lob);
                                        if ( paramtypes != NULL )
                                        {
                                            efree (types);
                                            if ( tmp_string != NULL)
                                            {
                                                efree (tmp_string);
                                            }
                                        }
                                        if (tmp_utf16_string)
                                        {
                                            efree (tmp_utf16_string);
                                            tmp_utf16_string = NULL;
                                        }

                                        return II_FAIL;
                                    }

                                    /* bump pointer for data by segment_length */
                                    tmp_lob_ptr += segment_length;
                                } 
                                putParmParm.pp_parmCount = 0;
                                if (tmp_lob != NULL)
                                {
                                    efree (tmp_lob);
                                    tmp_lob = NULL;
                                }
                                if (tmp_utf16_string)
                                {
                                    efree(tmp_utf16_string);
                                    tmp_utf16_string = NULL;
                                }
                                break;
                            default: /* everything else */
                                putParmParm.pp_parmData[0].dv_value = Z_STRVAL_PP(val);
                                putParmParm.pp_parmData[0].dv_length = Z_STRLEN_PP(val); 
                                break;
                        }
                    }
                    else if (ii_result->inputCount > 0)
                    {
                        /* Use the descriptor information to convert the data */
                        switch ((ii_result->inputDescr[param - param_offset]).ds_dataType)
                        {
#if defined (IIAPI_VERSION_3)
                            case IIAPI_NVCH_TYPE: /* NVARCHAR */
                                if (INGRESG(utf8)) {
                                    /* Convert the UTF-8 data we have to UTF-16 so Ingres will store it */
                                    tmp_utf16_string = emalloc((Z_STRLEN_PP(val) * 4) + 2);
                                    string_start = (UTF8 *)Z_STRVAL_PP(val);
                                    tmp_utf16_string_ptr = tmp_utf16_string + 1;
                                    result = ConvertUTF8toUTF16((const UTF8 **) &string_start,  string_start + Z_STRLEN_PP(val) + 1, &tmp_utf16_string_ptr, tmp_utf16_string_ptr + (Z_STRLEN_PP(val) * 4) , strictConversion);
                                    utf16_string_len = ((tmp_utf16_string_ptr - 1) - (tmp_utf16_string + 1)) * 2;
                                    *((II_INT2*)(tmp_utf16_string)) = utf16_string_len/2;
                                    putParmParm.pp_parmData[0].dv_value = tmp_utf16_string;
                                    putParmParm.pp_parmData[0].dv_length = utf16_string_len + 2; 
                                }
                                else /* assume UTF-16 */
                                {
                                    /* copy the data to a new buffer then set the size  */
                                    /* of the string in chars at the beggining of the buffer */
                                    tmp_string = emalloc(Z_STRLEN_PP(val) + 2);
                                    memcpy(tmp_string + 2, Z_STRVAL_PP(val), Z_STRLEN_PP(val));
                                    /* set the 1st 2 bytes as the length of the string in chars */
                                    *((II_INT2*)(tmp_string)) = Z_STRLEN_PP(val)/2; 
                                    putParmParm.pp_parmData[0].dv_value = tmp_string;
                                    putParmParm.pp_parmData[0].dv_length = Z_STRLEN_PP(val) + 2; 
                                }
                                
                                break;
                            case IIAPI_NCHA_TYPE: /* NCHAR - for UTF-8 source data only */
                                if (INGRESG(utf8)) {
                                    tmp_utf16_string = emalloc((Z_STRLEN_PP(val) * 4));
                                    string_start = (UTF8 *)Z_STRVAL_PP(val);
                                    tmp_utf16_string_ptr = tmp_utf16_string;
                                    result = ConvertUTF8toUTF16((const UTF8 **) &string_start,  string_start + Z_STRLEN_PP(val) + 1, &tmp_utf16_string_ptr, tmp_utf16_string_ptr + (Z_STRLEN_PP(val) * 4) , strictConversion);
                                    utf16_string_len = ((tmp_utf16_string_ptr - 1) - (tmp_utf16_string)) * 2;
                                    putParmParm.pp_parmData[0].dv_value = (II_PTR *)tmp_utf16_string;
                                    putParmParm.pp_parmData[0].dv_length = utf16_string_len; 
                                }
                                else
                                {
                                    putParmParm.pp_parmData[0].dv_value = Z_STRVAL_PP(val);
                                    putParmParm.pp_parmData[0].dv_length = Z_STRLEN_PP(val); 
                                }
                                break;
#endif
                            case IIAPI_BYTE_TYPE: /* byte */
                            case IIAPI_VBYTE_TYPE: /* byte varying */
                            case IIAPI_CHA_TYPE: /* char */
                            case IIAPI_CHR_TYPE: /* text fixed */
                            case IIAPI_DTE_TYPE: /* date */
#if defined(IIAPI_VERSION_5) 
                            case IIAPI_ADATE_TYPE:  /* SQL Date (aka ANSI DATE) */
                            case IIAPI_TIME_TYPE: /* Ingres Time */
                            case IIAPI_TMWO_TYPE: /* Time without Timezone */
                            case IIAPI_TMTZ_TYPE: /* Time with Timezone */
                            case IIAPI_TS_TYPE: /* Ingres Timestamp */
                            case IIAPI_TSWO_TYPE: /* Timestamp without Timezone */
                            case IIAPI_TSTZ_TYPE: /* Timestamp with Timezone */
                            case IIAPI_INTYM_TYPE: /* Interval Year to Month */
                            case IIAPI_INTDS_TYPE: /* Interval Day to Second */
#endif
                            case IIAPI_TXT_TYPE: /* text */
                            case IIAPI_DEC_TYPE: /* decimal - we need an exact value for decimal storage */
                            case IIAPI_VCH_TYPE: /* VARCHAR */
                                /* copy the data to a new buffer then set the size  */
                                /* of the string at the begining of the buffer */
                                tmp_string = emalloc(Z_STRLEN_PP(val) + 2);
                                memcpy(tmp_string + 2, Z_STRVAL_PP(val), Z_STRLEN_PP(val));
                                /* set the 1st 2 bytes as the length of the string */
                                *((II_INT2*)(tmp_string)) = Z_STRLEN_PP(val) ; 
                                putParmParm.pp_parmData[0].dv_value = tmp_string;
                                putParmParm.pp_parmData[0].dv_length = Z_STRLEN_PP(val) + 2; 
                                break;
                            case IIAPI_LNVCH_TYPE: /* LONG NVARCHAR */
                                unicode_lob = 1;
                            case IIAPI_LBYTE_TYPE: /* LONG BYTE */
                            case IIAPI_LTXT_TYPE: /* LONG TEXT */
                            case IIAPI_LVCH_TYPE: /* LONG VARCHAR */
                                putParmParm.pp_parmCount--; /* decrement parameter count */
                                
                                /* If any non LOB columns have been prepared we need to flush before "put"ing LOB data */
                                if ( putParmParm.pp_parmCount )
                                {
                                    putParmParm.pp_moreSegments = 0;

                                    IIapi_putParms( &putParmParm );
                                    ii_sync(&(putParmParm.pp_genParm));

                                    if (ii_success(&(putParmParm.pp_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
                                    {
                                        efree(descriptorInfo);
                                        efree(columnData);
                                        if ( paramtypes != NULL )
                                        {
                                            efree (types);
                                            if ( tmp_string != NULL)
                                            {
                                                efree (tmp_string);
                                            }
                                        }

                                        return II_FAIL;
                                    }
                                }

                                putParmParm.pp_parmCount  = 1;   /* soon to be "put" LOB */
                                putParmParm.pp_moreSegments = 1; 

                                if ((unicode_lob) && (INGRESG(utf8))) {
                                    /* Convert the UTF-8 data we have to UTF-16 so Ingres will store it */
                                    tmp_utf16_string = emalloc((Z_STRLEN_PP(val) * 4) + 2);
                                    string_start = (UTF8 *)Z_STRVAL_PP(val);
                                    tmp_utf16_string_ptr = tmp_utf16_string;
                                    result = ConvertUTF8toUTF16((const UTF8 **) &string_start,  string_start + Z_STRLEN_PP(val) + 1, &tmp_utf16_string_ptr, tmp_utf16_string_ptr + (Z_STRLEN_PP(val) * 4) , strictConversion);
                                    utf16_string_len = ((tmp_utf16_string_ptr - 1) - (tmp_utf16_string)) * 2;
                                    tmp_lob_ptr = (char *)tmp_utf16_string;
                                    /* Get the length of the new LOB */
                                    lob_len = utf16_string_len;
                                }
                                else
                                {
                                    tmp_lob_ptr = Z_STRVAL_PP(val);
                                    /* Get the length of the new LOB */
                                    lob_len = Z_STRLEN_PP(val);
                                }
                                /* setup a buffer to stream the data in */
                                tmp_lob = emalloc(INGRESG(blob_segment_length) + 2 );
                                
                                while (putParmParm.pp_moreSegments) 
                                {
                                    if ( lob_len <= INGRESG(blob_segment_length) )
                                    {
                                        putParmParm.pp_moreSegments = 0;
                                        segment_length = lob_len; 
                                    }
                                    else
                                    {
                                        putParmParm.pp_moreSegments = 1; 
                                        lob_len -= INGRESG(blob_segment_length);
                                        segment_length = INGRESG(blob_segment_length); 
                                    }

                                    memcpy(tmp_lob + 2, tmp_lob_ptr, segment_length);
                                    if (unicode_lob) 
                                    {
                                        /* Pass the length in terms of UTF-16 code-points */
                                        *((II_UINT2*)tmp_lob) = (II_UINT2)segment_length/2;
                                    }
                                    else
                                    {
                                        *((II_UINT2*)tmp_lob) = (II_UINT2)segment_length;
                                    }

                                    putParmParm.pp_parmData[0].dv_length = segment_length + 2;
                                    putParmParm.pp_parmData[0].dv_null = FALSE;
                                    putParmParm.pp_parmData[0].dv_value = tmp_lob;

                                    IIapi_putParms( &putParmParm );
                                    ii_sync(&(putParmParm.pp_genParm));

                                    if (ii_success(&(putParmParm.pp_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
                                    {
                                        efree(descriptorInfo);
                                        efree(columnData);
                                        efree(tmp_lob);
                                        if ( paramtypes != NULL )
                                        {
                                            efree (types);
                                            if ( tmp_string != NULL)
                                            {
                                                efree (tmp_string);
                                            }
                                        }
                                        if (tmp_utf16_string)
                                        {
                                            efree (tmp_utf16_string);
                                            tmp_utf16_string = NULL;
                                        }

                                        return II_FAIL;
                                    }

                                    /* bump pointer for data by segment_length */
                                    tmp_lob_ptr += segment_length;
                                } 
                                putParmParm.pp_parmCount = 0;
                                if (tmp_lob != NULL)
                                {
                                    efree (tmp_lob);
                                    tmp_lob = NULL;
                                }
                                if (tmp_utf16_string)
                                {
                                    efree (tmp_utf16_string);
                                    tmp_utf16_string = NULL;
                                }
                                break;
                            default: /* everything else */
                                putParmParm.pp_parmData[0].dv_value = Z_STRVAL_PP(val);
                                putParmParm.pp_parmData[0].dv_length = Z_STRLEN_PP(val); 
                                break;
                        }
                    }
                    else
                    {
                        /* copy the data to a new buffer then set the size  */
                        /* of the string at the begining of the buffer */
                        tmp_string = emalloc(Z_STRLEN_PP(val) + 2);
                        memcpy(tmp_string + 2, Z_STRVAL_PP(val), Z_STRLEN_PP(val));
                        /* set the 1st 2 bytes as the length of the string */
                        *((II_INT2*)(tmp_string)) = Z_STRLEN_PP(val) ; 
                        putParmParm.pp_parmData[0].dv_value = tmp_string;
                        putParmParm.pp_parmData[0].dv_length = Z_STRLEN_PP(val) + 2; 
                    }
                    putParmParm.pp_parmData[0].dv_null = FALSE;
                    break;
                case IS_NULL:
                    putParmParm.pp_parmData[0].dv_null = TRUE;
                    putParmParm.pp_parmData[0].dv_length = 0; 
                    putParmParm.pp_parmData[0].dv_value = NULL;
                    break;
                case IS_ARRAY:
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to use an array as a parameter to a query");
                    return II_FAIL;
                    break;
                case IS_OBJECT:
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to use an object as a parameter to a query");
                    return II_FAIL;
                    break;
                case IS_RESOURCE:
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to use a PHP resource as a parameter to a query");
                    return II_FAIL;
                    break;
                default:
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error putting a parameter of unknown type");
                    return II_FAIL;
                    break;
            }
        }

        if (((param_offset) && (param > 0)) || (!param_offset))
        {
            zend_hash_move_forward_ex(arr_hash, &pointer);
        }

        if ( putParmParm.pp_parmCount != 0 )
        {
            putParmParm.pp_moreSegments = 0;
            IIapi_putParms( &putParmParm );
            ii_sync(&(putParmParm.pp_genParm));

            if (ii_success(&(putParmParm.pp_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
            {
                efree(descriptorInfo);
                efree(columnData);
                if ( paramtypes != NULL )
                {
                    efree (types);
                    types = NULL;
                    if ( tmp_string != NULL)
                    {
                        efree (tmp_string);
                        tmp_string = NULL;
                    }
#if defined (IIAPI_VERSION_3)
                    if ( tmp_utf16_string != NULL)
                    {
                        efree (tmp_utf16_string);
                        tmp_utf16_string = NULL;
                    }
#endif
                }
                return II_FAIL;
            }
            if (tmp_string != NULL )
            {
                efree(tmp_string);
                tmp_string = NULL;
            }
#if defined (IIAPI_VERSION_3)
            if ( tmp_utf16_string != NULL)
            {
                efree (tmp_utf16_string);
                tmp_utf16_string = NULL;
            }
#endif
        }
        /* Reset Parameter Count */ 
        putParmParm.pp_parmCount  = 0;
     } /* param = 0 ; param < ii_result->paramCount ; param++ */

    if ( putParmParm.pp_parmCount ) {

        putParmParm.pp_moreSegments = 0;

        IIapi_putParms( &putParmParm );
        ii_sync(&(putParmParm.pp_genParm));

        if (ii_success(&(putParmParm.pp_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
        {
            efree(descriptorInfo);
            efree(columnData);
            if ( paramtypes != NULL )
            {
                efree (types);
                if ( tmp_string != NULL)
                {
                    efree (tmp_string);
                }
            }
            return II_FAIL;
        }
    }

    if ( setDescrParm.sd_descriptorCount )
    {
        efree(descriptorInfo);
        efree(columnData);
    }

    if ( paramtypes != NULL )
    {
        efree (types);
        if ( tmp_string != NULL)
        {
            efree (tmp_string);
            tmp_string = NULL;
        }
    }

    if (tmp_string != NULL)
    {
        efree (tmp_string);
        tmp_string = NULL;
    }

    return II_OK;
}
/* }}} */

/* {{{ static short php_ii_setup_return_value (INTERNAL_FUNCTION_PARAMETERS, IIAPI_DATAVALUE *columnData, II_RESULT *ii_result, int result_type) */
/* Binds and sends data for parameters passed via queryParams */
static short php_ii_setup_return_value (INTERNAL_FUNCTION_PARAMETERS, IIAPI_DATAVALUE *columnData, II_RESULT *ii_result, int col_no, int result_type)
{
    double value_double = 0;
    long value_long = 0;
    ingres_int64 value_long_long = 0;
    char value_long_long_str[21];
    int value_long_long_str_len=0;
    char *value_char_p;
    int len, should_copy, correct_length=0;

#if defined(IIAPI_VERSION_3)
    UTF8 *tmp_utf8_string = NULL;
    UTF8 *tmp_utf8_string_ptr = NULL;
    UTF16 *string_start;
    ConversionResult result;
#endif

    int k = 0 , l = 0;
    value_long_long_str[0] = '\0';

    if (columnData->dv_null)
    {    /* NULL value ? */

        if (result_type & II_NUM)
        {
            add_index_null(return_value, col_no + INGRESG(array_index_start));
        }
        if (result_type & II_ASSOC)
        {
            add_assoc_null(return_value, php_ii_field_name(ii_result, col_no + INGRESG(array_index_start) TSRMLS_CC));
        }

    } else {    /* non NULL value */
        correct_length = 0;

        switch ((ii_result->descriptor[col_no]).ds_dataType)
        {

            case IIAPI_DEC_TYPE:    /* decimal (fixed point number) */
            case IIAPI_MNY_TYPE:    /* money */
                /* convert to floating point number */
                php_ii_convert_data ( IIAPI_FLT_TYPE, sizeof(II_FLOAT8), 53, ii_result, &ii_result->metaData[(ii_result->rowNumber * ii_result->fieldCount)], ii_result->getColParm, k, col_no TSRMLS_CC );
                /* NO break */

            case IIAPI_FLT_TYPE:    /* floating point number */
                switch (columnData->dv_length)
                {

                    case 4:
                        value_double = (double) *((II_FLOAT4 *) columnData->dv_value);
                        break;

                    case 8:
                        value_double = (double) *((II_FLOAT8 *) columnData->dv_value);
                        break;

                    default:
                        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid size for IIAPI_FLT_TYPE data (%d)", columnData->dv_length);
                        break;
                }

                if (result_type & II_NUM)
                {
                    add_index_double(return_value, col_no + INGRESG(array_index_start), value_double);
                }

                if (result_type & II_ASSOC)
                {
                    add_assoc_double(return_value, php_ii_field_name(ii_result, col_no + INGRESG(array_index_start) TSRMLS_CC), value_double);

                }
                break;

            case IIAPI_INT_TYPE:    /* integer */
                switch (columnData->dv_length)
                {

                    case 1:
                        value_long = (long) *((II_INT1 *) columnData->dv_value);
                        break;

                    case 2:
                        value_long = (long) *((II_INT2 *) columnData->dv_value);
                        break;

                    case 4:
                        value_long = (long) *((II_INT4 *) columnData->dv_value);
                        break;
#if defined(IIAPI_VERSION_4)
                    case 8:
                        /* PHP does not support BIGINT/INTEGER8 so we have to return */
                        /* values greater/smaller than the max/min size of a LONG value as a string */
                        /* Anyone wanting to manipulate this value can use PECL big_int */
                        if ((*((ingres_int64 *) columnData->dv_value) > LONG_MAX ) ||
                            (*((ingres_int64 *) columnData->dv_value) < LONG_MIN ))
                        {
                            value_long_long = *((ingres_int64 *) columnData->dv_value);
                            sprintf(value_long_long_str, "%lld\0", value_long_long);
                            value_long_long_str_len = strlen(value_long_long_str);
                        }
                        else
                        {
                            value_long = (long) *((II_INT4 *) columnData->dv_value);
                        }
                        break;
#endif
                    default:
                        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid size for IIAPI_INT_TYPE data (%d)", columnData->dv_length);
                        break;
                }

                if (value_long_long_str[0] != '\0')
                {
                    should_copy = 1;
                    if (result_type & II_NUM)
                    {
                        add_index_stringl(return_value, col_no + INGRESG(array_index_start), value_long_long_str, value_long_long_str_len, should_copy);
                    }
                    if (result_type & II_ASSOC)
                    {
                        add_assoc_stringl(return_value, php_ii_field_name(ii_result, col_no + INGRESG(array_index_start) TSRMLS_CC), value_long_long_str, value_long_long_str_len, should_copy);
                    }
                    /* Init the first char to '\0' for later reuse */
                    value_long_long_str[0] = '\0';
                }
                else
                {
                    if (result_type & II_NUM)
                    {
                        add_index_long(return_value, col_no + INGRESG(array_index_start), value_long);
                    }
                    if (result_type & II_ASSOC)
                    {
                        add_assoc_long(return_value, php_ii_field_name(ii_result, col_no + INGRESG(array_index_start) TSRMLS_CC), value_long);
                    }
                }
                break;

#if defined(IIAPI_VERSION_3)
            case IIAPI_NVCH_TYPE:    /* variable length unicode character string */
                columnData->dv_length = *((II_INT2 *) columnData->dv_value) * 2;
                columnData->dv_value = (II_CHAR *)columnData->dv_value + 2;
                correct_length = 1;
            case IIAPI_NCHA_TYPE:    /* fixed length unicode character string */    
                /* use php_addslashes if asked to */
                if (PG(magic_quotes_runtime))
                {
                    value_char_p = php_addslashes((char *) columnData->dv_value,  columnData->dv_length, &len, 0 TSRMLS_CC);
                    should_copy = 0;
                } else {
                    value_char_p = (char *) columnData->dv_value;
                    len = columnData->dv_length;
                    should_copy = 1;
                }

                if (INGRESG(utf8)) {
                    /* User has requested the output in UTF-8 */
                    /* create a big enough buffer - each code point in UTF-16 can be upto 4 bytes in UTF-8 */
                    tmp_utf8_string = emalloc((len * 4) + 1);
                    for ( l = 0; l < len * 4 ; l++) {
                        tmp_utf8_string[l] = '\0';
                    }
                    string_start = (UTF16 *)columnData->dv_value;
                    tmp_utf8_string_ptr = tmp_utf8_string;
                    result = ConvertUTF16toUTF8((const UTF16 **) &string_start, string_start + len/2, &tmp_utf8_string_ptr, tmp_utf8_string_ptr + (len * 4), strictConversion);
                    len = tmp_utf8_string_ptr - tmp_utf8_string;
                    tmp_utf8_string[len] = '\0';
                    value_char_p = (char *)tmp_utf8_string;
                }

                if (result_type & II_NUM)
                {
                    add_index_stringl(return_value, col_no + INGRESG(array_index_start), value_char_p, len, should_copy);
                }
                if (result_type & II_ASSOC)
                {
                    add_assoc_stringl(return_value, php_ii_field_name(ii_result, col_no + INGRESG(array_index_start) TSRMLS_CC), value_char_p, len, should_copy);
                }
                if (tmp_utf8_string) {
                    efree(tmp_utf8_string);
                    tmp_utf8_string = NULL;
                }
                break;
#endif                                
            case IIAPI_TXT_TYPE:    /* variable length character string */
            case IIAPI_VBYTE_TYPE:    /* variable length binary string */
            case IIAPI_VCH_TYPE:    /* variable length character string */
                /* real length is stored in first 2 bytes of data, so adjust
                   length variable and data pointer */

                columnData->dv_length = *((II_INT2 *) columnData->dv_value);
                columnData->dv_value = (II_CHAR *)columnData->dv_value + 2;
                correct_length = 1;
                /* NO break */

            case IIAPI_BYTE_TYPE:    /* fixed length binary string */
            case IIAPI_CHA_TYPE:    /* fixed length character string */
            case IIAPI_CHR_TYPE:    /* fixed length character string */
            case IIAPI_LOGKEY_TYPE:    /* value unique to database */
            case IIAPI_TABKEY_TYPE:    /* value unique to table */
            case IIAPI_DTE_TYPE:    /* Ingres date */
#if defined(IIAPI_VERSION_5) 
            case IIAPI_ADATE_TYPE:  /* SQL Date (aka ANSI DATE) */
            case IIAPI_TIME_TYPE: /* Ingres Time */
            case IIAPI_TMWO_TYPE: /* Time without Timezone */
            case IIAPI_TMTZ_TYPE: /* Time with Timezone */
            case IIAPI_TS_TYPE: /* Ingres Timestamp */
            case IIAPI_TSWO_TYPE: /* Timestamp without Timezone */
            case IIAPI_TSTZ_TYPE: /* Timestamp with Timezone */
            case IIAPI_INTYM_TYPE: /* Interval Year to Month */
            case IIAPI_INTDS_TYPE: /* Interval Day to Second */
#endif
                /* convert date to variable length string */
                if ((ii_result->descriptor[col_no]).ds_dataType == IIAPI_DTE_TYPE 
#if defined(IIAPI_VERSION_5) 
                    || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_ADATE_TYPE
                    || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TIME_TYPE 
                    || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TMWO_TYPE
                    || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TMTZ_TYPE
                    || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TS_TYPE
                    || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TSWO_TYPE
                    || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TSTZ_TYPE
                    || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_INTYM_TYPE
                    || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_INTDS_TYPE
#endif
                    )
                {
                    php_ii_convert_data ( IIAPI_VCH_TYPE, 32, 0, ii_result, &ii_result->metaData[(ii_result->rowNumber * ii_result->fieldCount)], ii_result->getColParm, k, col_no TSRMLS_CC );
                    columnData->dv_length = *((II_INT2 *) columnData->dv_value);
                    columnData->dv_value = (II_CHAR *)columnData->dv_value + 2;
                    correct_length = 1;
                }

                /* use php_addslashes if asked to */
                if (PG(magic_quotes_runtime))
                {
                    value_char_p = php_addslashes((char *) columnData->dv_value,  columnData->dv_length, &len, 0 TSRMLS_CC);
                    should_copy = 0;
                }
                else 
                {
                    value_char_p = (char *) columnData->dv_value;
                    len = columnData->dv_length;
                    should_copy = 1;
                }

                if (result_type & II_NUM)
                {
                    add_index_stringl(return_value, col_no + INGRESG(array_index_start), value_char_p, len, should_copy);
                }

                if (result_type & II_ASSOC)
                {
                    add_assoc_stringl(return_value, php_ii_field_name(ii_result, col_no + INGRESG(array_index_start) TSRMLS_CC), value_char_p, len, should_copy);
                }
                break;

            default:
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid SQL data type in fetched field (%d -- length : %d)", (ii_result->descriptor[col_no]).ds_dataType, columnData->dv_length);
                break;
        }
        /* eventually restore data pointer state for variable length data types */
        if (correct_length)
        {
            columnData->dv_value = (II_CHAR *)columnData->dv_value - 2;
            if ((ii_result->descriptor[col_no]).ds_dataType == IIAPI_DTE_TYPE 
#if defined(IIAPI_VERSION_5) 
                || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_ADATE_TYPE
                || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TIME_TYPE 
                || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TMWO_TYPE
                || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TMTZ_TYPE
                || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TS_TYPE
                || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TSWO_TYPE
                || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_TSTZ_TYPE
                || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_INTYM_TYPE
                || (ii_result->descriptor[col_no]).ds_dataType == IIAPI_INTDS_TYPE
#endif
                )
            {
                efree(columnData->dv_value);
                columnData->dv_value = NULL;
            }
        }
        else if (((ii_result->descriptor[col_no]).ds_dataType == IIAPI_MNY_TYPE) ||
                ((ii_result->descriptor[col_no]).ds_dataType == IIAPI_DEC_TYPE))
        {
            efree(columnData->dv_value);
            columnData->dv_value = NULL;
        }
    }
}
/* }}} */

/* {{{ static void _free_resultdata (II_RESULT *ii_result) */
/* Free the memory associated with ii_result->metaData */
static void _free_resultdata (II_RESULT *ii_result)
{
    if ( ii_result->metaData )
    {
        if ( ii_result->dataBuffer )
        {
            efree(ii_result->dataBuffer);
            ii_result->dataBuffer = NULL;
        }
        efree(ii_result->metaData);
        ii_result->metaData = NULL;
        ii_result->rowsReturned = 0;
        ii_result->rowNumber = 0;
    }
}
/* }}} */

/* {{{ static short int php_ii_scroll_row_count (II_RESULT *ii_result TSRMLS_DC) */
static short int php_ii_scroll_row_count (II_RESULT *ii_result TSRMLS_DC)
{
#if defined(IIAPI_VERSION_6)

    IIAPI_GETQINFOPARM getQInfoParm;
    IIAPI_SCROLLPARM scrollParm;
    long cur_row = 0; /* Current row */

    /* Clear out any prior scroll */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = ii_result->stmtHandle;

    IIapi_getQueryInfo(&getQInfoParm);
    ii_sync(&(getQInfoParm.gq_genParm));

    if (ii_success(&(getQInfoParm.gq_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        return II_FAIL;
    }

    /* Find out where we are so we can come back here again */
    scrollParm.sl_genParm.gp_callback = NULL;
    scrollParm.sl_genParm.gp_closure = NULL;
    scrollParm.sl_stmtHandle = ii_result->stmtHandle;
    scrollParm.sl_orientation = IIAPI_SCROLL_CURRENT;
    scrollParm.sl_offset = 0;

    IIapi_scroll(&scrollParm);
    ii_sync(&(scrollParm.sl_genParm));

    if (ii_success(&(scrollParm.sl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Unable to determine current cursor position");
        return II_FAIL;
    }

    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = ii_result->stmtHandle;

    IIapi_getQueryInfo(&getQInfoParm);
    ii_sync(&(getQInfoParm.gq_genParm));

    if (ii_success(&(getQInfoParm.gq_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        return II_FAIL;
    }
    /* I am here ... */
    if ((getQInfoParm.gq_cursorType & IIAPI_CURSOR_SCROLL) && (getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT))
    {
        cur_row =  getQInfoParm.gq_rowPosition;
    }

    /* Scroll to the end of the data set */
    scrollParm.sl_genParm.gp_callback = NULL;
    scrollParm.sl_genParm.gp_closure = NULL;
    scrollParm.sl_stmtHandle = ii_result->stmtHandle;
    scrollParm.sl_orientation = IIAPI_SCROLL_AFTER;
    scrollParm.sl_offset = 0;

    IIapi_scroll(&scrollParm);
    ii_sync(&(scrollParm.sl_genParm));

    if (ii_success(&(scrollParm.sl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Unable to scroll to the end of the result set");
        return II_FAIL;
    }

    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = ii_result->stmtHandle;

    IIapi_getQueryInfo(&getQInfoParm);
    ii_sync(&(getQInfoParm.gq_genParm));

    if (ii_success(&(getQInfoParm.gq_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        return II_FAIL;
    }

    /* store the end row position */
    if ((getQInfoParm.gq_cursorType & IIAPI_CURSOR_SCROLL) && (getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT))
    {
        ii_result->rowCount =  getQInfoParm.gq_rowPosition;
    }

    /* Scroll back to where we were */
    scrollParm.sl_genParm.gp_callback = NULL;
    scrollParm.sl_genParm.gp_closure = NULL;
    scrollParm.sl_stmtHandle = ii_result->stmtHandle;
    scrollParm.sl_orientation = cur_row ? IIAPI_SCROLL_ABSOLUTE : IIAPI_SCROLL_BEFORE;
    scrollParm.sl_offset = cur_row;

    IIapi_scroll(&scrollParm);
    ii_sync(&(scrollParm.sl_genParm));

    if (ii_success(&(scrollParm.sl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Unable to scroll back to row %ld", cur_row);
        return II_FAIL;
    }

    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = ii_result->stmtHandle;

    IIapi_getQueryInfo(&getQInfoParm);
    ii_sync(&(getQInfoParm.gq_genParm));

    if (ii_success(&(getQInfoParm.gq_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL)
    {
        return II_FAIL;
    }
    
    return II_OK;

#else

    return II_FAIL;

#endif
}
/* }}} */

#if defined(IIAPI_VERSION_5)
/* {{{ static short _ii_describe_input (II_RESULT *ii_result TSRMLS_DC) */
/* Describe the input for a prepared query*/
static short _ii_describe_input (II_RESULT *ii_result, char *query TSRMLS_DC)
{
    IIAPI_QUERYPARM    queryParm;
    IIAPI_CLOSEPARM    closeParm;
    IIAPI_GETDESCRPARM getDescrParm;

    char *queryText = NULL;

    queryText = emalloc(strlen(ii_result->cursor_id) + 17);
    sprintf(queryText, "DESCRIBE INPUT %s", ii_result->cursor_id);

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = ii_result->connHandle;
    queryParm.qy_tranHandle = ii_result->tranHandle;
    queryParm.qy_stmtHandle = NULL;
    queryParm.qy_queryType  = IIAPI_QT_QUERY; 
    queryParm.qy_parameters = FALSE;
    queryParm.qy_queryText  = queryText;
#if defined (IIAPI_VERSION_6)
    queryParm.qy_flags = 0;
#endif

    IIapi_query(&queryParm);
    ii_sync(&(queryParm.qy_genParm));

    if (ii_success(&(queryParm.qy_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL) 
    {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "An error occurred when issuing a DESCRIBE INPUT");
        efree(queryText);
        return II_FAIL;
    }

    ii_result->tranHandle = queryParm.qy_tranHandle;
    ii_result->stmtHandle = queryParm.qy_stmtHandle;

    /* get description of params */
    getDescrParm.gd_genParm.gp_callback = NULL;
    getDescrParm.gd_genParm.gp_closure  = NULL;
    getDescrParm.gd_stmtHandle = ii_result->stmtHandle;

    IIapi_getDescriptor( &getDescrParm );
    ii_sync(&(getDescrParm.gd_genParm));

    if (ii_success(&(getDescrParm.gd_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL) 
    {
        efree(queryText);
        return II_FAIL;
    }

    /* Store the descriptors retrieved for later use */
    if (getDescrParm.gd_descriptorCount)
    {
        ii_result->inputDescr = (IIAPI_DESCRIPTOR *)emalloc(sizeof(IIAPI_DESCRIPTOR) * getDescrParm.gd_descriptorCount);
        memcpy (ii_result->inputDescr, getDescrParm.gd_descriptor, sizeof(IIAPI_DESCRIPTOR) * getDescrParm.gd_descriptorCount);
        ii_result->inputCount = getDescrParm.gd_descriptorCount;
    }

    /*
    ** Call IIapi_close() to release resources.
    */
    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_close( &closeParm );
    ii_sync(&(closeParm.cl_genParm));

    if (ii_success(&(closeParm.cl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL) 
    {
        efree(queryText);
        return II_FAIL;
    }
    efree(queryText);
    return II_OK;
}
/* }}} */

/*{{{ static short _ii_prepare (II_RESULT *ii_result, char *queryText) */
static short _ii_prepare (II_RESULT *ii_result, char *query TSRMLS_DC)
{
    IIAPI_QUERYPARM    queryParm;
    IIAPI_GETQINFOPARM    getQInfoParm;
    IIAPI_CLOSEPARM    closeParm;

    char *queryText = NULL;

    /* Generate a unique cursor name */
    php_ii_gen_cursor_id(ii_result TSRMLS_CC);
    queryText = emalloc(strlen(ii_result->cursor_id) + 20 + strlen(query));
    sprintf(queryText, "prepare %s from %s", ii_result->cursor_id, query);

    /*
    ** Call IIapi_query to execute statement.
    */
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = ii_result->connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = queryText;
    queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = ii_result->tranHandle;
    queryParm.qy_stmtHandle = NULL;
#if defined (IIAPI_VERSION_6)
    queryParm.qy_flags = 0;
#endif

    IIapi_query( &queryParm );
    ii_sync(&(queryParm.qy_genParm));

    if (ii_success(&(queryParm.qy_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL) 
    {
        efree(queryText);
        return II_FAIL;
    }
    /*
    ** Return transaction handle.
    */
    ii_result->tranHandle = queryParm.qy_tranHandle;

    /*
    ** Call IIapi_getQueryInfo( ) to get results.
    */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );
    ii_sync(&(getQInfoParm.gq_genParm));

    if (ii_success(&(getQInfoParm.gq_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL) 
    {
        efree(queryText);
        return II_FAIL;
    }
    /*
    ** Call IIapi_close( ) to release resources.
    */
    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_close( &closeParm );
    ii_sync(&(closeParm.cl_genParm));

    if (ii_success(&(closeParm.cl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL) 
    {
        efree(queryText);
        return II_FAIL;
    }
    efree(queryText);
    return II_OK;
}
/* }}} */
#endif /* IIAPI_VERSION_5 */

/* {{{ static short _ii_describe (II_RESULT *ii_result TSRMLS_DC) */
/* Describe a prepared query*/
static short _ii_describe (II_RESULT *ii_result, char *query TSRMLS_DC)
{
    IIAPI_QUERYPARM    queryParm;
    IIAPI_CANCELPARM   cancelParm;
    IIAPI_CLOSEPARM    closeParm;

    char *queryText = NULL;

    queryText = emalloc(strlen(ii_result->cursor_id) + 10);
    sprintf(queryText, "DESCRIBE %s", ii_result->cursor_id);

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = ii_result->connHandle;
    queryParm.qy_tranHandle = ii_result->tranHandle;
    queryParm.qy_stmtHandle = NULL;
    queryParm.qy_queryType  = IIAPI_QT_QUERY; 
    queryParm.qy_parameters = FALSE;
    queryParm.qy_queryText  = queryText;
#if defined (IIAPI_VERSION_6)
    queryParm.qy_flags = 0;
#endif

    IIapi_query(&queryParm);
    ii_sync(&(queryParm.qy_genParm));

    if (ii_success(&(queryParm.qy_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL) 
    {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "An error occurred when issuing a DESCRIBE of a prepared query");
        efree(queryText);
        return II_FAIL;
    }

    ii_result->tranHandle = queryParm.qy_tranHandle;
    ii_result->stmtHandle = queryParm.qy_stmtHandle;

    /*
    ** Call IIapi_close( ) to release resources.
    */
    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_close( &closeParm );
    ii_sync(&(closeParm.cl_genParm));

    if (ii_success(&(closeParm.cl_genParm), &ii_result->errorHandle TSRMLS_CC) == II_FAIL) 
    {
        efree(queryText);
        return II_FAIL;
    }
    efree(queryText);
    return II_OK;
}
/* }}} */

/* {{{ proto string ingres_charset(resource link)
   return the character set encoding of the client installation*/
#ifdef HAVE_INGRES2
PHP_FUNCTION(ingres2_charset)
#else
PHP_FUNCTION(ingres_charset)
#endif
{
    zval *link;
    II_LINK *ii_link;
    char *ii_installation_value = NULL;
    char ii_charset_var[13]; /* II_CHARSETxx\0 */
    char *ii_charset_value = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC ,"r" , &link) == FAILURE) 
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, &link, -1, "Ingres Link", le_ii_link, le_ii_plink);

    if (ii_link->charset == NULL)
    {
        /* The variable  for II_CHARSET is dependent on the installation code II_INSTALLATION */
        NMgtAt("II_INSTALLATION", &ii_installation_value);
        if ((ii_installation_value != (char *)NULL) && (*ii_installation_value != EOS))
        {
            sprintf(ii_charset_var,"II_CHARSET%2s", ii_installation_value);
            NMgtAt(ii_charset_var, &ii_charset_value);
            if ((ii_charset_value == (char *)NULL) || (*ii_charset_value == EOS))
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "No value available for %s. Check your Ingres environment is setup correctly.", ii_charset_var);
            }
            else
            {
                ii_link->charset = emalloc(strlen(ii_charset_value) + 1);
                memcpy(ii_link->charset, ii_charset_value, strlen(ii_charset_value) + 1);
            }
        }
        else
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "No value available for II_INSTALLATION. Check your Ingres environment is setup correctly.");
        }
    }

    if (ii_link->charset)
    {
        RETURN_STRING(ii_link->charset,1);
    }
    else
    {
        RETURN_NULL();
    }
}
/* }}} */


/* {{{ static short _ii_close (II_PTR *stmtHandle, II_PTR *errorHandle) */
/* Close / Cancel / close a query*/
static short _ii_close (II_PTR *stmtHandle, II_PTR *errorHandle TSRMLS_DC)
{
    IIAPI_CANCELPARM   cancelParm;
    IIAPI_CLOSEPARM    closeParm;

    if (stmtHandle)
    {
        /* see if we can close the query without cancelling it */
        /* Free query resources. */
        closeParm.cl_genParm.gp_callback = NULL;
        closeParm.cl_genParm.gp_closure = NULL;
        closeParm.cl_stmtHandle = *stmtHandle;

        IIapi_close(&closeParm);
        ii_sync(&(closeParm.cl_genParm));

        if (ii_success(&(closeParm.cl_genParm), errorHandle TSRMLS_CC) == II_FAIL)
        {
            /* unable to close */
            /* Cancel query processing. */
            cancelParm.cn_genParm.gp_callback = NULL;
            cancelParm.cn_genParm.gp_closure = NULL;
            cancelParm.cn_stmtHandle = *stmtHandle;

            IIapi_cancel(&cancelParm );

            ii_sync(&(cancelParm.cn_genParm));

            /* Free query resources. */
            closeParm.cl_genParm.gp_callback = NULL;
            closeParm.cl_genParm.gp_closure = NULL;
            closeParm.cl_stmtHandle = *stmtHandle;

            IIapi_close( &closeParm );

            ii_sync(&(closeParm.cl_genParm));

            if (ii_success(&(closeParm.cl_genParm), errorHandle TSRMLS_CC) == II_FAIL)
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close a previously executed query");
                return II_FAIL;
            }
        }
        *stmtHandle = NULL;
    }
    return II_OK;
}
/* }}} */

static long ii_result_row_width(II_RESULT *ii_result)
{
    long pad_bytes = 0;
    int col = 0;
    long row_width = 0;

#ifdef ALIGN_MEMORY
    /* Certain platforms/architectures require memory aligned
     * data structures. This requires that we add pad out the 
     * size of the row to allow for each column to be accessed
     * along memory boundaries to prevent any SIGBUS */
    pad_bytes=0;
    for( col = 0; col < ii_result->fieldCount; col++)
    {
        if (row_width > 0)
        {
            pad_bytes = row_width % ZEND_MM_ALIGNMENT ? 
                    ZEND_MM_ALIGNMENT - (row_width % ZEND_MM_ALIGNMENT) : 0;
        }
        row_width += ii_result->descriptor[col].ds_length + pad_bytes;
    }
    /* Since we allocate space for multiple rows make sure that following rows can
     * start on a memory boundary as well */
    if (row_width % ZEND_MM_ALIGNMENT)
    {
        row_width += ZEND_MM_ALIGNMENT - (ii_result->rowWidth % ZEND_MM_ALIGNMENT);
    }
#else
    for( col = 0; col < ii_result->fieldCount; col++)
    {
        row_width += ii_result->descriptor[col].ds_length;
    }  
#endif
    return row_width;
}

#endif /* HAVE_INGRES */

/*
 * Local variables:
 * tab-width: 4
 * shift-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker ff=unix fileencoding=latin1 expandtab
 * vim<600: sw=4 ts=4
 */
