/* chaosircd - CrowdGuard IRC daemon
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
 * $Id: m_nservice.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/dlink.h"
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/log.h"
#include "libchaos/str.h"
#include "libchaos/timer.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/usermode.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/chars.h"
#include "ircd/service.h"
#include "ircd/user.h"
#include "ircd/msg.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void ms_nservice(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *ms_nservice_help[] =
{
  "NSERVICE <nick> <user> <host> <uid> :<info>",
  "",
  "Introduces a client to a remote server.",
  NULL
};

static struct msg ms_nservice_msg = {
  "NSERVICE", 5, 5, MFLG_SERVER,
  { NULL, NULL, ms_nservice, NULL },
  ms_nservice_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_nservice_load(void)
{
  if(msg_register(&ms_nservice_msg) == NULL)
    return -1;

  return 0;
}

void m_nservice_unload(void)
{
  msg_unregister(&ms_nservice_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0]  - prefix                                                          *
 * argv[1]  - 'nservice'                                                      *
 * argv[2]  - nickname                                                        *
 * argv[3]  - user                                                            *
 * argv[4]  - host                                                            *
 * argv[5]  - uid                                                             *
 * argv[6]  - info                                                            *
 * -------------------------------------------------------------------------- */
static void ms_nservice(struct lclient *lcptr, struct client *cptr,
                        int             argc,  char         **argv)
{
  struct user    *uptr;
//  struct service *svptr;

  if(!client_is_server(cptr))
    return;

  /* Check UID collision */
  if((uptr = user_find_uid(argv[5])))
  {
    log(server_log, L_warning, "UID collision");

    client_send(cptr, "KILL %s :UID collision", argv[5]);
    return;
  }

  /* Validate nickname */
  if(!chars_valid_nick(argv[2]))
  {
    log(server_log, L_warning, "service has invalid nickname: %s",
        argv[2]);

    client_send(cptr, "KILL %s :Invalid nickname", argv[5]);
    return;
  }

  /* Validate username */
  if(!chars_valid_user(argv[3]))
  {
    log(server_log, L_warning, "service %s has invalid username: %s",
        argv[2], argv[5]);

    client_send(cptr, "KILL %s :Invalid username", argv[10]);
    return;
  }

  /* Check nick collision */
/*  if((acptr = client_find_nick(argv[2])))
  {
    if(acptr->ts <= ts)
    {
      ts = timer_systime;

      if(client_nick_rotate(argv[2], newnick))
        client_nick_random(newnick);

      client_send(cptr, ":%S NBOUNCE %s %s :%lu",
                  server_me, argv[10], newnick, ts);

      argv[2] = newnick;
    }
  }*/

  /* ...*/

  /* Register and introduce the remote client */
  /*svptr = */service_new(argv[2], argv[3], argv[4], argv[6]);

}

