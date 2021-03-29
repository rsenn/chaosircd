/* chaosircd - Chaoz's IRC daemon daemon
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
 * $Id: cm_ahalfop.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
#define CM_AHALFOP_CHAR 'H'
#define CM_HALFOP_CHAR  'h'

/* -------------------------------------------------------------------------- *
 * This hook gets called when a client successfully joined a channel          *
 * -------------------------------------------------------------------------- */
static void cm_ahalfop_hook(struct list *lptr, struct chanuser *cuptr);

/* -------------------------------------------------------------------------- *
 * Setup chanmode structure                                                   *
 * -------------------------------------------------------------------------- */
static const char *cm_ahalfop_help[] = {
  "+H <mask>       Users matching the mask will get auto-halfopped on join.",
  NULL
};

static struct chanmode cm_ahalfop_mode = {
  CM_AHALFOP_CHAR,         /* Mode character */
  '\0',                    /* No prefix, because its not a privilege */
  CHANMODE_TYPE_LIST,      /* Channel mode is a list */
  CHFLG(o) | CHFLG(h),     /* Only OPs and Halfops can change the modelist */
  RPL_AHOPLIST,            /* Use this reply when dumping modelist */
  chanmode_bounce_ban,     /* Bounce handler */
  cm_ahalfop_help          /* Help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_ahalfop_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_ahalfop_mode) == NULL)
    return -1;

  /* register a hook in channel_join */
  hook_register(channel_join, HOOK_3RD, cm_ahalfop_hook);

  /* set support flag */
  ircd_support_set("AUTOHALFOP", NULL);

  return 0;
}

void cm_ahalfop_unload(void)
{
  /* unset the support flag */
  ircd_support_unset("AUTOHALFOP");

  /* unregister the channel mode */
  chanmode_unregister(&cm_ahalfop_mode);

  /* unregister the hook in channel_join */
  hook_unregister(channel_join, HOOK_3RD, cm_ahalfop_hook);
}

/* -------------------------------------------------------------------------- *
 * This hook gets called when a client successfully joined a channel          *
 * -------------------------------------------------------------------------- */
static void cm_ahalfop_hook(struct list *lptr, struct chanuser *cuptr)
{
  struct channel *chptr = cuptr->channel;
  struct list    *mlptr;

  /* get the mode list for +H */
  mlptr = &chptr->modelists[chanmode_index(CM_AHALFOP_CHAR)];

  /* match the client against all masks in the list */
  if(chanmode_match_amode(cuptr->client, chptr, mlptr))
  {
    /* if the client matched, then give him +h mode */
    cuptr->flags |= CHFLG(h);

    /* update the nickname prefix (%) */
    chanmode_prefix_make(cuptr->prefix, cuptr->flags);

    /* add the mode to the current mode change list */
    chanmode_change_add(lptr, CHANMODE_ADD, CM_HALFOP_CHAR, NULL, cuptr);
  }
}
