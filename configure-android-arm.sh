if [ ! -e configure -o ! -e src/config.h.in ]; then
	aclocal -I . 
	autoheader
	autoconf
#	autoreconf --force --verbose
fi

ANDROID_ARM_TOOLCHAIN=/opt/arm-linux-androideabi-4.8

CC=arm-linux-androideabi-gcc
#WFLAGS="-Wall -Wno-uninitialized -Wno-unused -Wno-unused-parameter -Wno-sign-compare"

PATH="$ANDROID_ARM_TOOLCHAIN/bin:$PATH"

SYSROOT=$ANDROID_NDK_ROOT/platforms/android-16/arch-arm

NCURSES_DIR=/opt/arm-linux-androideabi-4.8/sysroot/usr
SSL_DIR=$NCURSES_DIR

NCURSES_LIB_DIR=$NCURSES_DIR/lib
NCURSES_INC_DIR=$NCURSES_DIR/include

export CFLAGS="--sysroot "$SYSROOT" -I$NCURSES_INC_DIR -Os -fomit-frame-pointer"
export LDFLAGS="--sysroot "$SYSROOT" -L$NCURSES_LIB_DIR"

#export LDFLAGS="-L$ANDROID_ARM_TOOLCHAIN/sysroot/usr/lib -L$ANDROID_NDK_ROOT/platforms/android-16/arch-arm/usr/lib"
#CPPFLAGS="-I$ANDROID_ARM_TOOLCHAIN/sysroot/usr/include -I$ANDROID_NDK_ROOT/platforms/android-16/arch-arm/usr/include $WFLAGS"


CPPFLAGS="$CPPFLAGS $WFLAGS" \
gl_cv_func_fseeko=no \
fu_cv_sys_mounted_getmnt=yes \
./configure \
	--build=`gcc -dumpmachine` \
	--host=`$CC -dumpmachine` \
	--prefix=/system \
  --disable-color \
  --disable-silent-rules \
	--disable-dependency-tracking \
	--enable-shared \
	"$@"
