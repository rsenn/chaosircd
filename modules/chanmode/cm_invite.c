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
 * $Id: cm_invite.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/hook.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/user.h>
#include <chaosircd/numeric.h>
#include <chaosircd/channel.h>
#include <chaosircd/chanmode.h>
#include <chaosircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * Mode charachters                                                           *
 * -------------------------------------------------------------------------- */
#define CM_INVITE_CHAR 'i'
#define CM_INVEX_CHAR  'I'

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void cm_invite_hook   (struct client         *cptr,  
                              struct channel        *chptr,
                              const char            *key,   
                              int                   *reply);
static int  cm_invite_bounce (struct lclient        *lcptr, 
                              struct client         *cptr,
                              struct channel        *chptr, 
                              struct chanuser       *cuptr,
                              struct list           *lptr,
                              struct chanmodechange *cmcptr);
static void cm_invite_clean  (struct channel        *chptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static const char *cm_invite_help[] = {
  "+i              Invite only channel. Users must be /INVITE'd to join the channel.",
  NULL
}; 

static struct chanmode cm_invite_mode = {
  CM_INVITE_CHAR,          /* Mode character */
  '\0',                    /* No prefix, because its not a privilege */
  CHANMODE_TYPE_SINGLE,    /* Channel mode is a single flag */
  CHFLG(o) | CHFLG(h),     /* Only OPs and Halfops can change the flag */
  0,                       /* No order and no reply */
  cm_invite_bounce,        /* Bounce handler */
  cm_invite_help           /* Help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_invite_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_invite_mode) == NULL)
    return -1;
  
  hook_register(channel_join, HOOK_1ST, cm_invite_hook);
  
  return 0;
}

void cm_invite_unload(void)
{
  /* unregister the channel mode */
  chanmode_unregister(&cm_invite_mode);
  
  hook_unregister(channel_join, HOOK_1ST, cm_invite_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void cm_invite_hook(struct client *cptr, struct channel *chptr, 
                           const char    *key,  int            *reply)
{
  struct invite *ivptr;
  
  /* We're already denied or an invex matched */
  if((*reply > 0 || *reply == -CM_INVEX_CHAR) && *reply != ERR_CHANNELISFULL && *reply != ERR_BADCHANNELKEY)
    return;
  
  dlink_foreach(&cptr->user->invites, ivptr)
  {
    if(ivptr->channel == chptr)
    {
      user_uninvite(ivptr);

      *reply = -CM_INVITE_CHAR;

      return;
    }
  }
  
  if(chptr->modes & CHFLG(i))
    *reply = ERR_INVITEONLYCHAN;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int cm_invite_bounce(struct lclient *lcptr, struct client         *cptr,
                     struct channel *chptr, struct chanuser       *cuptr,
                     struct list    *lptr,  struct chanmodechange *cmcptr)
{
  if(cmcptr->what == CHANMODE_DEL)
  {
    if(!(chptr->modes & cmcptr->mode->flag))
      return 1;
   
    cm_invite_clean(chptr);
  }

  return chanmode_bounce_simple(lcptr, cptr, chptr, cuptr, lptr, cmcptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void cm_invite_clean(struct channel *chptr)
{
  struct node *node;
  struct node *next;
  
  dlink_foreach_safe(&chptr->invites, node, next)
    user_uninvite(node->data);
}

