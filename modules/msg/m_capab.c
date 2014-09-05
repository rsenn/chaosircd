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
 * $Id: m_capab.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "defs.h"
#include "io.h"
#include "timer.h"
#include "log.h"
#include "net.h"
#include "str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/msg.h>
#include <ircd/chars.h>
#include <ircd/client.h>
#include <ircd/server.h>
#include <ircd/channel.h>
#include <ircd/lclient.h>
#include <ircd/numeric.h>
#include <ircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mr_capab(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mr_capab_help[] = {
  "CAPAB <capabilities>",
  "",
  "Informs the remote server about this servers capabilities.",
  NULL
};

static struct msg mr_capab_msg = {
  "CAPAB", 1, 1, MFLG_CLIENT | MFLG_UNREG,
  { mr_capab, m_ignore, m_ignore, m_ignore },
  mr_capab_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_capab_load(void)
{
  if(msg_register(&mr_capab_msg) == NULL)
    return -1;

  return 0;
}

void m_capab_unload(void)
{
  msg_unregister(&mr_capab_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'capab'                                                          *
 * argv[2] - capabilities                                                     *
 * -------------------------------------------------------------------------- */
static void mr_capab(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  char  *capv[64];
  int    capc;
  int    capi;
  size_t i;

  /* Ooops, already got caps? */
  if(lcptr->caps)
  {
    lclient_exit(lcptr, "CAPAB received twice");
    return;
  }

  /* Got caps :D */
  lcptr->caps |= CAP_CAP;

  log(server_log, L_status, "Received capabilities from %s [%s]",
      (lcptr->name[0] ? lcptr->name : lcptr->host), argv[2]);

  /* Parse capabilities */
  capc = str_tokenize(argv[2], capv, 63);

  /* Loop through tokens */
  for(i = 0; i < capc; i++)
  {
    /* Encryption capability needs special care */
    if(!str_nicmp(capv[i], "enc:", 4))
    {
      /* TODO */
    }
    else
    {
      /* Find the capab */
      capi = server_find_capab(capv[i]);

      /* Set the flag */
      if(capi >= 0)
        lcptr->caps |= server_caps[capi].cap;
    }
  }
}
