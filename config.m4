dnl $Id$
dnl config.m4 for extension hessian

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(hessian, for hessian support,
dnl Make sure that the comment is aligned:
dnl [  --with-hessian             Include hessian support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(hessian, whether to enable hessian support,
Make sure that the comment is aligned:
[  --enable-hessian           Enable hessian support])

if test "$PHP_HESSIAN" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-hessian -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/hessian.h"  # you most likely want to change this
  dnl if test -r $PHP_HESSIAN/$SEARCH_FOR; then # path given as parameter
  dnl   HESSIAN_DIR=$PHP_HESSIAN
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for hessian files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       HESSIAN_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$HESSIAN_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the hessian distribution])
  dnl fi

  dnl # --with-hessian -> add include path
  dnl PHP_ADD_INCLUDE($HESSIAN_DIR/include)

  dnl # --with-hessian -> check for lib and symbol presence
  dnl LIBNAME=hessian # you may want to change this
  dnl LIBSYMBOL=hessian # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $HESSIAN_DIR/lib, HESSIAN_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_HESSIANLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong hessian lib version or lib not found])
  dnl ],[
  dnl   -L$HESSIAN_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(HESSIAN_SHARED_LIBADD)

  PHP_NEW_EXTENSION(hessian, hessian.c, $ext_shared)
fi
