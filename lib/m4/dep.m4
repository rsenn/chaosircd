# $Id: dep.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# Implement dependency tracking
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for dependencies
# ---------------------------------------------------------------------------
AC_DEFUN([AC_CHECK_DEP], 
[AC_MSG_CHECKING([whether to enable dependencies])

AC_ARG_ENABLE([dep],
[  --enable-dependency-tracking             dependency tracking
  --disable-dependency-tracking            no dependency tracking (default)],
[case $enableval in
  yes)
    DEP="yes"
    AC_MSG_RESULT([yes])
    ;;
  *)
    DEP="no"
    AC_MSG_RESULT([no])
    ;;
  esac], 
  [DEP="no"
  AC_MSG_RESULT([no])])
if test "$DEP" = "yes"; then
  DEP_DISABLED="#"
else
  DEP_ENABLED="# "
fi
AM_CONDITIONAL([DEPS],[test "$DEP" = yes])
AC_SUBST(DEP_ENABLED,[$DEP_ENABLED])
AC_SUBST(DEP_DISABLED, [$DEP_DISABLED])
AC_SUBST(DEP)])