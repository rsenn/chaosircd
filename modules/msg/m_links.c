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
 * $Id: m_links.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/dlink.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/msg.h>
#include <ircd/user.h>
#include <ircd/client.h>
#include <ircd/server.h>
#include <ircd/numeric.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_links  (struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_links_help[] = {
  "LINKS [server]",
  "",
  "Displays a list of all servers in the network.",
  "This list also shows which server is connected to which,",
  "and also the server description field.",
  "If a server is specified then the list is displayed from",
  "the view of this server",
  NULL
};

static struct msg m_links_msg = {
  "LINKS", 0, 1, MFLG_CLIENT,
  { NULL, m_links, m_links, m_links },
  m_links_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_links_load(void)
{
  if(msg_register(&m_links_msg) == NULL)
    return -1;

  return 0;
}

void m_links_unload(void)
{
  msg_unregister(&m_links_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'links'                                                          *
 * argv[2] - server                                                           *
 * -------------------------------------------------------------------------- */
static void m_links(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  if(argv[2])
  {
    if(server_relay_always(lcptr, cptr, 2, ":%C LINKS %s", &argc, argv))
      return;
  }

  server_links(cptr, server_me);
}
