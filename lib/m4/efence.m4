# check for electric-fence
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_EFENCE], [
AC_MSG_CHECKING(whether to compile with Electric Fence support)
ac_cv_efence="auto"
AC_ARG_WITH(efence,
[  --with-efence[[=yes|no|auto]]   Electric Fence support [[auto]]],
[
  case "$enableval" in
    y*) ac_cv_efence="yes" ;;
    n*) ac_cv_efence="no" ;;
    *) ac_cv_efence="auto" ;;
  esac
])
AC_MSG_RESULT($ac_cv_efence)

if test "$ac_cv_efence" = yes; then
  EFENCE_LIBS="-lefence"
fi

AM_CONDITIONAL([EFENCE], [test "$ac_cv_efence" = yes])
AC_SUBST([EFENCE_CFLAGS])
AC_SUBST([EFENCE_LIBS])
])

