/* chaosircd - pi-networks irc server
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
 * $Id: m_names.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/dlink.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/msg.h>
#include <chaosircd/user.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/lclient.h>
#include <chaosircd/numeric.h>
#include <chaosircd/channel.h>
#include <chaosircd/chanuser.h>
#include <chaosircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_names  (struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_names_help[] = {
  "NAMES [channel]",
  "",
  "Displays a list of nicks in the given channel.",
  "Channeluser flags like op/halfop/voice are prepended",
  "to the names of the users with their appropriate",
  "symbols.",
  NULL
};

static struct msg m_names_msg = {
  "NAMES", 0, 1, MFLG_CLIENT,
  { NULL, m_names, NULL, m_names },
  m_names_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_names_load(void)
{
  if(msg_register(&m_names_msg) == NULL)
    return -1;

  return 0;
}

void m_names_unload(void)
{
  msg_unregister(&m_names_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'names'                                                          *
 * -------------------------------------------------------------------------- */
static void m_names(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  struct chanuser *cuptr;
  struct channel  *chptr;

  if(!client_is_user(cptr))
    return;

  if(argv[2] && argv[2][0] != '*')
  {
    struct channel *chptr;

    if(!chars_valid_chan(argv[2]))
    {
      client_send(cptr, numeric_format(ERR_BADCHANNAME),
                  client_me->name, cptr->name, argv[2]);
      return;
    }

    if((chptr = channel_find_warn(cptr, argv[2])) == NULL)
      return;

    cuptr = chanuser_find(chptr, cptr);

    chanuser_show(cptr, chptr, cuptr, 1);
  }
  else
  {
    channel_serial++;

    dlink_foreach(&cptr->user->channels, cuptr)
    {
      cuptr->channel->serial = channel_serial;

      chanuser_show(cptr, cuptr->channel, cuptr, 1);
    }

    dlink_foreach(&channel_list, chptr)
    {
      if(chptr->serial == channel_serial)
        continue;

      if(!(chptr->modes & CHFLG(s)))
        chanuser_show(cptr, chptr, NULL, 2);
    }
  }
}
