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
 * $Id: m_squit.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/log.h"
#include "libchaos/dlink.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/user.h"
#include "ircd/server.h"
#include "ircd/client.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mo_squit(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mo_squit_help[] = {
  "SQUIT [target] <victim> [reason]",
  "",
  "Disconnects victim from the target. If target is ommitted",
  "the victims origin will be assumed.",
  NULL
};

static struct msg mo_squit_msg = {
  "SQUIT", 1, 2, MFLG_OPER,
  { NULL, NULL, mo_squit, mo_squit },
  mo_squit_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_squit_load(void)
{
  if(msg_register(&mo_squit_msg) == NULL)
    return -1;

  return 0;
}

void m_squit_unload(void)
{
  msg_unregister(&mo_squit_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'squit'                                                          *
 * argv[2] - server/link to drop                                              *
 * argv[3] - link to drop                                                     *
 * -------------------------------------------------------------------------- */
static void mo_squit(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  struct server *sptr;
  struct server *asptr;

  if((sptr = server_find_namew(cptr, argv[2])) == NULL)
    return;

  if(argc == 4)
  {
    if((asptr = server_find_namew(cptr, argv[3])) == NULL)
      return;
  }
  else
  {
    asptr = sptr;

    if(client_is_remote(sptr->client))
      sptr = sptr->client->origin->server;
    else
      sptr = server_me;
  }

  if(sptr == server_me)
  {
    if(!client_is_local(asptr->client) || asptr == server_me)
    {
      log(server_log, L_warning,
          "Dropping invalid SQUIT from %N (%U@%H) for remote server %S",
          cptr, cptr, cptr, asptr);
      client_send(cptr, ":%S NOTICE %C :*** Server %S is not my client.",
                  server_me, cptr, asptr);
      return;
    }

    client_exit(asptr->client->lclient, asptr->client, "SQUIT by %N (%U@%H)",
                cptr, cptr, cptr);

    return;
  }

  client_send(sptr->client, ":%C SQUIT %S %S",
              cptr, sptr, asptr);
}

