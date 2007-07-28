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
 * $Id: m_restart.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
#include <chaosircd/msg.h>
#include <chaosircd/ircd.h>
#include <chaosircd/user.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/channel.h>
#include <chaosircd/numeric.h>
#include <chaosircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mo_restart(struct lclient *lcptr, struct client *cptr, 
                       int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mo_restart_help[] =
{
  "RESTART [server]",
  "",
  "Operator command causing a server to restart.",
  NULL
};

static struct msg mo_restart_msg = {
  "RESTART", 0, 1, MFLG_OPER,
  { NULL, NULL, mo_restart, mo_restart },
  mo_restart_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_restart_load(void)
{
  if(msg_register(&mo_restart_msg) == NULL)
    return -1;
  
  return 0;
}

void m_restart_unload(void)
{
  msg_unregister(&mo_restart_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'restart'                                                         *
 * argv[2] - name                                                             *
 * -------------------------------------------------------------------------- */
static void mo_restart(struct lclient *lcptr, struct client *cptr, 
                      int             argc,  char         **argv)
{
  if(argc == 3)
  {
    if(server_relay_always(lcptr, cptr, 2, ":%C restart %s", &argc, argv))
      return;
  }

  log(ircd_log, L_status, "%s (%s@%s) is restarting me.",
      cptr->name, cptr->user->name, cptr->host);

  ircd_restart();
}
