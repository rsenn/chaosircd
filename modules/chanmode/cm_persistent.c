/* 
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

  return 0;
}

void cm_persistent_unload(void)
{
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

