# check for PostgreSQL
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_PSQL],
[default_directory="/usr /usr/local /usr/pgsql /usr/local/pgsql"

AC_ARG_WITH(pgsql,
    [  --with-pgsql=DIR        support for PostgreSQL],
    [ with_pgsql="$withval" ],
    [ with_pgsql=no ])

if test "$with_pgsql" != no; then
  if test "$with_pgsql" = yes; then
    pgsql_directory="$default_directory "
    pgsql_fail=yes
  elif test -d $withval; then
    pgsql_directory="$withval $default_directory"
    pgsql_fail=yes
  elif test "$with_pgsql" = ""; then
    pgsql_directory="$default_directory"
    pgsql_fail=no
  fi

  AC_MSG_CHECKING(for PostgreSQL headers)

  for i in $pgsql_directory; do
    if test -r $i/include/pgsql/libpq-fe.h; then
      PGSQL_DIR=$i
      PGSQL_INC_DIR=$i/include/pgsql
    elif test -r $i/include/libpq-fe.h; then
      PGSQL_DIR=$i
      PGSQL_INC_DIR=$i/include
    elif test -r $i/include/postgresql/libpq-fe.h; then
      PGSQL_DIR=$i
      PGSQL_INC_DIR=$i/include/postgresql
    fi
    test -d "$PSQL_INC_DIR" && break
  done

  if test ! -d "$PGSQL_INC_DIR"; then
    AC_MSG_ERROR("PostgreSQL header file (libpq-fe.h)", "$PGSQL_DIR $PGSQL_INC_DIR")
  else
    AC_MSG_RESULT($MYSQL_INC_DIR)
  fi

  AC_MSG_CHECKING(for PostgreSQL client library)

  for i in lib lib/pgsql; do
    str="$PGSQL_DIR/$i/libpq.*"
    for j in `echo $str`; do
      if test -r $j; then
          PGSQL_LIB_DIR="$PGSQL_DIR/$i"
          break 
      fi
    done
      test -d "$PSQL_LIB_DIR" && break
  done

    if test -z "$PGSQL_LIB_DIR"; then
      if test "$postgresql_fail" != no; then
        AC_MSG_ERROR("PostgreSQL library libpq",
        "$PGSQL_DIR/lib $PGSQL_DIR/lib/pgsql")
      else
        AC_MSG_RESULT(no);
      fi
    else
      AC_MSG_RESULT($PGSQL_LIB_DIR)
      PSQL=true
      PSQL_LIBS="-L${PGSQL_LIB_DIR} -lpq"
      PSQL_CFLAGS="-I${PGSQL_INC_DIR}"
      DBSUPPORT="$DBSUPPORT PostgreSQL"
      AC_DEFINE_UNQUOTED(HAVE_PGSQL, "1", [Define this if you have PostgreSQL])
    fi

fi
AM_CONDITIONAL([PSQL],[test "$PSQL" = true])
AC_SUBST(PSQL_LIBS)
AC_SUBST(PSQL_CFLAGS)])

