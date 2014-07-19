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
 * $Id: m_kill.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
#include <ircd/ircd.h>
#include <ircd/user.h>
#include <ircd/chars.h>
#include <ircd/client.h>
#include <ircd/server.h>
#include <ircd/channel.h>
#include <ircd/numeric.h>
#include <ircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mo_kill(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mo_kill_help[] =
{
  "KILL <user> [reason]",
  "",
  "Operator command causing a user to disconnect",
  NULL
};

static struct msg mo_kill_msg = {
  "KILL", 1, 2, MFLG_OPER,
  { NULL, NULL, mo_kill, mo_kill },
  mo_kill_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_kill_load(void)
{
  if(msg_register(&mo_kill_msg) == NULL)
    return -1;

  return 0;
}

void m_kill_unload(void)
{
  msg_unregister(&mo_kill_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'kill'                                                           *
 * argv[2] - name                                                             *
 * -------------------------------------------------------------------------- */
static void mo_kill(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  int            ret;
  struct client *acptr;

  if(argc > 3)
  {
    if((ret = client_relay_always(lcptr, cptr, &acptr, 2, ":%C KILL %s :%s", &argc, argv)) == -1)
      return;
  }
  else
  {
    if((ret = client_relay_always(lcptr, cptr, &acptr, 2, ":%C KILL %s", &argc, argv)) == -1)
      return;
  }

  if(argc > 3)
  {
    if(cptr->user)
      log(ircd_log, L_status, "%s (%s@%s) killed %s (%s@%s): %s",
          cptr->name, cptr->user->name, cptr->host,
          acptr->name, acptr->user->name, acptr->host,
          argv[3]);
    else
      log(ircd_log, L_status, "%s killed %s (%s@%s): %s",
          cptr->name,
          acptr->name, acptr->user->name, acptr->host,
          argv[3]);
  }
  else
  {
    if(cptr->user)
      log(ircd_log, L_status, "%s (%s@%s) killed %s (%s@%s).",
          cptr->name, cptr->user->name, cptr->host,
          acptr->name, acptr->user->name, acptr->host);
    else
      log(ircd_log, L_status, "%s killed %s (%s@%s).",
          cptr->name,
          acptr->name, acptr->user->name, acptr->host);
  }

  if(ret == 1)
    return;

  if(argc > 3)
  {
    if(cptr->user)
      client_exit(NULL, acptr,
                  "killed by %s (%s@%s): %s",
                  cptr->name, cptr->user->name, cptr->host, argv[3]);
    else
      client_exit(NULL, acptr, "killed by %s: %s", cptr->name, argv[3]);
  }
  else
  {
    if(cptr->user)
      client_exit(NULL, acptr,
                  "killed by %s (%s@%s)",
                  cptr->name, cptr->user->name, cptr->host);
    else
      client_exit(NULL, acptr, "killed by %s", cptr->name);
  }
}
