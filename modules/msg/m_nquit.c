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
 * $Id: m_nquit.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
#include "ircd/client.h"
#include "ircd/server.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void ms_nquit(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *ms_nquit_help[] = {
  "NQUIT <uplink> <victim>",
  "",
  "Notifies remote servers about a network split.",
  NULL
};

static struct msg ms_nquit_msg = {
  "NQUIT", 1, 2, MFLG_SERVER,
  { NULL, NULL, ms_nquit, NULL },
  ms_nquit_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_nquit_load(void)
{
  if(msg_register(&ms_nquit_msg) == NULL)
    return -1;

  return 0;
}

void m_nquit_unload(void)
{
  msg_unregister(&ms_nquit_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'nquit'                                                          *
 * argv[2] - server                                                           *
 * argv[3] - reason                                                           *
 * -------------------------------------------------------------------------- */
static void ms_nquit(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  struct server *sptr;

  if((sptr = server_find_name(argv[2])) == NULL)
  {
    log(server_log, L_warning, "Dropping NQUIT from %s for unknown server %s.",
        cptr->name, argv[2]);
    return;
  }

  if(sptr == server_me)
  {
    log(server_log, L_warning, "I do not NQUIT myself!!! (from %s)",
        cptr->name);
    return;
  }

  client_exit(lcptr, sptr->client, "%s", argv[3]);
}

