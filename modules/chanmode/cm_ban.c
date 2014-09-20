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
 * $Id: cm_ban.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
 * Mode characters                                                            *
 * -------------------------------------------------------------------------- */
#define CM_BAN_CHAR    'b'
#define CM_EXCEPT_CHAR 'e'   /* exceptions work together with this module */

/* -------------------------------------------------------------------------- *
 * This hook gets called when someone wants to join a channel                 *
 * -------------------------------------------------------------------------- */
static void cm_ban_hook(struct client *cptr, struct channel *chptr,
                        const char    *key,  int            *reply);

/* -------------------------------------------------------------------------- *
 * Set up the channel mode structure                                          *
 * -------------------------------------------------------------------------- */
static const char *cm_ban_help[] = {
  "+b <mask>       Users matching the mask are banned from the channel.",
  NULL
};

static struct chanmode cm_ban_mode = {
  CM_BAN_CHAR,             /* mode character */
  '\0',                    /* no prefix, because its not a privilege */
  CHANMODE_TYPE_LIST,      /* channel mode is a list */
  CHFLG(o) | CHFLG(h),     /* only OPs and Halfops can change the modelist */
  RPL_BANLIST,             /* use this reply when dumping modelist */
  chanmode_bounce_ban,     /* bounce handler */
  cm_ban_help              /* help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_ban_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_ban_mode) == NULL)
    return -1;

  /* register a hook in channel_join */
  hook_register(channel_join, HOOK_DEFAULT, cm_ban_hook);

  return 0;
}

void cm_ban_unload(void)
{
  /* unregister the channel mode */
  chanmode_unregister(&cm_ban_mode);

  /* unregister the hook in channel_join */
  hook_unregister(channel_join, HOOK_DEFAULT, cm_ban_hook);
}

/* -------------------------------------------------------------------------- *
 * This hook gets called when someone wants to join a channel                 *
 * -------------------------------------------------------------------------- */
static void cm_ban_hook(struct client *cptr, struct channel *chptr,
                        const char    *key,  int            *reply)
{
  struct list *mlptr;

  /* client is already denied or excepted */
  if(*reply > 0 || *reply == -CM_EXCEPT_CHAR)
    return;

  /* get the mode list for +b */
  mlptr = &chptr->modelists[chanmode_index(CM_BAN_CHAR)];

  /* match all masks against the client */
  if(chanmode_match_ban(cptr, chptr, mlptr))
    *reply = ERR_BANNEDFROMCHAN;
}
