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
 * $Id: m_ts.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/log.h>
#include <libchaos/timer.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/msg.h>
#include <chaosircd/user.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/channel.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mo_ts(struct lclient *lcptr, struct client *cptr, 
                  int             argc,  char         **argv);
static void ms_ts(struct lclient *lcptr, struct client *cptr, 
                  int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mo_ts_help[] =
{
  "TS <channel> [value]",
  "",
  "Operator command setting the TS of a channel.",
  NULL
};

static struct msg mo_ts_msg = {
  "TS", 1, 2, MFLG_OPER,
  { NULL, NULL, ms_ts, mo_ts },
  mo_ts_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_ts_load(void)
{
  if(msg_register(&mo_ts_msg) == NULL)
    return -1;
  
  server_default_caps |= CAP_TS;
  
  return 0;
}

void m_ts_unload(void)
{
  server_default_caps &= ~CAP_TS;
  
  msg_unregister(&mo_ts_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'ts'                                                             *
 * argv[2] - name                                                             *
 * -------------------------------------------------------------------------- */
static void mo_ts(struct lclient *lcptr, struct client *cptr, 
                  int             argc,  char         **argv)
{
  struct channel *chptr;
  uint32_t        ts;
  
  if((chptr = channel_find_warn(cptr, argv[2])) == NULL)
    return;
  
  if(argv[3])
    ts = strtoul(argv[3], NULL, 10);
  else
    ts = timer_systime;
  
  log(channel_log, L_status, "%s (%s@%s) changed channel TS for %s from %u to %u.",
      cptr->name, cptr->user->name, cptr->host, chptr->name, chptr->ts, ts);

  chptr->ts = ts;

  server_send(NULL, NULL, CAP_TS, CAP_NONE,
              ":%C TS %s :%u", cptr, chptr->name, chptr->ts);
}

static void ms_ts(struct lclient *lcptr, struct client *cptr, 
                  int             argc,  char         **argv)
{
  struct channel *chptr;
  uint32_t        ts;
  
  if(argv[3] == NULL)
    return;
  
  if((chptr = channel_find_name(argv[2])) == NULL)
    return;
  
  ts = strtoul(argv[3], NULL, 10);
  
  if(client_is_server(cptr))
    log(channel_log, L_status, "%s changed channel TS for %s from %u to %u.",
        cptr->name, chptr->name, chptr->ts, ts);
  else
    log(channel_log, L_status, "%s (%s@%s) changed channel TS for %s from %u to %u.",
        cptr->name, cptr->user->name, cptr->host, chptr->name, chptr->ts, ts);

  chptr->ts = ts;

  server_send(lcptr, NULL, CAP_TS, CAP_NONE,
              ":%C TS %s :%u", cptr, chptr->name, chptr->ts);
}
