IFS="	
"

host=$(gcc -dumpmachine)
prefix=$(gcc -print-search-dirs | sed -n 's,\\,/,g ;; s,^install: \(.*\)/bin/.*,\1,p')

: ${DEBUG=enable}

#make -i distclean >/dev/null 2>/dev/null
set -- \
	--disable-dependency-tracking \
	--disable-maintainer-mode \
	--enable-static \
	--disable-shared \
	--enable-loadable-modules \
	--disable-color \
	--disable-silent-rules \
	--build=$host \
	--host=$host \
  --with-libowfat=$prefix \
  $(test -f $prefix/include/openssl/opensslv.h && echo --with-ssl=$prefix || echo --without-ssl) \
  $(test -f $prefix/include/mysql/mysql.h && echo --with-mysql=$prefix || echo --without-mysql) \
  $(test -f $prefix/include/sqlite3.h && echo --with-sqlite=$prefix || echo --without-sqlite) \
        ${DEBUG:+--${DEBUG}-debug} \
	"$@" 
(MSG="./configure"
for ARG; do
  MSG="$MSG \\
  $ARG"
done
 echo "+ $MSG" 1>&2
 ./configure "$@")

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
