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
 * $Id: m_nclock.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/*
 * - server a is connecting to server b
 * - server a sends pass/capab/server
 * - server b receives pass/capab/server and sends it itself
 * - server a sends nclock stage 1
 *
 * NCLOCK <mtime/l>
 * NCLOCK <mtime/l
 *
 *
 *
 *
 *
 *
 *
 *
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/io.h"
#include "libchaos/defs.h"
#include "libchaos/timer.h"
#include "libchaos/hook.h"
#include "libchaos/log.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/msg.h"
#include "ircd/server.h"
#include "ircd/client.h"
#include "ircd/lclient.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void ms_nclock      (struct lclient *lcptr, struct client *cptr,
                            int             argc,  char         **argv);
static int  ms_nclock_hook (struct lclient *lcptr, struct class  *clptr);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *ms_nclock_help[] = {
  "NCLOCK",
  "",
  "Synchronizes the clock between two servers.",
  NULL
};

static struct msg ms_nclock_msg = {
  "NCLOCK", 0, 3, MFLG_SERVER,
  { NULL, NULL, ms_nclock, NULL },
  ms_nclock_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_nclock_load(void)
{
  if(msg_register(&ms_nclock_msg) == NULL)
    return -1;

  hook_register(server_login, HOOK_DEFAULT, ms_nclock_hook);

  server_default_caps |= CAP_CLK;

  return 0;
}

void m_nclock_unload(void)
{
  server_default_caps &= ~CAP_CLK;

  msg_unregister(&ms_nclock_msg);

  hook_unregister(server_login, HOOK_DEFAULT, ms_nclock_hook);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'NCLOCK'                                                         *
 * -------------------------------------------------------------------------- */
static void ms_nclock(struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv)
{
  uint64_t clk;

  if(argc == 2)
  {
    server_register(lcptr, lcptr->class);
    return;
  }

  clk = str_toull(argv[2], NULL, 10);

  if(argc == 3)
  {
    log(server_log, L_status, "Time synchronisation request from %s.", lcptr->name);
    
    lclient_send(lcptr, "NCLOCK %llu %lld", clk, clk - timer_mtime - timer_offset);
    
    log(server_log, L_status, "Replying time delta %lld.", clk - timer_mtime - timer_offset);
  }
  else if(argc == 4)
  {
    lcptr->lag = timer_mtime + timer_offset - clk;
    
    lclient_send(lcptr, "NCLOCK %llu %lld :%llu",
                 timer_mtime + timer_offset, lcptr->lag >> 1, timer_mtime + timer_offset + (lcptr->lag >> 1));
  }
  else if(argc == 5)
  {
    uint64_t tm;
    int64_t  delta;

    tm = str_toull(argv[4], NULL, 10);

    delta = tm - timer_mtime - timer_offset;

    timer_offset += delta;
    timer_mtime += delta;
    timer_otime += delta;
    timer_shift(delta);
    timer_offset += delta;
/*    timer_mtime += timer_offset;*/

    log(server_log, L_status, "Timer delta: %lld New offset: %lld",
        delta, timer_offset);

    if(delta > 500LL || delta < -500LL)
    {
      lclient_send(lcptr, "NCLOCK %llu %lld", clk, delta);
    }
    else
    {
      if(lcptr->listen)
      {
        lclient_send(lcptr, "NCLOCK :%llu", timer_mtime + timer_offset);
      }
      else
      {
        lclient_send(lcptr, "NCLOCK");
        server_register(lcptr, lcptr->class);
      }
    }
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int ms_nclock_hook(struct lclient *lcptr, struct class  *clptr)
{
  if(lcptr->caps & CAP_CLK)
  {
    if(lcptr->listen == NULL)
    {
      lcptr->class = clptr;
      
      lclient_send(lcptr, "NCLOCK :%llu", timer_mtime + timer_offset);
    }

    return 1;
  }

  return 0;
}
