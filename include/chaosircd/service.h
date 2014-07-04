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
 * $Id: service.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_SERVICE_H
#define SRC_SERVICE_H

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/dlink.h>
#include <libchaos/net.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include <chaosircd/channel.h>
#include <chaosircd/lclient.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define USER_HASH_SIZE      16

/* -------------------------------------------------------------------------- *
 * Types                                                                      *
 * -------------------------------------------------------------------------- */
struct channel;
struct invite;

typedef void (service_callback_t)(struct lclient *, struct client *,
                                  struct channel *, const char *msg);

struct service_handler {
  struct node         node;
  char                name[32];
  hash_t              hash;
  service_callback_t *callback;
};

struct service {
  struct node    node;          /* node for service_list */
  struct node    hnode;         /* node for service_lists[] */
  uint32_t       id;            /* a unique id */
  uint32_t       refcount;      /* how many times this block is referenced */
  hash_t         nhash;
  hash_t         uhash;
  struct client *client;
  struct user   *user;
  struct list    handlers;
  char           name[IRCD_NICKLEN + 1];
};

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
IRCD_DATA(int)           service_log;      /* service log source */
extern struct sheap     service_heap;     /* heap for lclient_t */
IRCD_DATA(struct list)   service_list;     /* list with all of them */
IRCD_DATA(uint32_t)      service_id;

/* -------------------------------------------------------------------------- *
 * Initialize service module.                                                    *
 * -------------------------------------------------------------------------- */
IRCD_API(void)          service_init         (void);

/* -------------------------------------------------------------------------- *
 * Shutdown the service module.                                                  *
 * -------------------------------------------------------------------------- */
IRCD_API(void)          service_shutdown     (void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void)          service_collect      (void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct service*)service_new          (const char     *name,
                                              const char     *user,
                                              const char     *host,
                                              const char     *info);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void)          service_delete       (struct service *svptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct service*)service_find_id      (uint32_t        id);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct service*)service_find_uid     (const char     *uid);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct service*)service_find_name    (const char     *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void)          service_set_name     (struct service *svptr,
                                              const char     *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct service_handler *service_register     (struct service     *svptr,
                                              const char         *msg,
                                              service_callback_t *callback);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void)          service_vhandle      (struct service     *svptr,
                                              struct lclient     *lcptr,
                                              struct client      *cptr,
                                              struct channel     *chptr,
                                              const char         *cmd,
                                              const char         *format,
                                              va_list             args);

IRCD_API(void)          service_handle       (struct service     *svptr,
                                              struct lclient     *lcptr,
                                              struct client      *cptr,
                                              struct channel     *chptr,
                                              const char         *cmd,
                                              const char         *format,
                                              ...);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void)          service_release      (struct service *svptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct service*)service_pop          (struct service *svptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct service*)service_push         (struct service **ucptrptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void)          service_dump         (struct service*svptr);

#endif /* SRC_SERVICE_H */
