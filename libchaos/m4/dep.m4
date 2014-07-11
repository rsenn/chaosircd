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
[  --enable-dependency-tracking  dependency tracking
  --disable-dependency-tracking no dependency tracking (default)],
[case $enableval in
  yes)
    DEPENDENCY_TRACKING="yes"
    AC_MSG_RESULT([yes])
    ;;
  *)
    DEPENDENCY_TRACKING="no"
    AC_MSG_RESULT([no])
    ;;
  esac], 
  [DEPENDENCY_TRACKING="no"
  AC_MSG_RESULT([no])])
if test "$DEPENDENCY_TRACKING" = "yes"; then
  NO_DEPENDENCY_TRACKING=""
else
  NO_DEPENDENCY_TRACKING="# "
fi
AC_SUBST(NO_DEPENDENCY_TRACKING)
AC_SUBST(DEPENDENCY_TRACKING)])
