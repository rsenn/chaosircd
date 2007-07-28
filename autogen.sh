#!/bin/sh
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.
# If not, write to the Free Software Foundation,
# 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

DIRS="libchaos ."

# Gentoo/Mandrake stuff:
export WANT_AUTOCONF_2_5=1
export WANT_AUTOMAKE_1_6=1

function autogen()
{
# Against CVS timestamp mess
ag_pwd="$(pwd)"
ag_package="$(basename $ag_pwd)"
ag_name="${ag_package%%-*}"
ag_srcdir="$(dirname $0)"
ag_prefix="/usr/$ag_name"
ag_acdir="$(aclocal --print-ac-dir)"
ag_configure_args=""

# do not link tichu with dietlibc
if test "$ag_package" = "tichu"; then
  ag_configure_args="--without-dietlibc"
fi

for ag_option
do
  ag_optarg=${ag_option#*=}
  
  if test "$ag_prev" = "srcdir"; then
    ag_srcdir=$ag_option
  fi
  
  if test "$ag_prev" = "prefix"; then
    ag_prefix=$ag_option
  fi
  
  case "$ag_option" in
    --srcdir=*) ag_srcdir=$ag_optarg ;;
    --prefix=*) ag_prefix=$ag_optarg ;;
    --srcdir) ag_prev=srcdir ;;
    --prefix) ag_prev=prefix ;;
  esac
done

touch $ag_srcdir/configure.in $ag_srcdir/m4/*
ag_package="$(grep AC_INIT $ag_srcdir/configure.in | sed -e 's,^AC_INIT(\[,,;;s,\].*$,,')"
ag_configure="$ag_srcdir/configure"
ag_configure_in="$ag_configure.in"

# Programs
ag_aclocal="aclocal"
ag_autoheader="autoheader"
ag_autoconf="autoconf"
ag_libtoolize="libtoolize"

ag_headers="$(grep AC_CONFIG_HEADERS $ag_srcdir/configure.in)"

# execute the stuff
(cd "$ag_srcdir"
# $ag_aclocal --acdir=m4
#  cat $ag_srcdir/m4/*.m4 > $ag_srcdir/aclocal.m4
 rm -f aclocal.m4
 $ag_aclocal -I m4
 if test "$ag_headers"; then
   set -x
   $ag_autoheader
 else
   set -x
 fi
 $ag_autoconf -B m4 &&
 sed -i 's/###ESCAPE###/\[/' $ag_configure
  
 # ok, we don't really use libtool nor automake, but
 # we need config.guess and config.sub for host system
 # detection
 # the --automake argument is added so libtoolize doesn't
 # complain about missing AC_PROG_LIBTOOL
 $ag_libtoolize --copy --force --automake &&
 for guess in config.guess ../config.guess; do
   if test -f $ag_srcdir/$guess; then
     # so this one is for proper target detection on cygwin 
     # systems where mingw32 is wished explicitly by specifing
     # the compiler
     sed -i -e "s/cc gcc c89 c99/gcc c89 c99 cc/
/i\*:CYGWIN/c\\
    i*:CYGWIN*:* | i*:MINGW*:* | i*:mingw*:*)\\
        TMPDIR='C:\\\\\\\\WINDOWS\\\\\\\\Temp'\\
        eval \$set_cc_for_build\\
        sed 's/^        //' << EOF >\$dummy.c\\
        MACHINE=unknown\\
        #ifdef WIN32\\
        MACHINE=win32\\
        #endif\\
        #ifdef _WIN32\\
        MACHINE=win32\\
        #endif\\
        #ifdef __MINGW32__\\
        MACHINE=mingw32\\
        #endif\\
        #ifdef __CYGWIN__\\
        MACHINE=cygwin\\
        #endif\\
        #ifdef __CYGWIN32__\\
        MACHINE=cygwin\\
        #endif\\
EOF\\
        eval \`\$CC_FOR_BUILD -E \$dummy.c 2>/dev/null | grep ^MACHINE=\`
s/-cygwin\$/-\$MACHINE/" $ag_srcdir/$guess
   fi
 done
 
 rm -f $ag_srcdir/ltmain.sh $ag_srcdir/mkinstalldirs)

}

for dir in $DIRS
do
  pushd $dir
  autogen
  popd
done

# do not configure if there is "--"
if test "$1" = "--"; then
  exit 0
fi

# now configure
ag_configure_args="$ag_configure_args
                   --prefix=$ag_prefix
                   --enable-dep
                   --enable-color 
                   --enable-debug 
                   --enable-maintainer"
set -x
exec "$ag_configure" $ag_configure_args "$@"
