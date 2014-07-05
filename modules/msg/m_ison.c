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
 * $Id: m_ison.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/str.h>
#include <libchaos/dlink.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/msg.h>
#include <chaosircd/user.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/lclient.h>
#include <chaosircd/numeric.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_ison (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_ison_help[] = {
  "ISON <nickname[,nickname,...]>",
  "",
  "Shows, whether the given nickname are on IRC.",
  NULL
};

static struct msg m_ison_msg = {
  "ISON", 1, 1, MFLG_CLIENT,
  { NULL, m_ison, NULL, m_ison },
  m_ison_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_ison_load(void)
{
  if(msg_register(&m_ison_msg) == NULL)
    return -1;

  return 0;
}

void m_ison_unload(void)
{
  msg_unregister(&m_ison_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'ison'                                                       *
 * argv[2] - nick1                                                            *
 * argv[3] - [nick2]                                                          *
 * argv[4] - ...                                                              *
 * -------------------------------------------------------------------------- */
static void m_ison(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  struct client *acptr;
  char          *av[64];
  char           result[IRCD_LINELEN - 1];
  size_t         len;
  size_t         n;
  size_t         i;
  int            first = 1;

  len = str_snprintf(result, sizeof(result), ":%s 303 %s :",
                 client_me->name, cptr->name);

  n = str_tokenize(argv[2], av, 63);

  for(i = 0; i < n; i++)
  {
    acptr = client_find_nick(av[i]);

    if(acptr == NULL)
      continue;

    if(len + 1 + str_len(acptr->name) > IRCD_LINELEN - 2)
      break;

    if(!first)
      result[len++] = ' ';

    len += strlcpy(&result[len], acptr->name, IRCD_LINELEN - 1 - len);

    first = 0;
  }

  client_send(cptr, "%s", result);
}
