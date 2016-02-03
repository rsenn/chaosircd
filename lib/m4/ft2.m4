# check for Freetype 2
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_FT2],
[dnl Check for the FreeType 2 library
dnl
dnl Get the cflags and libraries from the freetype-config script
dnl
HAVE_FT2="no"
AC_ARG_WITH(ft2,[  --with-ft2[[=prefix]]      Support for FreeType 2 fonts],
            ft2_prefix="$withval", ft2_prefix="")

if test x$ft2_exec_prefix != x ; then
     ft2_args="$ft2_args --exec-prefix=$ft2_exec_prefix"
     if test x${FREETYPE_CONFIG+set} != xset ; then
        FREETYPE_CONFIG=$ft2_exec_prefix/bin/freetype-config
     fi
fi
if test x$ft2_prefix != x ; then
     ft2_args="$ft2_args --prefix=$ft2_prefix"
     if test x${FREETYPE_CONFIG+set} != xset ; then
        FREETYPE_CONFIG=$ft2_prefix/bin/freetype-config
     fi
     AC_PATH_PROG(FREETYPE_CONFIG, freetype-config, no)
     no_ft2=""
     if test "$FREETYPE_CONFIG" = "no" ; then
       AC_MSG_ERROR([
       *** Unable to find FreeType2 library (http://www.freetype.org/)
       ])
     else
       FT2_CFLAGS="`$FREETYPE_CONFIG $ft2conf_args --cflags`"
       FT2_LIBS="`$FREETYPE_CONFIG $ft2conf_args --libs`"

      TTFSUPPORT="ft2"
      AC_DEFINE_UNQUOTED(HAVE_FT2, "1", [Define this if you have freetype 2])
      HAVE_FT2="yes"
     fi
fi

AC_SUBST(FT2_CFLAGS)
AC_SUBST(FT2_LIBS)])
