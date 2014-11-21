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
 * $Id: m_rehash.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/timer.h"
#include "libchaos/log.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/msg.h"
#include "ircd/conf.h"
#include "ircd/user.h"
#include "ircd/server.h"
#include "ircd/client.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mo_rehash(struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mo_rehash_help[] =
{
  "REHASH [server]",
  "",
  "Operator command causing a server to rehash its config file.",
  NULL
};

static struct msg mo_rehash_msg = {
  "REHASH", 0, 1, MFLG_OPER,
  { NULL, NULL, mo_rehash, mo_rehash },
  mo_rehash_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_rehash_load(void)
{
  if(msg_register(&mo_rehash_msg) == NULL)
    return -1;

  return 0;
}

void m_rehash_unload(void)
{
  msg_unregister(&mo_rehash_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'rehash'                                                         *
 * argv[2] - name                                                             *
 * -------------------------------------------------------------------------- */
static void mo_rehash(struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv)
{
  if(argc == 3)
  {
    if(server_relay_always(lcptr, cptr, 2, ":%C REHASH %s", &argc, argv))
      return;
  }

  log(ircd_log, L_status, "%s (%s@%s) is rehashing me.",
      cptr->name, cptr->user->name, cptr->host);

  conf_rehash();
}
