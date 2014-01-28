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
 * $Id: m_server.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/io.h>
#include <libchaos/timer.h>
#include <libchaos/log.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/msg.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/lclient.h>
#include <chaosircd/server.h>
#include <chaosircd/channel.h>
#include <chaosircd/numeric.h>
#include <chaosircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mr_server (struct lclient *lcptr, struct client  *cptr,
                       int             argc,  char          **argv);

static void ms_server (struct lclient *lcptr, struct client  *cptr,
                       int             argc,  char          **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_server_help[] = {
  "SERVER <name> <info>",
  "",
  "Informs the remote server about an existing server.",
  NULL
};    

static struct msg m_server_msg = {
  "SERVER", 2, 2, MFLG_UNREG | MFLG_SERVER,
  { mr_server, NULL, ms_server, NULL },
  m_server_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_server_load(void)
{
  if(msg_register(&m_server_msg) == NULL)
    return -1;
  
  return 0;
}

void m_server_unload(void)
{
  msg_unregister(&m_server_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'server'                                                         *
 * argv[2] - servername                                                       *
 * argv[4] - serverinfo                                                       *
 * -------------------------------------------------------------------------- */
static void mr_server(struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv)
{
  struct server *sptr;
  
  if(lcptr->pass[0] == '\0' || lcptr->caps == 0)
  {
    lclient_exit(lcptr, "protocol mismatch");
    return;
  }
  
  lclient_set_type(lcptr, LCLIENT_SERVER);
  
  if((sptr = server_find_name(argv[2])))
  {
    lclient_exit(lcptr, "server already exists");
    log(server_log, L_warning, "Server %s already exists from %s",
        sptr->name, sptr->client->source->name);
    return;
  }
  
  if(!chars_valid_host(argv[2]))
  {
    lclient_set_type(lcptr, LCLIENT_SERVER);    
    lclient_exit(lcptr, "bogus server name");
    return;
  }
  
  if(!lcptr->name[0])
    strlcpy(lcptr->name, argv[2], sizeof(lcptr->name));
  
  if(!lcptr->info[0])
    strlcpy(lcptr->info, argv[3], sizeof(lcptr->info));
  
  log(server_log, L_status, "Received serverinfo from %s [%s]", 
      argv[2], argv[3]);

  server_login(lcptr);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'server'                                                         *
 * argv[2] - servername                                                       *
 * argv[3] - serverinfo                                                       *
 * -------------------------------------------------------------------------- */
static void ms_server(struct lclient *lcptr, struct client *cptr, 
                      int             argc,  char         **argv)
{
  struct server *sptr;
  int            hops;
  
  hops = str_toul(argv[3], NULL, 10);

  sptr = server_find_name(argv[2]);
  
  if(sptr)
  {
    log(server_log, L_warning, 
        "Server %s coming from %s already exists from %s.",
        sptr->name, cptr->name, sptr->client->origin->name);
    lclient_exit(lcptr, "server %s already exists from %s.",
                 sptr->name, sptr->client->origin->name);
    return;
  }
  
  server_send(lcptr, NULL, CAP_NONE, CAP_NSV,
              ":%s SERVER %s :%s",
              cptr->name, argv[2], argv[3]);
  server_send(lcptr, NULL, CAP_NSV, CAP_NONE,
              ":%s NSERVER %N %s :%s",
              cptr->name, cptr, argv[2], argv[3]);
  
  cptr->server->in.servers++;

  server_new_remote(lcptr, cptr, argv[2], argv[3]);
}
