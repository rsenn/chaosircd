sysroot="/opt/arm-linux-androideabi-4.8/sysroot"
host="arm-linux-androideabi"

./configure \
	--prefix=/system \
	--disable-dependency-tracking \
	--disable-maintainer-mode \
	--disable-shared \
	--enable-loadable-modules \
	--with-ssl="static" \
	--with-ssl-prefix="$sysroot/usr" \
	--disable-color \
	--disable-silent-rules \
	--build=`gcc -dumpmachine` \
	"$@" \
	CC="${host}-gcc -I$ANDROID_NDK_ROOT/platforms/android-16/arch-arm/usr/include -L$ANDROID_NDK_ROOT/platforms/android-16/arch-arm/usr/lib"

cat <<EOF | tee build-android.sh |sed -u "s|^|build-android.sh: |"
#!/bin/sh

DESTDIR="\$PWD-android"

rm -rf "\$DESTDIR"

#make clean
./config.status
make
make DESTDIR="\$DESTDIR" install

cmd='tar -C "\$DESTDIR" -c . |xz -v -e -6 -f -c >"\${DESTDIR##*/}.txz"'; eval "echo \"\$cmd\"; \$cmd"

EOF
