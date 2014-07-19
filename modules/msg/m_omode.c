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
 * $Id: m_omode.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/defs.h>
#include <libchaos/io.h>
#include <libchaos/log.h>
#include <libchaos/timer.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/msg.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/lclient.h>
#include <chaosircd/numeric.h>
#include <chaosircd/channel.h>
#include <chaosircd/usermode.h>
#include <chaosircd/chanmode.h>
#include <chaosircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mo_omode         (struct lclient *lcptr, struct client *cptr,
                              int             argc,  char         **argv);
static void mo_omode_channel (struct lclient *lcptr, struct client *cptr,
                              struct channel *chptr, int            argc,
                              char          **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_omode_help[] = {
  "OMODE <channel> <flags> [args]",
  "",
  "Operator command for changing mode on a given channel.",
  "For a detailed description of the channelmodes use the",
  "following command:",
  "/quote help chanmodes",
  NULL
};

static struct msg m_omode_msg = {
  "OMODE", 1, 3, MFLG_OPER,
  { NULL, NULL, NULL, mo_omode },
  m_omode_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_omode_load(void)
{
  if(msg_register(&m_omode_msg) == NULL)
    return -1;

  return 0;
}

void m_omode_unload(void)
{
  msg_unregister(&m_omode_msg);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void mo_omode_channel (struct lclient *lcptr, struct client *cptr,
                              struct channel *chptr, int            argc,
                              char          **argv)
{
  struct list           list;

  if(argc < 4)
  {
    chanmode_show(cptr, chptr);
    return;
  }

  chanmode_parse(lcptr, cptr, chptr, NULL, &list, argv[3], argv[4],
                 client_is_user(cptr) ? IRCD_MODESPERLINE : CHANMODE_PER_LINE);

  chanmode_apply(lcptr, cptr, chptr, NULL, &list);

  chanmode_send_local(cptr, chptr, list.head,
                      client_is_user(cptr) ? IRCD_MODESPERLINE : CHANMODE_PER_LINE);
  chanmode_send_remote(lcptr, cptr, chptr, list.head);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'mode'                                                           *
 * argv[2] - nick/channel                                                     *
 * argv[3] - [modebuf]                                                        *
 * argv[4] - [argbuf]                                                         *
 * -------------------------------------------------------------------------- */
static void mo_omode(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  if(channel_is_valid(argv[2]))
  {
    struct channel *chptr;

    chptr = channel_find_name(argv[2]);

    if(chptr == NULL)
    {
      client_send(cptr, numeric_format(ERR_NOSUCHCHANNEL),
                  client_me->name, cptr->name, argv[2]);
      return;
    }

    mo_omode_channel(lcptr, cptr, chptr, argc, argv);
  }
}
