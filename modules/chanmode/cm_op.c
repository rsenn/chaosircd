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
 * $Id: cm_op.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
 * -------------------------------------------------------------------------- */
#define CM_OP_CHAR 'o'

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_op_hook(struct list     *lptr,   struct chanuser  *cuptr);
static int cm_op_kick(struct lclient  *lcptr,  struct client    *cptr,
                      struct channel  *chptr,  struct chanuser  *cuptr,
                      struct chanuser *acuptr, const char       *reason);

/* -------------------------------------------------------------------------- *
 *  * -------------------------------------------------------------------------- */
static const char *cm_op_help[] = {
  "+o <nickname>   Chanop status. Chanops have full control of a channel.",
  "                They can set modes and the topic and kick members.",
  NULL
};

static struct chanmode cm_op_mode = {
  CM_OP_CHAR,              /* Mode character */
  '@',                     /* Privilege prefix */
  CHANMODE_TYPE_PRIVILEGE, /* Channel mode is a privilege */
  CHFLG(o),                /* Only OPs can change the privilege */
  100,                     /* Sorting order */
  chanuser_mode_bounce,    /* Bounce handler */
  cm_op_help               /* Help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_op_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_op_mode) == NULL)
    return -1;

  hook_register(channel_join, HOOK_2ND, cm_op_hook);
  hook_register(chanuser_kick, HOOK_DEFAULT, cm_op_kick);

  return 0;
}

void cm_op_unload(void)
{
  /* unregister the channel mode */
  chanmode_unregister(&cm_op_mode);

  hook_unregister(chanuser_kick, HOOK_DEFAULT, cm_op_kick);
  hook_unregister(channel_join, HOOK_2ND, cm_op_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_op_hook(struct list *lptr, struct chanuser *cuptr)
{
  cuptr->flags |= CHFLG(o);
  chanmode_prefix_make(cuptr->prefix, cuptr->flags);
  chanmode_change_add(lptr, CHANMODE_ADD, 'o', NULL, cuptr);

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_op_kick(struct lclient  *lcptr,  struct client    *cptr,
                      struct channel  *chptr,  struct chanuser  *cuptr,
                      struct chanuser *acuptr, const char       *reason)
{
  if(cuptr)
  {
    if(cuptr->flags & CHFLG(o))
      return 1;
  }

  return 0;
}

