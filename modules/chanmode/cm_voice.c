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
 * $Id: cm_voice.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/hook.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/numeric.h"
#include "ircd/channel.h"
#include "ircd/chanmode.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define CM_VOICE_CHAR 'v'

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_voice_hook(struct list *lptr, struct chanuser *cuptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static const char *cm_voice_help[] =
{
  "+v <nickname>   Voice status. User can talk even when the channel is +m.",
  NULL
};

static struct chanmode cm_voice_mode =
{
  CM_VOICE_CHAR,           /* mode character */
  '+',                     /* privilege prefix */
  CHANMODE_TYPE_PRIVILEGE, /* channel mode is a privilege */
  CHFLG(o) | CHFLG(h),     /* only OPs and Halfops can change the privilege */
  20,                      /* sorting order */
  chanuser_mode_bounce,    /* bounce handler */
  cm_voice_help            /* help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_voice_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_voice_mode) == NULL)
    return -1;

  hook_register(channel_join, HOOK_2ND, cm_voice_hook);

  return 0;
}

void cm_voice_unload(void)
{
  /* unregister the channel mode */
  chanmode_unregister(&cm_voice_mode);

  hook_unregister(channel_join, HOOK_2ND, cm_voice_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_voice_hook(struct list *lptr, struct chanuser *cuptr)
{
  cuptr->flags |= CHFLG(v);
  chanmode_prefix_make(cuptr->prefix, cuptr->flags);
  chanmode_change_add(lptr, CHANMODE_ADD, CM_VOICE_CHAR, NULL, cuptr);

  return 0;
}
