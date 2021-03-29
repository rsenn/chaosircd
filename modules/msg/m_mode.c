/* chaosircd - Chaoz's IRC daemon daemon
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
 * $Id: m_mode.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/log.h"
#include "libchaos/timer.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/usermode.h"
#include "ircd/channel.h"
#include "ircd/chanmode.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_mode         (struct lclient *lcptr, struct client *cptr,
                            int             argc,  char         **argv);
static void m_mode_user    (struct lclient *lcptr, struct client *cptr,
                            int             argc,  char         **argv);
static void m_mode_channel (struct lclient *lcptr, struct client *cptr,
                            struct channel *chptr, int            argc,
                            char          **argv);
static void ms_mode        (struct lclient *lcptr, struct client *cptr,
                            int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_mode_help[] = {
  "MODE <nick|channel> <flags> [args]",
  "",
  "Changes the mode of the given user or channel. For",
  "a detailed description of the user modes and the",
  "channelmodes use the following commands:",
  "/quote help usermodes",
  "/quote help chanmodes",
  NULL
};

static struct msg m_mode_msg = {
  "MODE", 1, 3, MFLG_CLIENT,
  { NULL, m_mode, ms_mode, m_mode },
  m_mode_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_mode_load(void)
{
  if(msg_register(&m_mode_msg) == NULL)
    return -1;

  return 0;
}

void m_mode_unload(void)
{
  msg_unregister(&m_mode_msg);
}

/* -------------------------------------------------------------------------- *
 * Call the umode message handler                                             *
 * -------------------------------------------------------------------------- */
static void m_mode_user (struct lclient *lcptr, struct client *cptr,
                         int argc,              char **argv)
{
  struct msg *msg;
  char **p;

  if((msg = msg_find("UMODE")) == NULL)
  {
    log(usermode_log, L_debug, "/umode ignored, module not loaded");
    return;
  }

  /* remove the username argument to pass to umode handler */
  for(p = argv + 2; *p != NULL; p++)
    *p = *(p + 1);

  if(msg->handlers[lcptr->type] != NULL)
    (msg->handlers[lcptr->type])(lcptr, cptr, argc - 1, argv);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_mode_channel (struct lclient *lcptr, struct client *cptr,
                            struct channel *chptr, int            argc,
                            char          **argv)
{
  struct chanuser      *cuptr = NULL;
  struct list           list;

  if(argc < 4)
  {
    chanmode_show(cptr, chptr);
    return;
  }

  if(client_is_user(cptr))
  {
    cuptr = chanuser_find(chptr, cptr);

    if(cuptr == NULL)
    {
      client_send(cptr, numeric_format(ERR_NOTONCHANNEL),
                  client_me->name, cptr->name, chptr->name);
      return;
    }
  }

  chanmode_parse(lcptr, cptr, chptr, cuptr, &list, argv[3], argv[4],
                 client_is_user(cptr) ? IRCD_MODESPERLINE : CHANMODE_PER_LINE);

  chanmode_apply(lcptr, cptr, chptr, cuptr, &list);

  chanmode_send_local(cptr, chptr, list.head,
                      client_is_user(cptr) ? IRCD_MODESPERLINE : CHANMODE_PER_LINE);
  chanmode_send_remote(lcptr, cptr, chptr, list.head);

  chanmode_change_destroy(&list);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'mode'                                                           *
 * argv[2] - nick/channel                                                     *
 * argv[3] - [modebuf]                                                        *
 * argv[4] - [argbuf]                                                         *
 * -------------------------------------------------------------------------- */
static void m_mode(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  if(channel_is_valid(argv[2]))
  {
    struct channel *chptr;

    chptr = channel_find_name(argv[2]);

    if(chptr == NULL)
    {
      client_send(cptr, numeric_format(ERR_NOSUCHCHANNEL),
                  client_me->name, cptr->name, argv[2]);
      return;
    }

    m_mode_channel(lcptr, cptr, chptr, argc, argv);
  }
  else
  {
    m_mode_user(lcptr, cptr, argc, argv);
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'mode'                                                           *
 * argv[2] - nick/channel                                                     *
 * argv[3] - [modebuf]                                                        *
 * argv[3] - [argbuf]                                                         *
 * -------------------------------------------------------------------------- */
static void ms_mode(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  if(channel_is_valid(argv[2]))
  {
    struct channel *chptr;

    chptr = channel_find_name(argv[2]);

    if(chptr == NULL)
    {
      log(chanmode_log, L_warning,
          "Dropping MODE from %s for unknown channel %s.",
          cptr->name, argv[2]);
      return;
    }

    m_mode_channel(lcptr, cptr, chptr, argc, argv);
  }
#ifdef DEBUG
  else
  {
    log(usermode_log, L_warning, "server %s MODE for instead of UMODE",
        cptr->name);
  }
#endif
}
