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
 * $Id: m_invite.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/log.h>
#include <libchaos/dlink.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/lclient.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/channel.h>
#include <chaosircd/numeric.h>
#include <chaosircd/msg.h>
#include <chaosircd/service.h>
#include <chaosircd/user.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_invite(struct lclient *lcptr, struct client *cptr,  
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_invite_help[] = {
  "INVITE <nickname> <channel> [key]",
  "",
  "INVITE sends a notice to the user that you have",
  "asked him/her to come to the specified channel.",
  NULL
};

static struct msg m_invite_msg = {
  "INVITE", 2, 3, MFLG_CLIENT | MFLG_UNREG, 
  { NULL, m_invite, m_invite, m_invite },
  m_invite_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_invite_load(void)
{
  if(msg_register(&m_invite_msg) == NULL)
    return -1;
  
  return 0;
}

void m_invite_unload(void)
{
  msg_unregister(&m_invite_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'invite'                                                         *
 * argv[2] - nick                                                             *
 * argv[3] - channel                                                          *
 * -------------------------------------------------------------------------- */
static void m_invite(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  struct channel *chptr;
  struct client  *acptr;
  
  if(client_is_local(cptr) && client_is_user(cptr))
  {
    if((acptr = client_find_nickhw(cptr, argv[2])) == NULL)
      return;
  }
  else
  {
    if((acptr = client_find_uid(argv[2])) == NULL)
      if((acptr = client_find_name(argv[2])) == NULL)
      {
        log(server_log, L_warning, "Dropping INVITE for invalid user %s.",
            argv[2]);
        return;
      }
  }
  
  if((chptr = channel_find_warn(cptr, argv[3])) == NULL)
    return;
  
  if(client_is_local(cptr))
    numeric_send(cptr, RPL_INVITING, acptr->name, chptr->name);
  
  if(client_is_service(acptr))
  {
    service_handle(acptr->service, lcptr, cptr, chptr, "INVITE", "%s",
                   argv[4] ? argv[4] : "");
  }
  else
  {  
    if(client_is_remote(acptr))
    {
      if(argv[4])
        client_send(acptr, ":%C INVITE %C %s :%s",
                    cptr, acptr, chptr->name, argv[4]);
      else
        client_send(acptr, ":%C INVITE %C %s",
                    cptr, acptr, chptr->name);
    }
    else
    {
      user_invite(acptr->user, chptr);

      if(argv[4])
        client_send(acptr, ":%N!%U@%H INVITE %N %s :%s",
                    cptr, cptr, cptr, acptr, chptr->name, argv[4]);
      else
        client_send(acptr, ":%N!%U@%H INVITE %N %s",
                    cptr, cptr, cptr, acptr, chptr->name);
    }  
  }
}
