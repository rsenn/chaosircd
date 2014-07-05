host=arm-linux-androideabi

./configure \
	--prefix=/system \
	--disable-dep \
	--disable-maintainer \
	--enable-shared \
	--with-ssl \
	--disable-color \
	--disable-quiet \
	--build=`gcc -dumpmachine` \
	"$@" \
	CC="${host}-gcc -I$ANDROID_NDK_ROOT/platforms/android-16/arch-arm/usr/include -L$ANDROID_NDK_ROOT/platforms/android-16/arch-arm/usr/lib"
