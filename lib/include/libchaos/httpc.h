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
 * $Id: httpc.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_HTTPC_H
#define LIB_HTTPC_H

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/dlink.h"

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#define HTTPC_LINELEN  1024
#define HTTPC_MAX_BUF  (256 * 1024)
#define HTTPC_TIMEOUT  30000ull
#define HTTPC_INTERVAL 0ull

#define HTTPC_TYPE_GET  0
#define HTTPC_TYPE_POST 1

#define HTTPC_IDLE       0
#define HTTPC_CONNECTING 1
#define HTTPC_SENT       2
#define HTTPC_HEADER     3
#define HTTPC_DATA       4
#define HTTPC_DONE       5

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct httpc;

typedef void (httpc_cb_t)(struct httpc *, void *, void *, void *, void *);

struct httpc_var {
  struct node node;
  hash_t      hash;
  char        name[64];
  char        value[128];
};

struct httpc {
  struct node        node;
  uint32_t           id;
  uint32_t           refcount;
  int                fd;
  int                type;
  hash_t             nhash;
  int                status;
  uint16_t           port;
  int                ssl;
  int                code;
  int                chunked;
  struct connect    *connect;
  httpc_cb_t        *callback;
  struct list        vars;
  struct list        header;
  char              *loc;
  char              *content;
  size_t             content_length;
  char              *data;
  size_t             data_length;
  size_t             chunk_length;
  void              *args[4];
  char               error[32];
  char               protocol[16];
  char               name[32];
  char               host[64];
  char               location[1024];
  char               url[2048];
};

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_DATA(int)           httpc_log;
CHAOS_DATA(struct sheap)  httpc_heap;
CHAOS_DATA(struct sheap)  httpc_var_heap;
CHAOS_DATA(struct dheap)  httpc_dheap;
CHAOS_DATA(struct list)   httpc_list;
CHAOS_DATA(uint32_t)      httpc_id;

/* ------------------------------------------------------------------------ */
CHAOS_API(int  httpc_get_log(void))

/* ------------------------------------------------------------------------ *
 * Initialize httpc heap.                                                     *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           httpc_init      (void))

/* ------------------------------------------------------------------------ *
 * Destroy httpc heap.                                                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           httpc_shutdown  (void))

/* ------------------------------------------------------------------------ *
 * Add a httpc.                                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct httpc * httpc_add       (const char    *url))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int            httpc_vconnect  (struct httpc  *hcptr,
                                          void          *cb,
                                          va_list        args))
CHAOS_API(int            httpc_connect   (struct httpc  *hcptr,
                                          void          *cb,
                                          ...))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int            httpc_url_parse (struct httpc  *hcptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(size_t         httpc_url_encode(char          *to,
                                          const char    *from,
                                          size_t         n,
                                          int            loc))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int            httpc_url_build (struct httpc  *hcptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int            httpc_update    (struct httpc  *httpc))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           httpc_clear     (struct httpc  *httpc))

/* ------------------------------------------------------------------------ *
 * Remove a httpc.                                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           httpc_delete    (struct httpc  *httpc))

/* ------------------------------------------------------------------------ *
 * Send HTTP request                                                          *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           httpc_send      (struct httpc  *hcptr))

/* ------------------------------------------------------------------------ *
 * Receive HTTP data                                                          *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           httpc_recv      (int            fd,
                                          struct httpc  *hcptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct httpc * httpc_find_name (const char    *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct httpc * httpc_find_id   (uint32_t       id))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           httpc_vset_args (struct httpc  *httpc,
                                          va_list        args))
CHAOS_API(void           httpc_set_args  (struct httpc  *httpc,
                                          ...))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           httpc_var_set   (struct httpc  *hcptr, const char *name,
                                          const char    *value, ...))

CHAOS_API(void           httpc_var_vset  (struct httpc  *hcptr, const char *name,
                                          const char    *value, va_list     args))


/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           httpc_var_build (struct httpc  *hcptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           httpc_set_name  (struct httpc  *hcptr,
                                          const char    *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct httpc * httpc_pop       (struct httpc  *httpc))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct httpc * httpc_push      (struct httpc **httpcptr))

/* ------------------------------------------------------------------------ *
 * Dump httpcs.                                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           httpc_dump      (struct httpc  *hcptr))

#endif
