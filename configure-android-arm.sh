#!/bin/sh

IFS=$'\n \t\r\v'

if [ ! -e configure -o ! -e src/config.h.in ]; then
     case "$PWD" in
       */libchaos) aclocal -I m4 ;;
       *) aclocal -I libchaos/m4 ;;
     esac
	autoheader
	autoconf
#	autoreconf --force --verbose
fi

ANDROID_ARM_TOOLCHAIN=F:/android-ndk-r9d/toolchains/arm-linux-androideabi-4.8/prebuilt/windows-x86_64

type cygpath &&
  ANDROID_ARM_TOOLCHAIN=`cygpath "$ANDROID_ARM_TOOLCHAIN"`

export CC="arm-linux-androideabi-gcc"
#WFLAGS="-Wall -Wno-uninitialized -Wno-unused -Wno-unused-parameter -Wno-sign-compare"

PATH="$ANDROID_ARM_TOOLCHAIN/bin:$PATH"

SYSROOT=$ANDROID_NDK_ROOT/platforms/android-16/arch-arm

type cygpath &&
  SYSROOT=`cygpath -m "$SYSROOT"`

SSL_DIR=v:/home/roman/Sources/openssl-1.0.2-beta1-android-arm7/system
type cygpath &&
  SSL_DIR=`cygpath -m "$SSL_DIR"`

SSL_LIB_DIR=$SSL_DIR/lib
SSL_INC_DIR=$SSL_DIR/include

export CFLAGS="--sysroot "$SYSROOT" -I$SSL_INC_DIR -ggdb -O0"
export LDFLAGS="--sysroot "$SYSROOT" -L$SSL_LIB_DIR"

#export LDFLAGS="-L$ANDROID_ARM_TOOLCHAIN/sysroot/usr/lib -L$ANDROID_NDK_ROOT/platforms/android-16/arch-arm/usr/lib"
#CPPFLAGS="-I$ANDROID_ARM_TOOLCHAIN/sysroot/usr/include -I$ANDROID_NDK_ROOT/platforms/android-16/arch-arm/usr/include $WFLAGS"


CPPFLAGS="$CPPFLAGS $WFLAGS" \
gl_cv_func_fseeko=no \
fu_cv_sys_mounted_getmnt=yes \
./configure \
	--build=`gcc -dumpmachine` \
	--host=`$CC -dumpmachine` \
	--prefix=/system \
	--with-mysql=no \
	--disable-color \
	--disable-silent-rules \
	--enable-debug \
	--disable-dependency-tracking \
	"$@" 2>&1
