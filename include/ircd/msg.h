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
 * $Id: msg.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_MSG_H
#define SRC_MSG_H

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include <libchaos/dlink.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client;
struct lclient;

/* -------------------------------------------------------------------------- *
 * Constants                                                                  *
 * -------------------------------------------------------------------------- */

/* Callback types */
#define MSG_UNREGISTERED 0x00         /* for unregistered clients */
#define MSG_CLIENT       0x01         /* for registered clients */
#define MSG_SERVER       0x02         /* for servers */
#define MSG_OPER         0x03         /* for opers */
#define MSG_LAST         MSG_OPER + 1 /* callback array size */

/* Hashtable size */
#define MSG_HASH_SIZE    32

/* Message flags */
#define MFLG_CLIENT 0x01 /* flood throttling */
#define MFLG_UNREG  0x02 /* available to unregistered clients */
#define MFLG_IGNORE 0x04 /* ignore from unregistered clients */
#define MFLG_HIDDEN 0x08 /* hidden from everyone */
#define MFLG_SERVER 0x10 /* server-only command */
#define MFLG_OPER   0x20 /* oper-only command */

/* -------------------------------------------------------------------------- *
 * Callback function type for message handlers                                *
 * -------------------------------------------------------------------------- */
typedef void (msg_handler_t)(struct lclient *, struct client *,
                             int,              char          **);

/* -------------------------------------------------------------------------- *
 * Message structure                                                          *
 * -------------------------------------------------------------------------- */
struct msg {
  char          *cmd;
  size_t         args;
  size_t         maxargs;
  size_t         flags;
  msg_handler_t *handlers[MSG_LAST];
  char         **help;
  hash_t         hash;
  uint32_t       counts[MSG_LAST];
  size_t         bytes;
  uint32_t       id;
  struct node    node;
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int         msg_log;
extern struct list msg_table[MSG_HASH_SIZE];

/* ------------------------------------------------------------------------ */
IRCD_API(int) msg_get_log(void);

/* -------------------------------------------------------------------------- *
 * Initialize message heap.                                                   *
 * -------------------------------------------------------------------------- */
extern void        msg_init       (void);

/* -------------------------------------------------------------------------- *
 * Destroy message heap.                                                      *
 * -------------------------------------------------------------------------- */

extern void        msg_shutdown   (void);
/* -------------------------------------------------------------------------- *
 * Find a message.                                                            *
 * -------------------------------------------------------------------------- */
extern struct msg *msg_find       (const char     *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct msg *msg_find_id    (uint32_t        id);

/* -------------------------------------------------------------------------- *
 * Register a message.                                                        *
 * -------------------------------------------------------------------------- */
extern struct msg *msg_register   (struct msg     *msg);

/* -------------------------------------------------------------------------- *
 * Unregister a message.                                                      *
 * -------------------------------------------------------------------------- */
extern void        msg_unregister (struct msg     *msg);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void        m_unregistered (struct lclient *lcptr,
                                   struct client  *cptr,
                                   int             argc,
                                   char          **argv);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void        m_registered   (struct lclient *lcptr,
                                   struct client  *cptr,
                                   int             argc,
                                   char          **argv);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void        m_ignore       (struct lclient *lcptr,
                                   struct client  *cptr,
                                   int             argc,
                                   char          **argv);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void        m_not_oper     (struct lclient *lcptr,
                                   struct client  *cptr,
                                   int             argc,
                                   char          **argv);

/* -------------------------------------------------------------------------- *
 * Dump message stack.                                                        *
 * -------------------------------------------------------------------------- */
extern void        msg_dump       (struct msg     *mptr);

#endif /* SRC_MSG_H */
