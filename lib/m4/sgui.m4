# check for libsgui
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_SGUI],
[if test -d "$srcdir/libsgui"
then
  libsgui="shipped"
else
  libsgui="installed"
fi
AC_ARG_WITH(libsgui,
[  --with-libsgui=installed          (default)
   --with-libsgui=shipped],
 [
  case "$withval" in
    installed)
      libsgui="installed"
      ;;
    shipped)
      libsgui="shipped"
      ;;
    no)
      libsgui="shipped"
      ;;
    yes)
      libsgui="installed"
      ;;
  esac])

if test "$libsgui" = "installed"
then
  AM_PATH_SGUI([2.0.0], libsgui="installed", libsgui="shipped")
fi

if test "$libsgui" = "shipped"
then
  if test -d "$srcdir/libsgui"
  then
    AC_CONFIG_SUBDIRS([libsgui])
    SGUI_CFLAGS="-isystem \$(top_srcdir)/libsgui/include -isystem \$(top_builddir)/libsgui/include"
    SGUI_LIBS="\$(top_builddir)/libsgui/src/libsgui.a"
    SGUI="libsgui"
  else
    AC_MSG_ERROR([No shipped and no installed libsgui!])
  fi
fi
AC_SUBST([SGUI_CFLAGS])
AC_SUBST([SGUI_LIBS])
AC_SUBST([SGUI])
 ])


