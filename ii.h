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

#ifndef II_H
#define II_H

#if HAVE_INGRES
#include "php_ii.h"
#include "iiapi.h"

typedef struct _ii_result_entry {
    char                *next_result_ptr;
    int                 result_id;
} ii_result_entry;

typedef struct _II_LINK {
    II_PTR				connHandle;
    II_PTR              tranHandle;
    II_PTR              stmtHandle;
    II_PTR              envHandle;
    II_PTR              errorHandle; /* error handle */
    int                 autocommit;
    ii_result_entry     *result_list_ptr; 
    int                 auto_multi;  /* Enable multiple cursors when autocommit is enabled */
} II_LINK;

typedef struct _II_RESULT {
    II_PTR              stmtHandle; /* statement handle for the result */
    II_PTR              connHandle; /* connection associated with this result */
    II_PTR              tranHandle; /* transaction handle */
    II_PTR              errorHandle; /* error handle */
    II_LONG             fieldCount;
    IIAPI_DESCRIPTOR    *descriptor;
    int                 paramCount;
    char                *cursor_id;
    long                cursor_mode;
    char                *procname;
#if defined IIAPI_VERSION_6
    int                 scrollable; /* is this a scrollable cursor */
#endif
    int                 link_id;    /* the link to which this result belongs */
} II_RESULT;

/* The following was added to allow the extension to build on Windows using */
/* Visual C++ 6.0 */
#ifdef WIN32
    typedef __int64 ingres_int64;
#else
    typedef long long int ingres_int64;
#endif

#define II_FAIL 0
#define II_OK 1
#define II_NO_DATA 2

#define II_FIELD_INFO_NAME 1
#define II_FIELD_INFO_TYPE 2
#define II_FIELD_INFO_NULLABLE 3
#define II_FIELD_INFO_LENGTH 4
#define II_FIELD_INFO_PRECISION 5
#define II_FIELD_INFO_SCALE 6

#define II_DATE_US IIAPI_EPV_DFRMT_US
#define II_DATE_MULTINATIONAL IIAPI_EPV_DFRMT_MULTI
#define II_DATE_MULTINATIONAL4 IIAPI_EPV_DFRMT_MLT4
#define II_DATE_FINNISH IIAPI_EPV_DFRMT_FINNISH
#define II_DATE_ISO IIAPI_EPV_DFRMT_ISO
#define II_DATE_ISO4 9 /* until IIAPI_EPV_DFRMT_ISO4 is released in a public build*/
#define II_DATE_GERMAN IIAPI_EPV_DFRMT_GERMAN
#define II_DATE_MDY IIAPI_EPV_DFRMT_MDY
#define II_DATE_DMY IIAPI_EPV_DFRMT_DMY
#define II_DATE_YMD IIAPI_EPV_DFRMT_YMD

#define II_MONEY_LEAD_SIGN IIAPI_CPV_MONEY_LEAD_SIGN
#define II_MONEY_TRAIL_SIGN IIAPI_CPV_MONEY_TRAIL_SIGN

#define II_STRUCTURE_ISAM IIAPI_CPV_ISAM
#define II_STRUCTURE_CISAM IIAPI_CPV_CISAM
#define II_STRUCTURE_BTREE IIAPI_CPV_BTREE
#define II_STRUCTURE_CBTREE IIAPI_CPV_CBTREE
#define II_STRUCTURE_HASH IIAPI_CPV_HASH
#define II_STRUCTURE_CHASH IIAPI_CPV_CHASH
#define II_STRUCTURE_HEAP IIAPI_CPV_HEAP
#define II_STRUCTURE_CHEAP IIAPI_CPV_CHEAP

#define BOM_UTF16_LE "\xff\xfe" 
#define BOM_UTF16_BE "\xfe\xff" 
#define BOM_UTF8 "\xef\xbb\xbf"

#define INGRES_SQL_SELECT 1
#define INGRES_SQL_INSERT 2
#define INGRES_SQL_UPDATE 3
#define INGRES_SQL_DELETE 4
#define INGRES_SQL_COMMIT 5
#define INGRES_SQL_ROLLBACK 6
#define INGRES_SQL_OPEN 7
#define INGRES_SQL_CLOSE 8
#define INGRES_SQL_CONNECT 9
#define INGRES_SQL_DISCONNECT 10
#define INGRES_SQL_GETDBEVENT 11
#define INGRES_SQL_SAVEPOINT 12
#define INGRES_SQL_AUTOCOMMIT 13
#define INGRES_SQL_EXECUTE_PROCEDURE 14
#define INGRES_SQL_CALL 15
#define INGRES_SQL_COPY 16
#define INGRES_SQL_CREATE 17
#define INGRES_SQL_ALTER 18

#define INGRES_NO_OF_COMMANDS 18

static struct
{
    char        *command;
    int         code;
} SQL_COMMANDS [INGRES_NO_OF_COMMANDS] =
{
    { "SELECT", INGRES_SQL_SELECT },
    { "INSERT", INGRES_SQL_INSERT },
    { "UPDATE", INGRES_SQL_UPDATE },
    { "DELETE", INGRES_SQL_DELETE },
    { "COMMIT", INGRES_SQL_COMMIT },
    { "ROLLBACK", INGRES_SQL_ROLLBACK },
    { "OPEN", INGRES_SQL_OPEN },
    { "CLOSE", INGRES_SQL_CLOSE },
    { "CONNECT", INGRES_SQL_CONNECT },
    { "DISCONNECT", INGRES_SQL_DISCONNECT },
    { "GET DBEVENT", INGRES_SQL_GETDBEVENT },
    { "SAVEPOINT", INGRES_SQL_SAVEPOINT },
    { "SET AUTOCOMMIT", INGRES_SQL_AUTOCOMMIT },
    { "EXECUTE PROCEDURE", INGRES_SQL_EXECUTE_PROCEDURE },
    { "CALL", INGRES_SQL_CALL },
    { "COPY", INGRES_SQL_COPY },
    { "CREATE", INGRES_SQL_CREATE },
    { "ALTER", INGRES_SQL_ALTER },
};


static int ii_sync(IIAPI_GENPARM *genParm);
static int ii_success(IIAPI_GENPARM *genParm, II_PTR *connHandle TSRMLS_DC);

static int _close_statement(II_RESULT *ii_result TSRMLS_DC);
static int _rollback_transaction(II_LINK *ii_link TSRMLS_DC);
static int _commit_transaction(II_LINK *ii_link  TSRMLS_DC);
static void _close_ii_link(II_LINK *link TSRMLS_DC);
static void _close_ii_plink(zend_rsrc_list_entry *link TSRMLS_DC);
static void php_ii_do_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent);
static char *php_ii_field_name(II_RESULT *ii_result, int index TSRMLS_DC);
static void php_ii_field_info(INTERNAL_FUNCTION_PARAMETERS, II_RESULT *ii_result, long index, int info_type);
static void php_ii_fetch(INTERNAL_FUNCTION_PARAMETERS, II_RESULT *ii_result, int result_type, long row_position, II_INT2 row_count);
static void php_ii_error(INTERNAL_FUNCTION_PARAMETERS, int mode);
static long php_ii_paramcount(char *statement TSRMLS_DC);
static void php_ii_gen_cursor_id(II_RESULT *ii_result TSRMLS_DC);
static char *php_ii_check_procedure(char *statement, II_LINK *ii_link TSRMLS_DC);
static short int php_ii_set_connect_options(zval *options, II_LINK *ii_link, char *database TSRMLS_DC);
static void php_ii_convert_param_markers (char *query, char *converted_query TSRMLS_DC);
static short php_ii_bind_params (INTERNAL_FUNCTION_PARAMETERS, II_RESULT *ii_result, zval *queryParams, char *paramtypes);
static II_LONG php_ii_convert_data ( short destType, int destSize, int precision, II_RESULT *ii_result, IIAPI_DATAVALUE *columnData, IIAPI_GETCOLPARM getColParm, int field, int column TSRMLS_DC );
static short int php_ii_set_environment_options (zval *options, II_LINK *ii_link TSRMLS_DC);
static void _ii_init_link (INTERNAL_FUNCTION_PARAMETERS,  II_LINK *ii_link );
static void _ii_init_result (INTERNAL_FUNCTION_PARAMETERS, II_RESULT *ii_result, II_LINK *ii_link );
static void _free_ii_link_result_list (II_LINK *ii_link TSRMLS_DC);
static int php_ii_query_type(char *statement TSRMLS_DC);
static int _rollback_transaction(II_LINK *ii_link  TSRMLS_DC);
static short php_ii_result_remove ( II_RESULT *ii_result, long result_id TSRMLS_DC );








#endif  /* HAVE_II */
#endif    /* II_H */


/*
 * Local variables:
 * tab-width: 4
 * shift-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker ff=unix expandtab
 * vim<600: sw=4 ts=4
 */
