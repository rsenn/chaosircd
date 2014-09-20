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
 * $Id: cm_topic.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/hook.h"

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
#define CM_TOPIC_CHAR 't'

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_topic_hook(struct lclient *lcptr, struct client   *cptr,
                         struct channel *chptr, struct chanuser *cuptr,
                         const char     *topic);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static const char *cm_topic_help[] =
{
  "+t              Topiclock. Lets only ops change the topic.",
  NULL
};

static struct chanmode cm_topic_mode =
{
  CM_TOPIC_CHAR,           /* mode character */
  '\0',                    /* no prefix, because its not a privilege */
  CHANMODE_TYPE_SINGLE,    /* channel mode is a single flag */
  CHFLG(o) | CHFLG(h),     /* only OPs and Halfops can change the flag */
  0,                       /* no order and no reply */
  chanmode_bounce_simple,  /* bounce handler */
  cm_topic_help            /* help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_topic_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_topic_mode) == NULL)
    return -1;

  hook_register(channel_topic, HOOK_DEFAULT, cm_topic_hook);

  return 0;
}

void cm_topic_unload(void)
{
  /* unregister the channel mode */
  chanmode_unregister(&cm_topic_mode);

  hook_unregister(channel_topic, HOOK_DEFAULT, cm_topic_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_topic_hook(struct lclient *lcptr, struct client   *cptr,
                         struct channel *chptr, struct chanuser *cuptr,
                         const char     *topic)
{
  if(cuptr)
  {
    if(!(cuptr->flags & CHFLG(o)) && !(cuptr->flags & CHFLG(h)))
    {
      if((chptr->modes & CHFLG(t)) && client_is_user(cptr) && client_is_local(cptr))
      {
        numeric_send(cptr, ERR_CHANOPRIVSNEEDED, chptr->name);
        return 1;
      }
    }
  }

  return 0;
}
