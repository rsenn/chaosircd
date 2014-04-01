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
 * $Id: m_opart.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
#include <chaosircd/user.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/channel.h>
#include <chaosircd/numeric.h>
#include <chaosircd/chanmode.h>
#include <chaosircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_opart (struct lclient *lcptr, struct client *cptr, 
                     int             argc,  char         **argv);
static void ms_opart(struct lclient *lcptr, struct client *cptr, 
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_opart_help[] = {
  "OPART <channel> [text]",
  "",
  "This command causes you to leave the given channel,",
  "but only if you're the owner (you have +OHV modes).",
  "The text can be shown to the channel when parting.",
  NULL  
};

static struct msg m_opart_msg = {
  "OPART", 1, 2, MFLG_CLIENT, 
  { m_unregistered, m_opart, ms_opart, m_opart },
  m_opart_help
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define channel_owner_flags ((cuptr->flags & CHFLG(o)) | \
                             (cuptr->flags & CHFLG(h)) | \
                             (cuptr->flags & CHFLG(v)))

#define is_channel_owner(cuptr) \
  ((cuptr->flags & channel_owner_flags) == channel_owner_flags)

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_opart_load(void)
{
  if(msg_register(&m_opart_msg) == NULL)
    return -1;
  
  return 0;
}

void m_opart_unload(void)
{
  msg_unregister(&m_opart_msg);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void m_opart_server_send(struct lclient *lcptr, struct chanuser *cuptr,
                         const char     *reason)
{
  if(reason)
  {
    server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                ":%s OPART %s :%s",
                cuptr->client->user->uid,
                cuptr->channel->name,
                reason);
    server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                ":%s OPART %s :%s",
                cuptr->client->name,
                cuptr->channel->name,
                reason);
  }
  else
  {
    server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                ":%s OPART %s",
                cuptr->client->user->uid,
                cuptr->channel->name);
    server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                ":%s OPART %s",
                cuptr->client->name,
                cuptr->channel->name);
  }  
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_opart_send(struct lclient *lcptr, struct client *cptr,
                         struct channel *chptr, const char    *reason);

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'part'                                                           *
 * argv[2] - channel                                                          *
 * argv[3] - reason                                                           *
 * -------------------------------------------------------------------------- */
static void m_opart(struct lclient *lcptr, struct client *cptr, 
                    int             argc,  char         **argv)
{
  char             reason[IRCD_TOPICLEN + 1];
  struct channel  *chptr;
  struct chanuser *cuptr;
  
  reason[0] = '\0';
  
  if(argc > 3)
    strlcpy(reason, argv[3], IRCD_TOPICLEN + 1);
  
  chptr = channel_find_name(argv[2]);
  
  if(chptr == NULL)
  {
    client_send(cptr, numeric_format(ERR_NOSUCHCHANNEL),
                client_me->name, cptr->name, argv[2]);
    return;
  }
  
  cuptr = chanuser_find(chptr, cptr);
  
  if(cuptr == NULL)
  {
    client_send(cptr, numeric_format(ERR_NOTONCHANNEL),
                client_me->name, cptr->name, argv[2]);
    return;
  }

  if(!is_channel_owner(cuptr))
  {
    numeric_send(cptr, ERR_CHANOPRIVSNEEDED, chptr->name);                                                                        
    return;
  }

  m_opart_send(NULL, cptr, chptr, reason);
  
  /* send server OPART */
  m_opart_server_send(NULL, cuptr, argv[3]);
  
  chanuser_delete(cuptr);
  
//  if(chptr->chanusers.size == 0)
    channel_delete(chptr);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'OPART'                                                          *
 * argv[2] - channel                                                          *
 * argv[3] - reason                                                           *
 * -------------------------------------------------------------------------- */
static void ms_opart(struct lclient *lcptr, struct client *cptr, 
                     int             argc,  char         **argv)
{
  struct channel  *chptr;
  struct chanuser *cuptr;
  
  if((chptr = channel_find_name(argv[2])) == NULL)
  {
    log(channel_log, L_warning, "Dropping OPART for invalid channel %s.",
        argv[2]);
    return;
  }
  
  if((cuptr = chanuser_find(chptr, cptr)) == NULL)
  {
    log(channel_log, L_warning, "Dropping OPART because user %s is not on %s.",
        cptr->name, chptr->name);
    return;
  }

  m_opart_send(lcptr, cptr, chptr, argv[3]);
  
  /* send server OPART */
  m_opart_server_send(lcptr, cuptr, argv[3]);

  chanuser_delete(cuptr);
  
 // if(chptr->chanusers.size == 0)
    channel_delete(chptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_opart_send(struct lclient *lcptr, struct client *optr,
                         struct channel *chptr, const char    *reason)
{
  struct chanuser *cuptr;
  struct node     *node;

  dlink_foreach_data(&chptr->lchanusers, node, cuptr)
  {
    struct client *acptr = cuptr->client;

    if(acptr->lclient && lcptr == acptr->lclient)
    {
      if(reason && reason[0])
        client_send(acptr, ":%N!%U@%H PART %s :%s",
                    acptr, acptr, acptr, chptr->name, reason);
      else
        client_send(acptr, ":%N!%U@%H PART %s",
                    acptr, acptr, acptr, chptr->name);
    }
    else
    {
      if(reason && reason[0])
        client_send(acptr, ":%N!%U@%H KICK %s %N :%s",
                    optr, optr, optr, chptr->name, acptr, reason);
      else
        client_send(acptr, ":%N!%U@%H KICK %s %N",
                    optr, optr, optr, chptr->name, acptr);
      
    }
  }
}
