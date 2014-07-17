/* chaosircd - pi-networks irc server
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: class.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_CLASS_H
#define SRC_CLASS_H

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct class {
  struct node node;
  uint32_t    id;
  uint32_t    refcount;
  hash_t      hash;
  uint64_t    ping_freq;
  uint32_t    max_clients;
  uint32_t    clients_per_ip;
  uint32_t    recvq;
  uint32_t    sendq;
  uint32_t    flood_trigger;
  uint64_t    flood_interval;
  uint32_t    throttle_trigger;
  uint64_t    throttle_interval;
  char        name[IRCD_CLASSLEN + 1];
};

/* -------------------------------------------------------------------------- *
  * -------------------------------------------------------------------------- */
extern int           class_log;
extern struct sheap  class_heap;
extern struct timer *class_timer;
extern uint32_t      class_serial;
extern struct list   class_list;

/* ------------------------------------------------------------------------ */
IRCD_API(int) class_get_log(void);

/* -------------------------------------------------------------------------- *
 * Initialize class heap and add garbage collect timer.                       *
 * -------------------------------------------------------------------------- */
extern void          class_init             (void);

/* -------------------------------------------------------------------------- *
 * Destroy class heap and cancel timer.                                       *
 * -------------------------------------------------------------------------- */
extern void          class_shutdown         (void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void          class_default          (struct class    *clptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct class *class_add              (const char    *name,
                                             uint64_t       ping_freq,
                                             uint32_t       max_clients,
                                             uint32_t       clients_per_ip,
                                             uint32_t       recvq,
                                             uint32_t       sendq,
                                             uint32_t       flood_trigger,
                                             uint64_t       flood_interval,
                                             uint32_t       throttle_trigger,
                                             uint64_t       throttle_interval);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int           class_update           (struct class  *clptr,
                                             uint64_t       ping_freq,
                                             uint32_t       max_clients,
                                             uint32_t       clients_per_ip,
                                             uint32_t       recvq,
                                             uint32_t       sendq,
                                             uint32_t       flood_trigger,
                                             uint64_t       flood_interval,
                                             uint32_t       throttle_trigger,
                                             uint64_t       throttle_interval);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void          class_delete           (struct class  *clptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct class *class_find_name        (const char    *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct class *class_find_id          (uint32_t       id);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct class *class_pop              (struct class  *clptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct class *class_push             (struct class **clptr);

/* -------------------------------------------------------------------------- *
 * Dump classes and class heap.                                               *
 * -------------------------------------------------------------------------- */
extern void          class_dump             (struct class  *clptr);

#endif /* SRC_CLASS_H */
