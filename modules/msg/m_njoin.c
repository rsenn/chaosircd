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
 * $Id: m_njoin.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/log.h>
#include <libchaos/dlink.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/msg.h>
#include <ircd/user.h>
#include <ircd/chars.h>
#include <ircd/client.h>
#include <ircd/server.h>
#include <ircd/channel.h>
#include <ircd/numeric.h>
#include <ircd/chanmode.h>
#include <ircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void ms_njoin(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *ms_njoin_help[] = {
  "NJOIN <channel name> <ts> <[prefix]uid> [[prefix]uid] ...",
  "",
  "Introduces the users in a channel to a remote server.",
  NULL
};

static struct msg ms_njoin_msg = {
  "NJOIN", 3, 3, MFLG_SERVER,
  { NULL, NULL, ms_njoin, NULL },
  ms_njoin_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_njoin_load(void)
{
  if(msg_register(&ms_njoin_msg) == NULL)
    return -1;

  return 0;
}

void m_njoin_unload(void)
{
  msg_unregister(&ms_njoin_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'njoin'                                                          *
 * argv[2] - channel name                                                     *
 * argv[3] - channel ts                                                       *
 * argv[4] - prefix:uid|nick*                                                 *
 * -------------------------------------------------------------------------- */
static void ms_njoin(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  struct chanuser *cuptr = NULL;
  struct channel  *chptr;
  struct node     *nptr;
  unsigned long    ts;
  struct list      chanusers;
//  int              dropts = 0;
  int              dropremote = 0;

  if(!client_is_server(cptr))
    return;

  /* Check for valid channel name */
  if(!chars_valid_chan(argv[2]))
  {
    log(channel_log, L_warning, "Dropping NJOIN for invalid channel %s.",
        argv[2]);
    return;
  }

  ts = str_toul(argv[3], NULL, 10);

  /* Try to find the channel */
  chptr = channel_find_name(argv[2]);

  if(chptr == NULL)
  {
    /* Add to channel list */
    chptr = channel_new(argv[2]);

    chptr->ts = ts;
  }
  else
  {
    /* Check the TS */
    if(chptr->ts > ts)
    {
//      dropts = 1;
      chptr->ts = ts;

      log(channel_log, L_warning, "Dropping TS for channel %s.",
          chptr->name);

      chanuser_drop(cptr, chptr);
      chanmode_drop(cptr, chptr);

      if(chptr->topic[0])
      {
        chptr->topic[0] = '\0';
        channel_send(NULL, chptr, CHFLG(NONE), CHFLG(NONE),
                     ":%C TOPIC %s :", cptr, chptr->name);
      }
    }

    if(ts > chptr->ts)
      dropremote = 1;
  }

  /* Uh, error adding channel */
  if(chptr == NULL)
  {
    log(channel_log, L_warning,
        "Dropping NJOIN to %s, out of memory.",
        chptr->name);
    return;
  }

  if(chptr->serial != cptr->server->cserial)
  {
    cptr->server->in.channels++;
    cptr->server->cserial = chptr->serial;
  }

  /* Parse prefixed nick list */
  cptr->server->in.chanmodes +=
    chanuser_parse(lcptr, &chanusers, chptr, argv[4], !dropremote);

  /* Send to servers */
  chanuser_introduce(lcptr, cptr, chanusers.head);

  /* Send to channel */
  chanuser_send_joins(NULL, chanusers.head);
  chanuser_send_modes(NULL, cptr, chanusers.head);

  /* Link users to the channel */
  dlink_foreach_data(&chanusers, nptr, cuptr)
  {
    dlink_add_tail(&chptr->chanusers, &cuptr->gnode, cuptr);
    dlink_add_tail(&chptr->rchanusers, &cuptr->rnode, cuptr);
    dlink_add_tail(&cuptr->client->user->channels, &cuptr->unode, cuptr);
  }

  /* Destroy temp. userlist */
  dlink_destroy(&chanusers);
}
