/* chaosircd - CrowdGuard IRC daemon
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
 * $Id: m_nick.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/dlink.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/client.h"
#include "ircd/chars.h"
#include "ircd/ircd.h"
#include "ircd/msg.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mr_nick(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);
static void m_nick (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

static void ms_nick(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_nick_help[] = {
  "NICK <nickname>",
  "",
  "Changes your nick to the given nickname.",
  NULL
};

static struct msg m_nick_msg = {
  "NICK", 0, 2, MFLG_CLIENT | MFLG_UNREG,
  { mr_nick, m_nick, ms_nick, m_nick },
  m_nick_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_nick_load(void)
{
  if(msg_register(&m_nick_msg) == NULL)
    return -1;

  return 0;
}

void m_nick_unload(void)
{
  msg_unregister(&m_nick_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'nick'                                                           *
 * argv[2] - nickname                                                         *
 * -------------------------------------------------------------------------- */
static void mr_nick(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  char *s;
  char  nick[IRCD_NICKLEN + 1];

  /* Check if we have a nick */
  if(argc < 3)
  {
    lclient_exit(lcptr, "protocol mismatch: need a nickname");
    return;
  }

  /* Truncate nick */
  strlcpy(nick, argv[2], IRCD_NICKLEN + 1);

  if((s = strchr(nick, '~')))
    *s = '\0';

  if(lcptr->name[0] == '\0')
  {
    lclient_set_name(lcptr, nick);

    if(lcptr->user)
      lclient_login(lcptr);
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'nick'                                                           *
 * argv[2] - nickname                                                         *
 * -------------------------------------------------------------------------- */
static void m_nick(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  char nick[IRCD_NICKLEN];
  struct client *acptr;

  if(argc < 3)
  {
    lclient_send(lcptr, numeric_format(ERR_NONICKNAMEGIVEN),
                 client_me->name, argv[0], nick);
    return;
  }

  strlcpy(nick, argv[2], IRCD_NICKLEN);

  if(!chars_valid_nick(nick))
  {
    lclient_send(lcptr, numeric_format(ERR_ERRONEUSNICKNAME),
                 client_me->name, argv[0], nick);
    return;
  }

  acptr = client_find_nick(nick);

  if(acptr)
  {
    if(acptr == cptr)
    {
      if(str_cmp(acptr->name, nick))
      {
        client_nick(lcptr, cptr, nick);
        return;
      }
      else
      {
        return;
      }
    }

    lclient_send(lcptr, numeric_format(ERR_NICKNAMEINUSE),
                 client_me->name, cptr->name, nick);
  }
  else
  {
    client_nick(lcptr, cptr, nick);
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'nick'                                                           *
 * argv[2] - nickname                                                         *
 * -------------------------------------------------------------------------- */
static void ms_nick(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  uint32_t ts;

  if(argc < 4)
    return;

  ts = str_toul(argv[3], NULL, 10);

  client_nick(lcptr, cptr, argv[2]);

  cptr->ts = ts;
}

