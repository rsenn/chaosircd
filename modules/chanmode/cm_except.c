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
 * $Id: cm_except.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
 * Mode characters                                                            *
 * -------------------------------------------------------------------------- */
#define CM_EXCEPT_CHAR 'e'

/* -------------------------------------------------------------------------- *
 * This hook gets called when someone wants to join a channel                 *
 * -------------------------------------------------------------------------- */
static void cm_except_hook(struct client *cptr, struct channel *chptr,
                           const char    *key,  int            *reply);
    
/* -------------------------------------------------------------------------- *
 * Setup chanmode structure                                                   *
 * -------------------------------------------------------------------------- */
static const char *cm_except_help[] = {
  "+e <mask>       Users matching the mask are excepted from bans and denies.",
  NULL
};

static struct chanmode cm_except_mode = {
  CM_EXCEPT_CHAR,          /* mode character */
  '\0',                    /* no prefix, because its not a privilege */
  CHANMODE_TYPE_LIST,      /* channel mode is a list */
  CHFLG(o) | CHFLG(h),     /* only OPs and Halfops can change the modelist */
  RPL_EXCEPTLIST,          /* use this reply when dumping modelist */
  chanmode_bounce_ban,     /* bounce handler */
  cm_except_help           /* help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_except_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_except_mode) == NULL)
    return -1;
  
  /* register a hook in channel_join */
  hook_register(channel_join, HOOK_1ST, cm_except_hook);
  
  /* set support flag */
  ircd_support_set("EXCEPTS", NULL);

  return 0;
}

void cm_except_unload(void)
{
  /* unset the support flag */
  ircd_support_unset("EXCEPTS");

  /* unregister the channel mode */
  chanmode_unregister(&cm_except_mode);
  
  /* unregister the hook in channel_join */  
  hook_unregister(channel_join, HOOK_1ST, cm_except_hook);
}

/* -------------------------------------------------------------------------- *
 * This hook gets called when someone wants to join a channel                 *
 * -------------------------------------------------------------------------- */
static void cm_except_hook(struct client *cptr, struct channel *chptr,
                           const char    *key,  int            *reply)
{
  struct list *mlptr;
  
  /* client not banned and not denied but something else */
  if(*reply > 0 && *reply != ERR_BANNEDFROMCHAN && *reply != ERR_DENIEDFROMCHAN)
    return;

  /* get the mode list for +e */
  mlptr = &chptr->modelists[chanmode_index(CM_EXCEPT_CHAR)];
   
  /* except the client from bans and denys */
  if(chanmode_match_ban(cptr, chptr, mlptr))
    *reply = -CM_EXCEPT_CHAR;
}
