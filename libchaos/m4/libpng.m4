# Configure paths for libpng
# Sam Lantinga 9/21/99
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

AC_DEFUN([AC_CHECK_LIBPNG],
[dnl 
dnl Get the cflags and libraries from the libpng-config script
dnl
AC_ARG_WITH(libpng-prefix,[  --with-libpng-prefix=PFX   Prefix where libpng is installed (optional)],
            libpng_prefix="$withval", libpng_prefix="")
AC_ARG_WITH(libpng-exec-prefix,[  --with-libpng-exec-prefix=PFX Exec prefix where libpng is installed (optional)],
            libpng_exec_prefix="$withval", libpng_exec_prefix="")

  if test x$libpng_exec_prefix != x ; then
     libpng_args="$libpng_args --exec-prefix=$libpng_exec_prefix"
     if test x${LIBPNG_CONFIG+set} != xset ; then
        LIBPNG_CONFIG=$libpng_exec_prefix/bin/libpng-config
     fi
  fi
  if test x$libpng_prefix != x ; then
     libpng_args="$libpng_args --prefix=$libpng_prefix"
     if test x${LIBPNG_CONFIG+set} != xset ; then
        LIBPNG_CONFIG=$libpng_prefix/bin/libpng-config
     fi
  fi

  PATH="$prefix/bin:$prefix/usr/bin:$PATH"
  AC_PATH_PROG(LIBPNG_CONFIG, libpng-config, no, [$PATH])
  AC_MSG_CHECKING(for libpng)
  no_libpng=""
  if test "$LIBPNG_CONFIG" = "no" ; then
    no_libpng=yes
  else
    LIBPNG_CFLAGS=`$LIBPNG_CONFIG --cflags`
    LIBPNG_LIBS=`$LIBPNG_CONFIG --libs`


    libpng_version=`$LIBPNG_CONFIG --version`
  fi

  AC_MSG_RESULT([$libpng_version])

  AC_SUBST([LIBPNG_CFLAGS])
  AC_SUBST([LIBPNG_LIBS])
])
