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
 * $Id: user.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_USER_H
#define SRC_USER_H

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define USER_HASH_SIZE      16

/* -------------------------------------------------------------------------- *
 * Types                                                                      *
 * -------------------------------------------------------------------------- */
struct channel;
struct invite;

struct user {
	struct node node; /* node for user_list */
	struct node hnode; /* node for user_lists[] */
	uint32_t id; /* a unique id */
	uint32_t refcount; /* how many times this block is referenced */
	hash_t nhash;
	hash_t uhash;
	struct client *client;
	struct oper *oper;
	struct list channels;
	struct list invites;
	uint64_t modes;
	time_t away_time;
	char name[IRCD_USERLEN + 1];
	char uid[IRCD_IDLEN + 1];
	char away[IRCD_AWAYLEN + 1];
	char mode[IRCD_MODEBUFLEN + 1];
};

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
CHAOS_DATA(int ) user_log; /* user log source */
CHAOS_DATA(struct sheap) user_heap; /* heap for lclient_t */
CHAOS_DATA(struct list ) user_list; /* list with all of them */
CHAOS_DATA(uint32_t ) user_id;

/* ------------------------------------------------------------------------ */
IRCD_API(int) user_get_log(void);

/* -------------------------------------------------------------------------- *
 * Initialize user module.                                                    *
 * -------------------------------------------------------------------------- */
IRCD_API(void) user_init(void);

/* -------------------------------------------------------------------------- *
 * Shutdown the user module.                                                  *
 * -------------------------------------------------------------------------- */
IRCD_API(void) user_shutdown(void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) user_collect(void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct user*) user_new(const char *name, const char *id);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) user_delete(struct user *uptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct user*) user_find_id(uint32_t id);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct user*) user_find_uid(const char *uid);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct user*) user_find_name(const char *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) user_set_name(struct user *uptr, const char *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) user_whois(struct client *cptr, struct user *auptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) user_invite(struct user *uptr, struct channel *chptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) user_uninvite(struct invite *ivptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) user_release(struct user *uptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct user*) user_pop(struct user *uptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct user*) user_push(struct user **ucptrptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void) user_dump(struct user *uptr);

#endif /* SRC_USER_H */
