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
#include "libchaos/net.h"

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
#include "libchaos/dlink.h"
#include "libchaos/net.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
/*#include "ircd/channel.h"
#include "ircd/lclient.h"
#include "ircd/ircd.h"*/

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
struct client {
	struct node node;
	struct node lnode;
	struct node gnode;
	struct node nnode;
	struct node dnode;
	uint32_t id; /* a unique id */
	uint32_t refcount; /* how many times this block is referenced */
	hash_t hash;
	uint32_t type;
	int location;
	struct lclient *lclient; /* if its a local client */
	struct lclient *source; /* local server the client comes from */
	struct client *origin; /* remote server the client comes from */
	struct user *user;
	struct oper *oper;
	struct server *server;
	struct service *service;
	uint32_t serial;
	uint32_t hops;
	time_t created;
	time_t lastmsg;
	time_t lastread;
	time_t ts;
	net_addr_t ip;
	hash_t hhash;
	hash_t ihash;
	hash_t rhash;
	char name[IRCD_HOSTLEN + 1];
	char host[IRCD_HOSTLEN + 1]; /* client's hostname */
	char hostreal[IRCD_HOSTLEN + 1];
	char hostip[IRCD_HOSTIPLEN + 1];
	char info[IRCD_INFOLEN + 1]; /* free form client info */
};

struct history {
	struct node node;
	hash_t hash;
	struct client *client;
	char nick[IRCD_NICKLEN + 1];
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
IRCD_DATA(int) client_log;
IRCD_DATA(struct client*) client_me;
IRCD_DATA(uint32_t) client_serial;
IRCD_DATA(struct list) client_list; /* contains all clients */
IRCD_DATA(struct list) client_lists[3][3];
IRCD_DATA(struct lclient*) client_source;

/* ------------------------------------------------------------------------ */
IRCD_API(int) client_get_log(void);

/* -------------------------------------------------------------------------- *
 * Initialize client heap and add garbage collect timer.                      *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_init(void);

/* -------------------------------------------------------------------------- *
 * Destroy client heap and cancel timer.                                      *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_shutdown(void);

/* -------------------------------------------------------------------------- *
 * Create a new client coming from a local connection.                        *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_new_local(int type, struct lclient *lcptr);

/* -------------------------------------------------------------------------- *
 * Create a new client coming from a remote connection                        *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_new_remote(int type, struct lclient *lcptr,
		struct user *uptr, struct client *origin, const char *name, int hops,
		const char *host, const char *hostip, const char *info);

/* -------------------------------------------------------------------------- *
 * Create a new client coming from a remote connection                        *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_new_service(struct user *uptr, const char *name,
		const char *host, const char *hostip, const char *info);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_delete(struct client *lcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_release(struct client *cptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
/*IRCD_API(user_t  *) client_user_new       (struct client  *client);*/

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_find_id(uint32_t id);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_find_name(const char *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_find_nick(const char *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct history*) client_history_find(const char *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct history*) client_history_add(struct client *cptr,
		const char *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_history_delete(struct history *hptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_history_clean(struct client *cptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_find_host(const char *host);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_find_uid(const char *uid);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_find_nickw(struct client *cptr, const char *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_find_nickh(const char *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_find_nickhw(struct client *cptr, const char *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_set_name(struct client *cptr, const char *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int) client_register_local(struct client *cptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int) client_welcome(struct client *cptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_lusers(struct client *cptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int) client_nick(struct lclient *lcptr, struct client *cptr, char *nick);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_nick_random(char *buf);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int) client_nick_rotate(const char *name, char *buf);

/* -------------------------------------------------------------------------- *
 * introduce acptr to the rest of the net                                     *
 *                                                                            *
 * <lcptr>         the local connection the new client is coming from         *
 * <acptr>         the new client                                             *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_introduce(struct lclient *lcptr, struct client *cptr,
		struct client *acptr);

/* -------------------------------------------------------------------------- *
 * burst acptr to cptr                                                        *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_burst(struct lclient *cptr, struct client *acptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_vexit(struct lclient *lcptr, struct client *cptr,
		const char *comment, va_list args);

IRCD_API(void) client_exit(struct lclient *lcptr, struct client *cptr,
		const char *comment, ...);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_vsend(struct client *cptr, const char *format, va_list args);

IRCD_API(void) client_send(struct client *cptr, const char *format, ...);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int) client_relay(struct lclient *lcptr, struct client *cptr,
		struct client *acptr, const char *format, char **argv);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int) client_relay_always(struct lclient *lcptr, struct client *cptr,
		struct client **acptrptr, uint32_t cindex, const char *format,
		int *argc, char **argv);

IRCD_API(void) client_message(struct lclient *lcptr, struct client *cptr,
		struct client *acptr, int type, const char *text);

/* -------------------------------------------------------------------------- *
 * Get a reference to a client block                                          *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_pop(struct client *cptr);

/* -------------------------------------------------------------------------- *
 * Push back a reference to a client block                                    *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*) client_push(struct client **cptrptr);

/* -------------------------------------------------------------------------- *
 * Dump clients and client heap.                                              *
 * -------------------------------------------------------------------------- */
IRCD_API(void) client_dump(struct client *cptr);

#endif /* SRC_CLIENT_H */
