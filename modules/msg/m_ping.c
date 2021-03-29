/* chaosircd - Chaoz's IRC daemon daemon
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
 * $Id: m_ping.c,v 1.3 2006/09/28 09:56:24 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/dlink.h"

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
static void m_ping  (struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_ping_help[] = {
  "PING [target] [text]",
  "",
  "Sends a irc ping to the target.",
  "If the target is ommited then it is sent to your local server.",
  NULL
};

static struct msg m_ping_msg = {
  "PING", 0, 2, MFLG_CLIENT,
  { NULL, m_ping, m_ping, m_ping },
  m_ping_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_ping_load(void)
{
  if(msg_register(&m_ping_msg) == NULL)
    return -1;

  return 0;
}

void m_ping_unload(void)
{
  msg_unregister(&m_ping_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'ping'                                                           *
 * -------------------------------------------------------------------------- */
static void m_ping(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  struct server *asptr;
  struct client *acptr;

  /* do pings on server-server links */
  if(client_is_server(cptr) && client_is_local(cptr) && argv[2] && chars_isdigit(argv[2][0]))
  {
    lclient_send(lcptr, "PONG :%s", argv[2]);
    return;
  }

  /* bitchx lag shit */
  if(argc == 4 && chars_isdigit(argv[2][0]))
  {
    if((asptr = server_find_name(argv[3])) == server_me)
    {
      client_send(cptr, ":%S PONG %S :%s",
                  server_me, server_me, argv[2]);
      return;
    }
  }

  /* rfc violating ping shit */
  if(argv[2] == NULL)
    argv[2] = server_me->name;

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
    acptr = client_me;
    argv[3] = argv[2];
    argv[2] = client_me->name;
  }

  if(acptr == client_me)
  {
    if(argv[3])
      client_send(cptr, ":%C PONG %C :%s", acptr, cptr, argv[3]);
    else
      client_send(cptr, ":%C PONG %C", acptr, cptr);
  }
  else
  {
    if(argv[3])
      client_send(acptr, ":%C PING %C :%s", cptr, acptr, argv[3]);
    else
      client_send(acptr, ":%C PING %C", cptr, acptr);
  }

}

