# check for OpenSSL
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_SSL],
[AC_MSG_CHECKING(whether to compile with SSL support)
ac_cv_ssl="auto"
AC_ARG_WITH(ssl,
[  --with-ssl[[=yes|no|auto]]   OpenSSL support [[auto]]],
[
  case "$enableval" in
    y*) ac_cv_ssl="yes" ;;
    n*) ac_cv_ssl="no" ;;
    *) ac_cv_ssl="auto" ;;
  esac
])
AC_MSG_RESULT($ac_cv_ssl)

SSL_LIBS=""
SSL_CFLAGS=""
OPENSSL=""
if test "$ac_cv_ssl" = "yes" || test "$ac_cv_ssl" = "auto"
then
  saved_libs="$LIBS"
  AC_CHECK_LIB(crypto, ERR_load_crypto_strings)

  if test "$ac_cv_lib_crypto_ERR_load_crypto_strings" = "no" && test "$ac_cv_ssl" = "yes"
  then
    AC_MSG_ERROR(could not find libcrypto, install openssl >= 0.9.7)
    exit 1
  fi

  if test "$ac_cv_lib_crypto_ERR_load_crypto_strings" = "yes"
  then
    SSL_LIBS="-lcrypto"
  fi

  LIBS="$SSL_LIBS $saved_libs"
  AC_CHECK_LIB(ssl, SSL_load_error_strings)

  if test "$ac_cv_lib_ssl_SSL_load_error_strings" = "no" && test "$ac_cv_ssl" = "yes"
  then
    AC_MSG_ERROR(could not find libssl, install openssl >= 0.9.7)
    exit 1
  fi

  if test "$ac_cv_lib_ssl_SSL_load_error_strings" = "yes"
  then
    SSL_LIBS="-lssl $SSL_LIBS"
  fi

  LIBS="$saved_libs"
  AC_CHECK_HEADERS(openssl/opensslv.h)

  if test "$ac_cv_header_openssl_opensslv_h" = "no" && test "$ac_cv_ssl" = "yes"
  then
    AC_MSG_ERROR(could not find openssl/opensslv.h, install openssl >= 0.9.7)
    exit 1
  fi

  AC_MSG_CHECKING(for the OpenSSL UI)

  OPENSSL=`which openssl 2>/dev/null`

  if test "x$OPENSSL" = "x"
  then
    if test -f /usr/bin/openssl
    then
      OPENSSL=/usr/bin/openssl
      AC_MSG_RESULT($OPENSSL)
    else
      AC_MSG_RESULT(not found)
    fi
  else
    AC_MSG_RESULT($OPENSSL)
  fi

  if test "x$OPENSSL" = "x" && test "$ac_cv_ssl" = "yes"
  then
    AC_MSG_ERROR(could not find OpenSSL command line tool, install openssl >= 0.9.7)
    exit 1
  fi

  if test "$ac_cv_lib_crypto_ERR_load_crypto_strings" = "yes" && test "$ac_cv_lib_ssl_SSL_load_error_strings" = "yes" && test "$ac_cv_header_openssl_opensslv_h" = "yes"
  then
    HAVE_SSL=yes
    AC_DEFINE_UNQUOTED(HAVE_SSL, 1, [Define this if you have OpenSSL])
  else
    HAVE_SSL=no
    SSL_LIBS=""
    SSL_CFLAGS=""
    OPENSSL=""
  fi
fi

AC_SUBST(SSL_LIBS)
AC_SUBST(SSL_CFLAGS)
])                        


