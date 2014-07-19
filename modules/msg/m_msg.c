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
 * $Id: m_msg.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/log.h>
#include <libchaos/str.h>
#include <libchaos/dlink.h>
#include <libchaos/timer.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/msg.h>
#include <ircd/user.h>
#include <ircd/chars.h>
#include <ircd/client.h>
#include <ircd/lclient.h>
#include <ircd/server.h>
#include <ircd/service.h>
#include <ircd/channel.h>
#include <ircd/numeric.h>
#include <ircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Constants                                                                  *
 * -------------------------------------------------------------------------- */
#define PRIVMSG 0
#define NOTICE  1

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_privmsg (struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv);
static void m_notice  (struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv);
static void m_multimsg(int             type,  const char    *cmd,
                       struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv);
static void m_msg     (int             type,  const char    *cmd,
                       struct lclient *lcptr, struct client *cptr,
                       const char     *rcpt,  const char    *msg);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_privmsg_help[] = {
  "PRIVMSG <target>[,target2[,target3]]... <text>",
  "",
  "Sends the given text to the given target.",
  "The target can be either a person, or a channel.",
  NULL
};

static struct msg m_privmsg_msg = {
  "PRIVMSG", 0, 2, MFLG_CLIENT,
  { NULL, m_privmsg, m_privmsg, m_privmsg },
  m_privmsg_help
};

static char *m_notice_help[] = {
  "NOTICE <target>[,target2[,target3]]... <text>",
  "",
  "Sends the given text to the given target.",
  "The target can be either a person, or a channel.",
  NULL
};

static struct msg m_notice_msg = {
  "NOTICE", 0, 2, MFLG_CLIENT,
  { NULL, m_notice, m_notice, m_notice },
  m_notice_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_msg_load(void)
{
  if(msg_register(&m_privmsg_msg) == NULL)
    return -1;

  if(msg_register(&m_notice_msg) == NULL)
  {
    msg_unregister(&m_privmsg_msg);
    return -1;
  }

  return 0;
}

void m_msg_unload(void)
{
  msg_unregister(&m_notice_msg);
  msg_unregister(&m_privmsg_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'privmsg/notice'                                                 *
 * argv[2] - recipient                                                        *
 * argv[3] - text                                                             *
 * -------------------------------------------------------------------------- */
static void m_privmsg(struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv)
{
  /* reject PRIVMSG coming from servers */
  if(!client_is_user(cptr))
    return;

  m_multimsg(PRIVMSG, "PRIVMSG", lcptr, cptr, argc, argv);
}

static void m_notice(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  m_multimsg(NOTICE, "NOTICE", lcptr, cptr, argc, argv);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_multimsg(int            type, const char *cmd,  struct lclient *lcptr,
                       struct client *cptr, int         argc, char          **argv)
{
  char  *recipients[IRCD_MAXTARGETS + 1];
  size_t n;
  int    i;

  if(argc < 3)
  {
    client_send(cptr, numeric_format(ERR_NORECIPIENT),
                client_me->name, cptr->name, cmd);
    return;
  }

  if(argc < 4)
  {
    client_send(cptr, numeric_format(ERR_NOTEXTTOSEND),
                client_me->name, cptr->name);
    return;
  }

  n = str_tokenize_s(argv[2], recipients, IRCD_MAXTARGETS, ',');

  for(i = 0; i < n; i++)
    m_msg(type, cmd, lcptr, cptr, recipients[i], argv[3]);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_msg(int            type, const char *cmd,  struct lclient *lcptr,
                  struct client *cptr, const char *rcpt, const char     *msg)
{
  struct channel *chptr;
  struct client  *acptr = NULL;

  if(channel_is_valid(rcpt) && chars_valid_chan(rcpt))
  {
    chptr = channel_find_name(rcpt);

    if(chptr)
      channel_message(lcptr, cptr, chptr, type, msg);
    else if(client_is_user(cptr))
      client_send(cptr, numeric_format(ERR_NOSUCHCHANNEL),
                  client_me->name, cptr->name, rcpt);
  }
  else
  {
    if(lclient_is_server(lcptr))
    {
      if(lcptr->caps & CAP_UID)
      {
        struct user *uptr;

        if((uptr = user_find_uid(rcpt)))
          acptr = uptr->client;
      }

      if(acptr == NULL)
      {
        acptr = client_find_nick(rcpt);

        if(acptr == NULL)
        {
          log(server_log, L_warning, "Dropping %s with unknown target: %s",
              cmd, rcpt);

          return;
        }
      }

      client_message(lcptr, cptr, acptr, type, msg);
    }
    else
    {
      if((acptr = client_find_nickhw(cptr, rcpt)))
        client_message(lcptr, cptr, acptr, type, msg);
    }
  }
}

