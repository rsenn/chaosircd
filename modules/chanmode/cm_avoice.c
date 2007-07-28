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
 * $Id: cm_avoice.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
#define CM_AVOICE_CHAR 'V'
#define CM_VOICE_CHAR  'v'

/* -------------------------------------------------------------------------- *
 * This hook gets called when a client successfully joined a channel          *
 * -------------------------------------------------------------------------- */
static void cm_avoice_hook(struct list *lptr, struct chanuser *cuptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static const char *cm_avoice_help[] = {
  "+V <mask>       Users matching the mask will get auto-voiced on join.",
  NULL
};

static struct chanmode cm_avoice_mode = {
  CM_AVOICE_CHAR,         /* Mode character */
  '\0',                    /* No prefix, because its not a privilege */
  CHANMODE_TYPE_LIST,      /* Channel mode is a list */
  CHFLG(o) | CHFLG(h),     /* Only OPs and Halfops can change the modelist */
  RPL_AVOICELIST,          /* Use this reply when dumping modelist */
  chanmode_bounce_ban,     /* Bounce handler */
  cm_avoice_help           /* Help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_avoice_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_avoice_mode) == NULL)
    return -1;
  
  /* register a hook in channel_join */
  hook_register(channel_join, HOOK_3RD, cm_avoice_hook);

  /* set support flag */
  ircd_support_set("AUTOVOICE", NULL);
  
  return 0;
}

void cm_avoice_unload(void)
{
  /* unset the support flag */
  ircd_support_unset("AUTOVOICE");
    
  /* unregister the channel mode */
  chanmode_unregister(&cm_avoice_mode);
  
  /* unregister the hook in channel_join */
  hook_unregister(channel_join, HOOK_3RD, cm_avoice_hook);
}

/* -------------------------------------------------------------------------- *
 * This hook gets called when a client successfully joined a channel          *
 * -------------------------------------------------------------------------- */
static void cm_avoice_hook(struct list *lptr, struct chanuser *cuptr)
{
  struct channel *chptr = cuptr->channel;
  struct list    *mlptr;

  /* get the mode list for +V */
  mlptr = &chptr->modelists[chanmode_index(CM_AVOICE_CHAR)];

  /* match the client against all masks in the list */
  if(chanmode_match_amode(cuptr->client, chptr, mlptr))
  {
    /* if the client matched, then give him +v mode */
    cuptr->flags |= CHFLG(v);
    
    /* update the nickname prefix (+) */
    chanmode_prefix_make(cuptr->prefix, cuptr->flags);
    
    /* add the mode to the current mode change list */
    chanmode_change_add(lptr, CHANMODE_ADD, CM_VOICE_CHAR, NULL, cuptr);
  }
}
