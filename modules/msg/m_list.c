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
 * $Id: m_list.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/dlink.h>
#include <libchaos/hook.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/msg.h>
#include <ircd/user.h>
#include <ircd/client.h>
#include <ircd/server.h>
#include <ircd/channel.h>
#include <ircd/chanuser.h>
#include <ircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_list      (struct lclient *lcptr, struct client  *cptr,
                         int             argc,  char          **argv);

static int  m_list_hook (struct client  *cptr,  struct channel *chptr);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_list_help[] = {
  "LIST",
  "",
  "Displays a list of channels, the number of users in them",
  "and also their topic.",
  NULL
};

static struct msg m_list_msg = {
  "LIST", 0, 0, MFLG_CLIENT,
  { NULL, m_list, m_ignore, m_list },
  m_list_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_list_load(void)
{
  if(msg_register(&m_list_msg) == NULL)
    return -1;

  hook_register(channel_show, HOOK_DEFAULT, m_list_hook);

  return 0;
}

void m_list_unload(void)
{
  hook_unregister(channel_show, HOOK_DEFAULT, m_list_hook);

  msg_unregister(&m_list_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'list'                                                           *
 * -------------------------------------------------------------------------- */
static void m_list(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  channel_show(cptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int m_list_hook(struct client *cptr, struct channel *chptr)
{
  if(chptr->modes & CHFLG(s))
  {
    if(!channel_is_member(chptr, cptr))
      return 1;
  }

  return 0;
}

