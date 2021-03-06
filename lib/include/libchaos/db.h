/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 *
 * $Id: db.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_DB_H
#define LIB_DB_H

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#define DB_TYPE_SQLITE 0
#define DB_TYPE_PGSQL 1
#define DB_TYPE_MYSQL 2

#ifdef HAVE_SQLITE
#include <sqlite3.h>
#endif
#ifdef HAVE_MYSQL
#include <mysql.h>
#endif
#ifdef HAVE_PGSQL
#include <libpq-fe.h>
#endif

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct db_result
{
  struct db  *db;
  union
  {
#ifdef HAVE_PGSQL
    PGresult  *pg;
#endif
#ifdef HAVE_MYSQL
    MYSQL_RES *my;
#endif
    void      *common;
  } res;

  uint64_t    row;
  uint64_t    rows;
  uint32_t    fields;
  char      **data;
  char      **fdata;
};

struct db
{
  struct node              node;
  uint32_t                 id;
  uint32_t                 refcount;
  int                      type;

  union
  {
#ifdef HAVE_SQLITE
    sqlite3               *sq;
#endif
#ifdef HAVE_PGSQL
    PGconn                *pg;
#endif
#ifdef HAVE_MYSQL
    MYSQL                 *my;
#endif
    void                  *common;
  } handle;
  uint64_t                 affected_rows;
  int                     error;
  char                     errormsg[256];
};

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_DATA(int)                 db_log;
CHAOS_DATA(struct sheap)        db_heap;
CHAOS_DATA(struct list)         db_list;
CHAOS_DATA(uint32_t)            db_serial;

/* ------------------------------------------------------------------------ *
 * Initialize DB heap                                                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void                 db_init              (void))

/* ------------------------------------------------------------------------ *
 * Destroy DB heap                                                          *
 * ------------------------------------------------------------------------ */
CHAOS_API(void                 db_shutdown          (void))

/* ------------------------------------------------------------------------ *
 * Destroy DB instance                                                      *
 * ------------------------------------------------------------------------ */
CHAOS_API(void                 db_destroy           (struct db  *db))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct db*          db_new               (int         type))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int                  db_connect           (struct db  *db,
                                                 const char* host,
                                                 const char* user,
                                                 const char* pass,
                                                 const char* dbname))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct db_result*    db_query             (struct db  *db,
                                                 const char *format,
                                                 ...))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void                 db_close             (struct db  *db))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(size_t               db_escape_string     (struct db *db,
                                                 char* to,
                                                 const char *from,
                                                 size_t      len))
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(char*               db_escape_string_dup     (struct db *db,
                                                 const char *from))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void                 db_free_result       (struct db_result *result))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(char**               db_fetch_row         (struct db_result *result))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(uint64_t             db_num_rows          (struct db_result *result))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(uint32_t             db_num_fields        (struct db_result *result))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(uint64_t             db_affected_rows     (struct db *db))

#endif
