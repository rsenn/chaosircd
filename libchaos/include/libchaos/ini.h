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
 * $Id: ini.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_INI_H
#define LIB_INI_H

#include "libchaos/image.h"

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#define INI_READ  0
#define INI_WRITE 1

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct ini;

typedef void (ini_callback_t)(struct ini *ini);

struct ini_key 
{
  struct node              node;
  uint32_t                 hash;
  char                    *name;
  char                    *value;
};

struct ini_section 
{
  struct node              node;
  struct list              keys;
  struct ini              *ini;
  uint32_t                 hash;
  char                    *name;
};

struct ini 
{
  struct node              node;
  uint32_t                 id;
  uint32_t                 refcount;
  uint32_t                 nhash;
  uint32_t                 phash;  
  struct list              keys;        /* keys & comments before first section */
  struct list              sections;
  int                      fd;
  char                     path[PATHLEN];
  char                     name[64];
  int                      mode;
  int                      changed;
  struct ini_section      *current;
  ini_callback_t          *cb;
  char                     error[256];
};

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)                 ini_log;
CHAOS_API(struct sheap)        ini_heap;
CHAOS_API(struct sheap)        ini_key_heap;
CHAOS_API(struct sheap)        ini_sec_heap;
CHAOS_API(struct list)         ini_list;
CHAOS_API(uint32_t)            ini_serial;
CHAOS_API(struct timer *)      ini_timer;

/* ------------------------------------------------------------------------ */
CHAOS_API(int) ini_get_log(void);

/* ------------------------------------------------------------------------ *
 * Initialize INI heaps and add garbage collect timers.                     *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)         ini_init(void);

/* ------------------------------------------------------------------------ *
 * Destroy INI heap and cancel timers.                                      *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)         ini_shutdown(void);
  
/* ------------------------------------------------------------------------ *
 * New INI context.                                                         *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini *)ini_add(const char *path);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)          ini_update(struct ini *ini);

/* ------------------------------------------------------------------------ *
 * Find INI context.                                                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini *)        ini_find_name        (const char         *name);

/* ------------------------------------------------------------------------ *
 * Find INI context.                                                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini *)        ini_find_path        (const char         *path);

/* ------------------------------------------------------------------------ *
 * Find INI context.                                                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini *)        ini_find_id          (uint32_t            id);

/* ------------------------------------------------------------------------ *
 * Destroy INI context.                                                     *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)                ini_remove           (struct ini         *ini);

/* ------------------------------------------------------------------------ *
 * Open INI file.                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)                 ini_open             (struct ini         *ini,
                                                     int                 mode);

/* ------------------------------------------------------------------------ *
 * Open INI file.                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)                 ini_open_fd          (struct ini         *ini,
                                                     int                 fd);

/* ------------------------------------------------------------------------ *
 * Load all sections of an INI file.                                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)                 ini_load             (struct ini         *ini);

/* ------------------------------------------------------------------------ *
 * Save all sections to an INI file.                                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)                 ini_save             (struct ini         *ini); 

/* ------------------------------------------------------------------------ *
 * Close INI file.                                                          *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)                ini_close            (struct ini         *ini);

/* ------------------------------------------------------------------------ *
 * Free INI context.                                                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)                ini_free             (struct ini         *ini);

/* ------------------------------------------------------------------------ *
 * Setup callback.                                                          *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)                ini_callback         (struct ini         *ini,
                                                 ini_callback_t     *cb);

/* ------------------------------------------------------------------------ *
 * Find a INI section by name.                                              *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini_section *)ini_section_find     (struct ini         *ini, 
                                                 const char         *name);

/* ------------------------------------------------------------------------ *
 * Find a INI section by name.                                              *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini_section *)ini_section_find_next(struct ini         *ini, 
                                                 const char         *name);

/* ------------------------------------------------------------------------ *
 * Create new section.                                                      *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini_section *)ini_section_new      (struct ini         *ini,
                                                 const char         *name);

/* ------------------------------------------------------------------------ *
 * Delete a section.                                                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)                ini_section_remove   (struct ini         *ini, 
                                                 struct ini_section *section);

/* ------------------------------------------------------------------------ *
 * Clear content.                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)                ini_clear            (struct ini         *ini);
  
/* ------------------------------------------------------------------------ *
 * Get current section name.                                                *
 * ------------------------------------------------------------------------ */
CHAOS_API(char *)              ini_section_name     (struct ini         *ini);

/* ------------------------------------------------------------------------ *
 * Set current section by index.                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini_section *)ini_section_index    (struct ini         *ini,
                                                 uint32_t            index);

/* ------------------------------------------------------------------------ *
 * Get pointer to first section.                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini_section *)ini_section_first    (struct ini         *ini);

/* ------------------------------------------------------------------------ *
 * Skip to next section and return current                                  *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini_section *)ini_section_next     (struct ini         *ini);

/* ------------------------------------------------------------------------ *
 * Return number of sections.                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(uint32_t)            ini_section_count    (struct ini         *ini);

/* ------------------------------------------------------------------------ *
 * Return position of current section.                                      *
 * ------------------------------------------------------------------------ */
CHAOS_API(uint32_t)            ini_section_pos      (struct ini         *ini);

/* ------------------------------------------------------------------------ *
 * Rewind to first section.                                                 *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)                ini_section_rewind   (struct ini         *ini);

/* ------------------------------------------------------------------------ *
 * Write data to .ini, creating new key when necessary.                     *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)                 ini_write_str        (struct ini_section *section,
                                                     const char         *key,
                                                     const char         *str);

CHAOS_API(int)                 ini_write_int        (struct ini_section *section,
                                                     const char         *key,
                                                     int                 i);

CHAOS_API(int)                 ini_write_ulong_long (struct ini_section *section,
                                                     const char         *key,
                                                     uint64_t            u);

CHAOS_API(int)                 ini_write_double     (struct ini_section *section,
                                                     const char         *key,
                                                     double              d);

/* ------------------------------------------------------------------------ *
 * Read data from .ini, returning -1 when key not found.                      *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)                 ini_read_str         (struct ini_section *section,
                                                     const char         *key,
                                                     char              **str);

CHAOS_API(int)                 ini_get_str          (struct ini_section *section, 
                                                     const char         *key, 
                                                     char               *str, 
                                                     size_t              len);

CHAOS_API(int)                 ini_read_int         (struct ini_section *section,
                                                     const char         *key,
                                                     int                *i);

CHAOS_API(int)                 ini_read_ulong_long  (struct ini_section *section,
                                                     const char         *key,
                                                     uint64_t           *u);

CHAOS_API(int)                 ini_read_double      (struct ini_section *section,
                                                     const char         *key,
                                                     double             *d);

/* ------------------------------------------------------------------------ *
 * Read data from .ini, defaulting to a given value.                          *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)                 ini_read_str_def     (struct ini_section *section,
                                                     const char         *key,
                                                     char              **str,
                                                     char               *def);

CHAOS_API(int)                 ini_get_str_def      (struct ini_section *section,
                                                     const char         *key,
                                                     char               *str,
                                                     size_t              len,
                                                     char               *def);

CHAOS_API(int)                 ini_read_int_def     (struct ini_section *section,
                                                     const char         *key,
                                                     int                *i,
                                                     int                 def);

CHAOS_API(int)                 ini_read_double_def  (struct ini_section *section,
                                                     const char         *key,
                                                     double             *d,
                                                     double              def);

CHAOS_API(int)                 ini_read_color       (struct ini_section *section,
                                                     const char         *key,
                                                     struct color       *color);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini *)        ini_pop              (struct ini        *ini);
  
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct ini *)        ini_push             (struct ini       **iniptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)                ini_dump             (struct ini        *ini);

#endif /* LIB_INI_H */
