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
 * $Id: m_kick.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/dlink.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/msg.h"
#include "ircd/user.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/channel.h"
#include "ircd/numeric.h"
#include "ircd/chanmode.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_kick (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_kick_help[] = {
  "KICK <channel> <nick[,nick,...]> [reason]",
  "",
  "A kick command causes the given nickname, or",
  "multiple nicknames from being kicked off the",
  "given channelname with the given reason.",
  "",
  "If no reason is specified, the name of the",
  "user originating the kick is used.",
  NULL
};

static struct msg m_kick_msg = {
  "KICK", 2, 3, MFLG_CLIENT,
  { NULL, m_kick, m_kick, m_kick },
  m_kick_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_kick_load(void)
{
  if(msg_register(&m_kick_msg) == NULL)
    return -1;

  return 0;
}

void m_kick_unload(void)
{
  msg_unregister(&m_kick_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'kick'                                                           *
 * argv[2] - channel                                                          *
 * argv[3] - target                                                           *
 * argv[4] - reason                                                           *
 * -------------------------------------------------------------------------- */
static void m_kick(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  struct channel  *chptr;
  struct chanuser *cuptr;
  char            *targetv[IRCD_MAXTARGETS + 1];
/*  uint32_t         n;*/

  if((chptr = channel_find_warn(cptr, argv[2])) == NULL)
    return;

  cuptr = chanuser_find(chptr, cptr);

  if(cuptr == NULL)
  {
    numeric_send(cptr, ERR_NOTONCHANNEL, chptr->name);
    return;
  }

  /*n =*/ str_tokenize_s(argv[3], targetv, IRCD_MAXTARGETS, ',');

  if(argv[4] == NULL)
    argv[4] = cptr->name;

  chanuser_kick(lcptr, cptr, chptr, cuptr, targetv,
                argv[4] ? argv[4] : cptr->name);
}
