/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003,2004  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: m_topic.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/log.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/channel.h"
#include "ircd/chanmode.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_topic  (struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv);
static void ms_topic (struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_topic_help[] = {
  "TOPIC <channel> [+][text]",
  "",
  "If used with a text, it changes the topic of the given",
  "channel. If used without text, the actual topic is shown.",
  "If + is prepended to the text then the text is added in",
  "front of the topic.",
  NULL
};

static struct msg m_topic_msg = {
  "TOPIC", 1, 2, MFLG_CLIENT,
  { m_unregistered, m_topic, ms_topic, m_topic },
  m_topic_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_topic_load(void)
{
  if(msg_register(&m_topic_msg) == NULL)
    return -1;

  return 0;
}

void m_topic_unload(void)
{
  msg_unregister(&m_topic_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'topic'                                                          *
 * argv[2] - channel                                                          *
 * argv[3] - [topic]                                                          *
 * -------------------------------------------------------------------------- */
static void m_topic(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  struct channel  *chptr;
  struct chanuser *cuptr;
  char             newtopic[IRCD_TOPICLEN + 1];

  if((chptr = channel_find_warn(cptr, argv[2])) == NULL)
    return;

  cuptr = chanuser_find(chptr, cptr);

  if(cuptr == NULL)
  {
    numeric_send(cptr, ERR_NOTONCHANNEL, chptr->name);
    return;
  }

  if(argc < 4)
  {
    if(chptr->topic[0] == '\0')
    {
      numeric_send(cptr, RPL_NOTOPIC, chptr->name);
    }
    else
    {
      numeric_send(cptr, RPL_TOPIC, chptr->name, chptr->topic);
      numeric_send(cptr, RPL_TOPICWHOTIME,
                   chptr->name, chptr->topic_info, chptr->topic_ts);
    }

    return;
  }

  strlcpy(newtopic, argv[3], sizeof(newtopic));

  if(newtopic[0] == '+')
  {
    strlcpy(newtopic, &argv[3][1], sizeof(newtopic));
    strlcat(newtopic, " | ", sizeof(newtopic));
    strlcat(newtopic, chptr->topic, sizeof(newtopic));
  }

  channel_topic(lcptr, cptr, chptr, cuptr, newtopic);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'topic'                                                          *
 * argv[2] - channel                                                          *
 * argv[3] - [topic]                                                          *
 * -------------------------------------------------------------------------- */
static void ms_topic(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  struct channel *chptr;

  if((chptr = channel_find_name(argv[2])) == NULL)
  {
    log(channel_log, L_warning, "Dropping TOPIC for unknown channel %s.",
        argv[2]);
    return;
  }

  channel_topic(lcptr, cptr, chptr, NULL, argv[3]);
}
