# check for MySQL
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_MYSQL],
[default_directory="/usr /usr/local /usr/mysql /opt/mysql"

with_mysql=yes
AC_ARG_WITH(mysql,
    [  --with-mysql=DIR        support for MySQL],
    [ with_mysql="$withval" ],
    [ with_mysql=no ])

if test "$with_mysql" != no; then
  if test "$with_mysql" = yes; then
    mysql_directory="$default_directory";
    mysql_fail=yes
  elif test -d $withval; then
    mysql_directory="$withval"
    mysql_fail=no
  elif test "$with_mysql" = ""; then
    mysql_directory="$default_directory";
    mysql_fail=no
  fi

  AC_MSG_CHECKING(for MySQL headers)

  for i in $mysql_directory; do
    if test -r $i/include/mysql/mysql.h; then
      MYSQL_DIR=$i
      MYSQL_INC_DIR=$i/include/mysql
    elif test -r $i/include/mysql.h; then
      MYSQL_DIR=$i
      MYSQL_INC_DIR=$i/include
    fi
    test -n "$MYSQL_INC_DIR" -a -d "$MYSQL_INC_DIR" && break
  done

  if test ! -d "$MYSQL_INC_DIR"; then
    AC_MSG_RESULT(not found)
    with_mysql=no
#    FAIL_MESSAGE("MySQL Headers", "$MYSQL_DIR $MYSQL_INC_DIR")
  else
    AC_MSG_RESULT($MYSQL_INC_DIR)
  fi

  AC_MSG_CHECKING(for MySQL client library)
	MYSQL_LIB_NAME="mysqlclient"

  for i in lib64 lib/x86_64-linux-gnu lib64/mysql lib lib/i386-linux-gnu lib/mysql; do
    str="$MYSQL_DIR/$i/lib{mariadb,mysql}client.*"
    for j in `eval echo $str`; do
      if test -r $j; then
				case "$j" in
				  *mariadb*) MYSQL_LIB_NAME="mariadbclient" ;;
				esac
        MYSQL_LIB_DIR="$MYSQL_DIR/$i"
        break
      fi
    done
    test -d "$MYSQL_LIB_DIR" && break
  done

  if test -z "$MYSQL_LIB_DIR" -o -z "$MYSQL_INC_DIR"; then
    if test "$mysql_fail" != no; then
      AC_MSG_RESULT(not found)
      with_mysql=no
#      FAIL_MESSAGE("mysqlclient library",
#                   "$MYSQL_LIB_DIR")
    else
      AC_MSG_RESULT(no)
    fi
    MYSQL=false
  else
    AC_MSG_RESULT($MYSQL_LIB_DIR)
    MYSQL=true
    MYSQL_LIBS="-L${MYSQL_LIB_DIR} -Wl,-rpath,${MYSQL_LIB_DIR} -l${MYSQL_LIB_NAME} -lpthread -ldl -lm -lz"
    MYSQL_CFLAGS="-I${MYSQL_INC_DIR}"
    AC_CHECK_LIB(z, compress)
    DBSUPPORT="$DBSUPPORT MySQL"
    AC_DEFINE_UNQUOTED(HAVE_MYSQL, "1", [Define this if you have MySQL])
  fi
fi
AM_CONDITIONAL([MYSQL],[test "$MYSQL" = true])
AC_SUBST([MYSQL_LIBS])
AC_SUBST([MYSQL_CFLAGS])
])

