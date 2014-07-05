host=$(gcc -dumpmachine)

make -i distclean >/dev/null 2>/dev/null

./configure \
	--disable-dep \
	--disable-maintainer \
	--enable-static \
	--with-ssl \
	--disable-color \
	--disable-quiet \
	--build=`gcc -dumpmachine` \
	"$@" 

./config.status

cat <<EOF | tee build-linux.sh |sed -u "s|^|build-linux.sh: |"
#!/bin/sh

DESTDIR="\$PWD-linux"

rm -rf "\$DESTDIR"

make
make DESTDIR="\$DESTDIR" install

cmd='tar -C "\$DESTDIR" -c . |xz -v -e -6 -f -c >"\${DESTDIR##*/}.txz"'
eval "echo \"\$cmd\"; \$cmd"

EOF
