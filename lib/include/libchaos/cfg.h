/* chaosircd - Chaoz's IRC daemon daemon
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
 * $Id: cfg.h,v 1.4 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_CFG_H
#define LIB_CFG_H

/* ------------------------------------------------------------------------ *
 * Library headers                                                            *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/dlink.h"

/* ------------------------------------------------------------------------ *
 * Constants                                                                  *
 * ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ *
 * cfg block structure.                                                     *
 * ------------------------------------------------------------------------ */
struct cfg
{
  struct node            node;        /* linking node for cfg_list */
  uint32_t               id;
  uint32_t               refcount;    /* times this block is referenced */
  hash_t                 hash;
  struct list            chain;       /* chain of ini files */
  char                   name[64];    /* user-definable name */
};

/* ------------------------------------------------------------------------ *
 * Global variables                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_DATA(int)               cfg_log;
CHAOS_DATA(struct sheap)      cfg_heap;     /* heap containing cfg blocks */
CHAOS_DATA(struct dheap)      cfg_data_heap;/* heap containing the actual cfgs */
CHAOS_DATA(struct list)       cfg_list;     /* list linking cfg blocks */
CHAOS_DATA(struct timer *)    cfg_timer;
CHAOS_DATA(uint32_t)          cfg_id;
CHAOS_DATA(int)               cfg_dirty;

/* ------------------------------------------------------------------------ *
 * Initialize cfg heap and add garbage collect timer.                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(void               cfg_init            (void))

/* ------------------------------------------------------------------------ *
 * Destroy cfg heap and cancel timer.                                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(void               cfg_shutdown        (void))

/* ------------------------------------------------------------------------ *
 * Garbage collect                                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(int                cfg_collect         (void))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void               cfg_default         (struct cfg  *iptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct cfg *       cfg_new             (const char  *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int                cfg_load            (struct cfg  *cfptr,
                                                  const char  *path))

/* ------------------------------------------------------------------------ *
 * Loose all references                                                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void               cfg_release         (struct cfg  *cfptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void               cfg_delete         (struct cfg  *cfptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void               cfg_set_name        (struct cfg  *cfptr,
                                                  const char    *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(const char *       cfg_get_name        (struct cfg  *cfptr))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct cfg *       cfg_find_name       (const char    *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct cfg *       cfg_find_id         (uint32_t       id))

/* ------------------------------------------------------------------------ *
 * Dump cfgers and cfg heap.                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(void               cfg_dump            (struct cfg  *cfptr))

#endif
