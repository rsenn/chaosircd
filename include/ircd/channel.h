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
 * $Id: channel.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_CHANNEL_H
#define SRC_CHANNEL_H

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/server.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include "libchaos/dlink.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct chanuser;
struct user;
struct client;
struct lclient;

struct logentry {
  struct node     node;
  time_t          ts;
  char            from  [IRCD_INFOLEN + 1];
  char            cmd   [32];
//  char            text  [IRCD_LINELEN + 1];
  char           *text;
};

struct channel {
  struct node      node;
  struct node      hnode;
  uint32_t         id;
  uint32_t         refcount;
  hash_t           hash;        /* channel name hash */
  time_t           ts;          /* channel timestamp */
  time_t           topic_ts;
  struct list      chanusers;   /* all channel members */
  struct list      lchanusers;  /* channel members coming from this server */
  struct list      rchanusers;  /* channel members coming from remote server */
  struct list      invites;     /* invite list */
  struct list      modelists[64];
  uint64_t         modes;
  uint32_t         serial;      /* burst serial */
  int              limit;
  char             name      [IRCD_CHANNELLEN + 1];
  char             topic     [IRCD_TOPICLEN + 1];
  char             topic_info[IRCD_PREFIXLEN + 1];
  char             modebuf   [IRCD_MODEBUFLEN + 1];
  char             parabuf   [IRCD_PARABUFLEN + 1];
  char             key       [IRCD_KEYLEN + 1];
  struct server   *server;      /* the server it was created on */
  struct list      backlog;     /* message log */
};

struct invite {
  struct node     unode;
  struct node     cnode;
  struct channel *channel;
  struct user    *user;
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define CHANNEL_HASH_SIZE 16

#define CHANNEL_PRIVMSG 0
#define CHANNEL_NOTICE 0

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define channel_is_member(chptr, cptr)  (chanuser_find(chptr, cptr) != NULL)

#define channel_is_valid(name)           ((name) && \
                                        (*(name) == '#'))

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int             channel_log;
extern struct sheap    channel_heap;
extern struct sheap    channel_invite_heap;
extern struct list     channel_list;
extern uint32_t        channel_serial;

/* ------------------------------------------------------------------------ */
IRCD_API(int) channel_get_log(void);

/* -------------------------------------------------------------------------- *
 * Initialize channel heap and add garbage collect timer.                     *
 * -------------------------------------------------------------------------- */
extern void            channel_init           (void);

/* -------------------------------------------------------------------------- *
 * Destroy channel heap and cancel timer.                                     *
 * -------------------------------------------------------------------------- */
extern void            channel_shutdown       (void);

/* -------------------------------------------------------------------------- *
 * Create a channel.                                                          *
 * -------------------------------------------------------------------------- */
extern struct channel *channel_new            (const char      *name);

/* -------------------------------------------------------------------------- *
 * Delete a channel.                                                          *
 * -------------------------------------------------------------------------- */
extern void            channel_delete         (struct channel  *chptr);

/* -------------------------------------------------------------------------- *
 * Loose all references of a channel block.                                   *
 * -------------------------------------------------------------------------- */
extern void            channel_release        (struct channel  *chptr);

/* -------------------------------------------------------------------------- *
 * Find a channel by name.                                                    *
 * -------------------------------------------------------------------------- */
extern struct channel *channel_find_name      (const char      *name);

/* -------------------------------------------------------------------------- *
 * Find a channel by id.                                                      *
 * -------------------------------------------------------------------------- */
extern struct channel *channel_find_id        (uint32_t         id);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct channel *channel_find_warn      (struct client   *cptr,
                                               const char      *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            channel_set_name       (struct channel  *chptr,
                                               const char      *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            channel_send           (struct lclient  *lcptr,
                                               struct channel  *chptr,
                                               uint64_t         flag,
                                               uint64_t         noflag,
                                               const char      *format,
                                               ...);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            channel_send_common    (struct client   *sptr,
                                               struct client   *one,
                                               const char      *format,
                                               ...);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            channel_delete_members (struct channel  *chptr,
                                               struct list     *list,
                                               int              delref);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int             channel_welcome        (struct channel  *chptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            channel_show_lusers    (struct channel  *chptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int             channel_nick           (struct channel  *cptr,
                                               struct channel  *sptr,
                                               char            *nick);
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int             channel_can_join       (struct channel  *chptr,
                                               struct client   *sptr,
                                               const char      *key);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern uint64_t        channel_get_automode   (struct channel  *chptr,
                                               struct client   *sptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            channel_topic          (struct lclient  *lcptr,
                                               struct client   *cptr,
                                               struct channel  *chptr,
                                               struct chanuser *cuptr,
                                               const char      *topic);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern size_t          channel_burst          (struct lclient *lcptr,
                                               struct channel *chptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            channel_message        (struct lclient *lcptr,
                                               struct client  *cptr,
                                               struct channel *chptr,
                                               intptr_t        type,
                                               const char     *text);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            channel_join           (struct lclient *lcptr,
                                               struct client  *cptr,
                                               const char     *name,
                                               const char     *key);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            channel_show           (struct client  *cptr);

/* -------------------------------------------------------------------------- *
 * Get a reference to an channel block                                        *
 * -------------------------------------------------------------------------- */
extern struct channel *channel_pop            (struct channel *chptr);

/* -------------------------------------------------------------------------- *
 * Push back a reference to a channel block                                   *
 * -------------------------------------------------------------------------- */
extern struct channel *channel_push           (struct channel **chptrptr);

/* -------------------------------------------------------------------------- *
 * Adds an entry to the channel backlog                                       *
 * -------------------------------------------------------------------------- */
extern void            channel_backlog        (struct channel *chptr,
                                               struct client  *cptr,
                                               const char     *cmd,
                                               const char     *text);

/* -------------------------------------------------------------------------- *
 * Dump channels and channel heap.                                            *
 * -------------------------------------------------------------------------- */
extern void            channel_dump           (struct channel *chptr);

#endif /* SRC_CHANNEL_H */
