/* chaosircd - pi-networks irc server
 *
 * Copyright (C) 2003  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: cm_persistent.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/hook.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/numeric.h>
#include <chaosircd/channel.h>
#include <chaosircd/chanmode.h>
#include <chaosircd/chanuser.h>

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
  hook_register(channel_join, HOOK_3RD, cm_persistent_join_hook);
  
  return 0;
}

void cm_persistent_unload(void)
{
  /* unregister the channel mode */
  chanmode_unregister(&cm_persistent_mode);
  
  hook_unregister(channel_join, HOOK_3RD, cm_persistent_join_hook);
  hook_unregister(channel_message, HOOK_DEFAULT, cm_persistent_msg_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_persistent_msg_hook(struct client   *cptr, struct channel *chptr,
                                  intptr_t         type, const char     *text)
{
  const char *cmd = (type == CHANNEL_PRIVMSG ? "PRIVMSG" : "NOTICE");

	if(chptr->server == server_me)
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
	if(chptr->server == server_me)
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
	}
	return 0;
}

