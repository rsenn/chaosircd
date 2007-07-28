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
 * $Id: m_lusers.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/dlink.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/msg.h>
#include <chaosircd/user.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/numeric.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_lusers(struct lclient *lcptr, struct client *cptr, 
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_lusers_help[] = {
  "LUSERS [server]",
  "",
  "Displays the amount of online users, and max amount of",
  "online users in the server, and globally in the network.",
  "If no parameter given, the local server is queried.",
  "",
  "Also displays information about connected clients,",
  "number of online operators, and used channels.",
  NULL
};  

static struct msg m_lusers_msg = {
  "LUSERS", 0, 1, MFLG_CLIENT,
  { NULL, m_lusers, m_lusers, m_lusers },
  m_lusers_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_lusers_load(void)
{
  if(msg_register(&m_lusers_msg) == NULL)
    return -1;
  
  return 0;
}

void m_lusers_unload(void)
{
  msg_unregister(&m_lusers_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'lusers'                                                         *
 * -------------------------------------------------------------------------- */
static void m_lusers(struct lclient *lcptr, struct client *cptr, 
                     int             argc,  char         **argv)
{
  if(argc > 2)
  {
    if(server_relay_always(lcptr, cptr, 2, ":%C LUSERS :%s", &argc, argv))
      return;
  }
  
  client_lusers(cptr);
}
