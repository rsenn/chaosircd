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
 * $Id: m_whois.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/defs.h>
#include <libchaos/io.h>
#include <libchaos/log.h>
#include <libchaos/str.h>
#include <libchaos/module.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/msg.h>
#include <ircd/user.h>
#include <ircd/chars.h>
#include <ircd/client.h>
#include <ircd/server.h>
#include <ircd/lclient.h>
#include <ircd/numeric.h>
#include <ircd/channel.h>
#include <ircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_whois (struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_whois_help[] = {
  "WHOIS <nickname>",
  "",
  "Gives whois information of the given nickname.",
  "If the target is on a remote server the query",
  "is forwarded to this server in order to get",
  "the idle time from everywhere in the network.",
  NULL
};

static struct msg m_whois_msg =
{
  "WHOIS", 1, 2, MFLG_CLIENT,
  { NULL, m_whois, m_whois, m_whois },
  m_whois_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_whois_load(void)
{
  if(msg_register(&m_whois_msg) == NULL)
    return -1;

  return 0;
}

void m_whois_unload(void)
{
  msg_unregister(&m_whois_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'whois'                                                          *
 * -------------------------------------------------------------------------- */
static void m_whois(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  struct client *acptr;

  if(lclient_is_server(lcptr))
  {
    if(lcptr->caps & CAP_UID)
      acptr = client_find_uid(argv[2]);
    else
      acptr = client_find_nick(argv[2]);

    if(acptr == NULL)
    {
      log(client_log, L_warning, "Dropping WHOIS for unknown user %s.",
          argv[2]);
      return;
    }
  }
  else
  {
    if((acptr = client_find_nickhw(cptr, argv[2])) == NULL)
      return;
  }

  if(client_is_local(acptr))
  {
    user_whois(cptr, acptr->user);
  }
  else
  {
    if(acptr->source->caps & CAP_UID)
      lclient_send(acptr->source, ":%s WHOIS %s",
                   cptr->user->uid, acptr->user->uid);
    else
      lclient_send(acptr->source, ":%s WHOIS %s",
                   cptr->name, acptr->name);
  }
}
