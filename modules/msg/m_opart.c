/* 
 * Copyright (C) 2013-2014  CrowdGuard organisation
 * All rights reserved.
 *
 * Author: Roman Senn <rls@crowdguard.org>
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "defs.h"
#include "io.h"
#include "timer.h"
#include "log.h"
#include "str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/user.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/channel.h"
#include "ircd/numeric.h"
#include "ircd/chanmode.h"
#include "ircd/chanuser.h"
#include "ircd/crowdguard.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_opart (struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);
static void ms_opart(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

static int m_opart_event_log;

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
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_opart_load(void)
{
  if(msg_register(&m_opart_msg) == NULL)
    return -1;
  
  m_opart_event_log = log_source_find("event");                                                                                                              
  if(m_opart_event_log == -1)                                                                                                                                
    m_opart_event_log = log_source_register("event");              
  
  return 0;
}

void m_opart_unload(void)
{
  log_source_unregister(m_opart_event_log);
  m_opart_event_log = -1;

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

  if(!channel_is_persistent(chptr))
  {
    client_send(cptr, ":%S NOTICE %N :*** Channel %s needs to be persistent (+P) for /OPART",
                server_me, cptr, chptr->name);
    return;
  }

  if((cuptr = chanuser_find(chptr, cptr)) == NULL)
  {
    client_send(cptr, numeric_format(ERR_NOTONCHANNEL),
                client_me->name, cptr->name, argv[2]);
    return;
  }

  if(!chanuser_is_owner(cuptr))
  {
    numeric_send(cptr, ERR_CHANOPRIVSNEEDED, chptr->name);
    return;
  }

  m_opart_send(lcptr, cptr, chptr, reason);

  /* send server OPART */
  m_opart_server_send(NULL, cuptr, argv[3]);


  if(chptr->server == server_me)
  {
    log(m_opart_event_log, L_status, "%s destroyed event %s", cptr->name, chptr->name);
  }
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
