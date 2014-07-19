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
 * $Id: m_away.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/timer.h>
#include <libchaos/hook.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/numeric.h>
#include <ircd/client.h>
#include <ircd/server.h>
#include <ircd/user.h>
#include <ircd/msg.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_away      (struct lclient *lcptr, struct client *cptr,
                         int             argc,  char         **argv);
static void m_away_whois(struct client  *cptr,  struct user   *auptr);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_away_help[] = {
  "AWAY [reason]",
  "",
  "If used with a reason, this will mark you as being away.",
  "To return to the normal state, use it without reason.",
  NULL
};

static struct msg m_away_msg = {
  "AWAY", 0, 1, MFLG_CLIENT,
  { NULL, m_away, m_away, m_away },
  m_away_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_away_load(void)
{
  if(msg_register(&m_away_msg) == NULL)
    return -1;

  hook_register(user_whois, HOOK_DEFAULT, m_away_whois);

  return 0;
}

void m_away_unload(void)
{
  hook_unregister(user_whois, HOOK_DEFAULT, m_away_whois);

  msg_unregister(&m_away_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'away'                                                           *
 * argv[2] - msg                                                               *
 * -------------------------------------------------------------------------- */
static void m_away(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  if(!client_is_user(cptr))
    return;

  if(argv[2] == NULL || argv[2][0] == '\0')
  {
    if(cptr->user->away[0])
    {
      server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                  ":%s AWAY", cptr->user->uid);
      server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                  ":%s AWAY", cptr->name);
      cptr->user->away[0] = '\0';

      if(client_is_local(cptr))
        numeric_send(cptr, RPL_UNAWAY);
    }

    return;
  }

  if(!str_cmp(cptr->user->away, argv[2]))
    return;

  strlcpy(cptr->user->away, argv[2], sizeof(cptr->user->away));
  cptr->user->away_time = timer_systime;

  server_send(lcptr, NULL, CAP_UID, CAP_NONE,
              ":%s AWAY :%s", cptr->user->uid, cptr->user->away);
  server_send(lcptr, NULL, CAP_NONE, CAP_UID,
              ":%s AWAY :%s", cptr->name, cptr->user->away);

  if(client_is_local(cptr))
    numeric_send(cptr, RPL_NOWAWAY);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_away_whois(struct client *cptr, struct user *auptr)
{
  if(auptr->away[0])
    numeric_send(cptr, RPL_AWAY, auptr->client->name,
                 auptr->away);
}
