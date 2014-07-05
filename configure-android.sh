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

cat <<EOF | tee build-android.sh |sed -u "s|^|build-android.sh: |"
#!/bin/sh

DESTDIR="\$PWD-android"

rm -rf "\$DESTDIR"

make clean
./config.status
make
make DESTDIR="\$DESTDIR" install

cmd='tar -C "\$DESTDIR" -c . |xz -v -e -6 -f -c >"\${DESTDIR##*/}.txz"'
eval "echo \"\$cmd\"; \$cmd"

EOF
