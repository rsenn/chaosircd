host=$(gcc -dumpmachine)

#make -i distclean >/dev/null 2>/dev/null
(set -x
./configure \
	--disable-dependency-tracking \
	--disable-maintainer-mode \
	--enable-static \
	--disable-shared \
	--enable-loadable-modules \
	--with-ssl \
	--disable-color \
	--disable-silent-rules \
	--build=`gcc -dumpmachine` \
        --with-sqlite=/usr \
	"$@" 
)
(set -x
./config.status)

cat <<EOF | tee build-linux.sh |sed -u "s|^|build-linux.sh: |"
#!/bin/sh

DESTDIR="\$PWD-linux"

rm -rf "\$DESTDIR"

make
make DESTDIR="\$DESTDIR" install

#cmd='tar -C "\$DESTDIR" -c . |xz -v -e -6 -f -c >"\${DESTDIR##*/}.txz"'; eval "echo \"\$cmd\"; \$cmd"

EOF
