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
 * $Id: filter.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_FILTER_H
#define LIB_FILTER_H

/* ------------------------------------------------------------------------ *
 * Library headers                                                            *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/net.h"
#include "libchaos/dlink.h"

/* ------------------------------------------------------------------------ *
 * Constants                                                                  *
 * ------------------------------------------------------------------------ */
#define FILTER_PROTO 0
#define FILTER_SRCIP 1
#define FILTER_DSTIP 2
#define FILTER_SRCNET 3
#define FILTER_DSTNET 4

#define FILTER_DENY   0
#define FILTER_ACCEPT 1

#define FILTER_LIFETIME (60 * 1000ull)
#define FILTER_INTERVAL (120 * 1000ull)

#define FILTER_MAX_INSTRUCTIONS 4096
#define FILTER_MAX_SIZE         (FILTER_MAX_INSTRUCTIONS * sizeof(struct bpf_insn))

/* ------------------------------------------------------------------------ *
 * filter block structure.                                                    *
 * ------------------------------------------------------------------------ */
struct filter_rule
{
  struct node         node;
  int                 type;
  int                 action;
  uint32_t            address;
  uint32_t            netmask;
  uint64_t            ts;
};

struct filter
{
  struct node         node;                 /* linking node for filter_list */
  uint32_t            id;
  uint32_t            refcount;             /* times this block is referenced */
  hash_t              hash;
  void               *prog;                 /* pointer to filter program struct */

  /* externally initialised */
  struct list         rules;
  struct list         sockets;      /* the sockets the filter is attached to */
  struct list         listeners;
  char                name[HOSTLEN + 1];    /* user-definable name */
};

/* ------------------------------------------------------------------------ *
 * Global variables                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_DATA(int)           filter_log;
CHAOS_DATA(struct sheap)  filter_heap;       /* heap containing filter blocks */
CHAOS_DATA(struct sheap)  filter_rule_heap;  /* heap containing filter rules */
CHAOS_DATA(struct dheap)  filter_prog_heap;  /* heap containing the actual filters */
CHAOS_DATA(struct list)   filter_list;       /* list linking filter blocks */
CHAOS_DATA(struct timer *)filter_timer;
CHAOS_DATA(uint32_t)      filter_id;
CHAOS_DATA(int)           filter_dirty;

/* ------------------------------------------------------------------------ */
CHAOS_API(int             filter_get_log(void))

/* ------------------------------------------------------------------------ *
 * Initialize filterer heap and add garbage collect timer.                    *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_init            (void))

/* ------------------------------------------------------------------------ *
 * Destroy filterer heap and cancel timer.                                    *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_shutdown        (void))

/* ------------------------------------------------------------------------ *
 * Garbage collect                                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(int             filter_collect         (void))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_default         (struct filter  *fptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct filter * filter_add             (const char     *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_delete          (struct filter  *fptr))

/* ------------------------------------------------------------------------ *
 * Loose all references                                                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_release         (struct filter  *fptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct filter * filter_pop             (struct filter  *fptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct filter * filter_push            (struct filter **fptrptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct filter * filter_find            (const char     *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_set_name        (struct filter  *fptr,
                                                  const char     *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(const char *    filter_get_name        (struct filter  *fptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct filter * filter_find_name       (const char     *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct filter * filter_find_id         (uint32_t        id))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_rule_add        (struct filter  *fptr,
                                                  int             type,
                                                  int             action,
                                                  uint32_t        data1,
                                                  uint32_t        data2,
                                                  uint64_t        lifetime))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_rule_insert     (struct filter  *fptr,
                                                  int             type,
                                                  int             action,
                                                  uint32_t        data1,
                                                  uint32_t        data2,
                                                  uint64_t        lifetime))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_rule_delete     (struct filter  *fptr,
                                                  int             type,
                                                  int             action,
                                                  uint32_t        data1,
                                                  uint32_t        data2))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_rule_compile    (struct filter  *fptr))

/* ------------------------------------------------------------------------ *
 * Attach a filter to a socket                                                *
 * ------------------------------------------------------------------------ */
CHAOS_API(int             filter_attach_socket   (struct filter  *fptr,
                                                  int             fd))

/* ------------------------------------------------------------------------ *
 * Detach filter from socket                                                  *
 * ------------------------------------------------------------------------ */
CHAOS_API(int             filter_detach_socket   (struct filter  *fptr,
                                                  int             fd))

/* ------------------------------------------------------------------------ *
 * Attach a filter to a listener                                              *
 * ------------------------------------------------------------------------ */
CHAOS_API(int             filter_attach_listener (struct filter  *fptr,
                                                  struct listen  *liptr))

/* ------------------------------------------------------------------------ *
 * Detach filter from listener                                                *
 * ------------------------------------------------------------------------ */
CHAOS_API(int             filter_detach_listener (struct filter  *fptr,
                                                  struct listen  *liptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_reattach_all    (struct filter  *fptr))

/* ------------------------------------------------------------------------ *
 * Dump filterers and filter heap.                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(void            filter_dump            (struct filter  *lptr))

#endif
