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
 * $Id: m_nserver.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/defs.h>
#include <libchaos/io.h>
#include <libchaos/timer.h>
#include <libchaos/log.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/msg.h>
#include <ircd/chars.h>
#include <ircd/server.h>
#include <ircd/client.h>
#include <ircd/lclient.h>
#include <ircd/channel.h>
#include <ircd/numeric.h>
#include <ircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void ms_nserver (struct lclient *lcptr, struct client  *cptr,
                        int             argc,  char          **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_nserver_help[] = {
  "NSERVER <uplink> <name> <info>",
  "",
  "Bursts a remote server to an existing server.",
  NULL
};

static struct msg m_nserver_msg = {
  "NSERVER", 3, 3, MFLG_SERVER,
  { NULL, NULL, ms_nserver, NULL },
  m_nserver_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_nserver_load(void)
{
  if(msg_register(&m_nserver_msg) == NULL)
    return -1;

  server_default_caps |= CAP_NSV;

  return 0;
}

void m_nserver_unload(void)
{
  server_default_caps &= ~CAP_NSV;

  msg_unregister(&m_nserver_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'server'                                                         *
 * argv[2] - uplink                                                           *
 * argv[3] - servername                                                       *
 * argv[4] - serverinfo                                                       *
 * -------------------------------------------------------------------------- */
static void ms_nserver(struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv)
{
  struct server *sptr;
  struct server *asptr;
//  int            hops;
  //hops = str_toul(argv[3], NULL, 10);

  sptr = server_find_name(argv[2]);
  asptr = server_find_name(argv[3]);

  if(asptr)
  {
    log(server_log, L_warning,
        "Server %s coming from %s already exists from %s.",
        asptr->name, cptr->name, asptr->client->origin->name);
    lclient_exit(lcptr, "server %s already exists from %s.",
                 asptr->name, asptr->client->origin->name);
    return;
  }

  if(sptr == NULL)
  {
    log(server_log, L_warning,
        "Uplink %s for server %s coming from %s doesn't exist.",
        argv[2], argv[3], cptr->name);
    lclient_exit(lcptr, "uplink %s for server %s doesn't exist.",
                 sptr->name, argv[3]);
    return;
  }

  server_send(lcptr, NULL, CAP_NSV, CAP_NONE,
              ":%N NSERVER %S %s :%s",
              cptr, sptr, argv[3], argv[4]);
  server_send(lcptr, NULL, CAP_NONE, CAP_NSV,
              ":%S SERVER %s :%s",
              sptr, argv[3], argv[4]);

  cptr->server->in.servers++;

  server_new_remote(lcptr, sptr->client, argv[3], argv[4]);
}
