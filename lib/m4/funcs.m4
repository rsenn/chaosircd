# $Id: funcs.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# Macros checking for system depedencies.
# Do so without preprocessor tests, because the preprocessor is almost
# never invoked separately
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

# check for PostgreSQL
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_PSQL],
[default_directory="/usr /usr/local /usr/pgsql /usr/local/pgsql"

AC_ARG_WITH(pgsql,
    [  --with-pgsql=DIR        support for PostgreSQL],
    [ with_pgsql="$withval" ],
    [ with_pgsql=no ])

if test "$with_pgsql" != no; then
  if test "$with_pgsql" = yes; then
    pgsql_directory="$default_directory "
    pgsql_fail=yes
  elif test -d $withval; then
    pgsql_directory="$withval $default_directory"
    pgsql_fail=yes
  elif test "$with_pgsql" = ""; then
    pgsql_directory="$default_directory"
    pgsql_fail=no
  fi

  AC_MSG_CHECKING(for PostgreSQL headers)

  for i in $pgsql_directory; do
    if test -r $i/include/pgsql/libpq-fe.h; then
      PGSQL_DIR=$i
      PGSQL_INC_DIR=$i/include/pgsql
    elif test -r $i/include/libpq-fe.h; then
      PGSQL_DIR=$i
      PGSQL_INC_DIR=$i/include
    elif test -r $i/include/postgresql/libpq-fe.h; then
      PGSQL_DIR=$i
      PGSQL_INC_DIR=$i/include/postgresql
    fi
    test -d "$PSQL_INC_DIR" && break
  done

  if test ! -d "$PGSQL_INC_DIR"; then
    AC_MSG_ERROR("PostgreSQL header file (libpq-fe.h)", "$PGSQL_DIR $PGSQL_INC_DIR")
  else
    AC_MSG_RESULT($MYSQL_INC_DIR)
  fi

  AC_MSG_CHECKING(for PostgreSQL client library)

  for i in lib lib/pgsql; do
    str="$PGSQL_DIR/$i/libpq.*"
    for j in `echo $str`; do
      if test -r $j; then
          PGSQL_LIB_DIR="$PGSQL_DIR/$i"
          break 
      fi
    done
      test -d "$PSQL_LIB_DIR" && break
  done

    if test -z "$PGSQL_LIB_DIR"; then
      if test "$postgresql_fail" != no; then
        AC_MSG_ERROR("PostgreSQL library libpq",
        "$PGSQL_DIR/lib $PGSQL_DIR/lib/pgsql")
      else
        AC_MSG_RESULT(no);
      fi
    else
      AC_MSG_RESULT($PGSQL_LIB_DIR)
      PSQL=true
      PSQL_LIBS="-L${PGSQL_LIB_DIR} -lpq"
      PSQL_CFLAGS="-I${PGSQL_INC_DIR}"
      DBSUPPORT="$DBSUPPORT PostgreSQL"
      AC_DEFINE_UNQUOTED(HAVE_PGSQL, "1", [Define this if you have PostgreSQL])
    fi

fi
AM_CONDITIONAL([PSQL],[test "$PSQL" = true])
AC_SUBST(PSQL_LIBS)
AC_SUBST(PSQL_CFLAGS)])

# check for SQLite
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_SQLITE],
[default_directory="/usr /usr/local" 

with_sqlite=yes
AC_ARG_WITH(sqlite,
    [  --with-sqlite=DIR        support for SQLite],
    [ with_sqlite="$withval" ],
    [ with_sqlite=no ])

if test "$with_sqlite" != no; then
  if test "$with_sqlite" = yes; then
    sqlite_directory="$default_directory";
    sqlite_fail=yes
  elif test -d $withval; then
    sqlite_directory="$withval"
    sqlite_fail=no
  elif test "$with_sqlite" = ""; then
    sqlite_directory="$default_directory";
    sqlite_fail=no
  fi

  AC_MSG_CHECKING(for SQLite headers)

  for i in $sqlite_directory; do
    if test -r $i/include/sqlite/sqlite3.h; then
      SQLITE_DIR=$i
      SQLITE_INC_DIR=$i/include/sqlite
    elif test -r $i/include/sqlite3.h; then
      SQLITE_DIR=$i
      SQLITE_INC_DIR=$i/include
    fi
    test -d "$SQLITE_INC_DIR" && break
  done

  if test ! -d "$SQLITE_INC_DIR"; then
    AC_MSG_RESULT(not found)
    with_sqlite=no
#    FAIL_MESSAGE("SQLite Headers", "$SQLITE_DIR $SQLITE_INC_DIR")
  else
    AC_MSG_RESULT($SQLITE_INC_DIR)
  fi

  AC_MSG_CHECKING(for SQLite client library)

  for i in lib64 lib lib64/sqlite lib/sqlite; do
    str="$SQLITE_DIR/$i/libsqlite3.*"
    for j in `echo $str`; do
      if test -r $j; then
        SQLITE_LIB_DIR="$SQLITE_DIR/$i"
      fi
      test -d "$SQLITE_LIB_DIR" && break
    done
    test -d "$SQLITE_LIB_DIR" && break
  done

  if test -z "$SQLITE_LIB_DIR"; then
    if test "$sqlite_fail" != no; then
      AC_MSG_RESULT(not found)
      with_sqlite=no
#      FAIL_MESSAGE("sqlite library",
#                   "$SQLITE_LIB_DIR")
    else
      AC_MSG_RESULT(no)
    fi
  else
    AC_MSG_RESULT($SQLITE_LIB_DIR)
    SQLITE=true
    SQLITE_LIBS="-L${SQLITE_LIB_DIR} -Wl,-rpath,${SQLITE_LIB_DIR} -lsqlite3"
    SQLITE_CFLAGS="-I${SQLITE_INC_DIR}"
    AC_CHECK_LIB(z, compress)
    DBSUPPORT="$DBSUPPORT SQLite"
    AC_DEFINE_UNQUOTED(HAVE_SQLITE, "1", [Define this if you have SQLite])
  fi
fi
AM_CONDITIONAL([SQLITE],[test "$SQLITE" = true])
AC_SUBST([SQLITE_LIBS])
AC_SUBST([SQLITE_CFLAGS])
])

# check for MySQL
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_MYSQL],
[default_directory="/usr /usr/local /usr/mysql /opt/mysql"

with_mysql=yes
AC_ARG_WITH(mysql,
    [  --with-mysql=DIR        support for MySQL],
    [ with_mysql="$withval" ],
    [ with_mysql=no ])

if test "$with_mysql" != no; then
  if test "$with_mysql" = yes; then
    mysql_directory="$default_directory";
    mysql_fail=yes
  elif test -d $withval; then
    mysql_directory="$withval"
    mysql_fail=no
  elif test "$with_mysql" = ""; then
    mysql_directory="$default_directory";
    mysql_fail=no
  fi

  AC_MSG_CHECKING(for MySQL headers)

  for i in $mysql_directory; do
    if test -r $i/include/mysql/mysql.h; then
      MYSQL_DIR=$i
      MYSQL_INC_DIR=$i/include/mysql
    elif test -r $i/include/mysql.h; then
      MYSQL_DIR=$i
      MYSQL_INC_DIR=$i/include
    fi
    test -n "$MYSQL_INC_DIR" -a -d "$MYSQL_INC_DIR" && break
  done

  if test ! -d "$MYSQL_INC_DIR"; then
    AC_MSG_RESULT(not found)
    with_mysql=no
#    FAIL_MESSAGE("MySQL Headers", "$MYSQL_DIR $MYSQL_INC_DIR")
  else
    AC_MSG_RESULT($MYSQL_INC_DIR)
  fi

  AC_MSG_CHECKING(for MySQL client library)
	MYSQL_LIB_NAME="mysqlclient"

  for i in lib64 lib/x86_64-linux-gnu lib64/mysql lib lib/i386-linux-gnu lib/mysql; do
    str="$MYSQL_DIR/$i/lib{mariadb,mysql}client.*"
    for j in `eval echo $str`; do
      if test -r $j; then
				case "$j" in
				  *mariadb*) MYSQL_LIB_NAME="mariadbclient" ;;
				esac
        MYSQL_LIB_DIR="$MYSQL_DIR/$i"
        break
      fi
    done
    test -d "$MYSQL_LIB_DIR" && break
  done

  if test -z "$MYSQL_LIB_DIR" -o -z "$MYSQL_INC_DIR"; then
    if test "$mysql_fail" != no; then
      AC_MSG_RESULT(not found)
      with_mysql=no
#      FAIL_MESSAGE("mysqlclient library",
#                   "$MYSQL_LIB_DIR")
    else
      AC_MSG_RESULT(no)
    fi
    MYSQL=false
  else
    AC_MSG_RESULT($MYSQL_LIB_DIR)
    MYSQL=true
    MYSQL_LIBS="-L${MYSQL_LIB_DIR} -Wl,-rpath,${MYSQL_LIB_DIR} -l${MYSQL_LIB_NAME} -lpthread -ldl -lm -lz"
    MYSQL_CFLAGS="-I${MYSQL_INC_DIR}"
    AC_CHECK_LIB(z, compress)
    DBSUPPORT="$DBSUPPORT MySQL"
    AC_DEFINE_UNQUOTED(HAVE_MYSQL, "1", [Define this if you have MySQL])
  fi
fi
AM_CONDITIONAL([MYSQL],[test "$MYSQL" = true])
AC_SUBST([MYSQL_LIBS])
AC_SUBST([MYSQL_CFLAGS])
])

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

if test "$ac_cv_ssl_link" = static; then
  SSL_LIBS="${SSL_PREFIX}/lib/libssl.a ${SSL_PREFIX}/lib/libcrypto.a"
fi

AM_CONDITIONAL([SSL_STATIC],[test "$ac_cv_ssl_link" = static])
AM_CONDITIONAL([SSL],[test "$ac_cv_ssl" != no])
AC_SUBST(SSL_PREFIX)
AC_SUBST(SSL_LIBS)
AC_SUBST(SSL_CFLAGS)
])                        

# check for libowfat
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_LIBOWFAT], [
 AC_MSG_CHECKING([for libowfat prefix])


ac_cv_with_libowfat=/usr
AC_ARG_WITH(libowfat,
[  --with-libowfat[[=DIR]]   libowfat [[auto]]],
[
  case "$withval" in
    yes|no ) ;;
    *) ac_cv_with_libowfat="$withval" ;;
  esac
])
AC_MSG_RESULT($ac_cv_with_libowfat)


old_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I${ac_cv_with_libowfat}/include"
AC_CHECK_HEADER([stralloc.h], [ac_cv_libowfat_stralloc_h_found=yes])
AC_CHECK_HEADER([libowfat/stralloc.h], [LIBOWFAT_CFLAGS="-I${ac_cv_with_libowfat}/include/libowfat"; ac_cv_libowfat_stralloc_h_found=yes])
CPPFLAGS="$old_CPPFLAGS"
AC_MSG_CHECKING([for libowfat])

if test "$ac_cv_libowfat_stralloc_h_found" = yes; then
  AC_MSG_RESULT([yes])

	for DIR in "${ac_cv_with_libowfat}/lib/${host:+dummy}/" "${ac_cv_with_libowfat}/lib64" "${ac_cv_with_libowfat}/lib"; do
		if test -d "$DIR" ; then
			LIBOWFAT_LIBDIR="$DIR"
		  break
		fi
	done

  LIBOWFAT_LIBS="-lowfat"
	if test -n "$LIBOWFAT_LIBDIR"; then
		LIBOWFAT_LIBS="-L${LIBOWFAT_LIBDIR} $LIBOWFAT_LIBS"
	fi
else
  AC_MSG_ERROR([Need libowfat!])
  dnl AC_MSG_RESULT([no])
fi

AM_CONDITIONAL([LIBOWFAT], [test "$ac_cv_libowfat_stralloc_h_found" = yes])
AC_SUBST([LIBOWFAT_CFLAGS])
AC_SUBST([LIBOWFAT_LIBS])
])

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

# check for termios
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_TERMIOS],
[AC_SYS_POSIX_TERMIOS
if test "$ac_cv_sys_posix_termios" = "yes"; then
  AC_DEFINE_UNQUOTED([HAVE_TERMIOS], 1, 
  [Define this if you have the POSIX termios library])
fi
])

# check for if_indextoname
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_N2I],
[AC_MSG_CHECKING([for if_indextoname])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

int main() {
  static char ifname[IFNAMSIZ];
  char *tmp=if_indextoname(0,ifname);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_N2I], 1,
[Define this if you have the if_indextoname() call])], [AC_MSG_RESULT([no])])])

# check for IPv6 scope ids
# ------------------------------------------------------------------
AC_DEFUN([AC_SCOPE_ID],
[AC_MSG_CHECKING([for ipv6 scope ids])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

main() {
  struct sockaddr_in6 sa;
  sa.sin6_family = PF_INET6;
  sa.sin6_scope_id = 23;
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_SCOPE_ID], 1,
[Define this if your libc supports ipv6 scope ids])], [AC_MSG_RESULT([no])])])


# check for IPv6 support
# ------------------------------------------------------------------
AC_DEFUN([AC_IPV6],
[AC_MSG_CHECKING([for ipv6 support])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

main() {
  struct sockaddr_in6 sa;
  sa.sin6_family = PF_INET6;
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_IPV6], 1,
[Define this if your libc supports ipv6])
AC_DEFINE_UNQUOTED([LIBC_HAS_IP6], 1,
[Define this if your libc supports ipv6])], [AC_MSG_RESULT([no])])])


# check for alloca() function and header
# ------------------------------------------------------------------
AN_HEADER([alloca.h], [AC_FUNC_ALLOCA])
AC_DEFUN([AC_FUNC_ALLOCA],
[AC_MSG_CHECKING([for alloca])
AC_COMPILE_IFELSE([
#include <stdlib.h>
#include <alloca.h>
dnl
main() {
  char* c=alloca(23);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_ALLOCA], 1,
[Define this if your compiler supports alloca()])], [AC_MSG_RESULT([no])])])


# check for epoll() system call and headers
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_EPOLL],
[AC_MSG_CHECKING([for epoll])
AC_COMPILE_IFELSE([
  int efd=epoll_create(10);
  struct epoll_event x;
  if (efd==-1) return 111;
  x.events=EPOLLIN;
  x.data.fd=0;
  if (epoll_ctl(efd,EPOLL_CTL_ADD,0 /* fd */,&x)==-1) return 111;
  {
    int i,n;
    struct epoll_event y[100];
    if ((n=epoll_wait(efd,y,100,1000))==-1) return 111;
    if (n>0)
      printf("event %d on fd #%d\n",y[0].events,y[0].data.fd);
  }
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_EPOLL], 1,
[Define this if your OS supports epoll()])], [AC_MSG_RESULT([no])])])

# check for select() system call and headers
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_SELECT],
[AC_MSG_CHECKING([for select])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/time.h>

#include <string.h>     /* BSD braindeadness */

#include <sys/select.h> /* SVR4 silliness */

int main()
{
  fd_set x;
  int ret;
  struct timeval tv;
  tv.tv_sec = 2; tv.tv_usec = 0;
  FD_ZERO(&x); FD_SET(22, &x);
  ret = select(23, &x, &x, NULL, &tv);
  if(FD_ISSET(22, &x)) FD_CLR(22, &x);
}
], [$1
AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_SELECT], 1,
[Define this if your OS supports select()])], [$2
AC_MSG_RESULT([no])])])

# check for poll() system call
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_POLL],
[AC_MSG_CHECKING([for poll])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>

int main()
{
  struct pollfd x;

  x.fd = open("trypoll.c",O_RDONLY);
  if (x.fd == -1) _exit(111);
  x.events = POLLIN;
  if (poll(&x,1,10) == -1) _exit(1);
  if (x.revents != POLLIN) _exit(1);

  /* XXX: try to detect and avoid poll() imitation libraries */

  _exit(0);
}
], [$1
AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_POLL], 1,
[Define this if your OS supports poll()])], [$2
AC_MSG_RESULT([no])])])

# check for kqueue functionality
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_KQUEUE],
[AC_MSG_CHECKING([for kqueue])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int main() {
  int kq=kqueue();
  struct kevent kev;
  struct timespec ts;
  if (kq==-1) return 111;
  EV_SET(&kev, 0 /* fd */, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, 0);
  ts.tv_sec=0; ts.tv_nsec=0;
  if (kevent(kq,&kev,1,0,0,&ts)==-1) return 111;

  {
    struct kevent events[100];
    int i,n;
    ts.tv_sec=1; ts.tv_nsec=0;
    switch (n=kevent(kq,0,0,events,100,&ts)) {
    case -1: return 111;
    case 0: puts("no data on fd #0"); break;
    }
    for (i=0; i<n; ++i) {
      printf("ident %d, filter %d, flags %d, fflags %d, data %d\n",
             events[i].ident,events[i].filter,events[i].flags,
             events[i].fflags,events[i].data);
    }
  }
  return 0;
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_KQUEUE], 1,
[Define this if your OS supports kqueues])], [AC_MSG_RESULT([no])])])

# check for sigio functionality
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_SIGIO],
[AC_MSG_CHECKING([for sigio])
AC_COMPILE_IFELSE([
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/poll.h>
#include <signal.h>
#include <fcntl.h>

int main() {
  int signum=SIGRTMIN+1;
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss,signum);
  sigaddset(&ss,SIGIO);
  sigprocmask(SIG_BLOCK,&ss,0);

  fcntl(0 /* fd */,F_SETOWN,getpid());
  fcntl(0 /* fd */,F_SETSIG,signum);
#if defined(O_ONESIGFD) && defined(F_SETAUXFL)
  fcntl(0 /* fd */, F_SETAUXFL, O_ONESIGFD);
#endif
  fcntl(0 /* fd */,F_SETFL,fcntl(0 /* fd */,F_GETFL)|O_NONBLOCK|O_ASYNC);

  {
    siginfo_t info;
    struct timespec timeout;
    int r;
    timeout.tv_sec=1; timeout.tv_nsec=0;
    switch ((r=sigtimedwait(&ss,&info,&timeout))) {
    case SIGIO:
      /* signal queue overflow */
      signal(signum,SIG_DFL);
      /* do poll */
      break;
    default:
      if (r==signum) {
  printf("event %c%c on fd #%d\n",info.si_band&POLLIN?'r':'-',info.si_band&POLLOUT?'w':'-',info.si_fd);
      }
    }
  }
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_SIGIO], 1,
[Define this if your OS supports I/O signales])], [AC_MSG_RESULT([no])])])

# check for /dev/poll i/o multiplexing 
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_DEVPOLL],
[AC_MSG_CHECKING([for /dev/poll])
AC_COMPILE_IFELSE([
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <sys/devpoll.h>

main() {
  int fd=open("/dev/poll",O_RDWR);
  struct pollfd p[100];
  int i,r;
  dvpoll_t timeout;
  p[0].fd=0;
  p[0].events=POLLIN;
  write(fd,p,sizeof(struct pollfd));
  timeout.dp_timeout=100;       /* milliseconds? */
  timeout.dp_nfds=1;
  timeout.dp_fds=p;
  r=ioctl(fd,DP_POLL,&timeout);
  for (i=0; i<r; ++i)
    printf("event %d on fd #%d\n",p[i].revents,p[i].fd);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_DEVPOLL], 1, 
[Define this if your OS supports /dev/poll])], [AC_MSG_RESULT([no])])])

# check for sendfile() system call
# ------------------------------------------------------------------
AC_DEFUN([AC_FUNC_SENDFILE], 
[m4_define([AC_HAVE_SENDFILE], [dnl
AC_DEFINE_UNQUOTED([HAVE_SENDFILE], 1, [Define this if you have the sendfile() system call])])
AC_MSG_CHECKING([for BSD sendfile])
AC_COMPILE_IFELSE([
/* for macos X, dont ask */
#define SENDFILE 1

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int main() {
  struct sf_hdtr hdr;
  struct iovec v[17+23];
  int r,fd=1;
  off_t sbytes;
  hdr.headers=v; hdr.hdr_cnt=17;
  hdr.trailers=v+17; hdr.trl_cnt=23;
  r=sendfile(0,1,37,42,&hdr,&sbytes,0);
}
], [AC_MSG_RESULT([yes])
AC_DEFINE_UNQUOTED([HAVE_BSDSENDFILE], 1, [Define this if you a BSD style sendfile()])],
[AC_MSG_RESULT([luckyl not])
AC_MSG_CHECKING([for sendfile])
AC_COMPILE_IFELSE([
#ifdef __hpux__
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

int main() {
/*
      sbsize_t sendfile(int s, int fd, off_t offset, bsize_t nbytes,
                    const struct iovec *hdtrl, int flags);
*/
  struct iovec x[2];
  int fd=open("configure",0);
  x[0].iov_base="header";
  x[0].iov_len=6;
  x[1].iov_base="footer";
  x[1].iov_len=6;
  sendfile(1 /* dest socket */,fd /* src file */,
           0 /* offset */, 23 /* nbytes */,
           x, 0);
  perror("sendfile");
  return 0;
}
#elif defined (__sun__) && defined(__svr4__)
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <stdio.h>

int main() {
  off_t o;
  o=0;
  sendfile(1 /* dest */, 0 /* src */,&o,23 /* nbytes */);
  perror("sendfile");
  return 0;
}
#elif defined (_AIX)

#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

int main() {
  int fd=open("configure",0);
  struct sf_parms p;
  int destfd=1;
  p.header_data="header";
  p.header_length=6;
  p.file_descriptor=fd;
  p.file_offset=0;
  p.file_bytes=23;
  p.trailer_data="footer";
  p.trailer_length=6;
  if (send_file(&destfd,&p,0)>=0)
    printf("sent %lu bytes.\n",p.bytes_sent);
}
#elif defined(__linux__)

#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <unistd.h>
#if defined(__GLIBC__)
#include <sys/sendfile.h>
#elif defined(__dietlibc__)
#include <sys/sendfile.h>
#else
#include <linux/unistd.h>
_syscall4(int,sendfile,int,out,int,in,long *,offset,unsigned long,count)
#endif
#include <stdio.h>

int main() {
  int fd=open("configure",0);
  off_t o=0;
  off_t r=sendfile(1,fd,&o,23);
  if (r!=-1)
    printf("sent %llu bytes.\n",r);
}
#endif
], [AC_HAVE_SENDFILE
AC_MSG_RESULT([yes])
])], [AC_MSG_RESULT([no])])])


