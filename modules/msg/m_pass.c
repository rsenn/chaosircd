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
 * $Id: m_pass.c,v 1.3 2006/09/28 09:56:24 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/io.h>
#include <libchaos/timer.h>
#include <libchaos/log.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/msg.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/lclient.h>
#include <chaosircd/server.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mr_pass (struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mr_pass_help[] = {
  "PASS <password> [flags]",
  "",
  "Used in the beginning of a irc session, to allow you",
  "to connect to the server. Also user on server connection",
  "for link authentication.",
  NULL
};

static struct msg mr_pass_msg = {
  "PASS", 1, 2, MFLG_CLIENT | MFLG_UNREG,
  { mr_pass, m_registered, NULL, m_registered },
  mr_pass_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_pass_load(void)
{
  if(msg_register(&mr_pass_msg) == NULL)
    return -1;
  
  return 0;
}

void m_pass_unload(void)
{
  msg_unregister(&mr_pass_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'pass'                                                           *
 * argv[2] - password                                                         *
 * argv[3] - optional ts info                                                 *
 * -------------------------------------------------------------------------- */
static void mr_pass(struct lclient *lcptr, struct client *cptr, 
                    int             argc,  char         **argv)
{
  strlcpy(lcptr->pass, argv[2], sizeof(lcptr->pass));
  
  if(argc > 3)
  {
    if(lcptr->ts == 0)
    {
      if(str_tolower(argv[3][0]) == 't' && str_tolower(argv[3][1]) == 's')
        lcptr->ts = SERVER_TS;
    }
  }
}
