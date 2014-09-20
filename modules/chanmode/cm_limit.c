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
 * $Id: cm_limit.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * System headers                                                             *
 * -------------------------------------------------------------------------- */
#include <limits.h>

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/str.h"
#include "libchaos/hook.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/user.h>
#include <ircd/numeric.h>
#include <ircd/channel.h>
#include <ircd/chanmode.h>
#include <ircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define CM_LIMIT_CHAR 'l'
#define CM_INVEX_CHAR 'I'
#define CM_INVITE_CHAR 'i'

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void cm_limit_hook   (struct client         *cptr,
                             struct channel        *chptr,
                             const char            *key,
                             int                   *reply);
static int  cm_limit_bounce (struct lclient        *lcptr,
                             struct client         *cptr,
                             struct channel        *chptr,
                             struct chanuser       *cuptr,
                             struct list           *lptr,
                             struct chanmodechange *cmcptr);
static void cm_limit_build  (char                  *dst,
                             struct channel        *chptr,
                             uint32_t              *di,
                             uint64_t              *flag);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static const char *cm_limit_help[] = {
  "+l <n>          Limit users in channel. Users can be invited to break limit.",
  NULL
};

static struct chanmode cm_limit_mode = {
  CM_LIMIT_CHAR,           /* Mode character */
  '\0',                    /* No prefix, because its not a privilege */
  CHANMODE_TYPE_KEY,       /* Channel mode is of key type */
  CHFLG(o) | CHFLG(h),     /* Only OPs and Halfops can change the key */
  0,                       /* No order and no reply */
  cm_limit_bounce,         /* Bounce handler */
  cm_limit_help            /* Help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_limit_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_limit_mode) == NULL)
    return -1;

  hook_register(channel_join, HOOK_1ST, cm_limit_hook);
  hook_register(chanmode_args_build, HOOK_1ST, cm_limit_build);

  return 0;
}

void cm_limit_unload(void)
{
  /* unregister the channel mode */
  chanmode_unregister(&cm_limit_mode);

  hook_unregister(chanmode_args_build, HOOK_1ST, cm_limit_build);
  hook_unregister(channel_join, HOOK_1ST, cm_limit_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void cm_limit_hook(struct client *cptr, struct channel *chptr,
                           const char    *key,  int            *reply)
{
  /* We're already denied or an invex matched */
  if(*reply > 0 || *reply == -CM_INVEX_CHAR || *reply == -CM_INVITE_CHAR)
    return;

  if((chptr->modes & CHFLG(l)) && chptr->chanusers.size + 1 > chptr->limit)
  {
    *reply = ERR_CHANNELISFULL;
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int cm_limit_bounce(struct lclient *lcptr, struct client         *cptr,
                    struct channel *chptr, struct chanuser       *cuptr,
                    struct list    *lptr,  struct chanmodechange *cmcptr)
{
  if(cmcptr->what == CHANMODE_DEL)
  {
    if(!(chptr->modes & cmcptr->mode->flag))
      return 1;

    chptr->limit = -1;
  }

  if(cmcptr->what == CHANMODE_ADD)
  {
    unsigned long limit;

    limit = str_toul(cmcptr->arg, NULL, 10);

    if(limit == ULONG_MAX || limit == 0)
      return 1;

    if((chptr->modes & cmcptr->mode->flag))
    {
      if(chptr->limit != limit)
        chptr->modes &= ~cmcptr->mode->flag;
    }

    chptr->limit = limit;
  }

  return chanmode_bounce_simple(lcptr, cptr, chptr, cuptr, lptr, cmcptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void cm_limit_build(char     *dst, struct channel *chptr,
                           uint32_t *di,  uint64_t       *flag)
{
  if(*flag == CHFLG(l) && chptr->modes & CHFLG(l) && chptr->limit > 0)
  {
    if(*di != 0)
      dst[(*di)++] = ' ';

    *di += str_snprintf(&dst[*di], IRCD_KEYLEN * 8 + 1, "%i", chptr->limit);
  }
}
