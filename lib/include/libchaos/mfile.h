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
 * $Id: mfile.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_MFILE_H
#define LIB_MFILE_H

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/dlink.h"

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#define MFILE_LINELEN 1024

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct mfile {
  struct node node;
  uint32_t    id;
  uint32_t    refcount;
  struct list lines;
  hash_t      nhash;
  hash_t      phash;
  int         fd;
  char        path[256];
  char        name[32];
};

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_DATA(int          )mfile_log;
CHAOS_DATA(struct sheap )mfile_heap;
CHAOS_DATA(struct dheap )mfile_dheap;
CHAOS_DATA(struct list  )mfile_list;
CHAOS_DATA(uint32_t     )mfile_id;

/* ------------------------------------------------------------------------ */
CHAOS_API(int  mfile_get_log(void))

/* ------------------------------------------------------------------------ *
 * Initialize mfile heap.                                                    *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           mfile_init      (void))

/* ------------------------------------------------------------------------ *
 * Destroy mfile heap.                                                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           mfile_shutdown  (void))

/* ------------------------------------------------------------------------ *
 * Add a mfile.                                                              *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct mfile * mfile_add       (const char    *path))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int            mfile_update    (struct mfile  *mfile))

/* ------------------------------------------------------------------------ *
 * Remove a mfile.                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           mfile_delete    (struct mfile *mfile))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct mfile * mfile_find_path (const char    *path))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct mfile * mfile_find_name (const char    *name))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct mfile * mfile_find_id   (uint32_t       id))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct mfile * mfile_pop       (struct mfile  *mfile))

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct mfile * mfile_push      (struct mfile **mfileptr))

/* ------------------------------------------------------------------------ *
 * Dump mfiles.                                                              *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           mfile_dump     (struct mfile   *mfptr))

#endif
