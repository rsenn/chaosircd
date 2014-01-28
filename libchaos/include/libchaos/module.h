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
 * $Id: module.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_MODULE_H
#define LIB_MODULE_H

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/dlink.h"

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct module;

typedef int  (module_load_t)(struct module *mptr);
typedef void (module_unload_t)(struct module *mptr);

struct module {
  struct node      node;
  uint32_t         id;
  uint32_t         refcount;
  uint32_t         nhash;
  uint32_t         phash;
  int              fd;
  void            *map;
  void            *handle;
  module_load_t   *load;
  module_unload_t *unload;
  char             name[32];
  char             path[64];
};

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int            )module_log;
CHAOS_API(struct sheap   )module_heap;
CHAOS_API(struct list    )module_list;
CHAOS_API(struct timer  *)module_timer;
CHAOS_API(uint32_t       )module_id;

/* ------------------------------------------------------------------------ */
CHAOS_API(int) module_get_log(void);

/* ------------------------------------------------------------------------ *
 * Initialize module heap.                                                    *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           )module_init      (void);

/* ------------------------------------------------------------------------ *
 * Destroy module heap.                                                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           )module_shutdown  (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           )module_setpath   (const char    *path);

  /* ------------------------------------------------------------------------ *
 * Add a module.                                                              *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct module *)module_add       (const char    *path);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int            )module_update    (struct module *mptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int            )module_reload    (struct module *mptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(const char    *)module_expand    (const char    *name);

/* ------------------------------------------------------------------------ *
 * Remove a module.                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_API(void           )module_delete    (struct module *module);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct module *)module_find_path (const char    *path);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct module *)module_find_name (const char    *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct module *)module_find_id   (uint32_t       id);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct module *)module_pop       (struct module *mptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct module *)module_push      (struct module **mptrptr);

/* ------------------------------------------------------------------------ *
 * Dump modules.                                                              *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)           module_dump      (struct module *mptr);

#endif /* LIB_MODULE_H */
