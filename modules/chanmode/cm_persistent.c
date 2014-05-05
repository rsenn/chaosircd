/* 
 * Copyright (C) 2013-2014  CrowdGuard organisation
 * All rights reserved.
 *
 * Author: Roman Senn <rls@crowdguard.org>
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/hook.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "chaosircd/ircd.h"
#include "chaosircd/numeric.h"
#include "chaosircd/channel.h"
#include "chaosircd/chanmode.h"
#include "chaosircd/chanuser.h"
#include "chaosircd/crowdguard.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define CM_PERSISTENT_CHAR 'P'

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_persistent_msg_hook(struct client   *cptr, struct channel *acptr,
                                  intptr_t         type, const char     *text);

static int cm_persistent_join_hook(struct lclient *lcptr, struct client *cptr,
                                   struct channel *chptr);

static int cm_persistent_participation_log;
static int cm_persistent_event_log;

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static const char *cm_persistent_help[] =
{
  "+P              Persistent channel. The channel will not be closed when the",
  "                last user parts. Channel messages will be kept and shown to",
  "                everyone who joins the channel.",
  NULL
};

static struct chanmode cm_persistent_mode =
{
  CM_PERSISTENT_CHAR,      /* Mode character */
  '\0',                    /* No prefix, because its not a privilege */
  CHANMODE_TYPE_SINGLE,    /* Channel mode is a single flag */
  CHFLG(o),                /* Only OPs can change the flag */
  0,                       /* No order and no reply */
  chanmode_bounce_simple,  /* Bounce handler */
  cm_persistent_help       /* Help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_persistent_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_persistent_mode) == NULL)
    return -1;

  hook_register(channel_message, HOOK_DEFAULT, cm_persistent_msg_hook);
  hook_register(channel_join, HOOK_4TH, cm_persistent_join_hook);

  cm_persistent_participation_log = log_source_find("participation");
  if(cm_persistent_participation_log == -1)
    cm_persistent_participation_log = log_source_register("participation");

  cm_persistent_event_log = log_source_find("event");
  if(cm_persistent_event_log == -1)
    cm_persistent_event_log = log_source_register("event");

  return 0;
}

void cm_persistent_unload(void)
{
  log_source_unregister(cm_persistent_event_log);
  cm_persistent_event_log = -1;

  log_source_unregister(cm_persistent_participation_log);
  cm_persistent_participation_log = -1;

  /* unregister the channel mode */
  chanmode_unregister(&cm_persistent_mode);

  hook_unregister(channel_join, HOOK_4TH, cm_persistent_join_hook);
  hook_unregister(channel_message, HOOK_DEFAULT, cm_persistent_msg_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_persistent_msg_hook(struct client   *cptr, struct channel *chptr,
                                  intptr_t         type, const char     *text)
{
  const char *cmd = (type == CHANNEL_PRIVMSG ? "PRIVMSG" : "NOTICE");

	if(chptr->server == server_me && (chptr->modes & CHFLG(P)))
	{
		channel_backlog(chptr, cptr, cmd, text);
	}

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_persistent_join_hook(struct lclient *lcptr, struct client *cptr,
                                   struct channel *chptr)
{
  int owner, created;

  if(chptr->server != server_me)
    return 0;

  owner = (0 == strncmp(cptr->name, &chptr->name[1], strlen(&chptr->name[1])));
  created = (chptr->chanusers.size <= 1);
  
  if(owner)
  {
    if(created)
      log(cm_persistent_event_log, L_status, "%s created event %s", cptr->name, chptr->name);
    else
      log(cm_persistent_event_log, L_status, "%s rejoined event %s", cptr->name, chptr->name);
  }
  else if(channel_is_persistent(chptr))
    log(cm_persistent_participation_log, L_status, "%s accepted to help on %s", cptr->name, chptr->name);
  

	if(chptr->server == server_me && (chptr->modes & CHFLG(P)))
  {
		struct logentry *e;

    dlink_foreach_down(&chptr->backlog, e)
		{
    	if(e->text && e->text[0])
  	    client_send(cptr, ":%S 610 %N %s %u %s %s :%s", server_me, cptr, chptr->name,
  	                e->ts, e->from, e->cmd, e->text);
    	else
  	    client_send(cptr, ":%S 610 %N %s %u %s %s", server_me, cptr, chptr->name,
  	                e->ts, e->from, e->cmd);
		}

    client_send(cptr, ":%S 611 %N %s :End of message replay",
                server_me, cptr, chptr->name);
	}
	return 0;
}

