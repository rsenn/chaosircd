# check for OpenSSL
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_SSL],
[AC_MSG_CHECKING([whether to compile with SSL support])


ac_cv_with_ssl="/usr /usr/local"
AC_ARG_WITH([ssl],
[  --with-ssl[[=yes|no|auto]]   OpenSSL support [[auto]]],
[
  case "$withval" in
    yes) ;;
    no) ac_cv_with_ssl="no" ;;
    *) ac_cv_with_ssl="$withval" ;;
  esac
])
AC_MSG_RESULT([$ac_cv_with_ssl])

SSL_LIBS=""
SSL_CFLAGS=""
OPENSSL=""

if test "$ac_cv_with_ssl" != no
then

  for ac_cv_ssldir in $ac_cv_with_ssl; do
    if test -d "$ac_cv_ssldir/include/openssl"; then
      SSL_INCLUDEDIR=$ac_cv_ssldir/include
      break
    fi
  done

  for ac_cv_ssldir in $ac_cv_with_ssl; do
    for l in $ac_cv_ssldir/lib64 $ac_cv_ssldir/lib/x86_64* $ac_cv_ssldir/lib; do
      if test -e "$l/libssl.a" -o -e "$l/libssl.dll.a" -o -e "$l/libssl.so"; then
        SSL_LIBDIR=$l
        break 2
      fi
    done
  done

  SSL_LIBS="${SSL_LIBDIR:+-L$SSL_LIBDIR}"
  SSL_CFLAGS="${SSL_INCLUDEDIR:+-I$SSL_INCLUDEDIR}"

  saved_LIBS="$LIBS"
  saved_CPPFLAGS="$CFLAGS"

  LIBS="$LIBS $SSL_LIBS"
  CPPFLAGS="$CPPFLAGS $SSL_CFLAGS"

  AC_CHECK_LIB([crypto], [ERR_load_crypto_strings])

  if test "$ac_cv_lib_crypto_ERR_load_crypto_strings" = no -a "$ac_cv_with_ssl" = yes; then
    AC_MSG_ERROR([could not find libcrypto, install openssl >= 0.9.7])
    exit 1
  fi

  if test "$ac_cv_lib_crypto_ERR_load_crypto_strings" = yes; then
    SSL_LIBS="-lcrypto"
  fi

  AC_CHECK_LIB([ssl], [SSL_load_error_strings])

  if test "$ac_cv_lib_ssl_SSL_load_error_strings" = no -a "$ac_cv_with_ssl" = yes; then
    AC_MSG_ERROR([could not find libssl, install openssl >= 0.9.7])
    exit 1
  fi

  if test "$ac_cv_lib_ssl_SSL_load_error_strings" = yes; then
    SSL_LIBS="$SSL_LIBS -lssl"
  fi

  AC_CHECK_HEADER([openssl/opensslv.h],[],[],[-])

  if test "$ac_cv_header_openssl_opensslv_h" = "no" -a "$ac_cv_with_ssl" = "yes"
  then
    AC_MSG_ERROR([could not find openssl/opensslv.h, install openssl >= 0.9.7])
    exit 1
  fi

  AC_MSG_CHECKING([for the OpenSSL UI])

  OPENSSL=`which openssl 2>/dev/null`

  if test "x$OPENSSL" = "x"
  then
    if test -f /usr/bin/openssl
    then
      OPENSSL=/usr/bin/openssl
      AC_MSG_RESULT([$OPENSSL])
    else
      AC_MSG_RESULT([not found])
    fi
  else
    AC_MSG_RESULT([$OPENSSL])
  fi

  if test "x$OPENSSL" = "x" -a "$ac_cv_with_ssl" = "yes"
  then
    AC_MSG_ERROR([could not find OpenSSL command line tool, install openssl >= 0.9.7])
    exit 1
  fi

  LIBS="$saved_LIBS"
  CPPFLAGS="$saved_CPPFLAGS"

  if test "$ac_cv_lib_crypto_ERR_load_crypto_strings" = "yes" -a "$ac_cv_lib_ssl_SSL_load_error_strings" = "yes" -a "$ac_cv_header_openssl_opensslv_h" = "yes"
  then
    HAVE_SSL=yes
    AC_DEFINE_UNQUOTED([HAVE_SSL], [1], [Define this if you have OpenSSL])
  else
    HAVE_SSL=no
    SSL_LIBS=""
    SSL_CFLAGS=""
    OPENSSL=""
  fi
fi

if test "$ac_cv_with_ssl_link" = static; then
  SSL_LIBS="${SSL_PREFIX}/lib/libssl.a ${SSL_PREFIX}/lib/libcrypto.a"
fi

AM_CONDITIONAL([SSL_STATIC],[test "$ac_cv_with_ssl_link" = static])
AM_CONDITIONAL([SSL],[test "$ac_cv_with_ssl" != no])
AC_SUBST([SSL_PREFIX])
AC_SUBST([SSL_LIBS])
AC_SUBST([SSL_CFLAGS])
])                        

