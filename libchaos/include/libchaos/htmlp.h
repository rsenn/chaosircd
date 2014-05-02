/* chaosircd - pi-networks irc server
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
 * $Id: htmlp.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_HTMLP_H
#define LIB_HTMLP_H

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/dlink.h"

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#define HTMLP_LINELEN  1024
#define HTMLP_MAX_BUF  (256 * 1024)

#define HTMLP_IDLE       0
#define HTMLP_DONE       1

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct htmlp;

typedef void (*htmlp_cb_t)(struct htmlp *, void *, void *, void *, void *);

struct htmlp_var {
  struct node node;
  uint32_t    hash;
  char        name[64];
  char        value[128];
};

struct htmlp_tag {
  struct node node;
  uint32_t    hash;
  int         closing;
  struct list vars;
  char       *text;
  char        name[64];
};

struct htmlp {
  struct node        node;
  uint32_t           id;
  uint32_t           refcount;
  int                fd;
  uint32_t           nhash;
  int                status;
  struct list        tags;
  char              *buf;
  struct htmlp_tag  *current;
  void              *args[4];
  char               name[32];
  char               path[64];
};

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)           htmlp_log;
CHAOS_API(struct sheap)  htmlp_heap;
CHAOS_API(struct sheap)  htmlp_var_heap;
CHAOS_API(struct dheap)  htmlp_dheap;
CHAOS_API(struct list)   htmlp_list;
CHAOS_API(uint32_t)      htmlp_id;

/* ------------------------------------------------------------------------ */
CHAOS_API(int) htmlp_get_log(void);

/* ------------------------------------------------------------------------ *
 * Initialize htmlp heap.                                                     *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          htmlp_init      (void);

/* ------------------------------------------------------------------------ *
 * Destroy htmlp heap.                                                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          htmlp_shutdown  (void);

/* ------------------------------------------------------------------------ *
 * Add a htmlp.                                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct htmlp *)htmlp_new       (const char    *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)           htmlp_update    (struct htmlp  *htmlp);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)           htmlp_parse     (struct htmlp  *htmlp,
                                          const char    *data,
                                          size_t         n);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          htmlp_clear     (struct htmlp  *htmlp);

/* ------------------------------------------------------------------------ *
 * Remove a htmlp.                                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          htmlp_delete    (struct htmlp  *htmlp);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct htmlp *)htmlp_find_name (const char    *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct htmlp *)htmlp_find_id   (uint32_t       id);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          htmlp_vset_args (struct htmlp  *htmlp,
                                          va_list        args);
CHAOS_API(void)          htmlp_set_args  (struct htmlp  *htmlp,
                                          ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct htmlp_tag *)htmlp_tag_first (struct htmlp *htptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct htmlp_tag *)htmlp_tag_next  (struct htmlp *htptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct htmlp_tag *)htmlp_tag_find  (struct htmlp *htptr,
                                              const char   *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(uint32_t)          htmlp_tag_count (struct htmlp *htptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct htmlp_tag *)htmlp_tag_index (struct htmlp *htptr,
                                              uint32_t      i);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct htmlp_var *)htmlp_var_find  (struct htmlp *htptr,
                                              const char   *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct htmlp_var *)htmlp_var_set   (struct htmlp *htptr,
                                              const char   *name,
                                              const char   *value);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct htmlp *)htmlp_pop       (struct htmlp  *htmlp);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct htmlp *)htmlp_push      (struct htmlp **htmlpptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(char *)       htmlp_decode    (const char    *s);


/* ------------------------------------------------------------------------ *
 * Dump htmlps.                                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          htmlp_dump      (struct htmlp   *hcptr);

#endif /* LIB_HTMLP_H */
