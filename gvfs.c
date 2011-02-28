/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2011 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Wojciech Kupiec <kupiec.w@gmail.com>                         |
   +----------------------------------------------------------------------+
*/


/* 
 *  GVFS bindings for the PHP language
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_gvfs.h"


#include <glib.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <gio/gio.h>


static function_entry gvfs_functions[] = {
    PHP_FE(gvfs_info, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry gvfs_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_GVFS_EXTNAME,
    gvfs_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#if ZEND_MODULE_API_NO >= 20010901
    PHP_GVFS_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_GVFS
ZEND_GET_MODULE(gvfs)
#endif


PHP_FUNCTION(gvfs_info)
{
    GFileQueryInfoFlags flags = 0x0;
    GFileInfo *info;
    GError *error;
    GFile *file;
    char **attributes;
    char *s;
    int i; 
    char *filename;
    int filename_len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
       return;
    }
    
    error = NULL;
    g_type_init ();

    flags |= G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS;
    file = g_file_parse_name (filename);

    info = g_file_query_info (file, "standard::content-type", flags, NULL, &error);

    if(error!=NULL) {
        RETURN_STRING(error->message, 1);
    } else {
        attributes = g_file_info_list_attributes (info, NULL);
        s = g_file_info_get_attribute_as_string (info, *attributes);
        RETURN_STRING(s, 1);
    }
}
