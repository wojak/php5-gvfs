#ifndef PHP_GVFS_H
#define PHP_GVFS_H 1

#define PHP_GVFS_VERSION "1.0"
#define PHP_GVFS_EXTNAME "gvfs"

PHP_FUNCTION(gvfs_info);
PHP_FUNCTION(gvfs_mount);
PHP_FUNCTION(gvfs_list_mounted);

extern zend_module_entry gvfs_module_entry;
#define phpext_gvfs_ptr &gvfs_module_entry

#endif
