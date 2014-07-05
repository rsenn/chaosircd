host=$(gcc -dumpmachine)

./configure \
	--disable-dep \
	--disable-maintainer \
	--enable-shared \
	--with-ssl \
	--disable-color \
	--disable-quiet \
	--build=`gcc -dumpmachine` \
	"$@" 

cat <<EOF | tee build-linux.sh |sed -u "s|^|build-linux.sh: |"
#!/bin/sh

DESTDIR="\$PWD-linux"

rm -rf "\$DESTDIR"

make clean
./config.status
make
make DESTDIR="\$DESTDIR" install

cmd='tar -C "\$DESTDIR" -c . |xz -v -e -6 -f -c >"\${DESTDIR##*/}.txz"'
eval "echo \"\$cmd\"; \$cmd"

EOF
