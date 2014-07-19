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
 * $Id: m_error.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/log.h>
#include <libchaos/dlink.h>
#include <libchaos/connect.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/msg.h>
#include <chaosircd/client.h>
#include <chaosircd/lclient.h>
#include <chaosircd/server.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_error (struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_error_help[] = {
  "ERROR <message>",
  "",
  "Servers generate this command to warn about errors.",
  NULL
};

static struct msg m_error_msg = {
  "ERROR", 1, 1, MFLG_UNREG,
  { m_error, m_ignore, m_error, m_ignore },
  m_error_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_error_load(void)
{
  if(msg_register(&m_error_msg) == NULL)
    return -1;

  return 0;
}

void m_error_unload(void)
{
  msg_unregister(&m_error_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'error'                                                          *
 * argv[2] - message                                                          *
 * -------------------------------------------------------------------------- */
static void m_error(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  if(lclient_is_unknown(lcptr))
  {
    if(lcptr->connect)
    {
      log(server_log, L_warning, "Connection to %s dropped: %s",
          lcptr->connect->name, argv[2]);

      connect_cancel(lcptr->connect);
    }

    lclient_exit(lcptr, "%s", argv[2]);
  }
  else
  {
    client_exit(lcptr, cptr, "%s", argv[2]);
  }
}
