# $Id: maintainer.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# Sophisticated debugging mode macro for autoconf
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for maintainer mode
# ---------------------------------------------------------------------------
AC_DEFUN([AC_CHECK_MAINTAINER],
[ac_cv_maintainer="no"

AC_MSG_CHECKING([whether to enable maintainer mode])

AC_ARG_ENABLE([maintainer],
[  --enable-maintainer      maintainer mode
  --disable-maintainer     no maintainer mode (default)],
[case "$enableval" in
  yes)
    ac_cv_maintainer="yes"
    ;;
  no)
    ac_cv_maintainer="no"
    ;;
esac])

if test "$ac_cv_maintainer" = "yes"; then
  MAINTAINER_MODE=""
  NO_MAINTAINER_MODE="#"
  AC_MSG_RESULT([yes])
else
  MAINTAINER_MODE="#"
  NO_MAINTAINER_MODE=""
  AC_MSG_RESULT([no])
fi
  
AC_SUBST(MAINTAINER_MODE)
AC_SUBST(NO_MAINTAINER_MODE)
])
