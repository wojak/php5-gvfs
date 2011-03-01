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
    GError *error = NULL;
    GFile *file;
    char **attributes;
    char *required_attr;
    int required_attr_len;
    char *s;
    int i; 
    char *filename;
    int filename_len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &filename, &filename_len, &required_attr, &required_attr_len) == FAILURE) {
       return;
    }
    if(required_attr_len <= 0) {
        required_attr = "*";
    }

    g_type_init ();

    flags |= G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS;
    file = g_file_parse_name (filename);

    info = g_file_query_info (file, required_attr, flags, NULL, &error);

    if (error!=NULL) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, error->message);
        RETURN_FALSE;
    } else {
        array_init(return_value);
        attributes = g_file_info_list_attributes (info, NULL);

        for (i = 0; attributes[i] != NULL; i++) {
            /* standard::icon is non human redable list, needs to be converted and returned as a sub array */
            if (strcmp (attributes[i], "standard::icon") == 0) {
                GIcon *icon;
                int j;
                const char * const *names = NULL;
                char *ico_item;
                zval *icons;
                icon = g_file_info_get_icon (info);
                if (G_IS_THEMED_ICON(icon)) {
                    names = g_themed_icon_get_names (G_THEMED_ICON (icon));
                    ALLOC_INIT_ZVAL(icons);
                    array_init(icons);
                    for (j = 0; names[j] != NULL; j++) {
                        add_index_string(icons, j, names[j], 1);
                    }
                    add_assoc_zval(return_value, attributes[i], icons);
                    g_free (names);
                }
            } else {
                s = g_file_info_get_attribute_as_string (info, attributes[i]);
                add_assoc_string(return_value, attributes[i], s, 1);
                g_free (s);
            }
        }
    }
}
