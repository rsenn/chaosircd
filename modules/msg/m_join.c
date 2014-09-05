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
 * $Id: m_join.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "defs.h"
#include "io.h"
#include "timer.h"
#include "log.h"
#include "net.h"
#include "str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/msg.h>
#include <ircd/ircd.h>
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
static void m_join (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_join_help[] = {
  "JOIN <channel[,channel,...]> [key[,key,...]]",
  "",
  "This command makes you to join the given channel, or",
  "multiple channels. If they have keys set, you should",
  "give the key, or the keys as the second parameter.",
  "Multiple channel names, or keys are seperated with",
  "commata.",
  NULL
};

static struct msg m_join_msg = {
  "JOIN", 1, 2, MFLG_CLIENT,
  { NULL, m_join, NULL, m_join },
  m_join_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_join_load(void)
{
  if(msg_register(&m_join_msg) == NULL)
    return -1;

  return 0;
}

void m_join_unload(void)
{
  msg_unregister(&m_join_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'join'                                                           *
 * argv[2] - channel                                                          *
 * argv[3] - key                                                              *
 * -------------------------------------------------------------------------- */
static void m_join(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  char    *chanv[IRCD_MAXCHANNELS + 1];
  char    *keyv[IRCD_MAXCHANNELS + 1];
  uint32_t n;
  uint32_t i;

  n = str_tokenize_s(argv[2], chanv, IRCD_MAXCHANNELS, ',');
  memset(keyv, 0, sizeof(keyv));

  if(argv[3])
    str_tokenize_s(argv[3], keyv, IRCD_MAXCHANNELS, ',');

  for(i = 0; i < n; i++)
  {
    /* Check for valid channel name */
    if(!chars_valid_chan(chanv[i]) || str_len(chanv[i]) > IRCD_CHANNELLEN)
    {
      numeric_send(cptr, ERR_BADCHANNAME, chanv[i]);
      return;
    }

    if(!channel_is_valid(chanv[i]))
    {
      numeric_send(cptr, ERR_NOSUCHCHANNEL, chanv[i]);
      return;
    }

    if(cptr->user->channels.size == IRCD_MAXCHANNELS)
    {
      numeric_send(cptr, ERR_TOOMANYCHANNELS, chanv[i]);
      return;
    }

    channel_join(lcptr, cptr, chanv[i], keyv[i]);
  }
}
