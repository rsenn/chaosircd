/* cgircd - CrowdGuard IRC daemon
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
 * $Id: m_quit.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "io.h"
#include "timer.h"
#include "log.h"
#include "str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/msg.h>
#include <ircd/chars.h>
#include <ircd/client.h>
#include <ircd/lclient.h>
#include <ircd/channel.h>
#include <ircd/numeric.h>
#include <ircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mr_quit (struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

static void m_quit  (struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_quit_help[] = {
  "QUIT [text]",
  "",
  "Disconnects your irc session. If a message is supplied,",
  "it will be sent to all channels you have been in.",
  NULL
};

static struct msg m_quit_msg = {
  "QUIT", 0, 1, MFLG_CLIENT | MFLG_UNREG,
  { mr_quit, m_quit, m_quit, m_quit },
  m_quit_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_quit_load(void)
{
  if(msg_register(&m_quit_msg) == NULL)
    return -1;

  return 0;
}

void m_quit_unload(void)
{
  msg_unregister(&m_quit_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'quit'                                                           *
 * argv[2] - comment                                                          *
 * -------------------------------------------------------------------------- */
static void mr_quit(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  lclient_exit(lcptr, "%s", argv[2] ? argv[2] : "client exited");
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'quit'                                                           *
 * argv[2] - comment                                                          *
 * -------------------------------------------------------------------------- */
static void m_quit(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  client_exit(lcptr, cptr, "%s", argv[2] ? argv[2] : "client exited");
}
