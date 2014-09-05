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
 * $Id: client.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_CLIENT_H
#define SRC_CLIENT_H

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include "net.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define CLIENT_LOCAL  0
#define CLIENT_REMOTE 1
#define CLIENT_GLOBAL 2

#define CLIENT_USER    0
#define CLIENT_SERVER  1
#define CLIENT_SERVICE 2

#define CLIENT_HASH_SIZE 32
#define CLIENT_HISTORY_SIZE 64

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include "dlink.h"
#include "net.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
/*#include "channel.h"
#include "lclient.h"
#include "ircd.h"*/

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct slink_reply {
  int      command;
  int      datalen;
  int      gotdatalen;
  int      readdata;
  uint8_t *data;
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client
{
  struct node      node;
  struct node      lnode;
  struct node      gnode;
  struct node      nnode;
  struct node      dnode;
  uint32_t         id;          /* a unique id */
  uint32_t         refcount;    /* how many times this block is referenced */
  hash_t           hash;
  uint32_t         type;
  uint32_t         location;
  struct lclient  *lclient;     /* if its a local client */
  struct lclient  *source;      /* local server the client comes from */
  struct client   *origin;      /* remote server the client comes from */
  struct user     *user;
  struct oper     *oper;
  struct server   *server;
  struct service  *service;
  uint32_t         serial;
  uint32_t         hops;
  time_t           created;
  time_t           lastmsg;
  time_t           lastread;
  time_t           ts;
  net_addr_t       ip;
  hash_t           hhash;
  hash_t           ihash;
  hash_t           rhash;
  char             name    [IRCD_HOSTLEN + 1];
  char             host    [IRCD_HOSTLEN + 1];   /* client's hostname */
  char             hostreal[IRCD_HOSTLEN + 1];
  char             hostip  [IRCD_HOSTIPLEN + 1];
  char             info    [IRCD_INFOLEN + 1];   /* free form client info */
};

struct history {
  struct node    node;
  hash_t         hash;
  struct client *client;
  char           nick[IRCD_NICKLEN + 1];
};

#define client_is_local(x)     ((x)->location == CLIENT_LOCAL)
#define client_is_remote(x)    ((x)->location == CLIENT_REMOTE)
#define client_is_user(x)      ((x)->type == CLIENT_USER)
#define client_is_server(x)    ((x)->type == CLIENT_SERVER)
#define client_is_service(x)   ((x)->type == CLIENT_SERVICE)

#define client_is_localuser(x)    (client_is_local(x) && client_is_user(x))
#define client_is_localserver(x)  (client_is_local(x) && client_is_server(x))
#define client_is_remoteuser(x)   (client_is_remote(x) && client_is_user(x))
#define client_is_remoteserver(x) (client_is_remote(x) && client_is_server(x))

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int             client_log;
extern struct client  *client_me;
extern uint32_t        client_serial;
extern struct list     client_list;          /* contains all clients */
extern struct list     client_lists[3][3];
extern struct lclient *client_source;

/* ------------------------------------------------------------------------ */
IRCD_API(int) client_get_log(void);

/* -------------------------------------------------------------------------- *
 * Initialize client heap and add garbage collect timer.                      *
 * -------------------------------------------------------------------------- */
extern void            client_init           (void);

/* -------------------------------------------------------------------------- *
 * Destroy client heap and cancel timer.                                      *
 * -------------------------------------------------------------------------- */
extern void            client_shutdown       (void);

/* -------------------------------------------------------------------------- *
 * Create a new client coming from a local connection.                        *
 * -------------------------------------------------------------------------- */
extern struct client  *client_new_local      (int             type,
                                              struct lclient *lcptr);

/* -------------------------------------------------------------------------- *
 * Create a new client coming from a remote connection                        *
 * -------------------------------------------------------------------------- */
extern struct client  *client_new_remote     (int             type,
                                              struct lclient *lcptr,
                                              struct user    *uptr,
                                              struct client  *origin,
                                              const char     *name,
                                              int             hops,
                                              const char     *host,
                                              const char     *hostip,
                                              const char     *info);

/* -------------------------------------------------------------------------- *
 * Create a new client coming from a remote connection                        *
 * -------------------------------------------------------------------------- */
extern struct client  *client_new_service    (struct user    *uptr,
                                              const char     *name,
                                              const char     *host,
                                              const char     *hostip,
                                              const char     *info);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            client_delete         (struct client  *lcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            client_release        (struct client  *cptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
/*extern user_t   *client_user_new       (struct client  *client);*/

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct client  *client_find_id        (uint32_t        id);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct client  *client_find_name      (const char     *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct client  *client_find_nick      (const char     *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct history *client_history_find   (const char     *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct history *client_history_add    (struct client  *cptr,
                                              const char     *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            client_history_delete (struct history *hptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            client_history_clean  (struct client  *cptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct client  *client_find_host      (const char     *host);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct client  *client_find_uid       (const char     *uid);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct client  *client_find_nickw     (struct client  *cptr,
                                              const char     *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct client  *client_find_nickh     (const char     *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct client  *client_find_nickhw    (struct client  *cptr,
                                              const char     *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            client_set_name       (struct client  *cptr,
                                              const char     *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int             client_register_local (struct client  *cptr);


/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int             client_welcome        (struct client  *cptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            client_lusers         (struct client  *cptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int             client_nick           (struct lclient *lcptr,
                                              struct client  *cptr,
                                              char           *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            client_nick_random    (char           *buf);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int             client_nick_rotate    (const char     *name,
                                              char           *buf);

/* -------------------------------------------------------------------------- *
 * introduce acptr to the rest of the net                                     *
 *                                                                            *
 * <lcptr>         the local connection the new client is coming from         *
 * <acptr>         the new client                                             *
 * -------------------------------------------------------------------------- */
extern void            client_introduce      (struct lclient *lcptr,
                                              struct client  *cptr,
                                              struct client  *acptr);

/* -------------------------------------------------------------------------- *
 * burst acptr to cptr                                                        *
 * -------------------------------------------------------------------------- */
extern void            client_burst          (struct lclient *cptr,
                                              struct client  *acptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            client_vexit          (struct lclient *lcptr,
                                              struct client  *cptr,
                                              const char     *comment,
                                              va_list         args);

extern void            client_exit           (struct lclient *lcptr,
                                              struct client  *cptr,
                                              const char     *comment,
                                              ...);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            client_vsend          (struct client  *cptr,
                                              const char     *format,
                                              va_list         args);

extern void            client_send           (struct client  *cptr,
                                              const char     *format,
                                              ...);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int             client_relay          (struct lclient *lcptr,
                                              struct client  *cptr,
                                              struct client  *acptr,
                                              const char     *format,
                                              char          **argv);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int             client_relay_always   (struct lclient *lcptr,
                                              struct client  *cptr,
                                              struct client **acptrptr,
                                              uint32_t        cindex,
                                              const char     *format,
                                              int            *argc,
                                              char          **argv);

extern void            client_message        (struct lclient *lcptr,
                                              struct client  *cptr,
                                              struct client  *acptr,
                                              int             type,
                                              const char     *text);

/* -------------------------------------------------------------------------- *
 * Get a reference to a client block                                          *
 * -------------------------------------------------------------------------- */
extern struct client  *client_pop            (struct client  *cptr);

/* -------------------------------------------------------------------------- *
 * Push back a reference to a client block                                    *
 * -------------------------------------------------------------------------- */
extern struct client  *client_push           (struct client **cptrptr);

/* -------------------------------------------------------------------------- *
 * Dump clients and client heap.                                              *
 * -------------------------------------------------------------------------- */
extern void            client_dump           (struct client  *cptr);

#endif /* SRC_CLIENT_H */
