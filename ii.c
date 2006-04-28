/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2004 The PHP Group                                |
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
   |                Computer Associates, http://ingres.ca.com             |
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
#include "php_ii.h"
#include "ii.h"
#include "ext/standard/php_string.h"

#if HAVE_II

ZEND_DECLARE_MODULE_GLOBALS(ii)

/* True globals, no need for thread safety */
static int le_ii_link, le_ii_plink;

#define SAFE_STRING(s) ((s)?(s):"")

/* {{{ Ingres module function list
 * Every user visible function must have an entry in ii_functions[].
*/
function_entry ii_functions[] = {
	PHP_FE(ingres_connect,			NULL)
	PHP_FE(ingres_pconnect,			NULL)
	PHP_FE(ingres_close,			NULL)
	PHP_FE(ingres_query,			NULL)
	PHP_FE(ingres_num_rows,			NULL)
	PHP_FE(ingres_num_fields,		NULL)
	PHP_FE(ingres_field_name,		NULL)
	PHP_FE(ingres_field_type,		NULL)
	PHP_FE(ingres_field_nullable,	NULL)
	PHP_FE(ingres_field_length,		NULL)
	PHP_FE(ingres_field_precision,	NULL)
	PHP_FE(ingres_field_scale,		NULL)
	PHP_FE(ingres_fetch_array,		NULL)
	PHP_FE(ingres_fetch_row,		NULL)
	PHP_FE(ingres_fetch_object,		NULL)
	PHP_FE(ingres_rollback,			NULL)
	PHP_FE(ingres_commit,			NULL)
	PHP_FE(ingres_autocommit,		NULL)
	PHP_FE(ingres_error,			NULL)
	PHP_FE(ingres_errno,			NULL)
	PHP_FE(ingres_errsqlstate,		NULL)
	PHP_FE(ingres_prepare,			NULL)
	PHP_FE(ingres_execute,			NULL)
	PHP_FE(ingres_cursor,			NULL)
	PHP_FE(ingres_set_environment,	NULL)

	{NULL, NULL, NULL}	/* Must be the last line in ii_functions[] */
};
/* }}} */

zend_module_entry ingres_module_entry = {
	STANDARD_MODULE_HEADER,
	"ingres",
	ii_functions,
	PHP_MINIT(ii),
	PHP_MSHUTDOWN(ii),
	PHP_RINIT(ii),
	PHP_RSHUTDOWN(ii),
	PHP_MINFO(ii),
	II_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_INGRES
ZEND_GET_MODULE(ingres)
#endif

#ifndef ZEND_ENGINE_2
#	define OnUpdateLong OnUpdateInt
#endif

/* {{{ php.ini entries */
PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN("ingres.allow_persistent", "1", PHP_INI_SYSTEM,	OnUpdateLong, allow_persistent, zend_ii_globals, ii_globals)
	STD_PHP_INI_ENTRY_EX("ingres.max_persistent", "-1", PHP_INI_SYSTEM, OnUpdateLong, max_persistent, zend_ii_globals, ii_globals, display_link_numbers)
	STD_PHP_INI_ENTRY_EX("ingres.max_links", "-1", PHP_INI_SYSTEM, OnUpdateLong, max_links, zend_ii_globals, ii_globals, display_link_numbers)
	STD_PHP_INI_ENTRY("ingres.default_database", NULL, PHP_INI_ALL, OnUpdateString, default_database, zend_ii_globals, ii_globals)
	STD_PHP_INI_ENTRY("ingres.default_user", NULL, PHP_INI_ALL, OnUpdateString, default_user, zend_ii_globals, ii_globals)
	STD_PHP_INI_ENTRY("ingres.default_password", NULL, PHP_INI_ALL, OnUpdateString, default_password, zend_ii_globals, ii_globals)
	STD_PHP_INI_BOOLEAN("ingres.report_db_warnings","1", PHP_INI_ALL, OnUpdateBool, report_db_warnings, zend_ii_globals, ii_globals)
	STD_PHP_INI_ENTRY("ingres.cursor_mode", "0", PHP_INI_ALL, OnUpdateLong, cursor_mode, zend_ii_globals, ii_globals)
	STD_PHP_INI_ENTRY("ingres.blob_segment_length", "4096", PHP_INI_ALL, OnUpdateLong, blob_segment_length, zend_ii_globals, ii_globals)
	STD_PHP_INI_BOOLEAN("ingres.trace_connect", "0", PHP_INI_ALL, OnUpdateLong, trace_connect, zend_ii_globals, ii_globals)
	STD_PHP_INI_ENTRY("ingres.timeout", "-1", PHP_INI_ALL, OnUpdateLong, connect_timeout, zend_ii_globals, ii_globals)
	STD_PHP_INI_ENTRY("ingres.array_index_start", "1", PHP_INI_ALL, OnUpdateLong, array_index_start, zend_ii_globals, ii_globals)
PHP_INI_END()
/* }}} */

/* {{{ static int _close_statement(II_LINK *ii_link TSRMLS_DC) */
/* closes statement in given link */
static int _close_statement(II_LINK *ii_link TSRMLS_DC)
{
	IIAPI_CLOSEPARM closeParm;

	closeParm.cl_genParm.gp_callback = NULL;
	closeParm.cl_genParm.gp_closure = NULL;
	closeParm.cl_stmtHandle = ii_link->stmtHandle;

	IIapi_close(&closeParm);
	ii_sync(&(closeParm.cl_genParm));

	if (ii_success(&(closeParm.cl_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		return 1;
	}

	ii_link->stmtHandle = NULL;
	ii_link->fieldCount = 0;
	ii_link->descriptor = NULL;
	if ( ii_link->procname != NULL )
	{
		free(ii_link->procname);
		ii_link->procname = NULL;
		
	}
	if ( ii_link->descriptor != NULL )
	{
		free(ii_link->descriptor);
		ii_link->descriptor = NULL;
	}
	if ( ii_link->cursor_id != NULL )
	{
		free(ii_link->cursor_id);
		ii_link->cursor_id = NULL;
	}

	ii_link->paramCount = 0;

	return 0;
}
/* }}} */

/* {{{ static int _rollback_transaction(II_LINK *ii_link  TSRMLS_DC) */
/* rolls back transaction in given link after closing the active transaction (if any) */
static int _rollback_transaction(II_LINK *ii_link  TSRMLS_DC)
{
	IIAPI_ROLLBACKPARM rollbackParm;

	if (ii_link->stmtHandle && _close_statement(ii_link TSRMLS_CC))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close statement");
		return 1;
	}

	rollbackParm.rb_genParm.gp_callback = NULL;
	rollbackParm.rb_genParm.gp_closure = NULL;
	rollbackParm.rb_tranHandle = ii_link->tranHandle;
	rollbackParm.rb_savePointHandle = NULL;

	IIapi_rollback(&rollbackParm);
	ii_sync(&(rollbackParm.rb_genParm));

	if (ii_success(&(rollbackParm.rb_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		return 1;
	}

	ii_link->tranHandle = NULL;
	return 0;
}
/* }}} */

/* {{{ static void _close_ii_link(II_LINK *ii_link TSRMLS_DC) */
static void _close_ii_link(II_LINK *ii_link TSRMLS_DC)
{
	IIAPI_DISCONNPARM disconnParm;

	if (ii_link->tranHandle && _rollback_transaction(ii_link TSRMLS_CC))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to rollback transaction");
	}

	disconnParm.dc_genParm.gp_callback = NULL;
	disconnParm.dc_genParm.gp_closure = NULL;
	disconnParm.dc_connHandle = ii_link->connHandle;

	IIapi_disconnect(&disconnParm);

	free(ii_link);

	IIG(num_links)--;
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

/* {{{ static void _close_ii_plink(zend_rsrc_list_entry *rsrc TSRMLS_DC) */
/* closes the given persistent link, see _close_ii_link */
static void _close_ii_plink(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	II_LINK *ii_link = (II_LINK *) rsrc->ptr;

	_close_ii_link(ii_link TSRMLS_CC);
	IIG(num_persistent)--;
}
/*  }}} */

/* {{{ static void _ai_clean_ii_plink(II_LINK *ii_link TSRMLS_DC) */
/* cleans up the given persistent link.  used when the request ends to 'refresh' the link for use */
/* by the next request */
static void _ai_clean_ii_plink(II_LINK *ii_link TSRMLS_DC)
{
	int ai_error = 0;
	IIAPI_DISCONNPARM disconnParm;
	IIAPI_AUTOPARM autoParm;

	/* if link as always been marked as broken do nothing */
	/* This because we call this function directly from close function */
	/* And it's called in the end of request */
	if (ii_link->connHandle == NULL)
	{
		return;
	}
	
	if (ii_link->stmtHandle && _close_statement(ii_link TSRMLS_CC))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close statement");
		ai_error = 1;
	}
	
	if (ii_link->autocommit)
	{
		autoParm.ac_genParm.gp_callback = NULL;
		autoParm.ac_genParm.gp_closure = NULL;
		autoParm.ac_connHandle = ii_link->connHandle;
		autoParm.ac_tranHandle = ii_link->tranHandle;

		IIapi_autocommit(&autoParm);
		ii_sync(&(autoParm.ac_genParm));

		if (ii_success(&(autoParm.ac_genParm), ii_link TSRMLS_CC) == II_FAIL)
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to disable autocommit");
		}

		ii_link->autocommit = 0;
		ii_link->tranHandle = NULL;
	}

	if (ii_link->tranHandle && _rollback_transaction(ii_link TSRMLS_CC))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to rollback transaction");
	}

	/* Assume link is broken, close it, and mark it as broken with conn Handle NULL */
	if (ai_error)
	{
		disconnParm.dc_genParm.gp_callback = NULL;
		disconnParm.dc_genParm.gp_closure = NULL;
		disconnParm.dc_connHandle = ii_link->connHandle;
		
		IIapi_disconnect(&disconnParm);
		ii_link->connHandle = NULL;
	}
	if (ii_link->errorText != NULL )
	{
		free(ii_link->errorText);
		ii_link->errorText = NULL;
	}
}
/*  }}} */

/* {{{ static void _clean_ii_plink(zend_rsrc_list_entry *rsrc TSRMLS_DC) */
static void _clean_ii_plink(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	II_LINK *ii_link = (II_LINK *)rsrc->ptr;
	_ai_clean_ii_plink(ii_link TSRMLS_CC);
}
/* }}} */

/* {{{ static void php_ii_set_default_link(int id TSRMLS_DC) */
/* sets the default link */
static void php_ii_set_default_link(int id TSRMLS_DC)
{
	if (IIG(default_link) != -1)
	{
		zend_list_delete(IIG(default_link));
	}
	IIG(default_link) = id;
	zend_list_addref(id);
}
/*  }}} */

/* {{{ static int php_ii_get_default_link(INTERNAL_FUNCTION_PARAMETERS)
 * gets the default link if none has been set, tries to open a new one with default parameters
*/
static int php_ii_get_default_link(INTERNAL_FUNCTION_PARAMETERS)
{
	if (IIG(default_link) == -1)
	{	/* no link opened yet, implicitly open one */
		ht = 0;
		php_ii_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
	}
	return IIG(default_link);
}
/* }}} */

/* {{{ static void php_ii_globals_init(zend_ii_globals *ii_globals) */
static void php_ii_globals_init(zend_ii_globals *ii_globals)
{
	IIAPI_INITPARM initParm;

	ii_globals->num_persistent = 0;

	/* Ingres api initialization */
	/* timeout in ms, -1, (default) = no timeout */
	initParm.in_timeout = ii_globals->connect_timeout;				

#if defined(IIAPI_VERSION_4) 
    initParm.in_version = IIAPI_VERSION_4;
#elif defined(IIAPI_VERSION_3)
    initParm.in_version = IIAPI_VERSION_3;
#elif defined(IIAPI_VERSION_2) 
    initParm.in_version = IIAPI_VERSION_2;
#else
    initParm.in_version = IIAPI_VERSION_1;
#endif

	IIapi_initialize(&initParm);

#ifdef IIAPI_VERSION_2
	if ( initParm.in_envHandle != NULL )
	{	
		ii_globals->envHandle = initParm.in_envHandle; 
	}
#else
	ii_globals->envHandle = NULL;
#endif

}
/* }}} */

/* {{{ static void php_ii_globals_shutdown(zend_ii_globals *ii_globals) */
static void php_ii_globals_shutdown(zend_ii_globals *ii_globals)
{
#ifdef IIAPI_VERSION_2
	IIAPI_RELENVPARM   relEnvParm;

	relEnvParm.re_envHandle = ii_globals->envHandle;
    IIapi_releaseEnv( &relEnvParm );
#endif

}
/* }}} */

/* {{{ Module initialization
*/
PHP_MINIT_FUNCTION(ii)
{
	
	ZEND_INIT_MODULE_GLOBALS(ii, php_ii_globals_init, php_ii_globals_shutdown);
	REGISTER_INI_ENTRIES();

	le_ii_link = zend_register_list_destructors_ex(php_close_ii_link, 	NULL, "ingres", module_number);
	le_ii_plink = zend_register_list_destructors_ex(_clean_ii_plink, _close_ii_plink, "ingres persistent", module_number);

	/* Constants registration */
	REGISTER_LONG_CONSTANT("INGRES_ASSOC",				II_ASSOC,				CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_NUM",				II_NUM,					CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_BOTH",				II_BOTH,				CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("INGRES_EXT_VERSION",			II_VERSION,				CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_API_VERSION",		IIAPI_VERSION,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_CURSOR_READONLY",	II_CURSOR_READONLY,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_CURSOR_UPDATE",		II_CURSOR_UPDATE,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_US",			II_DATE_US,				CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_MULTINATIONAL",	II_DATE_MULTINATIONAL,	CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_MULTINATIONAL4",	II_DATE_MULTINATIONAL4,	CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_FINNISH",		II_DATE_FINNISH,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_ISO",			II_DATE_ISO,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_ISO4",			II_DATE_ISO4,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_GERMAN",		II_DATE_GERMAN,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_MDY",			II_DATE_MDY,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_DMY",			II_DATE_DMY,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_YMD",			II_DATE_YMD,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_MONEY_LEADING",		II_MONEY_LEAD_SIGN,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_MONEY_TRAILING",		II_MONEY_TRAIL_SIGN,	CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
} 
/* }}} */

/* {{{ Module shutdown */
PHP_MSHUTDOWN_FUNCTION(ii)
{
	IIAPI_TERMPARM termParm;
	UNREGISTER_INI_ENTRIES();

	/* Ingres api termination */
	IIapi_terminate(&termParm);
	if (termParm.tm_status == IIAPI_ST_SUCCESS)
	{
		return SUCCESS;
	} else {
		return FAILURE;
	}
}
/* }}} */

/* {{{ New request initialization */
PHP_RINIT_FUNCTION(ii)
{
	IIG(default_link) = -1;
	IIG(num_links) = IIG(num_persistent);
	IIG(cursor_no) = 0;
	return SUCCESS;
}
/* }}} */

/* {{{ End of request */
PHP_RSHUTDOWN_FUNCTION(ii)
{
	if (IIG(default_link) != -1)
	{
		zend_list_delete(IIG(default_link));
		IIG(default_link) = -1;
	}
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION(ii)
 * Information reported to phpinfo()
*/
PHP_MINFO_FUNCTION(ii)
{
	char buf[32];

	php_info_print_table_start();
	php_info_print_table_header(2, "Ingres Support", "enabled");
	php_info_print_table_row(2, "Ingres Extension Version", II_VERSION);
	php_info_print_table_row(2, "Revision", "$Revision$");
	sprintf(buf, "%ld", IIAPI_VERSION );
	php_info_print_table_row(2, "Ingres OpenAPI Version", buf);
	sprintf(buf, "%ld", IIG(num_persistent));
	php_info_print_table_row(2, "Active Persistent Links", buf);
	sprintf(buf, "%ld", IIG(num_links));
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
	static IIAPI_WAITPARM waitParm = {
		-1,		/* no timeout, we don't want asynchronous queries */
		0		/* wt_status (output) */
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

/* {{{ static int ii_success(IIAPI_GENPARM *genParm, II_LINK *ii_link TSRMLS_DC)
 * Handles and stores errors for later retrieval from Ingres api
*/
static int ii_success(IIAPI_GENPARM *genParm, II_LINK *ii_link TSRMLS_DC)
{
	IIAPI_GETEINFOPARM *error_info;
	char *	no_message;

    no_message = emalloc(1);
	sprintf(no_message,"\0");

	/* Initialise global variables */
	IIG(errorCode) = 0;
	sprintf(IIG(sqlstate),"\0\0\0\0\0\0");
	if ( IIG(errorText) != NULL)
	{
		free(IIG(errorText));
		IIG(errorText) = NULL;
	}

	/* Initialise link */
	if (ii_link->connHandle != NULL)
	{
		ii_link->errorCode = 0;
		sprintf(ii_link->sqlstate,"\0\0\0\0\0\0");
		if ( ii_link->errorText != NULL)
		{
			free(ii_link->errorText);
			ii_link->errorText=NULL;
		}
	}

	efree(no_message);

	switch (genParm->gp_status)
	{
	
		case IIAPI_ST_SUCCESS:
			return II_OK;
			
		case IIAPI_ST_NO_DATA:
			return II_NO_DATA;

		default:
			if (genParm->gp_errorHandle == NULL)
			{	/* no error message available */
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Server or API error - no error message available");
			} else {
                error_info=(IIAPI_GETEINFOPARM *) emalloc(sizeof(IIAPI_GETEINFOPARM));
				error_info->ge_errorHandle = genParm->gp_errorHandle;
				IIapi_getErrorInfo(error_info);

			    /* load error information into globals */
				IIG(errorCode) = error_info->ge_errorCode;
				memcpy(IIG(sqlstate),error_info->ge_SQLSTATE,sizeof(error_info->ge_SQLSTATE));
				IIG(errorText) = strdup(error_info->ge_message);

				/* load error information into the passed link
                */
				if (ii_link->connHandle != NULL)
				{
					ii_link->errorCode = error_info->ge_errorCode;
					memcpy (ii_link->sqlstate ,error_info->ge_SQLSTATE,sizeof(error_info->ge_SQLSTATE));
					ii_link->errorText=strdup(error_info->ge_message);
				}
				if (IIG(report_db_warnings))
				{
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres error : %ld : %s",IIG(errorCode), IIG(errorText));
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres SQLSTATE : %s", IIG(sqlstate));
				}
				efree(error_info);
			}
			return II_FAIL;
	}
} 
/* }}} */

/* {{{ static void php_ii_do_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent)
 * Actually handles connection creation, either persistent or not
*/
static void php_ii_do_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent)
{
	zval **database, **username, **password, **options;
	char *db, *user, *pass;
	int argc, dblen;
	char *hashed_details;
	int hashed_details_length;
	IIAPI_CONNPARM connParm;
	II_LINK *ii_link;
	II_PTR	envHandle = (II_PTR)NULL;
	char *z_type;

	if (IIG(trace_connect)) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Enter php_ii_do_connect");
	}

	/* Setting db, user and pass according to sql_safe_mode, parameters and/or default values */
	argc = ZEND_NUM_ARGS();

	if (PG(sql_safe_mode))
	{	/* sql_safe_mode */

		if (IIG(trace_connect)) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "SQL safe mode in effect");
		}

		if (argc > 0)
		{
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "SQL safe mode in effect - ignoring host/user/password/option information");
		}

		db = NULL;
		pass = NULL;
		user = php_get_current_user();
		hashed_details_length = strlen(user) + sizeof("ingres___") - 1;
		hashed_details = (char *) emalloc(hashed_details_length + 1);
		sprintf(hashed_details, "Ingres__%s_", user);

	} else {					/* non-sql_safe_mode */
		if (IIG(trace_connect)) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "SQL safe mode not in effect");
		}
		db = IIG(default_database);
		user = IIG(default_user);
		pass = IIG(default_password);

		if (argc > 4 || zend_get_parameters_ex(argc, &database, &username, &password, &options) == FAILURE)
		{
			WRONG_PARAM_COUNT;
		}

		switch (argc)
		{
			case 4: 
  				if ( Z_TYPE_PP(options) != IS_ARRAY ) 
				{
					switch (Z_TYPE_PP(options))
					{
						case IS_STRING:
							z_type = emalloc(9);
							sprintf(z_type,"a STRING");
							break;
						case IS_LONG:
							z_type = emalloc(7);
							sprintf (z_type,"a LONG");
							break;
						case IS_DOUBLE:
							z_type = emalloc(9);
							sprintf (z_type,"a DOUBLE");
							break;
						case IS_RESOURCE:
							z_type = emalloc(11);
							sprintf (z_type,"a RESOURCE");
							break;
						case IS_OBJECT:
							z_type = emalloc(10);
							sprintf (z_type,"an OBJECT");
							break;
						case IS_NULL:
							z_type = emalloc(7);
							sprintf (z_type,"a NULL");
							break;
						default:
							z_type = emalloc(16);
							sprintf (z_type,"an UNKNOWN type");
							break;
					}

					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expected an array of connect options but got %s instead.", z_type );
					efree(z_type);
					RETURN_FALSE;
				}
				/* Fall-through.  */
			case 3:
				convert_to_string_ex(password);
				pass = Z_STRVAL_PP(password);
				/* Fall-through. */
		
			case 2:
				convert_to_string_ex(username);
				user = Z_STRVAL_PP(username);
				/* Fall-through. */
		
			case 1:
				convert_to_string_ex(database);
				db = Z_STRVAL_PP(database);
				/* Fall-through. */

			case 0:
				break;
		}
		
		/* Perform Sanity Check. If no database has been set then we have a problem */
		if ( db != NULL ) {
			dblen = strlen(db);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "No default database available to connect to" );
			RETURN_FALSE;
		}
		if ( dblen == 0 )
		{
		}
		if (IIG(trace_connect)) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Connecting to %s as %s", db, user);
		}

		hashed_details_length =	sizeof("ingres___") - 1 + 
								strlen(SAFE_STRING(db)) +
								strlen(SAFE_STRING(user)) + 
								strlen(SAFE_STRING(pass));

		hashed_details = (char *) emalloc(hashed_details_length + 1);
		sprintf(hashed_details, "Ingres_%s_%s_%s", 
								SAFE_STRING(db),	
								SAFE_STRING(user), 
								SAFE_STRING(pass));
	}

	/* if asked for unauthorized persistency, issue a warning
	   and go for a non-persistent link */
	if (persistent && !IIG(allow_persistent))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Persistent links disabled !");
		persistent = 0;
	}

	if (persistent)
	{
		list_entry *le;

		if (IIG(trace_connect)) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Enter persistent connection");
		}

		/* is this link already in the persistent list ? */
		if (zend_hash_find(&EG(persistent_list), hashed_details, hashed_details_length + 1, (void **) &le) == FAILURE)
		{ /* no, new persistent connection */

			list_entry new_le;

			if (IIG(trace_connect)) {
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "creating new persistent connection");
			}

			if (IIG(max_links) != -1 && IIG(num_links) >= IIG(max_links))
			{
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Too many open links (%d)", IIG(num_links));
				efree(hashed_details);
				RETURN_FALSE;
			}
			if (IIG(max_persistent) != -1 && IIG(num_persistent) >= IIG(max_persistent))
			{
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Too many open persistent links (%d)", IIG(num_persistent));
				efree(hashed_details);
				RETURN_FALSE;
			}

			/* setup the link */
			ii_link = (II_LINK *) malloc(sizeof(II_LINK));
			ii_link->envHandle = IIG(envHandle);
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
			ii_link->fieldCount = 0;
			ii_link->descriptor = NULL;
			ii_link->autocommit = 0;
			ii_link->errorCode = 0;
			ii_link->errorText = NULL;
			ii_link->cursor_id = NULL;
			ii_link->cursor_mode = -1;
			ii_link->paramCount = 0;
			ii_link->procname = NULL;

			if ( argc == 4 ) /* set options */
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
			connParm.co_timeout = -1;	/* -1 is no timeout */
			connParm.co_type = IIAPI_CT_SQL;
			if ( ii_link->connHandle == NULL ) 
			{	
				connParm.co_connHandle =  ii_link->envHandle;
			}
			else
			{
				connParm.co_connHandle =  ii_link->connHandle;
			}
			
			connParm.co_tranHandle = NULL;

			if (IIG(trace_connect)) {
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Start IIapi_connect()");
			}

			IIapi_connect(&connParm);

			if (!ii_sync(&(connParm.co_genParm)) || ii_success(&(connParm.co_genParm), ii_link TSRMLS_CC) == II_FAIL)
			{
				free(ii_link);
				efree(hashed_details);
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to connect to database (%s)", db);
				RETURN_FALSE;
			}
			if (IIG(trace_connect)) {
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "End IIapi_connect()");
			}

			ii_link->connHandle = connParm.co_connHandle;

			/* set environment params */

			if ( argc == 4 ) /* set options */
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
			IIG(num_persistent)++;
			IIG(num_links)++;

		} else { /* already open persistent connection */

			if (IIG(trace_connect)) {
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

				ii_link->envHandle = IIG(envHandle);
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
				ii_link->fieldCount = 0;
				ii_link->descriptor = NULL;
				ii_link->autocommit = 0;
				ii_link->errorCode = 0;
				ii_link->errorText = NULL;
				ii_link->cursor_id = NULL;
				ii_link->cursor_mode = -1;
				ii_link->paramCount = 0;
				ii_link->procname = NULL;

				if ( argc == 4 ) /* set options */
				{
					if ( php_ii_set_connect_options(options, ii_link, db TSRMLS_CC ) == II_FAIL )
					{
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to set options provided");
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
				connParm.co_type = IIAPI_CT_SQL;

				if (IIG(trace_connect)) {
					php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Start IIapi_connect()");
				}

				IIapi_connect(&connParm);

				if (!ii_sync(&(connParm.co_genParm)) || ii_success(&(connParm.co_genParm), ii_link TSRMLS_CC) == II_FAIL)
				{
					efree(hashed_details);
					php_error_docref(NULL TSRMLS_CC, E_WARNING,"Unable to connect to database (%s)", db);
					RETURN_FALSE;
				}

				if (IIG(trace_connect)) {
					php_error_docref(NULL TSRMLS_CC, E_NOTICE, "End IIapi_connect()");
				}

				ii_link->connHandle = connParm.co_connHandle;

				/* set environment params */

				if ( argc == 4 ) /* set options */
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

		if (IIG(trace_connect)) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Exit persistent connection");
		}

	} else { /* non persistent */

		list_entry *index_ptr, new_index_ptr;

		if (IIG(trace_connect)) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Enter non-persistent connection");
		}

		/* first we check the hash for the hashed_details key.  if it exists,
		 * it should point us to the right offset where the actual link sits.
		 * if it doesn't, open a new link, add it to the resource list,
		 * and add a pointer to it with hashed_details as the key.
		 */

		if (zend_hash_find(&EG(regular_list), hashed_details, hashed_details_length + 1, (void **) &index_ptr) == SUCCESS)
		{
			int type;
			void *ptr;

			if (Z_TYPE_P(index_ptr) != le_index_ptr)
			{
				RETURN_FALSE;
			}
			ii_link = (II_LINK *) index_ptr->ptr;
			ptr = zend_list_find((int) ii_link, &type);	/* check if the link is still there */
			if (ptr && (type == le_ii_link || type == le_ii_plink))
			{
				zend_list_addref((int) ii_link);
				Z_LVAL_P(return_value) = (int) ii_link;

				php_ii_set_default_link((int) ii_link TSRMLS_CC);

				Z_TYPE_P(return_value) = IS_RESOURCE;
				efree(hashed_details);
				return;
			} else {
				zend_hash_del(&EG(regular_list), hashed_details, hashed_details_length + 1);
			}
		}
		if (IIG(max_links) != -1 && IIG(num_links) >= IIG(max_links))
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Too many open links (%d)", IIG(num_links));
			efree(hashed_details);
			RETURN_FALSE;
		}

		/* setup the link */
		ii_link = (II_LINK *) malloc(sizeof(II_LINK));
		
		ii_link->envHandle = IIG(envHandle);
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
		ii_link->fieldCount = 0;
		ii_link->descriptor = NULL;
		ii_link->autocommit = 0;
		ii_link->errorCode = 0;
		ii_link->errorText = NULL;
		ii_link->cursor_id = NULL;
		ii_link->cursor_mode = -1;
		ii_link->paramCount = 0;
		ii_link->procname = NULL;

		IIG(errorCode)=0;

		if ( argc == 4 ) /* set options */
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
		connParm.co_timeout = -1;	/* -1 is no timeout */
		connParm.co_type = IIAPI_CT_SQL;
		if ( ii_link->connHandle == NULL ) 
		{
			connParm.co_connHandle =  ii_link->envHandle;
		}
		else
		{
			connParm.co_connHandle =  ii_link->connHandle;
		}

		connParm.co_tranHandle = NULL;

		if (IIG(trace_connect)) {
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Start IIapi_connect()");
		}

		IIapi_connect(&connParm);

		if (!ii_sync(&(connParm.co_genParm)) || ii_success(&(connParm.co_genParm), ii_link TSRMLS_CC) == II_FAIL)
		{
			efree(hashed_details);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to connect to database (%s)", db);
			RETURN_FALSE;
		}

		if (IIG(trace_connect)) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "End IIapi_connect()");
		}

		ii_link->connHandle = connParm.co_connHandle;		

		/* set environment params */

		if ( argc == 4 ) /* set options */
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
		IIG(num_links)++;

		if (IIG(trace_connect)) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Exit non-persistent connection");
		}
	}

	efree(hashed_details);
	php_ii_set_default_link(Z_LVAL_P(return_value) TSRMLS_CC);

	if (IIG(trace_connect)) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Exit php_ii_do_connect");
	}
} 
/* }}} */

/* {{{ proto resource ingres_connect([string database [, string username [, string password [, options ]]]])
   Open a connection to an Ingres database the syntax of database is [node_id::]dbname[/svr_class] */
PHP_FUNCTION(ingres_connect)
{
	php_ii_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto resource ingres_pconnect([string database [, string username [, string password [, options ]]]])
   Open a persistent connection to an Ingres database the syntax of database is [node_id::]dbname[/svr_class] */
PHP_FUNCTION(ingres_pconnect)
{
	php_ii_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto bool ingres_close([resource link])
   Close an Ingres database connection */
PHP_FUNCTION(ingres_close)
{
	zval **link = NULL;
	int link_id = -1;
	II_LINK *ii_link;

	switch (ZEND_NUM_ARGS())
	{
		case 0: 
			link_id = IIG(default_link);
			if (link_id == -1)
			{
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
				RETURN_FALSE;
			}

			break;

		case 1: 
			if (zend_get_parameters_ex(1, &link) == FAILURE)
			{
				RETURN_FALSE;
			}
			link_id = -1;
			break;

		default:
			WRONG_PARAM_COUNT;
			break;
	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	/* Call the clean function synchronously here */
	/* Otherwise we have to wait for request shutdown */
	/* This way we can reuse the link in the same script */
	_ai_clean_ii_plink(ii_link TSRMLS_CC);
  
	if (link_id == -1)
	{ /* explicit resource number */
		zend_list_delete(Z_RESVAL_PP(link));
	}
	
	if (link_id != -1 || (ii_link && Z_RESVAL_PP(link) == IIG(default_link)))
	{
		zend_list_delete(IIG(default_link));
		IIG(default_link) = -1;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ingres_query(string query [, resource link] [, array queryParams] [, array paramtypes] ) */
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
PHP_FUNCTION(ingres_query)
{
	zval **query, **link, **queryParams, **paramtypes=NULL;
	int argc;
	int link_id = -1;
	II_LINK *ii_link;
	IIAPI_QUERYPARM     queryParm;
	IIAPI_GETDESCRPARM  getDescrParm;
	
	HashTable *arr_hash;
	int elementCount;
	char *procname=NULL;
	char *statement;

	argc = ZEND_NUM_ARGS();
	if (argc < 1 || argc > 4 || zend_get_parameters_ex(argc, &query, &link, &queryParams, &paramtypes) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}
	
	if ( argc == 1 )
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}
	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	/* if there's already an active statement, close it */
	if (ii_link->stmtHandle && _close_statement(ii_link TSRMLS_CC))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close statement");
		RETURN_FALSE;
	}

	convert_to_string_ex(query);

	/* check to see if there are any parameters to the query */

	ii_link->paramCount = php_ii_paramcount(Z_STRVAL_PP(query) TSRMLS_CC);

	if ( ii_link->paramCount > 0 )
	{
		if ((argc < 3) || (Z_TYPE_PP(queryParams) != IS_ARRAY))
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expecting a parameter array but did not get one" );
			RETURN_FALSE;
		}

		arr_hash = Z_ARRVAL_PP(queryParams);

		if ((elementCount = zend_hash_num_elements(arr_hash)) != ii_link->paramCount )
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Incorrect number of parameters passed, expected %d got %d",ii_link->paramCount, elementCount );
			RETURN_FALSE;
		}
		
		zend_hash_internal_pointer_reset(Z_ARRVAL_PP(queryParams));

		
		/* if we have been provided with type hints for parameters check to see the number of params */
		/* passed matches the number of type hints recieved */
		if ( argc >= 4 && ( Z_TYPE_PP(paramtypes) != IS_STRING ) )
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expecting a string list of parameter types but did not recieve one. " );
			RETURN_FALSE;
		}

		if ((argc < 4 ) && (Z_STRLEN_PP(paramtypes) != ii_link->paramCount ))
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Incorrrect number of parameter types recieved, expected %d got %d", ii_link->paramCount, Z_STRLEN_PP(paramtypes) );
			RETURN_FALSE;
		}
	}

	/* check to see if this is a procedure or not
	load the procedure name into ii_link->procname.
	If ii_link->procname is NULL then there is no procedure */

	if ( ii_link->procname != NULL )
	{
		free(ii_link->procname);
		ii_link->procname = NULL;
	}

    ii_link->procname = php_ii_check_procedure(Z_STRVAL_PP(query), ii_link TSRMLS_CC);

	if ( ii_link->paramCount > 0  && ii_link->procname == NULL )
	{ /* convert ? to ~V so we don't have to prepare the query */
		statement = php_ii_convert_param_markers( Z_STRVAL_PP(query) TSRMLS_CC );
	}
	
	queryParm.qy_genParm.gp_callback = NULL;
	queryParm.qy_genParm.gp_closure = NULL;
	queryParm.qy_connHandle = ii_link->connHandle;
	queryParm.qy_tranHandle = ii_link->tranHandle;
	queryParm.qy_stmtHandle = NULL;

	if ( ii_link->procname == NULL )
	{
		queryParm.qy_queryType  = IIAPI_QT_QUERY;
		if ( ii_link->paramCount > 0 )
		{
			queryParm.qy_parameters = TRUE;
			queryParm.qy_queryText = statement;
		}
		else
		{
			queryParm.qy_parameters = FALSE;
			queryParm.qy_queryText  = Z_STRVAL_PP(query);
		}
	}
	else
	{
		queryParm.qy_queryType  = IIAPI_QT_EXEC_PROCEDURE;
		queryParm.qy_parameters = TRUE;
		queryParm.qy_queryText  = NULL;
	}

	IIapi_query(&queryParm);
	ii_sync(&(queryParm.qy_genParm));

	if (ii_success(&(queryParm.qy_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		RETURN_FALSE;
	} 

	/* store transaction and statement handles */
	ii_link->tranHandle = queryParm.qy_tranHandle;
	ii_link->stmtHandle = queryParm.qy_stmtHandle;

	if ( ii_link->paramCount > 0  ||  ii_link->procname != NULL )
	{
		if ( php_ii_bind_params (INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_link, queryParams, paramtypes) == II_FAIL)
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING,"Error binding parameters");
			RETURN_FALSE;
		}
	}
				
	/* get description of results */
	getDescrParm.gd_genParm.gp_callback = NULL;
	getDescrParm.gd_genParm.gp_closure  = NULL;
	getDescrParm.gd_stmtHandle = ii_link->stmtHandle;

	IIapi_getDescriptor(&getDescrParm);
	ii_sync(&(getDescrParm.gd_genParm));

	if (ii_success(&(getDescrParm.gd_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		RETURN_FALSE;
	}

	/* store the results */
	ii_link->fieldCount = getDescrParm.gd_descriptorCount;
	ii_link->descriptor = getDescrParm.gd_descriptor;

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ingres_prepare(string query [, long cursor_mode [, resource link]]) */
/* Prepare SQL for later execution */
PHP_FUNCTION(ingres_prepare)
{
	zval **query, **link, **cursor_mode;
	int argc;
	int link_id = -1;
	II_LINK *ii_link;

	IIAPI_QUERYPARM     	queryParm;
	IIAPI_CLOSEPARM	    	closeParm;

	int queryLen;
	char *statement;
	char *preparedStatement;

	int cursor_id_len;

	argc = ZEND_NUM_ARGS();
	if (argc < 1 || argc > 3 || zend_get_parameters_ex(argc, &query, &cursor_mode, &link) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}

	if  ( argc == 1 || argc == 2 )
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}

	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	if ( argc == 2 || argc == 3 )
	{
		convert_to_long_ex(cursor_mode);
		ii_link->cursor_mode = (Z_LVAL_PP(cursor_mode) < 0  || Z_LVAL_PP(cursor_mode) > 1 ? II_CURSOR_UPDATE : Z_LVAL_PP(cursor_mode));
	}
	else
	{
		/* set to the ini default */
		ii_link->cursor_mode = IIG(cursor_mode);
	}

	convert_to_string_ex(query);
	statement = Z_STRVAL_PP(query);

	/* figure how many parameters are expected */
	ii_link->paramCount = php_ii_paramcount(statement TSRMLS_CC);

	/* if there's already an active statement, close it */
	if (ii_link->stmtHandle && _close_statement(ii_link TSRMLS_CC))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close statement");
		RETURN_FALSE;
	}

	ii_link->procname = NULL;

	/* check to see if this is a procedure or not
	   load the procedure name into ii_link->procname.
	   If ii_link->procname is NULL then there is no procedure */

    ii_link->procname = php_ii_check_procedure(Z_STRVAL_PP(query), ii_link TSRMLS_CC);

	if ( ii_link->procname == NULL )
	{
		/* Adapt the query into a prepared statement */
		queryLen = strlen(statement);

		ii_link->cursor_id = malloc(33);
		php_ii_gen_cursor_id(ii_link TSRMLS_CC);
		cursor_id_len = strlen(ii_link->cursor_id);
		preparedStatement=ecalloc(queryLen + 15 + cursor_id_len, 1);
		sprintf (preparedStatement,"Prepare %s from %s\0", ii_link->cursor_id, statement);
		statement = preparedStatement;

		queryParm.qy_genParm.gp_callback = NULL;
		queryParm.qy_genParm.gp_closure = NULL;
		queryParm.qy_connHandle = ii_link->connHandle;
		queryParm.qy_tranHandle = ii_link->tranHandle;
		queryParm.qy_stmtHandle = NULL;
		queryParm.qy_queryType  = IIAPI_QT_QUERY; 
		queryParm.qy_parameters = FALSE;
		queryParm.qy_queryText  = statement;

		IIapi_query(&queryParm);
		ii_sync(&(queryParm.qy_genParm));

		if (ii_success(&(queryParm.qy_genParm), ii_link TSRMLS_CC) == II_FAIL) 
		{
			efree(preparedStatement);
			RETURN_FALSE;
		}

		ii_link->tranHandle = queryParm.qy_tranHandle;
		ii_link->stmtHandle = queryParm.qy_stmtHandle;

		/*
		** Call IIapi_close() to release resources.
		*/
		closeParm.cl_genParm.gp_callback = NULL;
		closeParm.cl_genParm.gp_closure = NULL;
		closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

		IIapi_close( &closeParm );
		ii_sync(&(closeParm.cl_genParm));
		if (ii_success(&(closeParm.cl_genParm), ii_link TSRMLS_CC) == II_FAIL) 
		{
			efree(preparedStatement);
			RETURN_FALSE;
		}
		efree(preparedStatement);
	}
}
/* }}} */

/* {{{ proto int ingres_execute([array params,[string paramtypes [, resource link ]]]) 
   execute a query prepared by ingres_prepare() */
PHP_FUNCTION (ingres_execute)
{

	zval **link, **queryParams, **paramtypes;
	int argc;
	int link_id = -1;
	II_LINK *ii_link;
	IIAPI_QUERYPARM     queryParm;
	IIAPI_GETDESCRPARM  getDescrParm;
	II_INT2				columnType;

	int elementCount;
	char *statement;

	argc = ZEND_NUM_ARGS();
	if (argc > 3 || zend_get_parameters_ex(argc, &queryParams, &paramtypes, &link) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}

	if ( argc <= 1 )
	{		
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}
	}
		
	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

    if ( ii_link->paramCount > 0 )
	{
		if (Z_TYPE_PP(queryParams) != IS_ARRAY )
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expecting a parameter array but did not get one" );
			RETURN_FALSE;
		}

		if ((elementCount = zend_hash_num_elements(Z_ARRVAL_PP(queryParams))) != ii_link->paramCount )
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Incorrect number of parameters passed, expected %d got %d",ii_link->paramCount, elementCount );
			RETURN_FALSE;
		}
		zend_hash_internal_pointer_reset(Z_ARRVAL_PP(queryParams));
	}

	if ( ii_link->procname == NULL )
	{
		statement = ecalloc(sizeof(ii_link->cursor_id) + 17, 1);

		/* Set the cursor mode according to ii_link->cursor_mode */
		switch (ii_link->cursor_mode)
		{
			case II_CURSOR_UPDATE:
				sprintf (statement,"%s", ii_link->cursor_id );
				break;
			case II_CURSOR_READONLY:
				sprintf (statement,"%s for readonly", ii_link->cursor_id );
				break;
			default:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Incorrect number of parameters passed, expected %d got %d",ii_link->paramCount, elementCount );
				efree(statement);
				RETURN_FALSE;
		}

		queryParm.qy_genParm.gp_callback = NULL;
		queryParm.qy_genParm.gp_closure = NULL;
		queryParm.qy_connHandle = ii_link->connHandle;
		queryParm.qy_tranHandle = ii_link->tranHandle;
		queryParm.qy_stmtHandle = NULL;
		queryParm.qy_queryType  = IIAPI_QT_OPEN; 

		if (ii_link->paramCount >0 ) 
		{
			queryParm.qy_parameters = TRUE;
		} 
		else 
		{
			queryParm.qy_parameters = FALSE;
		}

		queryParm.qy_queryText  = statement;

		columnType = IIAPI_COL_QPARM;

	}
	else
	{
		queryParm.qy_genParm.gp_callback = NULL;
		queryParm.qy_genParm.gp_closure = NULL;
		queryParm.qy_connHandle = ii_link->connHandle;
		queryParm.qy_tranHandle = ii_link->tranHandle;
		queryParm.qy_stmtHandle = NULL;
		queryParm.qy_queryType  = IIAPI_QT_EXEC_PROCEDURE; 
		queryParm.qy_parameters = TRUE;
		queryParm.qy_queryText  = NULL;

		columnType = IIAPI_COL_PROCPARM;

		/* allow for the procedure name as a param */
		ii_link->paramCount++;

	}

	IIapi_query(&queryParm);
	ii_sync(&(queryParm.qy_genParm));

	if (ii_success(&(queryParm.qy_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		if ( ii_link->procname == NULL )
		{
			efree (statement);
		}
		RETURN_FALSE;
	}

	ii_link->tranHandle = queryParm.qy_tranHandle;
	ii_link->stmtHandle = queryParm.qy_stmtHandle;

	if ( ii_link->paramCount > 0 )
	{
		if ( php_ii_bind_params (INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_link, queryParams, paramtypes) == II_FAIL)
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING,"Error binding parameters");
			RETURN_FALSE;
		}
	} else {
		/* not handling parameters */

		queryParm.qy_genParm.gp_callback = NULL;
		queryParm.qy_genParm.gp_closure = NULL;
		queryParm.qy_connHandle = ii_link->connHandle;
		queryParm.qy_tranHandle = ii_link->tranHandle;
		queryParm.qy_stmtHandle = NULL;
		queryParm.qy_queryType  = IIAPI_QT_QUERY;
		queryParm.qy_parameters = FALSE;
		queryParm.qy_queryText  = statement;

		IIapi_query(&queryParm);
		ii_sync(&(queryParm.qy_genParm));

		if (ii_success(&(queryParm.qy_genParm), ii_link TSRMLS_CC) == II_FAIL)
		{
			if ( ii_link->procname == NULL )
			{
				efree (statement);
			}
			RETURN_FALSE;
		}

	} /* if ii_link->paramCount > 0 */

	/* store transaction and statement handles */
	ii_link->tranHandle = queryParm.qy_tranHandle;
	ii_link->stmtHandle = queryParm.qy_stmtHandle;

	
	/* get description of results */
	getDescrParm.gd_genParm.gp_callback = NULL;
	getDescrParm.gd_genParm.gp_closure  = NULL;
	getDescrParm.gd_stmtHandle = ii_link->stmtHandle;

	IIapi_getDescriptor(&getDescrParm);
	ii_sync(&(getDescrParm.gd_genParm));

	if (ii_success(&(getDescrParm.gd_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		if ( ii_link->procname == NULL )
		{
			efree (statement);
		}
		RETURN_FALSE;
	}

	/* store the results */
	ii_link->fieldCount = getDescrParm.gd_descriptorCount;
	ii_link->descriptor = getDescrParm.gd_descriptor;
	if ( ii_link->procname == NULL )
	{
		efree (statement);
	}

}
/* }}} */

/* {{{ php_ii_gen_cursor_id(char *cursor_id TSRMLS_DC) 
   generates a cursor name */
static void  php_ii_gen_cursor_id(II_LINK *ii_link TSRMLS_DC)
{
	char *tmp_id = '\0';

	if (ii_link->cursor_id != NULL)
	{
		free(ii_link->cursor_id);
	}	

	tmp_id = ecalloc (33,1);
	IIG(cursor_no)++;
	sprintf (tmp_id,"php_%d_%d", II_THREAD_ID, IIG(cursor_no));
	ii_link->cursor_id = malloc(sizeof(tmp_id));
	strcpy(ii_link->cursor_id,tmp_id);
	efree(tmp_id);
}
/* }}} */

/* {{{ proto string ingres_cursor(resource link)
   Gets a cursor name for a given link resource */
PHP_FUNCTION(ingres_cursor)
{
	zval **link; 
	int argc;
	II_LINK *ii_link;
	int link_id = -1;

	argc = ZEND_NUM_ARGS();
	if (argc > 1  || zend_get_parameters_ex(argc, &link) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}

	if (argc < 1)
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}
	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	RETURN_STRING(ii_link->cursor_id,1);
}
/* }}} */

/* {{{ proto int ingres_num_rows([resource link])
   Return the number of rows affected/returned by the last query */

/* Warning : don't call ingres_num_rows() before ingres_fetch_xx() since IIapi_getQueryInfo()
 * nullifies the data set being returned and ingres_fetch_xx() will not find any data */
PHP_FUNCTION(ingres_num_rows)
{
	zval **link;
	int argc;
	int link_id = -1;
	II_LINK *ii_link;
	IIAPI_GETQINFOPARM getQInfoParm;

	argc = ZEND_NUM_ARGS();
	if (argc > 1 || zend_get_parameters_ex(argc, &link) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}

	if (argc < 1)
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}
	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	/* get number of affected rows */
	getQInfoParm.gq_genParm.gp_callback = NULL;
	getQInfoParm.gq_genParm.gp_closure = NULL;
	getQInfoParm.gq_stmtHandle = ii_link->stmtHandle;

	IIapi_getQueryInfo(&getQInfoParm);
	ii_sync(&(getQInfoParm.gq_genParm));

	if (ii_success(&(getQInfoParm.gq_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		RETURN_FALSE;
	}

	/* return the result */
	if (getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT)
	{
		RETURN_LONG(getQInfoParm.gq_rowCount);
	} else {
		RETURN_LONG(0);
	}
}
/* }}} */

/* {{{ proto int ingres_num_fields([resource link])
   Return the number of fields returned by the last query */
PHP_FUNCTION(ingres_num_fields)
{
	zval **link;
	int argc;
	int link_id = -1;
	II_LINK *ii_link;

	argc = ZEND_NUM_ARGS();
	if (argc > 1 || zend_get_parameters_ex(argc, &link) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}

	if (argc < 1)
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}
	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	RETURN_LONG(ii_link->fieldCount);
}
/* }}} */

/* {{{ static void php_ii_field_info(INTERNAL_FUNCTION_PARAMETERS, int info_type)
 *  Return information about a field in a query result
*/
static void php_ii_field_info(INTERNAL_FUNCTION_PARAMETERS, int info_type)
{
	zval **idx, **link;
	int argc;
	int link_id = -1;
	char *name, *fun_name;
	int index;
	II_LINK *ii_link;

	argc = ZEND_NUM_ARGS();
	if (argc < 1 || argc > 2 || zend_get_parameters_ex(argc, &idx, &link) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}

	if (argc < 2)
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}
	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	convert_to_long_ex(idx);
	index = Z_LVAL_PP(idx);

	if (index < 1 || index > ii_link->fieldCount)
	{
		switch (info_type)
		{

			case II_FIELD_INFO_NAME:
				fun_name = "ii_field_name";
				break;

			case II_FIELD_INFO_TYPE:
				fun_name = "ii_field_type";
				break;

			case II_FIELD_INFO_NULLABLE:
				fun_name = "ii_field_nullable";
				break;
	
			case II_FIELD_INFO_LENGTH:
				fun_name = "ii_field_length";
				break;
	
			case II_FIELD_INFO_PRECISION:
				fun_name = "ii_field_precision";
				break;

			case II_FIELD_INFO_SCALE:
				fun_name = "ii_field_scale";
				break;

			default:
				fun_name = "foobar";
				break;
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() called with wrong index (%d)", fun_name, index);
		RETURN_FALSE;
	}

	switch (info_type)
	{

		case II_FIELD_INFO_NAME:
			name = php_ii_field_name(ii_link, index TSRMLS_CC);
			if (name == NULL)
			{
				RETURN_FALSE;
			}
			RETURN_STRING(name, 1);
			break;

		case II_FIELD_INFO_TYPE:
			switch ((ii_link->descriptor[index - 1]).ds_dataType)
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
#endif
		
				default:
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown Ingres data type");
					RETURN_FALSE;
					break;
			}
			break;

		case II_FIELD_INFO_NULLABLE:
			if ((ii_link->descriptor[index - 1]).ds_nullable)
			{
				RETURN_TRUE;
			} else {
				RETURN_FALSE;
			}
			break;

		case II_FIELD_INFO_LENGTH:
			RETURN_LONG((ii_link->descriptor[index - 1]).ds_length);
			break;
	
		case II_FIELD_INFO_PRECISION:
			RETURN_LONG((ii_link->descriptor[index - 1]).ds_precision);
			break;

		case II_FIELD_INFO_SCALE:
			RETURN_LONG((ii_link->descriptor[index - 1]).ds_scale);
			break;
	
		default:
			RETURN_FALSE;
	}
}

/* }}} */

/* {{{ static char *php_ii_field_name(II_LINK *ii_link, int index TSRMLS_DC)
  Return the name of a field in a query result */
static char *php_ii_field_name(II_LINK *ii_link, int index TSRMLS_DC)
{

	char *colname;
	
	if (index < 1 || index > ii_link->fieldCount)
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "php_ii_field_name() called with wrong index (%d)", index);
		return NULL;
	}

	if ( (ii_link->descriptor[index - 1]).ds_columnName != NULL )
	{
		return (ii_link->descriptor[index - 1]).ds_columnName;
	}
	else
	{ /* need to make up a column name if one is not available */
		colname = emalloc(8); /* colxxxx - should be enough */
		sprintf(colname,"col%d",index);
		return colname;
	}
}
/* }}} */

/* {{{ proto string ingres_field_name(int index [, resource link])
   Return the name of a field in a query result index must be >0 and <= ingres_num_fields() */
PHP_FUNCTION(ingres_field_name)
{
	php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, II_FIELD_INFO_NAME);
}
/* }}} */

/* {{{ proto string ingres_field_type(int index [, resource link])
   Return the type of a field in a query result index must be >0 and <= ingres_num_fields() */
PHP_FUNCTION(ingres_field_type)
{
	php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, II_FIELD_INFO_TYPE);
}
/* }}} */

/* {{{ proto string ingres_field_nullable(int index [, resource link])
   Return true if the field is nullable and false otherwise index must be >0 and <= ingres_num_fields() */
PHP_FUNCTION(ingres_field_nullable)
{
	php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, II_FIELD_INFO_NULLABLE);
}
/* }}} */

/* {{{ proto string ingres_field_length(int index [, resource link])
   Return the length of a field in a query result index must be >0 and <= ingres_num_fields() */
PHP_FUNCTION(ingres_field_length)
{
	php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, II_FIELD_INFO_LENGTH);
}
/* }}} */

/* {{{ proto string ingres_field_precision(int index [, resource link])
   Return the precision of a field in a query result index must be >0 and <= ingres_num_fields() */
PHP_FUNCTION(ingres_field_precision)
{
	php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, II_FIELD_INFO_PRECISION);
}
/* }}} */

/* {{{ proto string ingres_field_scale(int index [, resource link])
   Return the scale of a field in a query result index must be >0 and <= ingres_num_fields() */
PHP_FUNCTION(ingres_field_scale)
{
	php_ii_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, II_FIELD_INFO_SCALE);
}
/* }}} */

/* {{{static short php_ii_convert_data ( II_LONG destType, int destSize, int precision, II_LINK *ii_link, IIAPI_DATAVALUE *columnData, IIAPI_GETCOLPARM getColParm TSRMLS_DC) */
/* Convert complex Ingres data types to php-usable ones */
static II_LONG php_ii_convert_data ( II_LONG destType, int destSize, int precision, II_LINK *ii_link, IIAPI_DATAVALUE *columnData, IIAPI_GETCOLPARM getColParm, int field, int column TSRMLS_DC )
{
#if defined (IIAPI_VERSION_2)
	IIAPI_FORMATPARM formatParm;

	formatParm.fd_envHandle = IIG(envHandle);

	formatParm.fd_srcDesc.ds_dataType = (ii_link->descriptor[field+column]).ds_dataType;
	formatParm.fd_srcDesc.ds_nullable = (ii_link->descriptor[field+column]).ds_nullable;
	formatParm.fd_srcDesc.ds_length = (ii_link->descriptor[field+column]).ds_length;
	formatParm.fd_srcDesc.ds_precision = (ii_link->descriptor[field+column]).ds_precision;
	formatParm.fd_srcDesc.ds_scale = (ii_link->descriptor[field+column]).ds_scale;
	formatParm.fd_srcDesc.ds_columnType = (ii_link->descriptor[field+column]).ds_columnType;
	formatParm.fd_srcDesc.ds_columnName = (ii_link->descriptor[field+column]).ds_columnName;
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
	formatParm.fd_dstValue.dv_value = emalloc(formatParm.fd_dstDesc.ds_length+1);

	IIapi_formatData(&formatParm);

	if (formatParm.fd_status != IIAPI_ST_SUCCESS ) {
	  return formatParm.fd_status;
	}

	columnData[column].dv_length = formatParm.fd_dstValue.dv_length;
	columnData[column].dv_value = formatParm.fd_dstValue.dv_value;
	efree(formatParm.fd_srcValue.dv_value);

#else
	IIAPI_CONVERTPARM convertParm;
	
	convertParm.cv_srcDesc.ds_dataType = (ii_link->descriptor[field+column]).ds_dataType;
	convertParm.cv_srcDesc.ds_nullable = (ii_link->descriptor[field+column]).ds_nullable;
	convertParm.cv_srcDesc.ds_length = (ii_link->descriptor[field+column]).ds_length;
	convertParm.cv_srcDesc.ds_precision = (ii_link->descriptor[field+column]).ds_precision;
	convertParm.cv_srcDesc.ds_scale = (ii_link->descriptor[field+column]).ds_scale;
	convertParm.cv_srcDesc.ds_columnType = (ii_link->descriptor[field+column]).ds_columnType;
	convertParm.cv_srcDesc.ds_columnName = (ii_link->descriptor[field+column]).ds_columnName;
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
	convertParm.cv_dstValue.dv_value = emalloc(convertParm.cv_dstDesc.ds_length+1);

	IIapi_convertData(&convertParm);

	if(convertParm.fd_status != IIAPI_ST_SUCCESS ) {
	  return convertParm.cv_status;
	}

	columnData[column].dv_length = convertParm.cv_dstValue.dv_length;
	columnData[column].dv_value = convertParm.cv_dstValue.dv_value;
	efree(convertParm.cv_srcValue.dv_value);
#endif

	return II_OK;
}
/* }}} */

/* {{{ static void php_ii_fetch(INTERNAL_FUNCTION_PARAMETERS, II_LINK *ii_link, int result_type) */
/* Fetch a row of result */
static void php_ii_fetch(INTERNAL_FUNCTION_PARAMETERS, II_LINK *ii_link, int result_type)
{
	IIAPI_GETCOLPARM getColParm;
	IIAPI_DATAVALUE *columnData;
	IIAPI_GETQINFOPARM getQInfoParm;

	int i, j, k;
	double value_double = 0;
	long value_long = 0;
	char *value_char_p;
	int len, should_copy, correct_length;
	int lob_len ;
	short int lob_segment_len, found_lob;
	char *lob_segment, *lob_ptr, *lob_data;

	/* array initialization */
	array_init(return_value);

	if ( ii_link->procname != NULL ) /* look to see if there is a return value*/
	{
		if ( ii_link->fieldCount == 0 ) 
		/* if > 0 then we are looking at a row producing procedure and don't want to call IIapi_getQueryInfo
		 * to nullify any results */
		{
			getQInfoParm.gq_genParm.gp_callback = NULL;
			getQInfoParm.gq_genParm.gp_closure = NULL;
			getQInfoParm.gq_stmtHandle = ii_link->stmtHandle;

			IIapi_getQueryInfo( &getQInfoParm );
			ii_sync(&(getQInfoParm.gq_genParm));

			if (ii_success(&(getQInfoParm.gq_genParm), ii_link TSRMLS_CC) == II_FAIL)
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

	/* going through all fields */
	for (i = 0; i < ii_link->fieldCount;)
	{
		j = 0;
		k = 0;
		found_lob = 0;

		/* as long as there are no long byte or long varchar fields, Ingres is able to fetch 
		   many fields at a time, so try to find these types and stop if they're found. variable 
		   j will get number of fields to fetch */
		while (	(i + j) < ii_link->fieldCount ) 
		{
			if ((ii_link->descriptor[i+j]).ds_dataType != IIAPI_LBYTE_TYPE &&
				(ii_link->descriptor[i+j]).ds_dataType != IIAPI_LVCH_TYPE &&
				(ii_link->descriptor[i+j]).ds_dataType != IIAPI_LNVCH_TYPE )
			{
				j++;
			}
			else
			{
				/* break out of loop */
				/* a lob needs to be processed separately */
				found_lob = 1;
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
				columnData[k].dv_value = (II_PTR) emalloc((ii_link->descriptor[i + k]).ds_length);
			}

			getColParm.gc_genParm.gp_callback = NULL;
			getColParm.gc_genParm.gp_closure = NULL;
			getColParm.gc_rowCount = 1;
			getColParm.gc_columnCount = j;
			getColParm.gc_columnData = columnData;
			getColParm.gc_stmtHandle = ii_link->stmtHandle;
			getColParm.gc_moreSegments = 0;

			IIapi_getColumns(&getColParm);
			ii_sync(&(getColParm.gc_genParm));

			if (ii_success(&(getColParm.gc_genParm), ii_link TSRMLS_CC) != II_OK)
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
				if (columnData[k].dv_null)
					{	/* NULL value ? */

					if (result_type & II_NUM)
					{
						add_index_null(return_value, i + k + IIG(array_index_start));
					}
					if (result_type & II_ASSOC)
					{
						add_assoc_null(return_value, php_ii_field_name(ii_link, i + k + IIG(array_index_start) TSRMLS_CC));
					}

				} else {	/* non NULL value */
					correct_length = 0;

					switch ((ii_link->descriptor[i + k]).ds_dataType)
					{

						case IIAPI_DEC_TYPE:	/* decimal (fixed point number) */
						case IIAPI_MNY_TYPE:	/* money */
							/* convert to floating point number */
							php_ii_convert_data ( IIAPI_FLT_TYPE, sizeof(II_FLOAT8), 53, ii_link, columnData, getColParm, i, k TSRMLS_CC );
							/* NO break */

						case IIAPI_FLT_TYPE:	/* floating point number */
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
								add_index_double(return_value, i + k + IIG(array_index_start), value_double);
							}

							if (result_type & II_ASSOC)
							{
								add_assoc_double(return_value, php_ii_field_name(ii_link, i + k + IIG(array_index_start) TSRMLS_CC), value_double);

							}
							break;

						case IIAPI_INT_TYPE:	/* integer */
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
#ifdef	IIAPI_VERSION_4
								case 8:
									value_long = (long) *((II_INT4 *) columnData[k].dv_value);
									break;
#endif
								default:
									php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid size for IIAPI_INT_TYPE data (%d)", columnData[k].dv_length);
									break;
							}

							if (result_type & II_NUM)
							{
								add_index_long(return_value, i + k + IIG(array_index_start), value_long);
							}

							if (result_type & II_ASSOC)
							{
								add_assoc_long(return_value, php_ii_field_name(ii_link, i + k + IIG(array_index_start) TSRMLS_CC), value_long);
							}
							break;

						case IIAPI_TXT_TYPE:	/* variable length character string */
						case IIAPI_VBYTE_TYPE:	/* variable length binary string */
#ifdef IIAPI_VERSION_3
						case IIAPI_NVCH_TYPE:	/* variable length unicode character string */
							/* Convert it to IIAPI_VCH_TYPE */
							if ((ii_link->descriptor[i + k]).ds_dataType == IIAPI_NVCH_TYPE)
							{
								php_ii_convert_data ( IIAPI_CHA_TYPE, (columnData[k]).dv_length, 0, ii_link, columnData, getColParm, i, k TSRMLS_CC );
							}
							/* let the next 'case' handle the conversion to a format usable by php */
#endif								
						case IIAPI_VCH_TYPE:	/* variable length character string */
							/* real length is stored in first 2 bytes of data, so adjust
							   length variable and data pointer */
							columnData[k].dv_length = *((II_INT2 *) columnData[k].dv_value);
							columnData[k].dv_value = (II_CHAR *)(columnData[k]).dv_value + 2;
							correct_length = 1;
							/* NO break */

						case IIAPI_NCHA_TYPE:	/* fixed length unicode character string */	
							if ((ii_link->descriptor[i + k]).ds_dataType == IIAPI_NCHA_TYPE)
							{
								php_ii_convert_data ( IIAPI_CHA_TYPE, (columnData[k]).dv_length, 0, ii_link, columnData, getColParm, i, k TSRMLS_CC );
							}
						case IIAPI_BYTE_TYPE:	/* fixed length binary string */
						case IIAPI_CHA_TYPE:	/* fixed length character string */
						case IIAPI_CHR_TYPE:	/* fixed length character string */
						case IIAPI_LOGKEY_TYPE:	/* value unique to database */
						case IIAPI_TABKEY_TYPE:	/* value unique to table */
						case IIAPI_DTE_TYPE:	/* date */

							/* eventualy convert date to string */
							if ((ii_link->descriptor[i + k]).ds_dataType == IIAPI_DTE_TYPE)
							{
								php_ii_convert_data ( IIAPI_CHA_TYPE, 32, 0, ii_link, columnData, getColParm, i, k TSRMLS_CC );
							}

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

							if (result_type & II_NUM)
							{
								add_index_stringl(return_value, i + k + IIG(array_index_start), value_char_p, len, should_copy);
							}

							if (result_type & II_ASSOC)
							{
								add_assoc_stringl(return_value, php_ii_field_name(ii_link, i + k + IIG(array_index_start) TSRMLS_CC), value_char_p, len, should_copy);
							}

							/* eventualy restore data pointer state for
							   variable length data types */
							if (correct_length)
							{
								columnData[k].dv_value = (II_CHAR *)(columnData[k]).dv_value - 2;
							}
							break;
		
						default:
							php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid SQL data type in fetched field (%d -- length : %d)", (ii_link->descriptor[i + k]).ds_dataType, columnData[k].dv_length);
							break;
					}
				}
			}

			/* free the memory buffers */
			for (k = 0; k < j; k++)
			{
				efree(columnData[k].dv_value);
			}
			efree(columnData);
		}

		if (found_lob) 
		{

			/* alloc memory for the size of the segment we need */

			columnData = (IIAPI_DATAVALUE *) safe_emalloc(sizeof(IIAPI_DATAVALUE), 1, 0);
			lob_segment = (char *) emalloc (IIG(blob_segment_length));
			lob_len = 0;
		    do
			{
				getColParm.gc_genParm.gp_callback = NULL;
				getColParm.gc_genParm.gp_closure = NULL;
				getColParm.gc_rowCount = 1;
				getColParm.gc_columnCount = 1; /* just the lob */
				getColParm.gc_columnData = columnData;
				getColParm.gc_columnData[0].dv_value = lob_segment;
				getColParm.gc_stmtHandle = ii_link->stmtHandle;

				IIapi_getColumns(&getColParm);
				ii_sync(&(getColParm.gc_genParm));

				if (ii_success(&(getColParm.gc_genParm), ii_link TSRMLS_CC) != II_OK)
				{
					efree(lob_segment);
					efree(columnData);
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured whilst fetching a BLOB");
					RETURN_FALSE;
				}

				if (columnData[k].dv_null)
				{
					break;
				}

				/* length of the segment fetched is in the first 2 bytes of
				   lob_segment */
				memcpy( (char *)&lob_segment_len, lob_segment, 2 );


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
				lob_data[lob_len] = '\0';
			
			} 
			while ( getColParm.gc_moreSegments );

			if (columnData[k].dv_null)
			{	/* NULL value ? */

				if (result_type & II_NUM)
				{
					add_index_null(return_value, i + k + IIG(array_index_start));
				}
				if (result_type & II_ASSOC)
				{
					add_assoc_null(return_value, php_ii_field_name(ii_link, i + k + IIG(array_index_start) TSRMLS_CC));
				}
			}
			else
			{
				if (result_type & II_NUM)
				{
					add_index_stringl(return_value, i + k + IIG(array_index_start), lob_data, lob_len, 1);
				}

				if (result_type & II_ASSOC)
				{
					add_assoc_stringl(return_value, php_ii_field_name(ii_link, i + k + IIG(array_index_start) TSRMLS_CC), lob_data, lob_len, 1);
				}
			}

			efree(lob_segment);
			efree(columnData);
			if (!columnData[k].dv_null)
			{
				efree(lob_data);
			}
			
		} 
	
			
		/* increase field pointer by number of fetched fields */
		/* include any LOB data fetched */
		i += j + found_lob;
	}
}
/* }}} */

/* {{{ proto array ingres_fetch_array([int result_type [, resource link]])
   Fetch a row of result into an array result_type can be 
   II_NUM for enumerated array, 
   II_ASSOC for associative array, or 
   II_BOTH (default) */
PHP_FUNCTION(ingres_fetch_array)
{
	zval **result_type, **link;
	int argc;
	II_LINK *ii_link;
	int link_id = -1; 

	argc = ZEND_NUM_ARGS();
	if (argc > 2 || zend_get_parameters_ex(argc, &result_type, &link) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}

	if (argc != 0)
	{
		convert_to_long_ex(result_type);
	}

	if (argc <= 1 )
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}

	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id , "Ingres Link", le_ii_link, le_ii_plink);

	php_ii_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_link, (argc == 0 ? II_BOTH : Z_LVAL_PP(result_type)));

}
/* }}} */

/* {{{ proto array ingres_fetch_row([resource link])
   Fetch a row of result into an enumerated array */
PHP_FUNCTION(ingres_fetch_row)
{
	zval **link;
	int argc;
	II_LINK *ii_link;
	int link_id = -1;

	argc = ZEND_NUM_ARGS();
	if (argc > 1 || zend_get_parameters_ex(argc, &link) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}

	if (argc == 0 )
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}

	}

	if (argc == 0 )
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}

	}
	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id , "Ingres Link", le_ii_link, le_ii_plink);

	php_ii_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_link, II_NUM);
}
/* }}} */

/* {{{ proto array ingres_fetch_object([int result_type [, resource link]])
   Fetch a row of result into an object result_type can be II_NUM for enumerated object, II_ASSOC for associative object, or II_BOTH (default) */
PHP_FUNCTION(ingres_fetch_object)
{

	zval **result_type, **link;
	int argc;
	II_LINK *ii_link;
	int link_id = -1;

	argc = ZEND_NUM_ARGS();
	if (argc > 2 || zend_get_parameters_ex(argc, &result_type, &link) == FAILURE) 
	{
		WRONG_PARAM_COUNT;
	}

	if ( argc == 1 )
	{
		convert_to_long_ex(result_type);
	}

	if (argc != 2)
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}
	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id , "Ingres Link", le_ii_link, le_ii_plink);

	php_ii_fetch(INTERNAL_FUNCTION_PARAM_PASSTHRU, ii_link, (argc == 0 ? II_BOTH : Z_LVAL_PP(result_type)));

	if (Z_TYPE_P(return_value) == IS_ARRAY)
	{
		convert_to_object(return_value);
	}
}
/* }}} */

/* {{{ proto bool ingres_rollback([resource link])
   Roll back a transaction */
PHP_FUNCTION(ingres_rollback)
{
	zval **link;
	int argc;
	int link_id = -1;
	II_LINK *ii_link;

	argc = ZEND_NUM_ARGS();
	if (argc > 1 || (argc && zend_get_parameters_ex(argc, &link) == FAILURE))
	{
		WRONG_PARAM_COUNT;
	}

	if (argc == 0)
	{
		link_id = IIG(default_link);
		if ( link_id == -1 )
		{
			RETURN_FALSE;
		}
	}
	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	if (_rollback_transaction(ii_link TSRMLS_CC))
	{
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ingres_commit([resource link])
   Commit a transaction */
PHP_FUNCTION(ingres_commit)
{
	zval **link;
	int argc;
	int link_id = -1;
	II_LINK *ii_link;
	IIAPI_COMMITPARM commitParm;

	argc = ZEND_NUM_ARGS();
	if (argc > 1 || (argc && zend_get_parameters_ex(argc, &link) == FAILURE))
	{
		WRONG_PARAM_COUNT;
	}

	if (argc == 0)
	{
		link_id = IIG(default_link);
		if (link_id == -1)
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}
	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	if (ii_link->stmtHandle && _close_statement(ii_link TSRMLS_CC))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close statement");
		RETURN_FALSE;
	}

	commitParm.cm_genParm.gp_callback = NULL;
	commitParm.cm_genParm.gp_closure = NULL;
	commitParm.cm_tranHandle = ii_link->tranHandle;

	IIapi_commit(&commitParm);
	ii_sync(&(commitParm.cm_genParm));

	if (ii_success(&(commitParm.cm_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		RETURN_FALSE;
	}

	ii_link->tranHandle = NULL;
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ingres_autocommit([resource link])
   Switch autocommit on or off */
PHP_FUNCTION(ingres_autocommit)
{
	zval **link;
	int argc;
	int link_id = -1;
	II_LINK *ii_link;
	IIAPI_AUTOPARM autoParm;

	argc = ZEND_NUM_ARGS();
	if (argc > 1 || (argc && zend_get_parameters_ex(argc, &link) == FAILURE))
	{
		WRONG_PARAM_COUNT;
	}

	if (argc == 0)
	{
		link_id = IIG(default_link);
		if (link_id == -1)
	{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occured getting the default link" );
			RETURN_FALSE;
		}

	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	autoParm.ac_genParm.gp_callback = NULL;
	autoParm.ac_genParm.gp_closure = NULL;
	autoParm.ac_connHandle = ii_link->connHandle;
	autoParm.ac_tranHandle = ii_link->tranHandle;

	IIapi_autocommit(&autoParm);
	ii_sync(&(autoParm.ac_genParm));

	if (ii_success(&(autoParm.ac_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		RETURN_FALSE;
	}

	ii_link->autocommit = (ii_link->autocommit ? 0 : 1);
	ii_link->tranHandle = autoParm.ac_tranHandle;
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto long ingres_errno([resource link])
   Gets the last ingres error code generated */
PHP_FUNCTION(ingres_errno)
{
	php_ii_error(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto string ingres_error ([resource link])
   Gets a meaningful error message for the last error generated  */
PHP_FUNCTION(ingres_error)
{
	php_ii_error(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto string ingres_errsqlstate([resource link])
   Gets the last SQLSTATE generated for an error */
PHP_FUNCTION(ingres_errsqlstate)
{
	php_ii_error(INTERNAL_FUNCTION_PARAM_PASSTHRU, 2);
}
/* }}} */

/* {{{ php_ii_error (INTERNAL_FUNCTION_PARAMETERS, int mode) */

static void php_ii_error(INTERNAL_FUNCTION_PARAMETERS, int mode)
{

	zval **link = NULL;
	int link_id = -1;
	int argc;	
	int len;	
	char *ptr;
	long return_code;
	II_LINK *ii_link;

	argc = ZEND_NUM_ARGS();

	switch  (argc)
	{
	    case 1:
			zend_get_parameters_ex(argc, &link);
			ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);
			break;
		case 0:
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}

	if (argc == 1)
	{
		switch (mode)
		{
			case 0:
				return_code = ii_link->errorCode;
				break;
			case 1:
				len = strlen(ii_link->errorText);
				ptr = ecalloc(len + 1,1);
				memcpy (ptr,ii_link->errorText,len+1);
				break;
			case 2:
				len = sizeof(ii_link->sqlstate);
				ptr = ecalloc(len + 1,1);
				memcpy (ptr,ii_link->sqlstate,len+1);
				break;
			default:
				break;
		}
		RETVAL_STRING(ptr, 0);
	} 
	else 
	{
		switch (mode)
		{
			case 0:
				return_code = IIG(errorCode);
				break;
			case 1:
				if ( IIG(errorText) != NULL ) 
				{
					len = strlen(IIG(errorText));
					ptr = ecalloc(len + 1, 1);
					memcpy (ptr, IIG(errorText), len + 1 );
				}
				else
				{
					ptr = ecalloc(1,1);
					sprintf (ptr, "\0");
				}
				break;
			case 2:
				len = sizeof(IIG(sqlstate));
				ptr = ecalloc(len + 1, 1);
				memcpy (ptr, IIG(sqlstate), len + 1 );
				break;
			default:
				break;
		}

	}

	switch (mode)
	{
		case 0:
			RETVAL_LONG(return_code);
			break;
		case 1:
		case 2:
			RETVAL_STRING(ptr, 0);
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

/* {{{ static short int php_ii_set_environment_options(zval **options, II_LINK *ii_link) */
/*     Sets up options provided to ingres_connect() via a parameter array */
static short int php_ii_set_environment_options (zval **options, II_LINK *ii_link TSRMLS_DC)
{

#ifdef IIAPI_VERSION_2
	II_LONG parameter_id;
	IIAPI_SETENVPRMPARM	setEnvPrmParm;
	zval **data;
	char *key;
	long index;
	int i;
	int num_options;
	char *temp_string;
	long temp_long;
	II_BOOL ignore;

    num_options = zend_hash_num_elements(Z_ARRVAL_PP(options));
	zend_hash_internal_pointer_reset(Z_ARRVAL_PP(options));

	setEnvPrmParm.se_envHandle = ii_link->envHandle;

	for ( i = 0; i < num_options; i++ )
	{
		ignore = FALSE;

		if (zend_hash_get_current_key(Z_ARRVAL_PP(options), &key, &index, 0) == HASH_KEY_IS_STRING)
	   	{
			zend_hash_get_current_data(Z_ARRVAL_PP(options), (void**)&data);

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
			else if ( strcmp("local_login", key) == 0 )
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
					IIG(blob_segment_length) = Z_LVAL_PP(data);
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

		if (ignore != TRUE) {

			setEnvPrmParm.se_paramID = parameter_id;

			switch (Z_TYPE_PP(data))
			{
				case IS_STRING:
					convert_to_string_ex(data);
					temp_string = Z_STRVAL_PP(data);
					setEnvPrmParm.se_paramValue = temp_string;
					break;
				case IS_LONG:
					convert_to_long_ex(data);
					temp_long = Z_LVAL_PP(data);
					setEnvPrmParm.se_paramValue = (II_PTR)&temp_long;
					break;
				case IS_BOOL:
					convert_to_long_ex(data);
					temp_long = Z_LVAL_PP(data);
					setEnvPrmParm.se_paramValue = (II_PTR)&temp_long;
					break;
				default:
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown option type, %l, in connection options", Z_TYPE_PP(data));
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

		zend_hash_move_forward(Z_ARRVAL_PP(options));

	}

	return II_OK;
#else
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Setting environment options requires Ingres II 2.5 or newer", key, Z_LVAL_PP(data));
	return II_FAIL;
#endif

}
/* }}} */

/* {{{ static short int php_ii_set_connect_options(zval **options, II_LINK *ii_link, char *database TSRMLS_DC) */
/*     Sets up options provided to ingres_connect() via a parameter array */
static short int php_ii_set_connect_options(zval **options, II_LINK *ii_link, char *database TSRMLS_DC)
{
	II_LONG parameter_id;
	IIAPI_SETCONPRMPARM	setConPrmParm;
	IIAPI_CONNPARM connParm;
	IIAPI_DISCONNPARM	disconnParm;
	zval **data;
	char *key;
	long index;
	int i;
	int num_options;
	char *temp_string;
	long temp_long;
	II_BOOL ignore;

    num_options = zend_hash_num_elements(Z_ARRVAL_PP(options));
	zend_hash_internal_pointer_reset(Z_ARRVAL_PP(options));

	connParm.co_genParm.gp_callback = NULL;
	connParm.co_genParm.gp_closure = NULL;
	connParm.co_target = database;
#ifdef IIAPI_VERSION_2
	/* Use the environment handle in ii_link->connHandle */
	connParm.co_connHandle = ii_link->envHandle;
#else
	connParm.co_connHandle = NULL;
#endif
	connParm.co_tranHandle = NULL;
	connParm.co_type = IIAPI_CT_SQL; 
	connParm.co_username = NULL;
	connParm.co_password = NULL;
	connParm.co_timeout = -1;

	IIapi_connect( &connParm );

	if (!ii_sync(&(connParm.co_genParm)) || ii_success(&(connParm.co_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to connect to database (%s), to setup options", database);
		return II_FAIL;
	}

	ii_link->connHandle = connParm.co_connHandle;

	disconnParm.dc_genParm.gp_callback = NULL;
	disconnParm.dc_genParm.gp_closure = NULL;
	disconnParm.dc_connHandle = ii_link->connHandle;

	IIapi_disconnect( &disconnParm );

	if (!ii_sync(&(disconnParm.dc_genParm)) || ii_success(&(disconnParm.dc_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to disconnect during setup of options");
		return II_FAIL;
	}

	ii_link->connHandle = NULL;

	for ( i = 0; i < num_options; i++ )
	{
		ignore = FALSE;

		if (zend_hash_get_current_key(Z_ARRVAL_PP(options), &key, &index, 0) == HASH_KEY_IS_STRING)
	   	{
			zend_hash_get_current_data(Z_ARRVAL_PP(options), (void**)&data);

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
			else if ( strcmp("local_login", key) == 0 )
			{
				parameter_id = IIAPI_CP_SECONDARY_INX;
			}
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
#ifdef IIAPI_VERSION_2
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
					convert_to_long_ex(data);
					temp_long = Z_LVAL_PP(data);
					setConPrmParm.sc_paramValue = (II_PTR)&temp_long;
					break;
				case IS_BOOL:
					convert_to_long_ex(data);
					temp_long = Z_LVAL_PP(data);
					setConPrmParm.sc_paramValue = (II_PTR)&temp_long;
					break;
				default:
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown option type, %l, in connection options", Z_TYPE_PP(data));
					return II_FAIL;
			}

			IIapi_setConnectParam( &setConPrmParm );

			if (!ii_sync(&(setConPrmParm.sc_genParm)) || ii_success(&(setConPrmParm.sc_genParm), ii_link TSRMLS_CC) == II_FAIL)
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

		zend_hash_move_forward(Z_ARRVAL_PP(options));

	}
	return II_OK;
}
/* }}} */

/* {{{ static void php_ii_convert_param_markers (char *statement TSRMLS_DC) */
/* takes a statement with ? param markers and converts them to ~V */
static char *php_ii_convert_param_markers (char *statement TSRMLS_DC)
{
	char *tmp_statement;
	char ch, tmp_ch;
	char *p, *tmp_p;
	long parameter_count;
	int j;


    /* work out how many param markers there are */
	/* used to know how much to memory to allocate */
	parameter_count = php_ii_paramcount (statement TSRMLS_CC);

	tmp_statement = emalloc ( strlen(statement) + (parameter_count*3) + 1); /* allow for space either side and a null*/
	sprintf(tmp_statement,"\0");

    j = 0;

	p = statement;
	tmp_p = tmp_statement;

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

	sprintf (statement,"%s", tmp_statement);
	efree(tmp_statement);
	return statement;
}
/* }}} */

/* {{{ proto string ingres_set_environment(resource link, array options)
   Gets the last SQLSTATE generated for an error */
PHP_FUNCTION(ingres_set_environment)
{

	zval **link, **options;
	int argc;
	II_LINK *ii_link;
	int link_id = -1;

	argc = ZEND_NUM_ARGS();
	if (argc > 2 || zend_get_parameters_ex(argc, &link, &options) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id , "Ingres Link", le_ii_link, le_ii_plink);

	if ( php_ii_set_environment_options(options, ii_link TSRMLS_CC) == II_FAIL )
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to set environment options provided");
		RETURN_FALSE;
	}	

}
/* }}} */

/* {{{ static short php_ii_bind_params (II_LINK *ii_link, zval **queryParams) */
/* Binds and sends data for parameters passed via queryParams */
static short php_ii_bind_params (INTERNAL_FUNCTION_PARAMETERS, II_LINK *ii_link, zval **queryParams, zval **paramtypes)
{

	zval **val;
	IIAPI_SETDESCRPARM	setDescrParm;
    IIAPI_PUTPARMPARM	putParmParm;
    IIAPI_DESCRIPTOR	*descriptorInfo;
    IIAPI_DATAVALUE		*columnData;
	int param;
	II_INT2				columnType;
    HashTable *arr_hash;
    HashPosition pointer;
	
	
	double tmp_double;
	long tmp_long;
	char *tmp_string;
	char *key;
	int key_len;
	long index;
	short with_procedure = 0;
	char *types;

    if ( ii_link->paramCount > 0 )
	{
		arr_hash = Z_ARRVAL_PP(queryParams);
		zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
	}

	/* if we are sending params then we need to describe them into to Ingres */
	/* if no parameters have been provided to a procedure call there is always 1 */
	/* parameter, the procedure name */

	setDescrParm.sd_genParm.gp_callback = NULL;
	setDescrParm.sd_genParm.gp_closure = NULL;
	setDescrParm.sd_stmtHandle = ii_link->stmtHandle;

	if ( ii_link->procname != NULL )
	{
		/* bump descriptorCount to allow for procedure name */
		setDescrParm.sd_descriptorCount = ii_link->paramCount + 1;
		with_procedure = 1;
	}
	else
	{
		setDescrParm.sd_descriptorCount = ii_link->paramCount;
	}

	descriptorInfo = (IIAPI_DESCRIPTOR *) safe_emalloc(sizeof(IIAPI_DESCRIPTOR),setDescrParm.sd_descriptorCount, 0);
	setDescrParm.sd_descriptor = descriptorInfo;

	if ( ii_link->procname == NULL )
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
		types = emalloc(Z_STRLEN_PP(paramtypes));
		memcpy(types,Z_STRVAL_PP(paramtypes),Z_STRLEN_PP(paramtypes));
	}

	for ( param=0; param < setDescrParm.sd_descriptorCount; param++ )
	{

		if (( ii_link->procname != NULL)  && (param == 0) ) 
		{ 
			/* setup the first parameter as the procedure name */
			setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_CHA_TYPE;
			setDescrParm.sd_descriptor[param].ds_length = strlen(ii_link->procname);
			setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
			setDescrParm.sd_descriptor[param].ds_precision = 0;
			setDescrParm.sd_descriptor[param].ds_scale = 0;
			setDescrParm.sd_descriptor[param].ds_columnType = IIAPI_COL_SVCPARM;
			setDescrParm.sd_descriptor[param].ds_columnName = NULL;

		} 
		else 
		{

			if (zend_hash_get_current_data_ex(arr_hash, (void **) &val, &pointer) == FAILURE)
			{
				efree(descriptorInfo);
				return II_FAIL;
			}

			if ((ii_link->procname != NULL) && (zend_hash_get_current_key_ex(arr_hash, &key, &key_len, &index, 0, &pointer) == FAILURE))
			{
				php_error_docref(NULL TSRMLS_CC, E_WARNING,"Error getting parameter key");
				efree(descriptorInfo);
				ii_link->errorCode = -1; /* PHP error */
				return II_FAIL;
			}

			if ( Z_TYPE_PP(paramtypes) == IS_STRING )
			{
				/* bind parameters using the types indicated */

				switch (types[param - with_procedure])
				{
					case 'B': /* long byte */
						break;
					case 'b': /* byte */
					case 'c': /* char */
					case 'd': /* date */
					case 't': /* text */
						convert_to_string_ex(val);
						setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_CHA_TYPE;
						setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
						setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val);
						setDescrParm.sd_descriptor[param].ds_precision = 0;
						setDescrParm.sd_descriptor[param].ds_scale = 0;
						setDescrParm.sd_descriptor[param].ds_columnType = columnType;
						if ( ii_link->procname == NULL )
						{
							setDescrParm.sd_descriptor[param].ds_columnName = NULL;
						}
						else
						{
							setDescrParm.sd_descriptor[param].ds_columnName = key;
						}
						break;
					case 'D': /* decimal */
						convert_to_double_ex(val);
						setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_DEC_TYPE;
						setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
						setDescrParm.sd_descriptor[param].ds_length = sizeof(Z_DVAL_PP(val));
						setDescrParm.sd_descriptor[param].ds_precision = 31;
						setDescrParm.sd_descriptor[param].ds_scale = 15;
						setDescrParm.sd_descriptor[param].ds_columnType = columnType;
						if ( ii_link->procname == NULL )
						{
							setDescrParm.sd_descriptor[param].ds_columnName = NULL;
						}
						else
						{
							setDescrParm.sd_descriptor[param].ds_columnName = key;
						}
						break;
					case 'f': /* float */
						convert_to_double_ex(val);
						setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_FLT_TYPE;
						setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
						setDescrParm.sd_descriptor[param].ds_length = sizeof(Z_DVAL_PP(val));
						setDescrParm.sd_descriptor[param].ds_precision = 31;
						setDescrParm.sd_descriptor[param].ds_scale = 15;
						setDescrParm.sd_descriptor[param].ds_columnType = columnType;
						if ( ii_link->procname == NULL )
						{
							setDescrParm.sd_descriptor[param].ds_columnName = NULL;
						}
						else
						{
							setDescrParm.sd_descriptor[param].ds_columnName = key;
						}
						break;
						convert_to_string_ex(val);
						setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_TXT_TYPE;
						setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
						setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val);
						setDescrParm.sd_descriptor[param].ds_precision = 0;
						setDescrParm.sd_descriptor[param].ds_scale = 0;
						setDescrParm.sd_descriptor[param].ds_columnType = columnType;
						if ( ii_link->procname == NULL )
						{
							setDescrParm.sd_descriptor[param].ds_columnName = NULL;
						}
						else
						{
							setDescrParm.sd_descriptor[param].ds_columnName = key;
						}
						break;
					case 'T': /* long text */
					case 'V': /* long varchar */
						break;
					case 'i': /* integer */
						convert_to_long_ex(val);
						setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_INT_TYPE;
						setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
						setDescrParm.sd_descriptor[param].ds_length = sizeof(Z_LVAL_PP(val));
						setDescrParm.sd_descriptor[param].ds_precision = 0;
						setDescrParm.sd_descriptor[param].ds_scale = 0;
						setDescrParm.sd_descriptor[param].ds_columnType = columnType;
						if ( ii_link->procname == NULL )
						{
							setDescrParm.sd_descriptor[param].ds_columnName = NULL;
						}
						else
						{
							setDescrParm.sd_descriptor[param].ds_columnName = key;
						}
						break;
					case 'n': /* nchar NFC/NFD UTF-16*/
						convert_to_string_ex(val);
						setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_NCHA_TYPE;
						setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
						setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val);
						setDescrParm.sd_descriptor[param].ds_precision = 0;
						setDescrParm.sd_descriptor[param].ds_scale = 0;
						setDescrParm.sd_descriptor[param].ds_columnType = columnType;
						if ( ii_link->procname == NULL )
						{
							setDescrParm.sd_descriptor[param].ds_columnName = NULL;
						}
						else
						{
							setDescrParm.sd_descriptor[param].ds_columnName = key;
						}
						break;
					case 'N': /* nvarchar NFC/NFD UTF-16*/ 
						convert_to_string_ex(val);
						setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_NVCH_TYPE;
						setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
						setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val) + 2;
						setDescrParm.sd_descriptor[param].ds_precision = 0;
						setDescrParm.sd_descriptor[param].ds_scale = 0;
						setDescrParm.sd_descriptor[param].ds_columnType = columnType;
						if ( ii_link->procname == NULL )
						{
							setDescrParm.sd_descriptor[param].ds_columnName = NULL;
						}
						else
						{
							setDescrParm.sd_descriptor[param].ds_columnName = key;
						}
						break;
					case 'v': /* varchar */ 
						convert_to_string_ex(val);
						setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_VCH_TYPE;
						setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
						setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val) + 2;
						setDescrParm.sd_descriptor[param].ds_precision = 0;
						setDescrParm.sd_descriptor[param].ds_scale = 0;
						setDescrParm.sd_descriptor[param].ds_columnType = columnType;
						if ( ii_link->procname == NULL )
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
						if ( ii_link->procname == NULL )
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
						if ( ii_link->procname == NULL )
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
						setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_CHA_TYPE;
						setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
						setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val);
						setDescrParm.sd_descriptor[param].ds_precision = 0;
						setDescrParm.sd_descriptor[param].ds_scale = 0;
						setDescrParm.sd_descriptor[param].ds_columnType = columnType;
						if ( ii_link->procname == NULL )
						{
							setDescrParm.sd_descriptor[param].ds_columnName = NULL;
						}
						else
						{
							setDescrParm.sd_descriptor[param].ds_columnName = key;
						}
						break;
					case IS_NULL:
						convert_to_string_ex(val);
						setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_CHA_TYPE;
						setDescrParm.sd_descriptor[param].ds_nullable = TRUE;
						setDescrParm.sd_descriptor[param].ds_length = Z_STRLEN_PP(val);
						setDescrParm.sd_descriptor[param].ds_precision = 0;
						setDescrParm.sd_descriptor[param].ds_scale = 0;
						setDescrParm.sd_descriptor[param].ds_columnType = columnType;
						if ( ii_link->procname == NULL )
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
						if ( ii_link->procname == NULL )
						efree(descriptorInfo);
						return II_FAIL;
				}
			}

			if (((ii_link->procname != NULL) && (param > 0)) || (ii_link->procname == NULL))
			{
				zend_hash_move_forward_ex(arr_hash, &pointer);
			}
			
		}

	} /* param=0; param < setDescrParm.sd_descriptorCount; param++ */

	if (((ii_link->procname == NULL) && (ii_link->paramCount > 0)) || ((ii_link->procname != NULL) && (setDescrParm.sd_descriptorCount > 1)))
	{
		zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
	}

	IIapi_setDescriptor( &setDescrParm );

	if (ii_success(&(setDescrParm.sd_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		efree(descriptorInfo);
		return II_FAIL;
	}

	/*  Put query parameter values.  */
	putParmParm.pp_genParm.gp_callback = NULL;
	putParmParm.pp_genParm.gp_closure = NULL;
	putParmParm.pp_stmtHandle = ii_link->stmtHandle;
	putParmParm.pp_parmCount = setDescrParm.sd_descriptorCount;

	columnData = (IIAPI_DATAVALUE *) safe_emalloc(sizeof(IIAPI_DATAVALUE),setDescrParm.sd_descriptorCount, 0);
	putParmParm.pp_parmData = columnData;

	for ( param = 0 ; param < putParmParm.pp_parmCount ; param++)
	{
		if ((ii_link->procname != NULL) && (param == 0))
		{
			/* place the procedure name as the first parameter */
			putParmParm.pp_parmData[param].dv_null = FALSE;
			putParmParm.pp_parmData[param].dv_length = strlen(ii_link->procname);
			putParmParm.pp_parmData[param].dv_value = ii_link->procname;
		}
		else
		{

			if (zend_hash_get_current_data_ex(arr_hash, (void **) &val, &pointer) == FAILURE)
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
					putParmParm.pp_parmData[param].dv_null = FALSE;
					putParmParm.pp_parmData[param].dv_length = sizeof(Z_LVAL_PP(val));
					tmp_long = Z_LVAL_PP(val);
					putParmParm.pp_parmData[param].dv_value = &tmp_long;
					break;
				case IS_DOUBLE:
					convert_to_double_ex(val);
					putParmParm.pp_parmData[param].dv_null = FALSE;
					putParmParm.pp_parmData[param].dv_length = sizeof(Z_DVAL_PP(val)); 
					tmp_double = Z_DVAL_PP(val);
					putParmParm.pp_parmData[param].dv_value = &tmp_double;
					break;
				case IS_STRING:
					convert_to_string_ex(val);
					if ( paramtypes != NULL ) 
					{
						switch (types[param - with_procedure] )
						{
							case 'N': /* NVARCHAR */
								/* copy the data to a new buffer then set the size  */
								/* of the string at the begining of the buffer */
								tmp_string = emalloc(Z_STRLEN_PP(val) + 2);
								memcpy(tmp_string + 2, Z_STRVAL_PP(val), Z_STRLEN_PP(val));
								/* set the 1st 2 bytes as the length of the string */
								*((II_INT2*)(tmp_string)) = Z_STRLEN_PP(val)/2 ; 
								putParmParm.pp_parmData[param].dv_value = tmp_string;
								putParmParm.pp_parmData[param].dv_length = Z_STRLEN_PP(val) + 2; 
								break;
							case 'v': /* VARCHAR */
								/* copy the data to a new buffer then set the size  */
								/* of the string at the begining of the buffer */
								tmp_string = emalloc(Z_STRLEN_PP(val) + 2);
								memcpy(tmp_string + 2, Z_STRVAL_PP(val), Z_STRLEN_PP(val));
								/* set the 1st 2 bytes as the length of the string */
								*((II_INT2*)(tmp_string)) = Z_STRLEN_PP(val) ; 
								putParmParm.pp_parmData[param].dv_value = tmp_string;
								putParmParm.pp_parmData[param].dv_length = Z_STRLEN_PP(val) + 2; 
								break;
							default: /* everything else */
								putParmParm.pp_parmData[param].dv_value = Z_STRVAL_PP(val);
								putParmParm.pp_parmData[param].dv_length = Z_STRLEN_PP(val); 
								break;
						}
					}
					else
					{
						putParmParm.pp_parmData[param].dv_value = Z_STRVAL_PP(val);
					}
					putParmParm.pp_parmData[param].dv_null = FALSE;
					break;
				case IS_NULL: /* TODO need to check this */
					putParmParm.pp_parmData[param].dv_null = TRUE;
					putParmParm.pp_parmData[param].dv_length = Z_STRLEN_PP(val); 
					putParmParm.pp_parmData[param].dv_value = Z_STRVAL_PP(val);
					break;
				default:
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error putting a parameter of unknown type" );
					return II_FAIL;
					break;
			}
		}

		if (((ii_link->procname != NULL) && (param > 0)) || (ii_link->procname == NULL))
		{
			zend_hash_move_forward_ex(arr_hash, &pointer);
		}

	} /* param = 0 ; param < ii_link->paramCount ; param++ */

	putParmParm.pp_moreSegments = 0;

	IIapi_putParms( &putParmParm );
	ii_sync(&(putParmParm.pp_genParm));

	if (ii_success(&(putParmParm.pp_genParm), ii_link TSRMLS_CC) == II_FAIL)
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

	return II_OK;
}
/* }}} */

#endif /* HAVE_II */

/*
 * Local variables:
 * tab-width: 4
 * shift-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker ff=unix
 * vim<600: sw=4 ts=4
 */
