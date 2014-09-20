/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: cm_halfop.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/dlink.h"
#include "libchaos/hook.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/server.h"
#include "ircd/numeric.h"
#include "ircd/channel.h"
#include "ircd/chanmode.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * Mode characters                                                            *
 * -------------------------------------------------------------------------- */
#define CM_HALFOP_CHAR   'h'
#define CM_HALFOP_PREFIX '%'

/* -------------------------------------------------------------------------- *
 * This hook gets called when a client successfully CREATED a channel         *
 * -------------------------------------------------------------------------- */
static int cm_halfop_hook(struct list *lptr, struct chanuser *cuptr);

/* -------------------------------------------------------------------------- *
 * This hook gets called when a client tries to kick someone off a channel    *
 * -------------------------------------------------------------------------- */
static int cm_halfop_kick(struct lclient *lcptr, struct client *cptr,
		struct channel *chptr, struct chanuser *cuptr, struct chanuser *acuptr,
		const char *reason);

/* -------------------------------------------------------------------------- *
 * Setup chanmode structure                                                   *
 * -------------------------------------------------------------------------- */
static const char *cm_halfop_help[] = {
  "+h <nickname>   Halfop status. Halfops can do the same as chanops but",
  "                nothing that influences any operator (+/-o, +/-O, op-kick).",
  NULL
};

static struct chanmode cm_halfop_mode = {
  CM_HALFOP_CHAR,          /* mode character */
  CM_HALFOP_PREFIX,        /* privilege prefix */
  CHANMODE_TYPE_PRIVILEGE, /* channel mode is a privilege */
  CHFLG(o) | CHFLG(h),     /* only OPs and Halfops can change the privilege */
  50,                      /* sorting order */
  chanuser_mode_bounce,    /* bounce handler */
  cm_halfop_help           /* help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_halfop_load(void) {
	/* register the channel mode */
	if(chanmode_register(&cm_halfop_mode) == NULL)
		return -1;

	hook_register(channel_join, HOOK_2ND, cm_halfop_hook);
	hook_register(chanuser_kick, HOOK_DEFAULT, cm_halfop_kick);

	server_default_caps |= CAP_HOP;

	return 0;
}

void cm_halfop_unload(void) {
	server_default_caps &= ~CAP_HOP;

	hook_unregister(chanuser_kick, HOOK_DEFAULT, cm_halfop_kick);
	hook_unregister(channel_join, HOOK_DEFAULT, cm_halfop_hook);

	/* unregister the channel mode */
	chanmode_unregister(&cm_halfop_mode);
}

/* -------------------------------------------------------------------------- *
 * This hook gets called when a client successfully CREATED a channel         *
 * -------------------------------------------------------------------------- */
static int cm_halfop_hook(struct list *lptr, struct chanuser *cuptr) {
	cuptr->flags |= CHFLG(h);

	chanmode_prefix_make(cuptr->prefix, cuptr->flags);

	chanmode_change_add(lptr, CHANMODE_ADD, CM_HALFOP_CHAR, NULL, cuptr);

	return 0;
}

/* -------------------------------------------------------------------------- *
 * This hook gets called when a client tries to kick someone off a channel    *
 * -------------------------------------------------------------------------- */
static int cm_halfop_kick(struct lclient *lcptr, struct client *cptr,
		struct channel *chptr, struct chanuser *cuptr, struct chanuser *acuptr,
		const char *reason) {
	if(cuptr) {
		if(cuptr->flags & CHFLG(o))
			return 1;

		if((cuptr->flags & CHFLG(h)) && !(acuptr->flags & CHFLG(o)))
			return 1;
	}

	return 0;
}

