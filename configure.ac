# $Id: configure.ac,v 1.16 2006/09/28 09:47:03 roman Exp $
# ===========================================================================
#
# GNU autoconf template for chaosircd
#
# initialize autoconf (require version 2.5x)
# ---------------------------------------------------------------------------
AC_PREREQ(2.59)
AC_INIT([chaosircd],[2.2.1])
AC_CONFIG_SUBDIRS([lib])

# check for compiling modes
# ---------------------------------------------------------------------------
AC_CHECK_COLOR

# set some info stuff
# ---------------------------------------------------------------------------
AC_SET_ARGS
PACKAGE_RELEASE="peace & love hippie edition"
AC_DEFINE_UNQUOTED(PACKAGE_RELEASE, "$PACKAGE_RELEASE", [Define to the release name of this package.])

VERSION=${PACKAGE_VERSION%%[[a-z]]*}

VERSION_MAJOR=${VERSION%%.[[0-9]]*}
VERSION_MINOR=${VERSION#*[0-9].}
VERSION_MINOR=${VERSION_MINOR%.[[0-9]]*}
VERSION_PATCH=${VERSION##*[0-9].}

AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_PATCH)

CREATION=$(date)
PLATFORM=$(uname -a)
AC_DEFINE_UNQUOTED(CREATION, "$CREATION", [Creation time of this server])
AC_DEFINE_UNQUOTED(PLATFORM, "$PLATFORM", [Platform this server runs on])

# set directories
# ---------------------------------------------------------------------------
AC_CONFIG_HEADERS([config.h include/ircd/config.h])
AC_CONFIG_SRCDIR([src/ircd.c])

test "$prefix" = "NONE" && prefix=${ac_default_prefix}
test "$exec_prefix" = "NONE" && exec_prefix=${prefix}
 
eval "tmp_prefix=\"${prefix}\""

eval sysconfdir=${sysconfdir}
case $sysconfdir in
  *ircd*) sysconfdir=${sysconfdir} ;;
  *) sysconfdir=${sysconfdir}/chaosircd ;;
esac

relsysconfdir=${sysconfdir#$tmp_prefix/}
test "$relsysconfdir" = "$sysconfdir" && relsysconfdir=etc${sysconfdir##*/etc}
AC_SUBST([relsysconfdir])


eval "rellibexecdir=\"${libexecdir}\""
rellibexecdir=${rellibexecdir#$tmp_prefix/}
test "$rellibexecdir" = "$libexecdir" && rellibexecdir=libexec${libexecdir##*libexec}
AC_SUBST([rellibexecdir])

eval libexecdir=${libexecdir}
eval localstatedir=${localstatedir}

eval pidfile=${localstatedir}/run/chaosircd.pid
AC_ARG_WITH([pidfile], [  --with-pidfile=PATH     location of the pid-file [[LOCALSTATEDIR/run/chaosircd.pid]]], 
[if test "$withval" != "yes" && test "$withval" != "no"; then eval pidfile=$withval; fi])
AC_SUBST([pidfile])

#if test "$prefix" = "NONE"; then
#  prefix=$ac_default_prefix
#fi
#eval prefix=${prefix}
#eval libdir=${libdir}

AC_MSG_CHECKING([for prefix])
AC_MSG_RESULT([$tmp_prefix])

case $tmp_prefix in
  *ircd*)
    eval "plugindir=${libdir}/modules"
    ;;
  *)
    eval "plugindir=${libdir}/chaosircd"
    ;;
esac
relplugindir=${plugindir#$tmp_prefix/}
test "$relplugindir" = "$plugindir" && relplugindir=lib/${plugindir##*/lib/}
AC_ARG_WITH([plugindir], [  --with-plugindir=PATH   location of the plugins [[LIBDIR/chaosircd]]], 
[if test "$withval" != "yes" && test "$withval" != "no"; then eval plugindir=$withval; fi])
AC_SUBST([plugindir])
AC_SUBST([relplugindir])
case $tmp_prefix in
  *ircd*) eval "inidir=${localstatedir}/lib"  ;;
  *) eval "inidir=${localstatedir}/lib/chaosircd"  ;;
esac
relinidir=${inidir#$tmp_prefix/}
if test "$relinidir" = "$inidir"; then
   relinidir=${localstatedir##*/}${inidir##*${localstatedir##*/}}
fi 
AC_ARG_WITH([inidir], [  --with-inidir=PATH   location of the inis [[SHAREDSTATEDIR/ini]]],
[if test "$withval" != "yes" && test "$withval" != "no"; then eval inidir=$withval; fi])

AC_MSG_CHECKING([For .ini directory])
AC_SUBST([inidir])
AC_SUBST([relinidir])
AC_MSG_RESULT([$inidir $relinidir])

case $tmp_prefix in
  *ircd*) eval "logdir=${localstatedir}/log" ;; 
  *) eval "logdir=${localstatedir}/log/chaosircd" ;;
esac
rellogdir=${logdir#$tmp_prefix/}
test "$rellogdir" = "$logdir" && rellogdir=${localstatedir##*/}${logdir##*${localstatedir##*/}}
AC_ARG_WITH([logdir], [  --with-logdir=PATH   location of the logs [[LOCALSTATEDIR/log]]],
[if test "$withval" != "yes" && test "$withval" != "no"; then eval logdir=$withval; fi])
AC_SUBST([logdir])
AC_SUBST([rellogdir])

eval "rundir=${localstatedir}/run"
relrundir=${rundir#$tmp_prefix/}
test "$relrundir" = "$rundir" && relrundir=${localstatedir##*/}${rundir##*${localstatedir##*/}}
AC_ARG_WITH([rundir], [  --with-rundir=PATH   location of the runs [[LOCALSTATEDIR/run]]],
[if test "$withval" != "yes" && test "$withval" != "no"; then eval rundir=$withval; fi])
AC_SUBST([rundir])
AC_SUBST([relrundir])

pidfile="$rundir/chaosircd.pid"
relpidfile="$relrundir/chaosircd.pid"
AC_SUBST([pidfile])
AC_SUBST([relpidfile])

# set extensions and flags
# ---------------------------------------------------------------------------
SHEXT="so"
DFLAGS="-g -ggdb"
WFLAGS="-Wall -Werror"
FFLAGS=""

# check for libs and programs needed to build tichu-server
# ---------------------------------------------------------------------------
AC_DIETLIBC([/opt/diet /usr/diet])
AC_PROG_CC([cc gcc])
AC_PROG_AR
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN
AC_PROG_DLLWRAP
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_CYGPATH
AC_CHECK_PROG([NASM], [nasm], [$as_dir/$ac_word$ac_exec_ext], [], [$PATH])

# check some system dependencies
# ---------------------------------------------------------------------------
AC_CHECK_LIB([m], [cos])

AC_CANONICAL_HOST

AC_CONFIG_HOST
AC_CHECK_DLFCN
AC_CONFIG_DYLIB


echo "Host is: $host" 1>&2
case "$host" in
  *mingw* | *msys* | *cygwin* | *msvc* | *win32* | *win64*)
AX_CHECK_LINK_FLAG([-Wl,--export-all-symbols], [LDFLAGS="$LDFLAGS -Wl,--export-all-symbols"], [])
;;
  *)
AX_CHECK_LINK_FLAG([-Wl,--export-dynamic], [LDFLAGS="$LDFLAGS -Wl,--export-dynamic"], [])

 ;;
 esac

dnl AC_CHECK_HEADERS([fcntl.h io.h signal.h unistd.h sys/ioctl.h sys/mman.h sys/resource.h sys/socket.h sys/time.h sys/timeb.h sys/wait.h dlfcn.h netinet/in.h cygwin/in.h])
AC_CHECK_HEADERS([signal.h stdint.h sys/mman.h sys/resource.h sys/wait.h unistd.h])

dnl AC_FUNC_MMAP

AC_CHECK_SSL
AC_CHECK_LIBOWFAT
AC_CHECK_EFENCE
dnl AC_CHECK_FT2
dnl AC_CHECK_PSQL
dnl AC_CHECK_MYSQL

dnl AC_CHECK_FUNC([ltoa], AC_DEFINE_UNQUOTED([HAVE_LTOA], [Define this if you have the ltoa() function (defined in stdlib.h usually)]))
dnl AC_CHECK_FUNC([lltoa], AC_DEFINE_UNQUOTED([HAVE_LLTOA], [Define this if you have the ltoa() function (defined in stdlib.h usually)]))

dnl AC_C_BIGENDIAN([ENDIAN="ENDIAN_BIG"],
dnl                [ENDIAN="ENDIAN_LIL"])
    
dnl AC_SUBST(ENDIAN)
dnl AC_DEFINE_UNQUOTED(ENDIAN_LIL, 0, [Little endian machine])
dnl AC_DEFINE_UNQUOTED(ENDIAN_BIG, 1, [Big endian machine])
dnl AC_DEFINE_UNQUOTED(ENDIAN, $ENDIAN, [Define your machines endian])

IA32INLINE="0"

# check for winsuck
dnl AC_CHECK_HEADER([winsock2.h])

# FIXME: must be declared as pascal call and must have 2 arguments (8 bytes)
dnl AC_CHECK_LIB([ws2_32], [WSAStartup])
dnl AC_CHECK_LIB([wsock32], [WSAStartup])


dnl ac_cv_LIBS="$LIBS"
dnl LIBS="$LIBS -lws2_32"

dnl AC_MSG_CHECKING([for winsock2])
dnl AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <winsock2.h>]], [[dnl   WSAStartup(0,0)]])],[dnl   AC_MSG_RESULT(yes)],[dnl   AC_MSG_RESULT([no])
dnl   LIBS="$ac_cv_libs"])
dnl
dnl # runtime linker
dnl case "$host" in
dnl   *darwin*)
dnl     PIE_LOADER="dlfcn_darwin"
dnl     ;;
dnl   *mingw32*)
dnl     PIE_LOADER="dlfcn_win32"
dnl     ;;
dnl esac

dnl AC_SUBST([DLFCN_LDADD])

if test "$HAVE_SSL" = yes; then
  SSL_FALSE='#' 
else
 SSL_TRUE='#'
fi
AC_SUBST([SSL_TRUE])
AC_SUBST([SSL_FALSE])

# set debugging stuff after compiler test (-Werror could fuck it up)
# ---------------------------------------------------------------------------
AC_CHECK_DEP
AC_CHECK_DEBUG
AC_CHECK_MAINTAINER
AC_CHECK_WERROR

# ---------------------------------------------------------------------------

eval "case $libdir in
  /lib64 | /usr/lib64 | /usr/local/lib64) ;;
  /lib | /usr/lib | /usr/local/lib) ;;
  *)
    LDFLAGS=\"\${LDFLAGS:+\$LDFLAGS }-Wl,-rpath=\\\${libdir}\"
    ;;
esac"

case $host in
  *-linux*)
    LDFLAGS="${LDFLAGS:+$LDFLAGS }-rdynamic"
    ;;
esac

AC_SUBST([LDFLAGS])
AC_SUBST([CPPFLAGS])

# library subdirectories
# ---------------------------------------------------------------------------
AC_CONFIG_FILES([
Makefile
config.mk
build.mk
conf/Makefile
conf/children.conf
conf/classes.conf
conf/inis.conf
conf/ircd.conf
conf/logs.conf
conf/modules.conf
conf/opers.conf
conf/ssl.conf
contrib/Makefile
contrib/debian/Makefile
doc/Makefile
include/Makefile
include/ircd/Makefile
modules/Makefile
modules/chanmode/Makefile
modules/lclient/Makefile
modules/msg/Makefile
modules/service/Makefile
modules/stats/Makefile
modules/usermode/Makefile
src/Makefile
test/Makefile
tools/Makefile
tools/ircd-config
tools/mkca
tools/mkcrt
tools/mkkeys
tools/mkreq
tools/openssl.cnf
tools/sign])

AC_CONFIG_STATUS

# be verbose :)
# ---------------------------------------------------------------------------
AC_SUMMARIZE([prefix bindir datadir includedir],
             [COLOR DEP DEBUG |
              CC CFLAGS LDFLAGS LIBS |
              host build |
							DLM_ENABLE PIE_ENABLE A_ENABLE],
             "%20s: %s")
