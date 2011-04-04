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
    PHP_FE(gvfs_mount, NULL)
    PHP_FE(gvfs_info, NULL)
    PHP_FE(gvfs_list_mounted, NULL)
    PHP_FE(gvfs_unmount, NULL)
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

static int outstanding_mounts = 0;
static GMainLoop *main_loop;

// Account the mounting proces should deal with
typedef struct  {
   char *username;
   int username_len;
   char *password;
   int password_len;
} gvfs_account;





static void
unmount_done_cb (GObject *object,
                 GAsyncResult *res,
                 gpointer user_data)
{
    gboolean succeeded;
    GError *error = NULL;

    succeeded = g_mount_unmount_with_operation_finish (G_MOUNT (object), res, &error);

    g_object_unref (G_MOUNT (object));

    if (!succeeded)
      g_printerr (_("Error unmounting mount: %s\n"), error->message);

    outstanding_mounts--;

    if (outstanding_mounts == 0)
      g_main_loop_quit (main_loop);
}


PHP_FUNCTION(gvfs_unmount) {
    char *filename = NULL;
    int filename_len = 0;

    g_type_init();
    GMount *mount;
    GError *error = NULL;
    GMountOperation *mount_op;
    GFile *file;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
       return;
    }

    file = g_file_parse_name (filename);
    if (file == NULL)
      return;

    mount = g_file_find_enclosing_mount (file, NULL, &error);
    if (mount == NULL)
      {
        g_printerr (_("Error finding enclosing mount: %s\n"), error->message);
        return;
      }

    GMountOperation *op;
    mount_op = g_mount_operation_new ();

    g_mount_unmount_with_operation (mount, 0, mount_op, NULL, unmount_done_cb, NULL);
    g_object_unref (mount_op);

    outstanding_mounts++;
    
    main_loop = g_main_loop_new (NULL, FALSE);
    if (outstanding_mounts > 0)
      g_main_loop_run (main_loop);

}


static void
mount_done_cb (GObject *object,
               GAsyncResult *res,
               gpointer user_data)
{
    gboolean succeeded;
    GError *error = NULL;

    succeeded = g_file_mount_enclosing_volume_finish (G_FILE (object), res, &error);

    if (!succeeded)
      g_printerr (_("Error mounting location: %s\n"), error->message);

    outstanding_mounts--;

    if (outstanding_mounts == 0)
      g_main_loop_quit (main_loop);
}



static void
ask_password_cb (GMountOperation *op,
                 const char      *message,
                 const char      *default_user,
                 const char      *default_domain,
                 GAskPasswordFlags flags,
                 gvfs_account *account)
{
//  g_print ("%s\n", message);
  if (flags & G_ASK_PASSWORD_NEED_USERNAME)
    {
      g_mount_operation_set_username (op, account->username);
    }

/*  if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
    {
      s = prompt_for ("Domain", default_domain, TRUE);
      g_mount_operation_set_domain (op, s);
      g_free (s);
    }*/

  if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
    {
      g_mount_operation_set_password (op, account->password);
    }

  g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);

}

static GMountOperation *new_mount_op (gvfs_account *account)
{
  GMountOperation *op;
  op = g_mount_operation_new ();
  g_signal_connect (op, "ask_password", G_CALLBACK (ask_password_cb), account);
  return op;
}


PHP_FUNCTION(gvfs_mount)
{
    char *filename, *password = NULL, *username = NULL;
    int filename_len, password_len, username_len;
    GFile *file;
    gvfs_account account;
    main_loop = g_main_loop_new (NULL, FALSE);

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ss", &filename, &filename_len, &account.password, &account.password_len, &account.username, &account.username_len) == FAILURE) {
       return;
    }

    g_type_init ();
    file = g_file_parse_name (filename);
    GMountOperation *op;

    if (file == NULL)
      return;

    op = new_mount_op (&account);
    g_file_mount_enclosing_volume (file, 0, op, NULL, mount_done_cb, op);
    outstanding_mounts++;

  if (outstanding_mounts > 0)
    g_main_loop_run (main_loop);
}


PHP_FUNCTION(gvfs_list_mounted) {
    GList *l;
    int c;
    GMount *mount;
    GVolume *volume;
    char *name, *uuid, *uri;
    GFile *root, *default_location;
    GIcon *icon;
    char **x_content_types;
    GVolumeMonitor *volume_monitor;
    GList *drives, *volumes, *mounts;
    char s[1024];

    volume_monitor = g_volume_monitor_get();
    mounts = g_volume_monitor_get_mounts (volume_monitor);
    array_init(return_value);
    for (c = 0, l = mounts; l != NULL; l = l->next, c++) {
        mount = (GMount *) l->data;
        volume = g_mount_get_volume (mount);
        if (volume != NULL) {
            g_object_unref (volume);
            continue;
        }

    name = g_mount_get_name (mount);
    root = g_mount_get_root (mount);
    uri = g_file_get_uri (root);

    add_assoc_string(return_value, name, uri, 1);
    }

    g_list_foreach (mounts, (GFunc)g_object_unref, NULL);
    g_list_free (mounts);

  g_object_unref (volume_monitor);
}


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
