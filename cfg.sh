host=$(gcc -dumpmachine)
prefix=$(gcc -print-search-dirs | sed -n 's,\\,/,g ;; s,^install: \(.*\)/bin/.*,\1,p')

: ${DEBUG=enable}

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
  --with-libowfat="$prefix" \
	--build=`gcc -dumpmachine` \
  $(test -f "$prefix/include/mysql/mysql.h" && echo --with-mysql="$prefix" || echo --without-mysql) \
  $(test -f "$prefix/include/sqlite3.h" && echo --with-sqlite="$prefix" || echo --without-sqlite) \
        ${DEBUG:+--${DEBUG}-debug} \
	"$@" 
)
(set -x
./config.status)

cat <<EOF | tee build.sh |sed "s|^|build.sh: |"
#!/bin/sh

DESTDIR="\$PWD-${host##*-}"

rm -rf "\$DESTDIR"

make
make DESTDIR="\$DESTDIR" install

#cmd='tar -C "\$DESTDIR" -c . |xz -v -e -6 -f -c >"\${DESTDIR##*/}.txz"'; eval "echo \"\$cmd\"; \$cmd"

EOF
