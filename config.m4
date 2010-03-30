dnl
dnl $Id$
dnl

PHP_ARG_WITH(ingres, for improved Ingres support,
[  --with-ingres[=DIR]     Include Ingres support. DIR is the Ingres
                          base directory (default $II_SYSTEM)])
PHP_ARG_ENABLE(ingres2, Use ingres2 as the extension name,
[  --enable-ingres2]     Use ingres2 as the extension name  
                          allowing the driver to co-exist with the older ingres release, ,no)

if test "$PHP_INGRES" != "no" && test "$PHP_INGRES2" = "no"; then
  AC_DEFINE(HAVE_INGRES, 1, [Whether you have Ingres])
  PHP_NEW_EXTENSION(ingres, ingres.c convertUTF.c, $ext_shared)
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

  if test -r $II_DIR/lib/libiiapi.1.$SHLIB_SUFFIX_NAME; then
    II_LIB_DIR=$II_DIR/lib
  else
    AC_MSG_ERROR(Cannot find libiiapi.1.$SHLIB_SUFFIX_NAME under $II_DIR/lib - is \$II_SYSTEM set?)
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
  PHP_NEW_EXTENSION(ingres2, ingres.c convertUTF.c, $ext_shared)

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

# Certain platforms require memory to be accessed/addressed
# along memory boundaries. The list of host operating systems
# ($host_os) is based on what are known systems that require
# aligned memory access. It should be noted that this list is
# possibly incomplete. As a general rule the Intel X86/X86-64
# based CPUs do not require aligned memory addressing.

align_memory="no"

# Determine the processor architecture
arch=`uname -p`
case $host_os in
  solaris*[)]
    if test "X$arch" = "Xsparc"; then
      align_memory="yes"
    fi
    ;; 
esac

if test "$align_memory" = "yes"; then
  AC_DEFINE(ALIGN_MEMORY, 1, [Enforce memory alignement])
fi

dnl vim: ff=unix ts=2 sw=2 expandtab
