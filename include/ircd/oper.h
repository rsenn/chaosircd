/* cgircd - CrowdGuard IRC daemon
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
 * $Id: oper.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_OPER_H
#define SRC_OPER_H

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include <libchaos/dlink.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include <ircd/class.h>
#include <ircd/client.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct oper
{
  struct node   node;
  hash_t        hash;
  char          name[IRCD_USERLEN + 1];
  char          passwd[IRCD_PASSWDLEN + 1];
  struct class *clptr;
  int           level;
  struct list   online;
  uint64_t      sources;
  uint32_t      flags;
  uint32_t      refcount;
  char          privs[33];
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define OPER_FLAG_GLOBAL      0x0001
#define OPER_FLAG_REMOTE      0x0002
#define OPER_FLAG_KLINE       0x0004
#define OPER_FLAG_GLINE       0x0008
#define OPER_FLAG_UNKLINE     0x0010
#define OPER_FLAG_UNGLINE     0x0020
#define OPER_FLAG_NICKCHG     0x0040
#define OPER_FLAG_DIE         0x0080
#define OPER_FLAG_REHASH      0x0100
#define OPER_FLAG_ADMIN       0x0200
#define OPER_FLAG_ENFORCE     0x0400

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int           oper_log;
extern struct sheap  oper_heap;
extern struct timer *oper_timer;
extern struct list   oper_list;
extern struct dlog  *oper_drain;

/* -------------------------------------------------------------------------- *
 * Initialize oper heap and add garbage collect timer.                        *
 * -------------------------------------------------------------------------- */
extern void         oper_init             (void);

/* -------------------------------------------------------------------------- *
 * Destroy oper heap and cancel timer.                                        *
 * -------------------------------------------------------------------------- */
extern void         oper_shutdown         (void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void         oper_default          (struct oper   *optr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct oper *oper_add              (const char    *name,
                                           const char    *passwd,
                                           struct class  *clptr,
                                           int            level,
                                           uint64_t       sources,
                                           uint32_t       flags);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int          oper_update           (struct oper   *optr,
                                           const char    *passwd,
                                           struct class  *clptr,
                                           int            level,
                                           uint64_t       sources,
                                           uint32_t       flags);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void         oper_delete           (struct oper   *optr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct oper *oper_find             (const char    *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void         oper_up               (struct oper   *optr,
                                           struct client *sptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void         oper_down             (struct oper   *optr,
                                           struct client *sptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct oper *oper_pop              (struct oper   *optr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct oper *oper_push             (struct oper  **optr);

/* -------------------------------------------------------------------------- *
 * Dump opers and oper heap.                                                  *
 * -------------------------------------------------------------------------- */
#ifdef DEBUG
extern void         oper_dump             (void);
#endif /* DEBUG */

#endif /* SRC_OPER_H */
