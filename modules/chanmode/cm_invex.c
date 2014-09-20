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
 * $Id: cm_invex.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/hook.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/user.h"
#include "ircd/server.h"
#include "ircd/numeric.h"
#include "ircd/channel.h"
#include "ircd/chanmode.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * Mode characters                                                            *
 * -------------------------------------------------------------------------- */
#define CM_INVEX_CHAR  'I'
#define CM_INVITE_CHAR 'i'

/* -------------------------------------------------------------------------- *
 * This hook gets called when a client successfully joined a channel          *
 * -------------------------------------------------------------------------- */
static const char *cm_invex_help[] =
{
  "+I <mask>       Users matching the mask are excepted from invite only.",
  NULL
};

static struct chanmode cm_invex_mode =
{
  CM_INVEX_CHAR,           /* mode character */
  '\0',                    /* no prefix, because its not a privilege */
  CHANMODE_TYPE_LIST,      /* channel mode is a list */
  CHFLG(o) | CHFLG(h),     /* only OPs and Halfops can change the modelist */
  RPL_INVEXLIST,           /* use this reply when dumping modelist */
  chanmode_bounce_ban,     /* bounce handler */
  cm_invex_help            /* help text */
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void cm_invex_hook(struct client *cptr, struct channel *chptr,
                          const char    *key,  int            *reply);

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_invex_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_invex_mode) == NULL)
    return -1;

  hook_register(channel_join, HOOK_1ST, cm_invex_hook);

  ircd_support_set("INVEX", NULL);

  return 0;
}

void cm_invex_unload(void)
{
  /* unregister the channel mode */
  ircd_support_unset("INVEX");

  chanmode_unregister(&cm_invex_mode);

  hook_unregister(channel_join, HOOK_1ST, cm_invex_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void cm_invex_hook(struct client *cptr, struct channel *chptr,
                          const char    *key,  int            *reply)
{
  struct list   *mlptr;
  struct invite *ivptr;

  /* client is already denied and not banned */
  if((*reply > 0 && *reply != ERR_INVITEONLYCHAN &&
      *reply != ERR_CHANNELISFULL && *reply != ERR_BADCHANNELKEY) || *reply == -CM_INVITE_CHAR)
    return;

  /* get the mode list for +I */
  mlptr = &chptr->modelists[chanmode_index(CM_INVEX_CHAR)];

  if(chanmode_match_ban(cptr, chptr, mlptr))
  {
    *reply = -CM_INVEX_CHAR;

    dlink_foreach(&cptr->user->invites, ivptr)
    {
      if(ivptr->channel == chptr)
      {
        user_uninvite(ivptr);
        return;
      }
    }
  }
}
