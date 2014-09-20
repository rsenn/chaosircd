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
 * $Id: cm_key.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/hook.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/user.h"
#include "ircd/numeric.h"
#include "ircd/channel.h"
#include "ircd/chanmode.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define CM_KEY_CHAR    'k'
#define CM_INVEX_CHAR  'I'
#define CM_INVITE_CHAR 'i'

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void cm_key_hook(struct client *cptr, struct channel *chptr,
		const char *key, int *reply);

static int cm_key_bounce(struct lclient *lcptr, struct client *cptr,
		struct channel *chptr, struct chanuser *cuptr, struct list *lptr,
		struct chanmodechange *cmcptr);

static void cm_key_build(char *dst, struct channel *chptr, uint32_t *di,
		uint64_t *flag);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static const char *cm_key_help[] = {
  "+k <key>        Set a channel key. Users can be invited to break key.",
  NULL
};

static struct chanmode cm_key_mode = {
  CM_KEY_CHAR,             /* Mode character */
  '\0',                    /* No prefix, because its not a privilege */
  CHANMODE_TYPE_KEY,       /* Channel mode is of key type */
  CHFLG(o) | CHFLG(h),     /* Only OPs and Halfops can change the key */
  0,                       /* No order and no reply */
  cm_key_bounce,           /* Bounce handler */
  cm_key_help              /* Help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_key_load(void) {
	/* register the channel mode */
	if(chanmode_register(&cm_key_mode) == NULL)
		return -1;

	hook_register(channel_join, HOOK_1ST, cm_key_hook);
	hook_register(chanmode_args_build, HOOK_1ST, cm_key_build);

	return 0;
}

void cm_key_unload(void) {
	/* unregister the channel mode */
	chanmode_unregister(&cm_key_mode);

	hook_unregister(chanmode_args_build, HOOK_1ST, cm_key_build);
	hook_unregister(channel_join, HOOK_1ST, cm_key_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void cm_key_hook(struct client *cptr, struct channel *chptr,
		const char *key, int *reply) {
	/* We're already denied or an invex matched */
	if(*reply > 0 || *reply == -CM_INVEX_CHAR || *reply == -CM_INVITE_CHAR)
		return;

	if(chptr->modes & CHFLG(k)) {
		if(key == NULL) {
			*reply = ERR_BADCHANNELKEY;
			return;
		}

		if(str_cmp(chptr->key, key)) {
			*reply = ERR_BADCHANNELKEY;
			return;
		}
	}
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int cm_key_bounce(struct lclient *lcptr, struct client *cptr,
		struct channel *chptr, struct chanuser *cuptr, struct list *lptr,
		struct chanmodechange *cmcptr) {
	if(cmcptr->what == CHANMODE_DEL) {
		if(!(chptr->modes & cmcptr->mode->flag))
			return 1;

		chptr->key[0] = '\0';
	}

	if(cmcptr->what == CHANMODE_ADD) {
		if(cmcptr->arg[0] == '\0')
			return 1;

		cmcptr->arg[IRCD_KEYLEN] = '\0';

		if((chptr->modes & cmcptr->mode->flag)) {
			if(!str_cmp(chptr->key, cmcptr->arg))
				chptr->modes &= ~cmcptr->mode->flag;
		}

		strlcpy(chptr->key, cmcptr->arg, sizeof(chptr->key));
	}

	return chanmode_bounce_simple(lcptr, cptr, chptr, cuptr, lptr, cmcptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void cm_key_build(char *dst, struct channel *chptr, uint32_t *di,
		uint64_t *flag) {
	if(*flag == CHFLG(k) && (chptr->modes & CHFLG(k)) && chptr->key[0]) {
		if(*di != 0)
			dst[(*di)++] = ' ';

		*di += strlcpy(&dst[*di], chptr->key, IRCD_KEYLEN * 8 + 1);
	}
}

