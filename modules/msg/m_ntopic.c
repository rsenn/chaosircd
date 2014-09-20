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
 * $Id: m_ntopic.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/log.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/channel.h"
#include "ircd/chanmode.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void ms_ntopic(struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *ms_ntopic_help[] = {
  "NTOPIC <channel> <ts> <topic info> <topic ts> [topic]",
  "",
  "Synchronizes channel topics with a remote server.",
  NULL
};

static struct msg ms_ntopic_msg = {
  "NTOPIC", 4, 5, MFLG_SERVER,
  { NULL, NULL, ms_ntopic, NULL },
  ms_ntopic_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_ntopic_load(void)
{
  if(msg_register(&ms_ntopic_msg) == NULL)
    return -1;

  return 0;
}

void m_ntopic_unload(void)
{
  msg_unregister(&ms_ntopic_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'ntopic'                                                         *
 * argv[2] - channel                                                          *
 * argv[3] - channel ts                                                       *
 * argv[4] - topic info                                                       *
 * argv[5] - topic time                                                       *
 * argv[6] - topic                                                            *
 * -------------------------------------------------------------------------- */
static void ms_ntopic(struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv)
{
  struct channel *chptr;
  time_t          ts;
  time_t          topic_ts;
  int             changed = 0;
  char            topic[IRCD_TOPICLEN];

  topic[0] = '\0';

  if(argc == 7)
    strlcpy(topic, argv[6], sizeof(topic));

  if((chptr = channel_find_name(argv[2])) == NULL)
  {
    log(client_log, L_warning, "Dropping invalid NTOPIC for channel %s.",
        argv[2]);
    return;
  }

  ts = str_toul(argv[3], NULL, 10);

  if(ts > chptr->ts || chptr->topic[0])
  {
    log(client_log, L_verbose, "Dropping NTOPIC for %s with too recent TS",
        chptr->name);
    return;
  }

  topic_ts = str_toul(argv[5], NULL, 10);

  if(chptr->ts == ts)
  {
    if(chptr->topic_ts > topic_ts)
    {
      log(client_log, L_verbose, "Dropping NTOPIC for %s because its older",
          chptr->name);
      return;
    }

    if(chptr->topic_ts == topic_ts)
    {
      size_t nlen;
      size_t olen;

      nlen = topic[0] ? str_len(topic) : 0;
      olen = str_len(chptr->topic);

      if(nlen < olen)
      {
        log(client_log, L_warning, "TOPIC collision on %s, ignoring.",
            chptr->name);
        return;
      }

      if(nlen == olen)
      {
        if((topic == NULL && chptr->topic[0] == '\0'))
          return;

        if(!str_cmp(topic, chptr->topic))
          return;
      }
    }
  }

  if(chptr->ts != ts)
  {
    log(channel_log, L_warning, "TS for channel %s changed from %lu to %lu.",
        chptr->name, chptr->ts, ts);

    chptr->ts = ts;
  }

  if(str_cmp(chptr->topic, topic))
    changed = 1;

  if(changed)
  {
    if(topic[0])
      strlcpy(chptr->topic, topic, sizeof(chptr->topic));
    else
      chptr->topic[0] = '\0';
  }

  strlcpy(chptr->topic_info, argv[4], sizeof(chptr->topic_info));
  chptr->topic_ts = topic_ts;

  if(chptr->topic[0])
    server_send(lcptr, NULL, CAP_NONE, CAP_NONE,
                ":%C NTOPIC %s %lu %s %lu :%s",
                cptr, chptr->name, chptr->ts,
                chptr->topic_info, chptr->topic_ts, chptr->topic);
  else
    server_send(lcptr, NULL, CAP_NONE, CAP_NONE,
                ":%C NTOPIC %s %lu %s %lu",
                cptr, chptr->name, chptr->ts,
                chptr->topic_info, chptr->topic_ts);

  if(changed)
  {
    channel_send(NULL, chptr, CHFLG(NONE), CHFLG(NONE),
                 ":%C TOPIC %s :%s",
                 cptr, chptr->name, chptr->topic);
  }
}

