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
[  --enable-dep             dependency tracking
  --disable-dep            no dependency tracking (default)],
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
  NODEP=""
else
  NODEP="# "
fi
AC_SUBST(NODEP)
AC_SUBST(DEP)])
