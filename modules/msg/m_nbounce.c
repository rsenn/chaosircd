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
 * $Id: m_nbounce.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/log.h"
#include "libchaos/dlink.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/msg.h"
#include "ircd/user.h"
#include "ircd/server.h"
#include "ircd/client.h"
#include "ircd/lclient.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void ms_nbounce(struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *ms_nbounce_help[] =
{
  "NBOUNCE <uid> <nick> <ts>",
  "",
  "Bounces back a nick that has been introduced during a netjoin.",
  NULL
};

static struct msg ms_nbounce_msg =
{
  "NBOUNCE", 3, 3, MFLG_SERVER,
  { NULL, NULL, ms_nbounce, NULL },
  ms_nbounce_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_nbounce_load(void)
{
  if(msg_register(&ms_nbounce_msg) == NULL)
    return -1;

  return 0;
}

void m_nbounce_unload(void)
{
  msg_unregister(&ms_nbounce_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'nbounce'                                                        *
 * argv[2] - old nick/uid                                                     *
 * argv[3] - new nick                                                         *
 * argv[4] - ts                                                               *
 * -------------------------------------------------------------------------- */
static void ms_nbounce(struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv)
{
  struct client *acptr;

  if(lcptr->caps & CAP_UID)
    acptr = client_find_uid(argv[2]);
  else
    acptr = client_find_nick(argv[2]);

  if(acptr == NULL)
  {
    log(client_log, L_warning, "Dropping invalid NBOUNCE for user %s.",
        argv[2]);
    return;
  }

  if(client_is_local(acptr))
  {
    client_send(acptr, ":%S NOTICE %C :*** Your nick has been bounced at %C",
                server_me, acptr, cptr);
  }

  acptr->ts = str_toul(argv[4], NULL, 10);

  chanuser_send(NULL, acptr, ":%N!%U@%H NICK :%s",
                acptr, acptr, acptr, argv[3]);

  server_send(lcptr, NULL, CAP_UID, CAP_NONE,
              ":%s NBOUNCE %s %s :%u",
              cptr->name, acptr->user->uid, argv[3], acptr->ts);
  server_send(lcptr, NULL, CAP_NONE, CAP_UID,
              ":%s NBOUNCE %s %s :%u",
              cptr->name, acptr->name, argv[3], acptr->ts);

  client_set_name(acptr, argv[3]);
}

