# check for libchaos
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_CHAOS],
[if test -d $srcdir/libchaos
then
  libchaos="shipped"
else
  libchaos="installed"
fi

AC_ARG_WITH(libchaos,
[  --with-libchaos=installed          [(default)]
  --with-libchaos=shipped],
[
  case "$withval" in
  installed)
    libchaos="installed"
    ;;
  shipped)
    libchaos="shipped"
    ;;
  no)
    libchaos="shipped"
    ;;
  yes)
    libchaos="installed"
    ;;
  esac
])

if test $libchaos = "installed"
then
  AM_PATH_CHAOS([2.1.0], libchaos="installed", libchaos="shipped")
fi

if test $libchaos = "shipped"
then
  if test -d $srcdir/libchaos
  then
    AC_CONFIG_SUBDIRS(libchaos)
    SUBDIRS="libchaos $SUBDIRS"
    CHAOS_CFLAGS="-isystem \$(top_srcdir)/libchaos/include -isystem \$(top_builddir)/libchaos/include"
    CHAOS_LIBS="../libchaos/src/libchaos.a -ldl -lm"
  else
    AC_MSG_ERROR([No shipped and no installed libchaos!])
  fi
fi
AC_SUBST([CHAOS_CFLAGS])
AC_SUBST([CHAOS_LIBS])
])

