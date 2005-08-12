dnl
dnl $Id$
dnl

PHP_ARG_WITH(ingres, for Ingres support,
[  --with-ingres[=DIR]     Include Ingres support. DIR is the Ingres
                          base directory (default $II_SYSTEM/ingres)])

if test "$PHP_INGRES" != "no"; then
  AC_DEFINE(HAVE_II, 1, [Whether you have Ingres])
  PHP_NEW_EXTENSION(ingres, ii.c, $ext_shared)
  PHP_SUBST(II_SHARED_LIBADD)

  if test "$PHP_INGRES" = "yes"; then
    II_DIR=$II_SYSTEM/ingres
  else
    II_DIR=$PHP_INGRES
  fi

  if test -r $II_DIR/files/iiapi.h; then
    II_INC_DIR=$II_DIR/files
  else
    AC_MSG_ERROR(Cannot find iiapi.h under $II_DIR/files)
  fi

  if test -r $II_DIR/lib/libiiapi.1.so; then
    II_LIB_DIR=$II_DIR/lib
  else
    AC_MSG_ERROR(Cannot find libiiapi.1.so under $II_DIR/lib)
  fi

  PHP_ADD_LIBRARY_WITH_PATH(iiapi.1, $II_LIB_DIR, INGRES_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(q.1, $II_LIB_DIR, INGRES_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(frame.1, $II_LIB_DIR, INGRES_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(compat.1, $II_LIB_DIR, INGRES_SHARED_LIBADD)
  PHP_ADD_INCLUDE($II_INC_DIR)
  PHP_SUBST(INGRES_SHARED_LIBADD)
fi
