dnl $Id$
dnl config.m4 for extension PulseFlow

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(PulseFlow, for PulseFlow support,
dnl Make sure that the comment is aligned:
[  --with-PulseFlow             Include PulseFlow support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(PulseFlow, whether to enable PulseFlow support,
dnl Make sure that the comment is aligned:
dnl [  --enable-PulseFlow           Enable PulseFlow support])

if test "$PHP_PULSEFLOW" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-PulseFlow -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/PulseFlow.h"  # you most likely want to change this
  dnl if test -r $PHP_PULSEFLOW/$SEARCH_FOR; then # path given as parameter
  dnl   PULSEFLOW_DIR=$PHP_PULSEFLOW
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for PulseFlow files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       PULSEFLOW_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$PULSEFLOW_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the PulseFlow distribution])
  dnl fi

  dnl # --with-PulseFlow -> add include path
  dnl PHP_ADD_INCLUDE($PULSEFLOW_DIR/include)

  dnl # --with-PulseFlow -> check for lib and symbol presence
  dnl LIBNAME=PulseFlow # you may want to change this
  dnl LIBSYMBOL=PulseFlow # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $PULSEFLOW_DIR/$PHP_LIBDIR, PULSEFLOW_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_PULSEFLOWLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong PulseFlow lib version or lib not found])
  dnl ],[
  dnl   -L$PULSEFLOW_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(PULSEFLOW_SHARED_LIBADD)

  PHP_NEW_EXTENSION(PulseFlow, PulseFlow.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
