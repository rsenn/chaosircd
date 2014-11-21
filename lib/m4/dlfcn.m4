# check for libdl
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_DLFCN], [
AC_CHECK_HEADER([dlfcn.h], [ac_cv_have_dlfcn_h=yes
AC_DEFINE_UNQUOTED([HAVE_DLFCN_H], [1], [Define this if you have <dlfcn.h>])
])
AC_CHECK_LIB([dl],[dlopen], [ac_cv_have_libdl_dlopen=yes])

if test "$ac_cv_have_dlfcn_h" = yes -a "$ac_cv_have_libdl_dlopen" = yes; then
  HAVE_DLFCN_DLOPEN=yes
fi

AM_CONDITIONAL([DLFCN], [test "$HAVE_DLFCN_DLOPEN" = yes])

])

