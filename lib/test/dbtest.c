#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

#include "libchaos/io.h"
#include "libchaos/mem.h"
#include "libchaos/log.h"
#include "libchaos/db.h"


int dbtest_type = DB_TYPE_MYSQL;
char *dbtest_host = "127.0.0.1";
char *dbtest_user = "root";
char *dbtest_pass = ""; /*OttovPauwid";*/
char *dbtest_dbname = "chaosircd";
/*int dbtest_type = DB_TYPE_PGSQL;
char *dbtest_host = "localhost";
char *dbtest_user = "enki";
char *dbtest_pass = "***";
char *dbtest_dbname = "babel";*/

int dbtest_log;

#if (defined HAVE_PGSQL) || (defined HAVE_MYSQL)

void dbtest()
{
  struct db *db;
  struct db_result *result;
  char **row;

  db = db_new(dbtest_type);

  if(!db_connect(db, dbtest_host, dbtest_user, dbtest_pass, dbtest_dbname))
    log(dbtest_log, L_status, "Database connection OK (Type = %s)", (db->type == DB_TYPE_PGSQL ? "PostgreSQL" : "MySQL"));
  else
    return;

  result = db_query(db, "SELECT * FROM chaosircd.users ORDER BY uid;");

  while((row = db_fetch_row(result)))
  {
    log(dbtest_log, L_status, " %-10s %-10s %-10s", row[0], row[1], row[2]);
  }

  db_free_result(result);

  db_close(db);
}
#endif

int main()
{
  log_init(STDOUT_FILENO, LOG_ALL, L_status);
  dbtest_log = log_source_register("dbtest");
  io_init_except(STDOUT_FILENO, STDOUT_FILENO, STDOUT_FILENO);
  mem_init();
  dlink_init();
  
#if (defined HAVE_PGSQL) || (defined HAVE_MYSQL)

  db_init();

  dbtest();

  db_shutdown();
#endif
  dlink_shutdown();
  mem_shutdown();
  log_shutdown();
  io_shutdown();

  return 0;
}

