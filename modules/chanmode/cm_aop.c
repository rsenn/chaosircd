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
 * $Id: cm_aop.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "hook.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/numeric.h>
#include <ircd/channel.h>
#include <ircd/chanmode.h>
#include <ircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * Mode characters                                                            *
 * -------------------------------------------------------------------------- */
#define CM_AOP_CHAR 'O'
#define CM_OP_CHAR  'o'

/* -------------------------------------------------------------------------- *
 * This hook gets called when a client successfully joined a channel          *
 * -------------------------------------------------------------------------- */
static void cm_aop_hook(struct list *lptr, struct chanuser *cuptr);

/* -------------------------------------------------------------------------- *
 * Setup chanmode structure                                                   *
 * -------------------------------------------------------------------------- */
static const char *cm_aop_help[] =
{
  "+O <mask>       Users matching the mask will get auto-opped on join.",
  NULL
};

static struct chanmode cm_ahalfop_mode =
{
  CM_AOP_CHAR,             /* mode character */
  '\0',                    /* no prefix, because its not a privilege */
  CHANMODE_TYPE_LIST,      /* channel mode is a list */
  CHFLG(o) | CHFLG(h),     /* only OPs and Halfops can change the modelist */
  RPL_AOPLIST,             /* use this reply when dumping modelist */
  chanmode_bounce_ban,     /* bounce handler */
  cm_aop_help              /* help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_aop_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_ahalfop_mode) == NULL)
    return -1;

  hook_register(channel_join, HOOK_3RD, cm_aop_hook);

  ircd_support_set("AUTOOP", NULL);

  return 0;
}

void cm_aop_unload(void)
{
  /* unregister the channel mode */
  ircd_support_unset("AUTOOP");

  chanmode_unregister(&cm_ahalfop_mode);

  hook_unregister(channel_join, HOOK_3RD, cm_aop_hook);
}

/* -------------------------------------------------------------------------- *
 * This hook gets called when a client successfully joined a channel          *
 * -------------------------------------------------------------------------- */
static void cm_aop_hook(struct list *lptr, struct chanuser *cuptr)
{
  struct channel *chptr = cuptr->channel;
  struct list    *mlptr;

  /* get the mode list for +O */
  mlptr = &chptr->modelists[chanmode_index(CM_AOP_CHAR)];

  /* match the client against all masks in the list */
  if(chanmode_match_amode(cuptr->client, chptr, mlptr))
  {
    /* if the client matched, then give him +o mode */
    cuptr->flags |= CHFLG(o);

    /* update the nickname prefix (@) */
    chanmode_prefix_make(cuptr->prefix, cuptr->flags);

    /* add the mode to the current mode change list */
    chanmode_change_add(lptr, CHANMODE_ADD, CM_OP_CHAR, NULL, cuptr);
  }
}
