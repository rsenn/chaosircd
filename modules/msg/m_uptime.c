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
 * $Id: m_uptime.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/defs.h>
#include <libchaos/io.h>
#include <libchaos/log.h>
#include <libchaos/module.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/msg.h>
#include <chaosircd/user.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/lclient.h>
#include <chaosircd/numeric.h>
#include <chaosircd/channel.h>
#include <chaosircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_uptime (struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_uptime_help[] = {
  "UPTIME [server]",
  "",
  "Displays uptime of the given server. If used",
  "without parameters, the uptime of the local",
  "server is displayed.",
  NULL
};    

static struct msg m_uptime_msg = {
  "UPTIME", 0, 1, MFLG_CLIENT,
  { NULL, m_uptime, m_uptime, m_uptime },
  m_uptime_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_uptime_load(void)
{
  if(msg_register(&m_uptime_msg) == NULL)
    return -1;
  
  return 0;
}

void m_uptime_unload(void)
{
  msg_unregister(&m_uptime_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'uptime'                                                         *
 * -------------------------------------------------------------------------- */
static void m_uptime(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  if(argc > 2)
  {
    if(server_relay_always(lcptr, cptr, 2, ":%C UPTIME :%s", &argc, argv))
      return;
  }
  
  numeric_send(cptr, RPL_STATSUPTIME, ircd_uptime());
}

