dnl
dnl $Id$
dnl

PHP_ARG_WITH(ingres, for improved Ingres support,
[  --with-ingres[=DIR]     Include Ingres support. DIR is the Ingres
                          base directory (default $II_SYSTEM)])
PHP_ARG_ENABLE(ingres2, Use ingres2 as the extension name,
[  --enable-ingres2]     Use ingres2 as the extension name  
                          allowing the driver to co-exist with the older ingres release, ,no)

if test "$PHP_INGRES" != "no" && test "$PHP_INGRES2" == "no"; then
  AC_DEFINE(HAVE_INGRES, 1, [Whether you have Ingres])
  PHP_NEW_EXTENSION(ingres, ii.c convertUTF.c, $ext_shared)
  PHP_SUBST(II_SHARED_LIBADD)

  if test "$PHP_INGRES" = "yes"; then
    II_DIR=$II_SYSTEM/ingres
  else
    II_DIR=$PHP_INGRES/ingres
  fi

  if test -r $II_DIR/files/iiapi.h; then
    II_INC_DIR=$II_DIR/files
  else
    AC_MSG_ERROR(Cannot find iiapi.h under $II_DIR/files - is \$II_SYSTEM set?)
  fi

  if test -r $II_DIR/lib/libiiapi.1.so; then
    II_LIB_DIR=$II_DIR/lib
  else
    AC_MSG_ERROR(Cannot find libiiapi.1.so under $II_DIR/lib - is \$II_SYSTEM set?)
  fi

  PHP_ADD_LIBRARY_WITH_PATH(iiapi.1, $II_LIB_DIR, INGRES_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(q.1, $II_LIB_DIR, INGRES_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(frame.1, $II_LIB_DIR, INGRES_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(compat.1, $II_LIB_DIR, INGRES_SHARED_LIBADD)
  PHP_ADD_INCLUDE($II_INC_DIR)
  PHP_SUBST(INGRES_SHARED_LIBADD)
fi

if test "$PHP_INGRES2" != "no"; then
  AC_DEFINE(HAVE_INGRES, 1, [Whether you have Ingres])
  AC_DEFINE(HAVE_INGRES2, 1, [use ingres2 as the extension name])
  PHP_NEW_EXTENSION(ingres2, ii.c convertUTF.c, $ext_shared)

  PHP_SUBST(II_SHARED_LIBADD)

  if test "$PHP_INGRES" = "yes"; then
    II_DIR=$II_SYSTEM/ingres
  else
    II_DIR=$PHP_INGRES/ingres
  fi

  if test -r $II_DIR/files/iiapi.h; then
    II_INC_DIR=$II_DIR/files
  else
    AC_MSG_ERROR(Cannot find iiapi.h under $II_DIR/files - is \$II_SYSTEM set?)
  fi

  if test -r $II_DIR/lib/libiiapi.1.so; then
    II_LIB_DIR=$II_DIR/lib
  else
    AC_MSG_ERROR(Cannot find libiiapi.1.so under $II_DIR/lib - is \$II_SYSTEM set?)
  fi

  PHP_ADD_LIBRARY_WITH_PATH(iiapi.1, $II_LIB_DIR, INGRES2_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(q.1, $II_LIB_DIR, INGRES2_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(frame.1, $II_LIB_DIR, INGRES2_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(compat.1, $II_LIB_DIR, INGRES2_SHARED_LIBADD)
  PHP_ADD_INCLUDE($II_INC_DIR)
  PHP_SUBST(INGRES2_SHARED_LIBADD)

fi


dnl vim: ff=unix
