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
 * $Id: m_connect.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/dlink.h"
#include "libchaos/connect.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/msg.h>
#include <ircd/client.h>
#include <ircd/server.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mo_connect(struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mo_connect_help[] = {
  "CONNECT [from] <to>",
  "",
  "Operator command causing a server to connect to",
  "another server. If [from] is ommitted then the",
  "current server will connect.",
  NULL
};

static struct msg mo_connect_msg = {
  "CONNECT", 1, 2, MFLG_OPER,
  { NULL, NULL, mo_connect, mo_connect },
  mo_connect_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_connect_load(void)
{
  if(msg_register(&mo_connect_msg) == NULL)
    return -1;

  return 0;
}

void m_connect_unload(void)
{
  msg_unregister(&mo_connect_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'connect'                                                        *
 * argv[2] - server to connect from/to                                        *
 * argv[3] - [server to connect to]                                           *
 * -------------------------------------------------------------------------- */
static void mo_connect(struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv)
{
  struct connect *cnptr;
  struct server  *asptr;

  /* We have 2 arguments, the message must be relayed */
  if(argv[3])
  {
    if(server_relay_always(lcptr, cptr, 2, ":%C CONNECT %s %s", &argc, argv))
      return;
  }

  /* Message is targeted to us, search connect{} block */
  if((cnptr = connect_find_name(argv[2])) == NULL)
  {
    client_send(cptr, ":%S NOTICE %C :*** no connect{} block for %s",
                server_me, cptr, argv[3]);
    return;
  }

  /* Cannot have redundant links (currently) */
  if((asptr = server_find_name(cnptr->name)))
  {
    if(asptr->client->origin != client_me)
      client_send(cptr,
                  ":%S NOTICE %C :*** server %S already exists from %S.",
                  server_me, cptr, asptr, asptr->client->origin->server);
    else
      client_send(cptr,
                  ":%S NOTICE %C :*** server %S already exists.",
                  server_me, cptr, asptr);
    return;
  }

  /* Intitiate the connect{} block */
  if(server_connect(cptr, cnptr))
  {
    client_send(cptr, ":%S NOTICE %C :*** couldn't connect to %s[%s:%u]",
                server_me, cptr, cnptr->name,
                cnptr->address, cnptr->port_remote);
  }
}
