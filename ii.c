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
	NO_VERSION_YET,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_INGRES
ZEND_GET_MODULE(ingres)
#endif

/* {{{ php.ini entries */
PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN("ingres.allow_persistent", "1", PHP_INI_SYSTEM,	OnUpdateLong, allow_persistent, zend_ii_globals, ii_globals)
	STD_PHP_INI_ENTRY_EX("ingres.max_persistent", "-1", PHP_INI_SYSTEM, OnUpdateLong, max_persistent, zend_ii_globals, ii_globals, display_link_numbers)
	STD_PHP_INI_ENTRY_EX("ingres.max_links", "-1", PHP_INI_SYSTEM, OnUpdateLong, max_links, zend_ii_globals, ii_globals, display_link_numbers)
	STD_PHP_INI_ENTRY("ingres.default_database", NULL, PHP_INI_ALL, OnUpdateString, default_database, zend_ii_globals, ii_globals)
	STD_PHP_INI_ENTRY("ingres.default_user", NULL, PHP_INI_ALL, OnUpdateString, default_user, zend_ii_globals, ii_globals)
	STD_PHP_INI_ENTRY("ingres.default_password", NULL, PHP_INI_ALL, OnUpdateString, default_password, zend_ii_globals, ii_globals)
	STD_PHP_INI_BOOLEAN("ingres.report_db_warnings","0", PHP_INI_ALL, OnUpdateBool, report_db_warnings, zend_ii_globals, ii_globals)
	STD_PHP_INI_ENTRY("ingres.cursor_mode", "0", PHP_INI_ALL, OnUpdateLong, cursor_mode, zend_ii_globals, ii_globals)
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
	}
	if ( ii_link->descriptor != NULL )
	{
		free(ii_link->descriptor);
	}
	if ( ii_link->cursor_id != NULL )
	{
		free(ii_link->cursor_id);
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
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to close statement !!");
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
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to rollback transaction !!");
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
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to close statement !!");
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to disable autocommit");
		}

		ii_link->autocommit = 0;
		ii_link->tranHandle = NULL;
	}

	if (ii_link->tranHandle && _rollback_transaction(ii_link TSRMLS_CC))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to rollback transaction !!");
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
	ii_globals->num_persistent = 0;

}
/* }}} */

/* {{{ Module initialization
*/
PHP_MINIT_FUNCTION(ii)
{
	IIAPI_INITPARM initParm;

	ZEND_INIT_MODULE_GLOBALS(ii, php_ii_globals_init, NULL);
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
	REGISTER_LONG_CONSTANT("INGRES_DATE_MUTLINATIONAL4",II_DATE_MULTINATIONAL4,	CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_FINNISH",		II_DATE_FINNISH,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_ISO",			II_DATE_ISO,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_GERMAN",		II_DATE_GERMAN,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_MDY",			II_DATE_MDY,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_DATE_DMY",			II_DATE_DMY,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_MONEY_LEADING",		II_MONEY_LEAD_SIGN,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INGRES_MONEY_TRAILING",		II_MONEY_TRAIL_SIGN,	CONST_CS | CONST_PERSISTENT);

	/* Ingres api initialization */
	initParm.in_timeout = -1;				/* timeout in ms, -1 = no timeout */
	initParm.in_version = IIAPI_VERSION;	/* api version used */

	IIapi_initialize(&initParm);
	if (initParm.in_status == IIAPI_ST_SUCCESS)
	{
		return SUCCESS;
	} else {
		return FAILURE;
	}
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
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unexpected failure of IIapi_wait()");
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
	switch (genParm->gp_status)
	{
	
		case IIAPI_ST_SUCCESS:
			return II_OK;
			
		case IIAPI_ST_NO_DATA:
			return II_NO_DATA;

		default:
			if (genParm->gp_errorHandle == NULL)
			{	/* no error message available */
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Server or API error - no error message available");
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
	int option_count;
	IIAPI_CONNPARM connParm;
	II_LINK *ii_link;
	II_PTR	envHandle = (II_PTR)NULL;
	II_LOGIN *user_details;

	char *z_type;

	/* Setting db, user and pass according to sql_safe_mode, parameters and/or default values */
	argc = ZEND_NUM_ARGS();

	user_details = (II_LOGIN *)emalloc(sizeof(II_LOGIN));

	if (PG(sql_safe_mode))
	{	/* sql_safe_mode */

		if (argc > 0)
		{
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "SQL safe mode in effect - ignoring host/user/password information");
		}

		user_details->database = NULL;
		user_details->password = NULL;
		user_details->user = php_get_current_user();
		hashed_details_length = strlen(user_details->user) + sizeof("ingres___") - 1;
		hashed_details = (char *) emalloc(hashed_details_length + 1);
		sprintf(hashed_details, "Ingres__%s_", user_details->user);

	} else {					/* non-sql_safe_mode */
		user_details->database = IIG(default_database);
		user_details->user = IIG(default_user);
		user_details->password = IIG(default_password);

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

					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: expected an array of connect options but got %s instead.", z_type );
					efree(z_type);
					RETURN_FALSE;
				}
				/* Fall-through.  */
			case 3:
				convert_to_string_ex(password);
				user_details->password = Z_STRVAL_PP(password);
				/* Fall-through. */
		
			case 2:
				convert_to_string_ex(username);
				user_details->user = Z_STRVAL_PP(username);
				/* Fall-through. */
		
			case 1:
				convert_to_string_ex(database);
				user_details->database = Z_STRVAL_PP(database);
				/* Fall-through. */

			case 0:
				break;
		}
		
		/* Perform Sanity Check. If no database has been set then we have a problem */
        dblen = strlen(user_details->database);
		if ( dblen == 0 )
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: No default database available to connect to" );
			RETURN_FALSE;
		}

		hashed_details_length =	sizeof("ingres___") - 1 + 
								strlen(SAFE_STRING(user_details->database)) +
								strlen(SAFE_STRING(user_details->user)) + 
								strlen(SAFE_STRING(user_details->password));

		hashed_details = (char *) emalloc(hashed_details_length + 1);
		sprintf(hashed_details, "Ingres_%s_%s_%s", 
								SAFE_STRING(user_details->database),	
								SAFE_STRING(user_details->user), 
								SAFE_STRING(user_details->password));
	}

	/* if asked for unauthorized persistency, issue a warning
	   and go for a non-persistent link */
	if (persistent && !IIG(allow_persistent))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Persistent links disabled !");
		persistent = 0;
	}

	if (persistent)
	{
		list_entry *le;

		/* is this link already in the persistent list ? */
		if (zend_hash_find(&EG(persistent_list), hashed_details, hashed_details_length + 1, (void **) &le) == FAILURE)
		{ /* no, new persistent connection */
			list_entry new_le;

			if (IIG(max_links) != -1 && IIG(num_links) >= IIG(max_links))
			{
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Too many open links (%d)", IIG(num_links));
				efree(hashed_details);
				RETURN_FALSE;
			}
			if (IIG(max_persistent) != -1 && IIG(num_persistent) >= IIG(max_persistent))
			{
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Too many open persistent links (%d)", IIG(num_persistent));
				efree(hashed_details);
				RETURN_FALSE;
			}

			/* setup the link */
			ii_link = (II_LINK *) malloc(sizeof(II_LINK));
			ii_link->connHandle = NULL;
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
				if ( php_ii_set_connect_options(options, ii_link, user_details TSRMLS_CC) == II_FAIL )
				{
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: unable to set options provided", IIG(num_persistent));
					efree(user_details);
					RETURN_FALSE;
				}	
			}

			/* create the link */
			connParm.co_genParm.gp_callback = NULL;
			connParm.co_genParm.gp_closure = NULL;
			connParm.co_target = user_details->database;
			connParm.co_username = user_details->user;
			connParm.co_password = user_details->password;
			connParm.co_timeout = -1;	/* -1 is no timeout */
			if ( ii_link->connHandle != NULL )
			{ /* use the connection handle which has had options set */
				connParm.co_connHandle = ii_link->connHandle;
			}
			else
			{
				connParm.co_connHandle = NULL;
			}
			connParm.co_tranHandle = NULL;

			IIapi_connect(&connParm);

			if (!ii_sync(&(connParm.co_genParm)) || ii_success(&(connParm.co_genParm), ii_link TSRMLS_CC) == II_FAIL)
			{
				free(ii_link);
				efree(hashed_details);
				efree(user_details);
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to connect to database (%s)", user_details->database);
				RETURN_FALSE;
			}

			ii_link->connHandle = connParm.co_connHandle;
			
			/* hash it up */
			Z_TYPE(new_le) = le_ii_plink;
			new_le.ptr = ii_link;
			if (zend_hash_update(&EG(persistent_list), hashed_details, hashed_details_length + 1, (void *) &new_le, sizeof(list_entry), NULL) == FAILURE)
			{
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to hash (%s)", hashed_details);
				free(ii_link);
				efree(hashed_details);
				efree(user_details);
				RETURN_FALSE;
			}
			IIG(num_persistent)++;
			IIG(num_links)++;

		} else { /* already open persistent connection */

			if (Z_TYPE_P(le) != le_ii_plink)
			{
				efree(hashed_details);
				efree(user_details);
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
				php_error_docref(NULL TSRMLS_CC, E_WARNING,"Ingres:  Broken link (%s),reconnect", user_details->database);
				
				/* Recreate the link */
				connParm.co_genParm.gp_callback = NULL;
				connParm.co_genParm.gp_closure = NULL;
				connParm.co_target = user_details->database;
				connParm.co_username = user_details->user;
				connParm.co_password = user_details->password;
				connParm.co_timeout = -1; /* no timeout */
				connParm.co_connHandle = NULL;
				connParm.co_tranHandle = NULL;

				ii_link->connHandle = NULL;
				ii_link->tranHandle = NULL;
				ii_link->stmtHandle = NULL;
				ii_link->fieldCount = 0;
				ii_link->descriptor = NULL;
				ii_link->autocommit = 0;
				ii_link->errorCode = 0;
				ii_link->errorText = NULL;
				
				IIapi_connect(&connParm);

				if (!ii_sync(&(connParm.co_genParm)) || ii_success(&(connParm.co_genParm), ii_link TSRMLS_CC) == II_FAIL)
				{
					efree(hashed_details);
					php_error_docref(NULL TSRMLS_CC, E_WARNING,"Ingres:  Unable to connect to database (%s)", user_details->database);
					RETURN_FALSE;
				}

				ii_link->connHandle = connParm.co_connHandle;

			}
		}
		
		ZEND_REGISTER_RESOURCE(return_value, ii_link, le_ii_plink);

	} else { /* non persistent */
		list_entry *index_ptr, new_index_ptr;

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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Too many open links (%d)", IIG(num_links));
			efree(hashed_details);
			RETURN_FALSE;
		}

		/* setup the link */
		ii_link = (II_LINK *) malloc(sizeof(II_LINK));
		ii_link->connHandle = NULL;
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
			if ( php_ii_set_connect_options(options, ii_link, user_details TSRMLS_CC) == II_FAIL )
			{
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: unable to set options provided", IIG(num_persistent));
				efree(user_details);
				RETURN_FALSE;
			}	
		}

		/* create the link */
		connParm.co_genParm.gp_callback = NULL;
		connParm.co_genParm.gp_closure = NULL;
		connParm.co_target = user_details->database;
		connParm.co_username = user_details->user;
		connParm.co_password = user_details->password;
		connParm.co_timeout = -1;	/* -1 is no timeout */
		if ( ii_link->connHandle != NULL )
		{ /* use the connection handle which has had options set */
			connParm.co_connHandle = ii_link->connHandle;
		}
		else
		{
			connParm.co_connHandle = NULL;
		}
		connParm.co_tranHandle = NULL;

		IIapi_connect(&connParm);

		if (!ii_sync(&(connParm.co_genParm)) || ii_success(&(connParm.co_genParm), ii_link TSRMLS_CC) == II_FAIL)
		{
			efree(hashed_details);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to connect to database (%s)", user_details->database);
			RETURN_FALSE;
		}
		ii_link->connHandle = connParm.co_connHandle;		
		
		/* add it to the list */
		ZEND_REGISTER_RESOURCE(return_value, ii_link, le_ii_link);

		/* add it to the hash */
		new_index_ptr.ptr = (void *) Z_LVAL_P(return_value);
		Z_TYPE(new_index_ptr) = le_index_ptr;
		if (zend_hash_update(&EG(regular_list), hashed_details, hashed_details_length + 1, (void *) &new_index_ptr, sizeof(list_entry), NULL) == FAILURE)
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to hash (%s)", hashed_details);
			free(ii_link);
			efree(hashed_details);
			RETURN_FALSE;
		}
		IIG(num_links)++;
	}

	efree(hashed_details);
	php_ii_set_default_link(Z_LVAL_P(return_value) TSRMLS_CC);
} 
/* }}} */

/* {{{ proto resource ingres_connect([string database [, string username [, string password]]])
   Open a connection to an Ingres database the syntax of database is [node_id::]dbname[/svr_class] */
PHP_FUNCTION(ingres_connect)
{
	php_ii_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto resource ingres_pconnect([string database [, string username [, string password]]])
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
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
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

/* {{{ proto bool ingres_query(string query [, resource link] [, array queryParams] ) */
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
	zval **query, **link, **procParams;
	int argc;
	int link_id = -1;
	II_LINK *ii_link;
	IIAPI_QUERYPARM     queryParm;
	IIAPI_GETDESCRPARM  getDescrParm;
    IIAPI_SETDESCRPARM  setDescrParm;
    IIAPI_PUTPARMPARM   putParmParm;
    IIAPI_DESCRIPTOR	*DescrBuffer; /* 1 entry for the procedure name */
    IIAPI_DATAVALUE		*DataBuffer;
	IIAPI_GETQINFOPARM getQInfoParm;

	char *procname=NULL;

	argc = ZEND_NUM_ARGS();
	if (argc < 1 || argc > 3 || zend_get_parameters_ex(argc, &query, &link, &procParams) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}
	
	if ( argc == 1 )
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
			RETURN_FALSE;
		}
	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	/* if there's already an active statement, close it */
	if (ii_link->stmtHandle && _close_statement(ii_link TSRMLS_CC))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to close statement !!");
		RETURN_FALSE;
	}
	
	convert_to_string_ex(query);

	if ( ii_link->procname != NULL )
	{
		free(ii_link->procname);
		ii_link->procname = NULL;
	}
	/* check to see if this is a procedure or not
	   load the procedure name into ii_link->procname.
	   If ii_link->procname is NULL then there is no procedure */

    php_ii_check_procedure(Z_STRVAL_PP(query), ii_link TSRMLS_CC);
	
	queryParm.qy_genParm.gp_callback = NULL;
	queryParm.qy_genParm.gp_closure = NULL;
	queryParm.qy_connHandle = ii_link->connHandle;
	queryParm.qy_tranHandle = ii_link->tranHandle;
	queryParm.qy_stmtHandle = NULL;
	if ( ii_link->procname == NULL )
	{
		queryParm.qy_queryType  = IIAPI_QT_QUERY;
		queryParm.qy_parameters = FALSE;
	}
	else
	{
		queryParm.qy_queryType  = IIAPI_QT_EXEC_PROCEDURE;
		queryParm.qy_parameters = TRUE;
	}

	if ( ii_link->procname == NULL )
	{
		queryParm.qy_queryText  = Z_STRVAL_PP(query);
	}
	else /* qt_queryText is null for procedures */
	{
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

	/* if we are executing a procedure supply the procedure name and execute */

	if ( ii_link->procname != NULL )
	{
		DescrBuffer = (IIAPI_DESCRIPTOR *)ecalloc(sizeof(IIAPI_DESCRIPTOR),1);
		setDescrParm.sd_genParm.gp_callback = NULL;
		setDescrParm.sd_genParm.gp_closure = NULL;
		setDescrParm.sd_stmtHandle = ii_link->stmtHandle;
		setDescrParm.sd_descriptorCount = 1; /* no params just the name of the procedure */
		setDescrParm.sd_descriptor = DescrBuffer;

		setDescrParm.sd_descriptor[0].ds_dataType = IIAPI_CHA_TYPE;
	    setDescrParm.sd_descriptor[0].ds_length = strlen(ii_link->procname);
		setDescrParm.sd_descriptor[0].ds_nullable = FALSE;
		setDescrParm.sd_descriptor[0].ds_precision = 0;
		setDescrParm.sd_descriptor[0].ds_scale = 0;
		setDescrParm.sd_descriptor[0].ds_columnType = IIAPI_COL_SVCPARM;
		setDescrParm.sd_descriptor[0].ds_columnName = NULL;

		IIapi_setDescriptor( &setDescrParm );

		ii_sync(&(setDescrParm.sd_genParm));

		if (ii_success(&(setDescrParm.sd_genParm), ii_link TSRMLS_CC) == II_FAIL)
		{
			efree(DescrBuffer);
			RETURN_FALSE;
		} 

		/*  Send procedure parameters. */ 
		DataBuffer = (IIAPI_DATAVALUE *)ecalloc(sizeof(IIAPI_DATAVALUE),1);
		putParmParm.pp_genParm.gp_callback = NULL;
		putParmParm.pp_genParm.gp_closure = NULL;
		putParmParm.pp_stmtHandle = ii_link->stmtHandle;
		putParmParm.pp_parmCount = setDescrParm.sd_descriptorCount;
		putParmParm.pp_parmData =  DataBuffer;
		putParmParm.pp_moreSegments = 0;

		putParmParm.pp_parmData[0].dv_null = FALSE;
		putParmParm.pp_parmData[0].dv_length = strlen( ii_link->procname );
	    putParmParm.pp_parmData[0].dv_value = ii_link->procname;

		IIapi_putParms( &putParmParm );

		ii_sync(&(putParmParm.pp_genParm));

		if (ii_success(&(putParmParm.pp_genParm), ii_link TSRMLS_CC) == II_FAIL)
		{
			efree(DescrBuffer);
			efree(DataBuffer);
			RETURN_FALSE;
		} 

		efree(DescrBuffer);
		efree(DataBuffer);

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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
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
	ii_link->paramCount = php_ii_queryparse(statement TSRMLS_CC);

	/* if there's already an active statement, close it */
	if (ii_link->stmtHandle && _close_statement(ii_link TSRMLS_CC))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to close statement !!");
		RETURN_FALSE;
	}

	ii_link->procname = NULL;

	/* check to see if this is a procedure or not
	   load the procedure name into ii_link->procname.
	   If ii_link->procname is NULL then there is no procedure */

    php_ii_check_procedure(Z_STRVAL_PP(query), ii_link TSRMLS_CC);

	if ( ii_link->procname == NULL )
	{
		/* Adapt the query into a prepared statement */
		queryLen = strlen(statement);

		ii_link->cursor_id = malloc(33);
		php_ii_gen_cursor_id(ii_link TSRMLS_CC);
		cursor_id_len = strlen(ii_link->cursor_id);
		preparedStatement=ecalloc(queryLen + 15 + cursor_id_len, 1);
		sprintf (preparedStatement,"prepare %s from %s\0", ii_link->cursor_id, statement);
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

/* {{{ proto int ingres_execute([array params [, resource link ]]) 
   execute a query prepared by ingres_prepare() */
PHP_FUNCTION (ingres_execute)
{

	zval **link, **queryParams, **val;
	char *key;
	int keylen;
	long index;
	int argc;
	int link_id = -1;
	II_LINK *ii_link;
	IIAPI_QUERYPARM     queryParm;
	IIAPI_GETDESCRPARM  getDescrParm;
	IIAPI_SETDESCRPARM	setDescrParm;
    IIAPI_PUTPARMPARM	putParmParm;
    IIAPI_DESCRIPTOR	*descriptorInfo;
    IIAPI_DATAVALUE		*columnData;
	II_INT2				columnType;

	int paramCount, param;
	int elementCount;
	char *statement;

	argc = ZEND_NUM_ARGS();
	if (argc > 2 || zend_get_parameters_ex(argc, &queryParams, &link) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}

	if ( argc <= 1 )
	{		
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
			RETURN_FALSE;
		}
	}
		
	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	/* figure how many parameters are expected */
	paramCount = ii_link->paramCount;

    if ( paramCount > 0 )
	{
		if (Z_TYPE_PP(queryParams) != IS_ARRAY )
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: Expecting a parameter array but did not get one" );
			RETURN_FALSE;
		}

		if ((elementCount = zend_hash_num_elements(Z_ARRVAL_PP(queryParams))) != paramCount )
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: incorrect number of parameters passed, expected %d got %d",paramCount, elementCount );
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
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: incorrect number of parameters passed, expected %d got %d",paramCount, elementCount );
				efree(statement);
				RETURN_FALSE;
		}

		queryParm.qy_genParm.gp_callback = NULL;
		queryParm.qy_genParm.gp_closure = NULL;
		queryParm.qy_connHandle = ii_link->connHandle;
		queryParm.qy_tranHandle = ii_link->tranHandle;
		queryParm.qy_stmtHandle = NULL;
		queryParm.qy_queryType  = IIAPI_QT_OPEN; 

		if (paramCount >0 ) 
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
		paramCount++;

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

	if ( paramCount > 0 )
	{
	
		/* if we are sending params then we need to describe them into to Ingres */
		/* if no parameters have been provided to a procedure call there is always 1 */
		/* parameter, the procedure name */

		setDescrParm.sd_genParm.gp_callback = NULL;
		setDescrParm.sd_genParm.gp_closure = NULL;
		setDescrParm.sd_stmtHandle = ii_link->stmtHandle;
	
		setDescrParm.sd_descriptorCount = paramCount;

		descriptorInfo = (IIAPI_DESCRIPTOR *) safe_emalloc(setDescrParm.sd_descriptorCount, sizeof(IIAPI_DESCRIPTOR), 0);
		setDescrParm.sd_descriptor = descriptorInfo;
			

		for ( param=0; param < paramCount; param++ )
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

				if (zend_hash_get_current_data(Z_ARRVAL_PP(queryParams), (void **) &val) == FAILURE)
				{
					php_error_docref(NULL TSRMLS_CC, E_WARNING,"Ingres: Error getting parameter value");
					if ( ii_link->procname == NULL )
					{
						efree (statement);
					}
					efree(descriptorInfo);
					RETURN_FALSE;
				}

				if ((ii_link->procname != NULL) && (zend_hash_get_current_key(Z_ARRVAL_PP(queryParams), &key, &index, 0) == FAILURE))
				{
					php_error_docref(NULL TSRMLS_CC, E_WARNING,"Ingres: Error getting parameter key");
					efree(descriptorInfo);
					if ( ii_link->procname == NULL )
					{
						efree (statement);
					}
					RETURN_FALSE;
				}

				/* Process each parameter into our descriptor buffer */
				switch (Z_TYPE_PP(val))
				{
					case IS_LONG:
						/* does not handle int8 yet */
						convert_to_long_ex(val);
						setDescrParm.sd_descriptor[param].ds_dataType = IIAPI_INT_TYPE;
						setDescrParm.sd_descriptor[param].ds_nullable = FALSE;
						setDescrParm.sd_descriptor[param].ds_length = sizeof(val);
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
						setDescrParm.sd_descriptor[param].ds_length = sizeof(val);
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
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: a parameter has been passed of unknown type" );
						if ( ii_link->procname == NULL )
						{
							efree (statement);
						}
						efree(descriptorInfo);
						RETURN_FALSE;
				}

				if (((ii_link->procname != NULL) && (param > 0)) || (ii_link->procname == NULL))
				{
					zend_hash_move_forward(Z_ARRVAL_PP(queryParams));
				}
			}
					

		} /* param=0; param < paramCount; param++ */

		if (((ii_link->procname == NULL) && (paramCount > 0)) || ((ii_link->procname != NULL) && (paramCount > 1)))
		{
			zend_hash_internal_pointer_reset(Z_ARRVAL_PP(queryParams));
		}

		IIapi_setDescriptor( &setDescrParm );

		if (ii_success(&(setDescrParm.sd_genParm), ii_link TSRMLS_CC) == II_FAIL)
		{
			efree(descriptorInfo);
			RETURN_FALSE;
		}

		/*  Put query parameter values.  */
		putParmParm.pp_genParm.gp_callback = NULL;
		putParmParm.pp_genParm.gp_closure = NULL;
		putParmParm.pp_stmtHandle = ii_link->stmtHandle;
		putParmParm.pp_parmCount = setDescrParm.sd_descriptorCount;

		columnData = (IIAPI_DATAVALUE *) safe_emalloc(setDescrParm.sd_descriptorCount, sizeof(IIAPI_DATAVALUE), 0);
		putParmParm.pp_parmData = columnData;

		for ( param = 0 ; param < setDescrParm.sd_descriptorCount ; param++)
		{

			if ((ii_link->procname != NULL) && (param == 0))
			{
				putParmParm.pp_parmData[param].dv_null = FALSE;
				putParmParm.pp_parmData[param].dv_length = strlen(ii_link->procname);
				putParmParm.pp_parmData[param].dv_value = ii_link->procname;
			}
			else
			{

				if (zend_hash_get_current_data(Z_ARRVAL_PP(queryParams), (void **) &val) == FAILURE)
				{
					php_error_docref(NULL TSRMLS_CC, E_WARNING,"Ingres: Error getting parameter");
					efree(columnData);
					efree(descriptorInfo);
					if ( ii_link->procname == NULL )
					{
						efree (statement);
					}
					RETURN_FALSE;
				}

				switch (Z_TYPE_PP(val))
				{
					case IS_LONG:
						convert_to_long_ex(val);
						putParmParm.pp_parmData[param].dv_null = FALSE;
						putParmParm.pp_parmData[param].dv_length = sizeof(Z_LVAL_PP(val));
						putParmParm.pp_parmData[param].dv_value = val;
					case IS_DOUBLE:
						convert_to_double_ex(val);
						putParmParm.pp_parmData[param].dv_null = FALSE;
						putParmParm.pp_parmData[param].dv_length = sizeof(Z_DVAL_PP(val)); 
						putParmParm.pp_parmData[param].dv_value = val;
					case IS_STRING:
						convert_to_string_ex(val);
						putParmParm.pp_parmData[param].dv_null = FALSE;
						putParmParm.pp_parmData[param].dv_length = Z_STRLEN_PP(val); 
						putParmParm.pp_parmData[param].dv_value = Z_STRVAL_PP(val);
						break;
					case IS_NULL: /* need to check this */
						putParmParm.pp_parmData[param].dv_null = TRUE;
						putParmParm.pp_parmData[param].dv_length = Z_STRLEN_PP(val); 
						putParmParm.pp_parmData[param].dv_value = Z_STRVAL_PP(val);
					default:
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: error putting a parameter of unknown type" );
						RETURN_FALSE;
				}
			}

			if (((ii_link->procname != NULL) && (param > 0)) || (ii_link->procname == NULL))
			{
				zend_hash_move_forward(Z_ARRVAL_PP(queryParams));
			}

		} /* param = 0 ; param < paramCount ; param++ */

		putParmParm.pp_moreSegments = 0;

		IIapi_putParms( &putParmParm );
		ii_sync(&(putParmParm.pp_genParm));

		if (ii_success(&(putParmParm.pp_genParm), ii_link TSRMLS_CC) == II_FAIL)
		{
			efree(descriptorInfo);
			efree(columnData);
			if ( ii_link->procname == NULL )
			{
				efree (statement);
			}
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

	} /* if paramCount > 0 */

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
		if (paramCount > 0)
		{
			efree(columnData);
			efree(descriptorInfo);
		}
		if ( ii_link->procname == NULL )
		{
			efree (statement);
		}
		RETURN_FALSE;
	}

	/* store the results */
	ii_link->fieldCount = getDescrParm.gd_descriptorCount;
	ii_link->descriptor = getDescrParm.gd_descriptor;
	if (paramCount > 0)
	{
		efree(columnData);
		efree(descriptorInfo);
	}
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
   gets a cursor name for a given result resource */
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
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
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  %s() called with wrong index (%d)", fun_name, index);
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
#ifdef INGRES_UNICODE
				case IIAPI_NCHA_TYPE:
					RETURN_STRING("IIAPI_NCHA_TYPE", 1);

				case IIAPI_NVCH_TYPE:
					RETURN_STRING("IIAPI_NVCH_TYPE", 1);
#endif
		
				default:
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unknown Ingres data type");
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
	if (index < 1 || index > ii_link->fieldCount)
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  php_ii_field_name() called with wrong index (%d)", index);
		return NULL;
	}

	return (ii_link->descriptor[index - 1]).ds_columnName;
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

/* {{{ #define IIAPI_CONVERT(destType, destSize, precision) */
/* Convert complex Ingres data types to php-usable ones */
#define IIAPI_CONVERT(destType, destSize, precision) {\
      convertParm.cv_srcDesc.ds_dataType = (ii_link->descriptor[i+k-2]).ds_dataType;\
      convertParm.cv_srcDesc.ds_nullable = (ii_link->descriptor[i+k-2]).ds_nullable;\
      convertParm.cv_srcDesc.ds_length = (ii_link->descriptor[i+k-2]).ds_length;\
      convertParm.cv_srcDesc.ds_precision = (ii_link->descriptor[i+k-2]).ds_precision;\
      convertParm.cv_srcDesc.ds_scale = (ii_link->descriptor[i+k-2]).ds_scale;\
      convertParm.cv_srcDesc.ds_columnType = (ii_link->descriptor[i+k-2]).ds_columnType;\
      convertParm.cv_srcDesc.ds_columnName = (ii_link->descriptor[i+k-2]).ds_columnName;\
      convertParm.cv_srcValue.dv_null = columnData[k-1].dv_null;\
      convertParm.cv_srcValue.dv_length = columnData[k-1].dv_length;\
      convertParm.cv_srcValue.dv_value = columnData[k-1].dv_value;\
      convertParm.cv_dstDesc.ds_dataType = destType;\
      convertParm.cv_dstDesc.ds_nullable = FALSE;\
      convertParm.cv_dstDesc.ds_length = destSize;\
      convertParm.cv_dstDesc.ds_precision = precision;\
      convertParm.cv_dstDesc.ds_scale = 0;\
      convertParm.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;\
      convertParm.cv_dstDesc.ds_columnName = NULL;\
      convertParm.cv_dstValue.dv_null = FALSE;\
      convertParm.cv_dstValue.dv_length = convertParm.cv_dstDesc.ds_length;\
      convertParm.cv_dstValue.dv_value = emalloc(convertParm.cv_dstDesc.ds_length+1);\
\
      IIapi_convertData(&convertParm);\
\
      if(ii_success(&(getColParm.gc_genParm), ii_link TSRMLS_CC)!=II_OK) {\
          RETURN_FALSE;\
      }\
\
      columnData[k-1].dv_length = convertParm.cv_dstValue.dv_length;\
      columnData[k-1].dv_value = convertParm.cv_dstValue.dv_value;\
      efree(convertParm.cv_srcValue.dv_value);\
}
/* }}} */

/* {{{ static void php_ii_fetch(INTERNAL_FUNCTION_PARAMETERS, II_LINK *ii_link, int result_type) */
/* Fetch a row of result */
static void php_ii_fetch(INTERNAL_FUNCTION_PARAMETERS, II_LINK *ii_link, int result_type)
{
	IIAPI_GETCOLPARM getColParm;
	IIAPI_DATAVALUE *columnData;
	IIAPI_GETQINFOPARM getQInfoParm;
	IIAPI_CONVERTPARM convertParm;

	int i, j, k;
	int more;
	double value_double = 0;
	long value_long = 0;
	char *value_char_p;
	int len, should_copy, correct_length;

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
	for (i = 1; i <= ii_link->fieldCount;)
	{
		j = 1;

		/* as long as there are no long byte or long varchar fields,
		   Ingres is able to fetch many fields at a time, so try to find
		   these types and stop if they're found.
		   variable j will get number of fields to fetch */
		if ((ii_link->descriptor[i]).ds_dataType != IIAPI_LBYTE_TYPE &&
			(ii_link->descriptor[i]).ds_dataType != IIAPI_LVCH_TYPE)
		{
			while (	(ii_link->descriptor[i + j - 1]).ds_dataType != IIAPI_LBYTE_TYPE &&
					(ii_link->descriptor[i + j - 1]).ds_dataType != IIAPI_LVCH_TYPE && 
					i + j <= ii_link->fieldCount)
			{
				j++;
			}
		}

		/* allocate memory for j fields */
		columnData = (IIAPI_DATAVALUE *) safe_emalloc(j, sizeof(IIAPI_DATAVALUE), 0);
		for (k = 1; k <= j; k++)
		{
			columnData[k - 1].dv_value = (II_PTR) emalloc((ii_link->descriptor[i + k - 2]).ds_length);
		}

		more = 1;				/* this is for multi segment LBYTE and LVCH elements */

		while (more)
		{
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
				RETURN_FALSE;
			}

			more = getColParm.gc_moreSegments;

			if (more)
			{			/* more segments of LBYTE or LVCH element to come */

				/* Multi segment LBYTE and LVCH elements not supported yet */
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Ingres:  Multi segment LBYTE and LVCH elements not supported yet");

			} else {

				for (k = 1; k <= j; k++)
				{
					if (columnData[k - 1].dv_null)
						{	/* NULL value ? */

						if (result_type & II_NUM)
						{
							add_index_null(return_value, i + k - 1);
						}
						if (result_type & II_ASSOC)
						{
							add_assoc_null(return_value, php_ii_field_name(ii_link, i + k - 1 TSRMLS_CC));
						}

					} else {	/* non NULL value */
						correct_length = 0;

						switch ((ii_link->descriptor[i + k - 2]).ds_dataType)
						{
	
							case IIAPI_DEC_TYPE:	/* decimal (fixed point number) */
							case IIAPI_MNY_TYPE:	/* money */
								/* convert to floating point number */
								IIAPI_CONVERT(IIAPI_FLT_TYPE, sizeof(II_FLOAT8), 53);
								/* NO break */
	
							case IIAPI_FLT_TYPE:	/* floating point number */
								switch (columnData[k - 1].dv_length)
								{

									case 4:
										value_double = (double) *((II_FLOAT4 *) columnData[k - 1].dv_value);
										break;

									case 8:
										value_double = (double) *((II_FLOAT8 *) columnData[k - 1].dv_value);
										break;

									default:
										php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Invalid size for IIAPI_FLT_TYPE data (%d)", columnData[k - 1].dv_length);
										break;
								}

								if (result_type & II_NUM)
								{
									add_index_double(return_value, i + k - 1, value_double);
								}

								if (result_type & II_ASSOC)
								{
									add_assoc_double(return_value, php_ii_field_name(ii_link, i + k - 1 TSRMLS_CC), value_double);
								}
								break;

							case IIAPI_INT_TYPE:	/* integer */
								switch (columnData[k - 1].dv_length)
								{

									case 1:
										value_long = (long) *((II_INT1 *) columnData[k - 1].dv_value);
										break;

									case 2:
										value_long = (long) *((II_INT2 *) columnData[k - 1].dv_value);
										break;
			
									case 4:
										value_long = (long) *((II_INT4 *) columnData[k - 1].dv_value);
										break;
		
									default:
										php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Invalid size for IIAPI_INT_TYPE data (%d)", columnData[k - 1].dv_length);
										break;
								}

								if (result_type & II_NUM)
								{
									add_index_long(return_value, i + k - 1, value_long);
								}
	
								if (result_type & II_ASSOC)
								{
									add_assoc_long(return_value, php_ii_field_name(ii_link, i + k - 1 TSRMLS_CC), value_long);
								}
								break;

							case IIAPI_TXT_TYPE:	/* variable length character string */
							case IIAPI_VBYTE_TYPE:	/* variable length binary string */
#ifdef INGRES_UNICODE
							case IIAPI_NVCH_TYPE:	/* variable length unicode character string */
								/* Convert it to IIAPI_VCH_TYPE */
								if ((ii_link->descriptor[i + k - 2]).ds_dataType == IIAPI_NVCH_TYPE)
								{
									IIAPI_CONVERT(IIAPI_CHA_TYPE, (columnData[k - 1]).dv_length, 0);
								}
								/* let the next 'case' handle the conversion to a format usable by php */
#endif								
							case IIAPI_VCH_TYPE:	/* variable length character string */
								/* real length is stored in first 2 bytes of data, so adjust
								   length variable and data pointer */
								columnData[k - 1].dv_length = *((II_INT2 *) columnData[k - 1].dv_value);
								((II_INT2 *) columnData[k - 1].dv_value)++;
								correct_length = 1;
								/* NO break */

							case IIAPI_NCHA_TYPE:	/* fixed length unicode character string */	
								if ((ii_link->descriptor[i + k - 2]).ds_dataType == IIAPI_NCHA_TYPE)
								{
									IIAPI_CONVERT(IIAPI_CHA_TYPE, (columnData[k - 1]).dv_length, 0);
								}
							case IIAPI_BYTE_TYPE:	/* fixed length binary string */
							case IIAPI_CHA_TYPE:	/* fixed length character string */
							case IIAPI_CHR_TYPE:	/* fixed length character string */
							case IIAPI_LOGKEY_TYPE:	/* value unique to database */
							case IIAPI_TABKEY_TYPE:	/* value unique to table */
							case IIAPI_DTE_TYPE:	/* date */

								/* eventualy convert date to string */
								if ((ii_link->descriptor[i + k - 2]).
									ds_dataType == IIAPI_DTE_TYPE)
								{
									IIAPI_CONVERT(IIAPI_CHA_TYPE, 32, 0);
								}
	
								/* use php_addslashes if asked to */
								if (PG(magic_quotes_runtime))
								{
									value_char_p = php_addslashes((char *) columnData[k - 1].dv_value, columnData[k - 1].dv_length, &len, 0 TSRMLS_CC);
									should_copy = 0;
								} else {
									value_char_p = (char *) columnData[k - 1].dv_value;
									len = columnData[k - 1].dv_length;
									should_copy = 1;
								}

								if (result_type & II_NUM)
								{
									add_index_stringl(return_value, i + k - 1, value_char_p, len, should_copy);
								}

								if (result_type & II_ASSOC)
								{
									add_assoc_stringl(return_value, php_ii_field_name(ii_link, i + k - 1 TSRMLS_CC), value_char_p, len, should_copy);
								}

								/* eventualy restore data pointer state for
							 	   variable length data types */
								if (correct_length)
								{
									if ((ii_link->descriptor[i + k - 2]).ds_dataType == IIAPI_NVCH_TYPE)
									{
										((II_UINT2 *) columnData[k - 1].dv_value)--;
									} else {
										((II_INT2 *) columnData[k - 1].dv_value)--;
									}
								}
								break;
			
							default:
								php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Invalid SQL data type in fetched field (%d -- length : %d)", (ii_link->descriptor[i + k - 2]).ds_dataType, columnData[k - 1].dv_length);
								break;
						}
					}
				}
			}
		}

		/* free the memory buffers */
		for (k = 1; k <= j; k++)
		{
			efree(columnData[k - 1].dv_value);
		}
		efree(columnData);

		/* increase field pointer by number of fetched fields */
		i += j;
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
			RETURN_FALSE;
		}

	}

	if (argc == 0 )
	{
		link_id = php_ii_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if ( link_id == -1 ) /* There was a problem in php_ii_get_default_link */
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
			RETURN_FALSE;
		}
	}

	ZEND_FETCH_RESOURCE2(ii_link, II_LINK *, link, link_id, "Ingres Link", le_ii_link, le_ii_plink);

	if (ii_link->stmtHandle && _close_statement(ii_link TSRMLS_CC))
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to close statement !!");
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: An error occured getting the default link" );
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
				ptr = ecalloc (33,1); /* max number of chars for long including sign and null */
				itoa(ii_link->errorCode,ptr,10);
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
	} else {
		switch (mode)
		{
			case 0:
				ptr = ecalloc(33,1);   /* maximum num of chars in a long value including sign and null */
				itoa( IIG(errorCode), ptr, 10);
				break;
			case 1:
				len = strlen(IIG(errorText));
				ptr = ecalloc(len + 1, 1);
				memcpy (ptr, IIG(errorText), len + 1 );
				
				break;
			case 2:
				len = sizeof(IIG(sqlstate));
				ptr = ecalloc(len + 1, 1);
				memcpy (ptr, IIG(sqlstate), len + 1 );
				break;
			default:
				break;
		}
		RETVAL_STRING(ptr, 0);
	}
}
/* }}} */

/* {{{ static long php_ii_queryparse(char *statement TSRMLS_DC) */
/* ----------------------------------------------------------------------
 * int php_ii_queryparse(char *statement TSRMLS_DC)
 *
 * Count the placeholders (?) parameters in the statement
 * return -1 for error. 0 or number of question marks
 *
 * Thanks to ext/informix (based on php_intifx_preparse).
 *
 * ----------------------------------------------------------------------
*/
static long php_ii_queryparse(char *statement TSRMLS_DC)
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
static void php_ii_check_procedure(char *statement, II_LINK *ii_link TSRMLS_DC)
{
	char *src;
	char *end_space;
	char *end_term;
	char *end_bracket;
	char *end_addr;
	char exec_proc[19];
	char call_proc[6];
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

		ii_link->procname = malloc(proc_len + 1);
		for ( pos = 0; pos <= proc_len; pos++)
		{
			ii_link->procname[pos] = src[pos];
		}
		ii_link->procname[proc_len]='\0';
	}
}
/* }}} */

/* {{{ static short int php_ii_set_connect_options(INTERNAL_FUNCTION_PARAMETERS, zval **options, II_LINK *ii_link, II_LOGIN *user_details) */
/*     Sets up options provided to ingres_connect() via a parameter array */
static short int php_ii_set_connect_options(zval **options, II_LINK *ii_link, II_LOGIN *user_details TSRMLS_DC)
{
	zval **data;
	char *key;
	II_LONG parameter_id;
	II_PTR  parameter_value;
	IIAPI_SETCONPRMPARM	setConPrmParm;
	IIAPI_CONNPARM connParm;
	IIAPI_DISCONNPARM	disconnParm;
	long index;
	int i;
	int num_options;
	char *temp_string;
	long temp_long;

    num_options = zend_hash_num_elements(Z_ARRVAL_PP(options));
	zend_hash_internal_pointer_reset(Z_ARRVAL_PP(options));

	connParm.co_genParm.gp_callback = NULL;
	connParm.co_genParm.gp_closure = NULL;
	connParm.co_target = user_details->database;
	connParm.co_connHandle = NULL;
	connParm.co_tranHandle = NULL;
	connParm.co_type = IIAPI_CT_SQL; 
	connParm.co_username = user_details->user;
	connParm.co_password = user_details->password;
	connParm.co_timeout = -1;

	IIapi_connect( &connParm );

	if (!ii_sync(&(connParm.co_genParm)) || ii_success(&(connParm.co_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to connect to database (%s), to setup options", user_details->database);
		return II_FAIL;
	}

	ii_link->connHandle = connParm.co_connHandle;

	disconnParm.dc_genParm.gp_callback = NULL;
	disconnParm.dc_genParm.gp_closure = NULL;
	disconnParm.dc_connHandle = ii_link->connHandle;

	IIapi_disconnect( &disconnParm );

	if (!ii_sync(&(disconnParm.dc_genParm)) || ii_success(&(disconnParm.dc_genParm), ii_link TSRMLS_CC) == II_FAIL)
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  Unable to disconnect during setup of options");
		return II_FAIL;
	}

	for ( i = 0; i < num_options; i++ )
	{
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
			else if ( strcmp("timezone", key) == 0 )
			{
					parameter_id = IIAPI_CP_TIMEZONE;
			}
			else if (strcmp( "date_format", key) == 0 )
			{
					/* this has no effect unless casting a date column to varchar first */
				    /* IIapi_formatData() would need to be used instead */
					parameter_id = IIAPI_CP_DATE_FORMAT;
			}
			else if ( strcmp("decimal_separator", key) == 0 ) 
			{
					/* this has no effect unless casting a decimal/float column to varchar first */
				    /* IIapi_formatData() would need to be used instead */
					parameter_id = IIAPI_CP_DECIMAL_CHAR;
			}
			else if ( strcmp("date_century_boundary", key) == 0 )
			{
					parameter_id = IIAPI_CP_CENTURY_BOUNDARY;
			}
			else if ( strcmp("money_lort", key) == 0 ) /* leading or trailing money sign, default is leading*/
			{
					parameter_id = IIAPI_CP_MONEY_LORT;
			}
			else if ( strcmp("money_sign", key) == 0 ) /* defaults to the ingres variable II_MONEY_FORMAT or "$" */
			{
					parameter_id = IIAPI_CP_MONEY_SIGN;
			}
			else if ( strcmp("money_precision", key) == 0 ) /* defaults to 2 if not set */
			{
					parameter_id = IIAPI_CP_MONEY_PRECISION;
			}
			else 
			{
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: unknown connection option '%s'",key );
					return II_FAIL;
			}
		}
		else
		{
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: unexpected index in connection options array.");
			return II_FAIL;
		}

		setConPrmParm.sc_genParm.gp_callback = NULL;
		setConPrmParm.sc_connHandle = NULL;
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
				setConPrmParm.sc_paramValue =(II_PTR)&temp_long;
				break;
			case IS_BOOL:
				convert_to_long_ex(data);
				temp_long = Z_LVAL_PP(data);
				setConPrmParm.sc_paramValue =(II_PTR)&temp_long;
				break;
			default:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres: unknown option type, %l, in connection options", Z_TYPE_PP(data));
				return II_FAIL;
		}

		IIapi_setConnectParam( &setConPrmParm );

		if (!ii_sync(&(setConPrmParm.sc_genParm)) || ii_success(&(setConPrmParm.sc_genParm), ii_link TSRMLS_CC) == II_FAIL)
		{
			if ( Z_TYPE_PP(data) == IS_STRING )
			{
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  failed to set option, %s, with value, %s", key, Z_STRVAL_PP(data));
				return II_FAIL;
			}
			else
			{
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Ingres:  failed to set option, %s, with value, %ld", key, Z_LVAL_PP(data));
				return II_FAIL;
			}
		}

		ii_link->connHandle = setConPrmParm.sc_connHandle;

		zend_hash_move_forward(Z_ARRVAL_PP(options));

	}
	return II_OK;
}
/* }}} */

#endif /* HAVE_II */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
