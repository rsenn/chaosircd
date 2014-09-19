/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003  Roman Senn <r.senn@nexbyte.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: userdb.c,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libowfat/stralloc.h>

#include "defs.h"
#include "io.h"
#include "timer.h"
#include "log.h"
#include "mem.h"
#include "net.h"
#include "str.h"

#include "userdb.h"

/* -------------------------------------------------------------------------- *
 * System headers                                                             *
 * -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- *
 * callback which is executed when an userdb client times out                   *
 * -------------------------------------------------------------------------- */
static int userdb_timeout(struct userdb_client *userdb)
{
  /*userdb->recvbuf[0] = '\0';

  io_shutup(userdb_fd(userdb));
  */
  if(userdb->callback)
    userdb->callback(userdb);

  if(userdb->timer)
  {
    timer_remove(userdb->timer);
    userdb->timer = NULL;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * builds and sends userdb request to socket                                    *
 * -------------------------------------------------------------------------- */
static int userdb_send(struct userdb_client *userdb)
{

  //userdb->status = USERDB_ST_SENT;
  return 0;
}

/* -------------------------------------------------------------------------- *
 * parse userdb reply                                                           *
 * -------------------------------------------------------------------------- */
static int userdb_parse(struct userdb_client *userdb)
{
  /* null terminate reply */
  userdb->reply[0] = '\0';

  if(userdb->callback)
    userdb->callback(userdb);

  return 1;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void userdb_event_rd(int fd, void *ptr)
{
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void userdb_event_cn(int fd, void *ptr)
{
  struct userdb_client *userdb = ptr;


  if(0) {
    if(userdb->callback)
      userdb->callback(userdb);
  }
  else
  {
    //int64_t timeout = timer_systime - userdb->deadline;

    //if(timeout < 0LL)      timeout = 1LL;

  }
}

/* -------------------------------------------------------------------------- *
 * zero authentication client struct                                          *
 * -------------------------------------------------------------------------- */
void userdb_zero(struct userdb_client *userdb)
{
  memset(userdb, 0, sizeof(struct userdb_client));
}

/* -------------------------------------------------------------------------- *
 * close userdb client socket and zero                                          *
 * -------------------------------------------------------------------------- */
void userdb_clear(struct userdb_client *userdb)
{
  if(userdb->handle) {
    db_close(userdb->handle);
  }

  timer_push(&userdb->timer);

  userdb_zero(userdb);
}

/* -------------------------------------------------------------------------- *
* start userdb client                                                *
* -------------------------------------------------------------------------- */
int userdb_connect(struct userdb_client *userdb, const char* host, const char* user, const char* password)
{
  userdb->handle = db_new(DB_TYPE_MYSQL);

  db_connect(userdb->handle, host, user, password, "cgircd");

}
/* -------------------------------------------------------------------------- *
* start userdb client                                                *
* -------------------------------------------------------------------------- */
int userdb_lookup(struct userdb_client *userdb, const char* uid)
{
  userdb->result = db_query(userdb->handle, "SELECT * FROM users WHERE uid='%s' SORT BY uid;", uid);

  if(userdb->result)
  {

    return 0;
  }

  return -1;
}

static int userdb_build_values(stralloc* sa, const char* v[], size_t n, char sep, char q , char stop, char start) {
  size_t i;

  stralloc_init(sa);

  for(i = 0; i < n; ++i)
  {
    if(!v[i]) v[i] = "";
    size_t len = str_len(v[i]);
    if(sa->len) stralloc_catb(sa, &sep,1);
    int quote =  (q != '\0') && (sep == ',' )||(!len || str_chr(v[i], sep) != NULL);

    if(quote)
      stralloc_catb(sa, &q, 1);

    char *str = (char*)v[i];

    if(start) {
      while(len) {
        if(*str == start) {
          ++str;
          break;
        }
        ++str;
        --len;
      }
    }

    while(len) {
      if(*str == stop)
        break;
      if(*str == '\'')
        stralloc_catb(sa, "\\'", 2);
      else
        stralloc_catb(sa, str, 1);
      ++str;
      --len;
    }

    if(quote)
      stralloc_catb(sa, &q, 1);
  }
  int len = sa->len;
  stralloc_catb(sa, "\0", 1);
  return len;
}


int   userdb_verify       (struct userdb_client *userdb,
                           const char*           uid,
                           const char*           password,
                           char**                retstr)
{
  *retstr=0;
  userdb->result = db_query(userdb->handle, "SELECT msisdn,imsi,imei,flag_level,last_login,last_location FROM cgircd.users WHERE uid='%s' AND password='%s';", uid, password);

  if(!userdb->result) {
    return 0;
  }

  int r = db_num_rows(userdb->result);

  if(r) {

    stralloc sa;

    char **row = db_fetch_row(userdb->result);

    /*size_t i;

    for(i = 0; i < db_num_fields(userdb->result); ++i)
    {
      if(row[i] == NULL)
        row[i] = "";
    }*/

    userdb_build_values(&sa, row, db_num_fields(userdb->result), ' ', '\'', 0, 0);

//if(&sa.len)     stralloc_catb(&sa, "'", 1);
    *retstr = sa.s;
    sa.s = 0;
  }

  db_free_result(userdb->result);

  return r;
}



int userdb_register     (struct userdb_client *userdb, const char* uid, const char**         v,
                         size_t num_values)
{

  stralloc fields;
  stralloc values;

  userdb_build_values(&fields, v, num_values, ',', '\'', '=', '\0');
  userdb_build_values(&values, v, num_values, ',', '`', '\0', '=');

  userdb->result = db_query(userdb->handle, "INSERT INTO cgircd.users (uid,%s) VALUES ('%s',%s)", fields.s, uid, values.s);
    
  if(db_affected_rows(userdb->handle) <= 1)
    return -1;

  return 0;
}

int   userdb_mutate       (struct userdb_client *userdb,
                           const char*           values,
                           size_t num_values)
{}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void userdb_set_userarg(struct userdb_client *userdb, void *arg)
{
  userdb->userarg = arg;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void *userdb_get_userarg(struct userdb_client *userdb)
{
  return userdb->userarg;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void userdb_set_callback(struct userdb_client *userdb, userdb_callback_t *cb,
                         uint64_t timeout)
{
  userdb->callback = cb;
  userdb->timeout = timeout;
}
