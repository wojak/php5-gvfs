PHP_ARG_ENABLE(gvfs, whether to enable gvfs support,
[--enable-gvfs   Enable gvfs support])

if test "$PHP_GVFS" != "no"; then

  if test "$PHP_GVFS" != "yes"; then
    GVFS_SEARCH_DIRS=$PHP_GVFS
  else
    GVFS_SEARCH_DIRS="/usr/local /usr"
  fi

  for i in $GVFS_SEARCH_DIRS; do
    if test -f $i/include/glib-2.0/glib.h; then
      GLIB_DIR=$i
      GLIB_INCDIR=$i/include/glib-2.0/
    fi
  done


  if test -z "$GLIB_DIR"; then
    AC_MSG_ERROR(Can not find glib-2.0)
  fi


  for x in $GVFS_SEARCH_DIRS; do
    test -f $x/$PHP_LIBDIR/libglib-2.0.$SHLIB_SUFFIX_NAME || test -f $x/$PHP_LIBDIR/libglib-2.0.a && GLIB_LIB_DIR=$x && break
  done

  if test -z "$GLIB_LIB_DIR"; then
    AC_MSG_ERROR([libglib-2.0.(a|so) not found.])
  fi

  PHP_CHECK_LIBRARY(glib-2.0,g_test_init,
  [
    PHP_ADD_LIBRARY_WITH_PATH(glib-2.0, $GLIB_LIB_DIR/$PHP_LIBDIR, GLIB_SHARED_LIBADD)
  ], [
 dnl   AC_MSG_ERROR([libgnomevfs-2.(a|so) not found.])
  ], [
dnl   -L$GVFS_LIB_DIR -lgnomevfs-2
  ])

  PHP_ADD_INCLUDE("/usr/include/glib-2.0/")
  PHP_ADD_INCLUDE("/usr/lib/glib-2.0/include/")
  PHP_ADD_INCLUDE("/usr/include/")
  PHP_SUBST(GLIB_SHARED_LIBADD)

  PHP_ADD_INCLUDE($GLIB_INCDIR)
  AC_DEFINE(HAVE_L,1,[ ])
  LDFLAGS="-lglib-2.0 -lgio-2.0"

  AC_DEFINE(HAVE_GVFS, 1, [Whether you have gvfs-2.0])
  PHP_NEW_EXTENSION(gvfs, gvfs.c, $ext_shared)

fi

