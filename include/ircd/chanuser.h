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
 * $Id: chanuser.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_CHANUSER_H
#define SRC_CHANUSER_H

/* -------------------------------------------------------------------------- *
 * Types                                                                      *
 * -------------------------------------------------------------------------- */
struct user;
struct client;
struct lclient;
struct channel;
struct chanmodechange;

struct chanuser {
  struct node     unode;          /* links to the users channel list */
  struct node     rnode;          /* links to the remote memberlist */
  struct node     lnode;          /* links to the local memberlist */
  struct node     gnode;
  int             local;
  uint32_t        serial;
  struct channel *channel;
  struct client  *client;
  uint64_t        flags;
  char            modebuf[IRCD_MODEBUFLEN + 1];
  char            prefix[sizeof(uint64_t) * 8 + 1];
};

#define CHANUSER_NONE   0x00
#define CHANUSER_VOICE  0x01
#define CHANUSER_HALFOP 0x02
#define CHANUSER_OP     0x04

#define CHANUSER_PER_LINE 32

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
CHAOS_API(int             ) chanuser_log;
CHAOS_API(struct sheap    ) chanuser_heap;

/* -------------------------------------------------------------------------- *
 * Initialize the chanuser module                                             *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_init(void);

/* -------------------------------------------------------------------------- *
 * Shut down the chanuser module                                              *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_shutdown(void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(struct chanuser*) chanuser_new(struct channel *chptr,
		struct client *sptr);

/* -------------------------------------------------------------------------- *
 * Create a chanuser block, link it to the channel and the user and           *
 * set the flags                                                              *
 * -------------------------------------------------------------------------- */
CHAOS_API(struct chanuser*) chanuser_add(struct channel *chptr,
		struct client *sptr);

/* -------------------------------------------------------------------------- *
 * Find a chanuser block                                                      *
 * -------------------------------------------------------------------------- */
CHAOS_API(struct chanuser*) chanuser_find(struct channel *chptr,
		struct client *sptr);

/* -------------------------------------------------------------------------- *
 * Find a chanuser block and warn                                             *
 * -------------------------------------------------------------------------- */
CHAOS_API(struct chanuser*) chanuser_find_warn(struct client *cptr, struct channel *chptr,
		struct client *acptr);

/* -------------------------------------------------------------------------- *
 * Delete a chanuser block                                                    *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_delete(struct chanuser *cuptr);

/* -------------------------------------------------------------------------- *
 * Show all chanusers to a client                                             *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_show(struct client *cptr, struct channel *chptr,
		struct chanuser *cuptr, int eon);

/* -------------------------------------------------------------------------- *
 * Parse chanusers                                                            *
 * -------------------------------------------------------------------------- */
CHAOS_API(uint32_t) chanuser_parse(struct lclient *lcptr, struct list *lptr,
		struct channel *chptr, char *args, int prefix);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(struct node*) chanuser_assemble(char *buf, struct node *nptr, size_t n,
		int of, int uid);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_introduce(struct lclient *lcptr, struct client *cptr,
		struct node *nptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(size_t) chanuser_burst(struct lclient *lcptr, struct channel *chptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_discharge(struct lclient *lcptr, struct chanuser *cuptr,
		const char *reason);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_mode_set(struct chanuser *cuptr, uint64_t flags);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(int) chanuser_mode_bounce(struct lclient *lcptr, struct client *cptr,
		struct channel *chptr, struct chanuser *cuptr, struct list *lptr,
		struct chanmodechange *cmcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_send_joins(struct lclient *lcptr, struct node *nptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_send_modes(struct lclient *lcptr, struct client *cptr,
		struct node *nptr);

/* -------------------------------------------------------------------------- *
 * Walk through all channels a client is in and message all members           *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_vsend(struct lclient *one, struct client *sptr,
		const char *format, va_list args);

CHAOS_API(void) chanuser_send(struct lclient *one, struct client *sptr,
		const char *format, ...);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_drop(struct client *cptr, struct channel *chptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_whois(struct client *cptr, struct user *auptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_kick(struct lclient *lcptr, struct client *cptr,
		struct channel *chptr, struct chanuser *cuptr, char **targetv,
		const char *reason);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
CHAOS_API(void) chanuser_support(void);

#endif /* SRC_CHANUSER_H */
