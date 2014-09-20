/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: userdb.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SERVAUTH_USERDB_H
#define SERVAUTH_USERDB_H

#include "libchaos/db.h"

#define USERDB_TIMEOUT   10000 /* msecs timeout */

#define USERDB_ST_IDLE       0
#define USERDB_ST_CONNECTING 1
#define USERDB_ST_SENT       2
#define USERDB_ST_DONE       3
#define USERDB_ST_ERROR     -1
#define USERDB_ST_TIMEOUT   -2

struct userdb_client;

typedef void (userdb_callback_t)(struct userdb_client *);

struct userdb_client {
	struct db *handle;
	struct db_result *result;
	uint64_t timeout;
	void *userarg;
	struct timer *timer;
	char reply[256];
	userdb_callback_t *callback;
};

#define userdb_is_idle(userdb) (!((struct userdb_client *)(userdb))->handle)
#define userdb_is_busy(userdb)  (((struct userdb_client *)(userdb))->handle)

#define userdb_is_ident_char(c) (((c) >= 'A' && (c) <= 'Z') || \
                               ((c) >= 'a' && (c) <= 'z') || \
                               ((c) >= '0' && (c) <= '9') || \
                               (c) == '-' || (c) == '_' || \
                               (c) == '.')

extern void userdb_zero(struct userdb_client *userdb);
extern void userdb_clear(struct userdb_client *userdb);
extern int userdb_connect(struct userdb_client *userdb, const char* host,
		const char* user, const char* password);
extern int userdb_verify(struct userdb_client *userdb, const char* uid,
		const char* msisdn, char** retstr);
extern int userdb_register(struct userdb_client *userdb, char* uid,
		char** values, size_t num_values);
extern int userdb_search(struct userdb_client *userdb, char** v, size_t,
		char** s);
extern int userdb_mutate(struct userdb_client *userdb, const char* uid,
		char** values, size_t num_values);
extern void userdb_set_userarg(struct userdb_client *userdb, void *arg);
extern void *userdb_get_userarg(struct userdb_client *userdb);
extern void userdb_set_callback(struct userdb_client *userdb,
		userdb_callback_t *cb, uint64_t timeout);

#endif /* SERVAUTH_USERDB_H */
