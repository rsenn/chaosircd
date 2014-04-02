/* chaosircd - pi-networks irc server
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
 * $Id: cm_moderated.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/hook.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/numeric.h>
#include <chaosircd/channel.h>
#include <chaosircd/chanmode.h>
#include <chaosircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define CM_MODERATED_CHAR 'm'

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_moderated_hook(struct client   *cptr, struct channel *chptr,
	                     intptr_t         type, const char     *text);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static const char *cm_moderated_help[] = 
{
  "+m              Moderated. Lets only ops and voices send to the channel.",
  NULL
};

static struct chanmode cm_moderated_mode = 
{
  CM_MODERATED_CHAR,       /* Mode character */
  '\0',                    /* No prefix, because its not a privilege */
  CHANMODE_TYPE_SINGLE,    /* Channel mode is a single flag */
  CHFLG(o) | CHFLG(h),     /* Only OPs and Halfops can change the flag */
  0,                       /* No order and no reply */
  chanmode_bounce_simple,  /* Bounce handler */
  cm_moderated_help        /* Help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_moderated_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_moderated_mode) == NULL)
    return -1;
  
  hook_register(channel_message, HOOK_DEFAULT, cm_moderated_hook);
  
  return 0;
}

void cm_moderated_unload(void)
{
  /* unregister the channel mode */
  chanmode_unregister(&cm_moderated_mode);
  
  hook_unregister(channel_message, HOOK_DEFAULT, cm_moderated_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_moderated_hook(struct client   *cptr, struct channel *chptr,
	                     intptr_t         type, const char     *text)
{
  struct chanuser *cuptr = chanuser_find(chptr, cptr);

  if(chptr->modes & CHFLG(m) &&
     (cuptr == NULL || (cuptr->flags & (CHFLG(o) | CHFLG(h) | CHFLG(v))) == 0LLU))
  {
    numeric_send(cptr, ERR_CANNOTSENDTOCHAN, chptr->name);
    return 1;
  }
  
  return 0;
}

