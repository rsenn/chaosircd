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
 * $Id: userdb.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_USERDB_H
#define LIB_USERDB_H

/* ------------------------------------------------------------------------ *
 * Library headers                                                            *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/dlink.h"
#include "libchaos/timer.h"
#include "libchaos/net.h"

/* ------------------------------------------------------------------------ *
 * Constants                                                                  *
 * ------------------------------------------------------------------------ */

#define USERDB_DONE       0x00
#define USERDB_ERROR      0x01
#define USERDB_TIMEDOUT   0x02

#define USERDB_QUERY_VERIFY   0x00
#define USERDB_QUERY_REGISTER 0x01
#define USERDB_QUERY_SEARCH   0x02
#define USERDB_QUERY_MUTATE   0x03

#define USERDB_TIMEOUT    20000
#define USERDB_RELAUNCH   15000        /* 15 secs relaunch interval */

/* ------------------------------------------------------------------------ *
 * Types                                                                      *
 * ------------------------------------------------------------------------ */

struct userdb;
typedef void (userdb_callback_t)(struct userdb *, void *, void *, void *, void *);

/* ------------------------------------------------------------------------ *
 * Sauth block structure.                                                     *
 * ------------------------------------------------------------------------ */
typedef struct userdb {
  struct node       node;        /* linking node for userdb block list */
  uint32_t          id;
  uint32_t          refcount;
  int query;
  int result;

  /* externally initialised */
  char uid[65];
  char* params;
  int               status;
  struct timer     *timer;
  userdb_callback_t *callback;
  void* args[4];
} userdb_t;

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct sheap)  userdb_heap;
CHAOS_API(struct list)   userdb_list;
CHAOS_API(uint32_t)      userdb_serial;
CHAOS_API(struct timer *)userdb_timer;
CHAOS_API(struct timer *)userdb_rtimer;
CHAOS_API(struct child *)userdb_child;
CHAOS_API(int)           userdb_log;
CHAOS_API(int)           userdb_fds[2];
CHAOS_API(char)          userdb_readbuf[BUFSIZE];
CHAOS_API(const char *)  userdb_types[6];
CHAOS_API(const char *)  userdb_replies[8];
CHAOS_DATA(const char* ) userdb_query_names[];

/* ------------------------------------------------------------------------ */
CHAOS_API(int) userdb_get_log(void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          userdb_init         (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          userdb_shutdown     (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          userdb_collect      (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct userdb *)userdb_query_search  (const char    *expr,
                                             void          *callback,
                                             ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct userdb *)userdb_query_register(const char* uid,
                                                const char* values,
                                             void          *callback,
                                             ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct userdb *)userdb_query_mutate(const char* uid,
                                                const char* values,
                                             void          *callback,
                                             ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct userdb *)userdb_query_verify(const char* uid,
                                                const char* password,
                                             void          *callback,
                                             ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)           userdb_launch       (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct userdb *)userdb_auth         (net_addr_t     address,
                                             net_port_t     local,
                                             net_port_t     remote,
                                             void          *callback,
                                             ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)           userdb_query_reply  (const char    *reply);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)           userdb_query_type   (const char    *type);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          userdb_delete       (struct userdb  *userdb);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct userdb *)userdb_find         (uint32_t       id);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct userdb *)userdb_pop          (struct userdb  *userdb);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct userdb *)userdb_push         (struct userdb **userdb);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          userdb_cancel       (struct userdb  *userdb);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          userdb_vset_args    (struct userdb  *userdb,
                                             va_list        args);

CHAOS_API(void)          userdb_set_args     (userdb_t       *userdb,
                                             ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          userdb_dump         (struct userdb  *saptr);

#endif /* LIB_USERDB_H */
