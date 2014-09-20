# $Id: werror.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# Treat warnings as errors
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for werror mode
# ---------------------------------------------------------------------------
AC_DEFUN([AC_CHECK_WERROR], 
[ac_cv_werror="no"

AC_MSG_CHECKING([whether to enable werrorging mode])

AC_ARG_ENABLE([werror],
[  --enable-werror          treat warnings as errors],
[case "$enableval" in
  yes) ac_cv_werror=yes ;;
  no | *) ac_cv_werror=no ;;
esac])

# set WERROR variable
if test "$ac_cv_werror" = "yes"; then
  CFLAGS=`echo $CFLAGS -Werror`
  WERROR=yes
  AC_MSG_RESULT([yes])
else
  CPPFLAGS=`echo $CPPFLAGS | sed 's|\s*-Werror\s*| |g'`
  CFLAGS=`echo $CFLAGS | sed 's|\s*-Werror\s*| |g'`
  WERROR=no
  AC_MSG_RESULT([no])
fi
  
AC_SUBST(WERROR)])
