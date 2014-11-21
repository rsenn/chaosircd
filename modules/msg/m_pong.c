/* chaosircd - CrowdGuard IRC daemon
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
 * $Id: m_pong.c,v 1.3 2006/09/28 09:56:24 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/dlink.h"
#include "libchaos/io.h"
#include "libchaos/timer.h"
#include "libchaos/log.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/lclient.h"
#include "ircd/server.h"
#include "ircd/numeric.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_pong (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_pong_help[] = {
  "PONG [origin] [text]",
  "",
  "Reply to a received PING command.",
  NULL
};

static struct msg m_pong_msg = {
  "PONG", 1, 2, MFLG_CLIENT | MFLG_UNREG,
  { NULL, m_pong, m_pong, m_pong },
  m_pong_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_pong_load(void)
{
  if(msg_register(&m_pong_msg) == NULL)
    return -1;

  return 0;
}

void m_pong_unload(void)
{
  msg_unregister(&m_pong_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'pong'                                                           *
 * -------------------------------------------------------------------------- */
static void m_pong(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  struct server *asptr;
  struct client *acptr;

  /* do pings on server-server links */
  if(client_is_server(cptr) && client_is_local(cptr) &&
     argv[2] && chars_isdigit(argv[2][0]))
  {
    lcptr->ping = str_toull(argv[2], NULL, 10);
    lcptr->lag = timer_mtime - lcptr->ping;

    return;
  }

  asptr = server_find_name(argv[2]);

  if(asptr == NULL)
  {
    if(lcptr->caps & CAP_UID)
      acptr = client_find_uid(argv[2]);
    else
      acptr = client_find_nick(argv[2]);
  }
  else
  {
    acptr = asptr->client;
  }

  if(acptr == NULL)
  {
    numeric_send(cptr, ERR_NOSUCHSERVER, argv[2]);
    return;
  }

  if(acptr == client_me)
  {
    if(client_is_user(cptr) && client_is_local(cptr))
      lcptr->lag = timer_mtime - lcptr->ping;
  }
  else
  {
    if(argv[3])
      client_send(acptr, ":%C PONG %C :%s", cptr, acptr, argv[3]);
    else
      client_send(acptr, ":%C PONG %C", cptr, acptr, argv[3]);
  }
}

