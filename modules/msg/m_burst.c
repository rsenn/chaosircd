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
 * $Id: m_burst.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/timer.h"
#include "libchaos/log.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/msg.h"
#include "ircd/client.h"
#include "ircd/lclient.h"
#include "ircd/server.h"
#include "ircd/channel.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void ms_burst(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *ms_burst_help[] =
{ "BURST <sendq> <servers> <clients> <channels> <chanmodes>",
  "",
  "Informs a server about the start and end of netjoin bursts",
  "and tells it how big the send queue is and how many servers,",
  "clients, channels and channel modes we're gonna send.",
  NULL 
};

static struct msg ms_burst_msg = {
  "BURST", 0, 0, MFLG_SERVER,
  { NULL, NULL, ms_burst, NULL },
  ms_burst_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_burst_load(void)
{
  if(msg_register(&ms_burst_msg) == NULL)
    return -1;
  
  return 0;
}

void m_burst_unload(void)
{
  msg_unregister(&ms_burst_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'burst'                                                          *
 * argv[2] - [sendq]     (if EOB)                                             *
 * argv[3] - [servers]   (if EOB)                                             *
 * argv[4] - [clients]   (if EOB)                                             *
 * argv[5] - [channels]  (if EOB)                                             *
 * argv[6] - [chanmodes] (if EOB)                                             *
 * -------------------------------------------------------------------------- */
static void ms_burst(struct lclient *lcptr, struct client *cptr, 
                     int             argc,  char         **argv)
{
  size_t burst_servers;
  size_t burst_clients;
  size_t burst_channels;
  size_t burst_chanmodes;
  
  if(!client_is_server(cptr))
    return;
  
  if(argc == 2)
  {
    if(client_is_local(cptr))
      log(server_log, L_status, "Start of burst from %N", cptr);
    else
      log(server_log, L_status, "Start of burst from %N via %s",
          cptr, lcptr->name);
    
    cptr->server->bstart = timer_mtime;
    cptr->server->cserial = channel_serial + 666; /* SATAN INSIDE */
    cptr->server->in.servers = 0;
    cptr->server->in.clients = 0;
    cptr->server->in.channels = 0;
    cptr->server->in.chanmodes = 0;
    
    server_send(lcptr, NULL, CAP_NONE, CAP_NONE,
                ":%N BURST", cptr);
  }
  else
  {
    uint64_t burst_time = (uint64_t)(timer_mtime - cptr->server->bstart);
    uint32_t burst_sendq = (uint32_t)str_toul(argv[2], NULL, 10);

    if(burst_time == 0ull)
      burst_time = 1ull;
    
    burst_servers = (uint32_t)str_toul(argv[3], NULL, 10);
    burst_clients = (uint32_t)str_toul(argv[4], NULL, 10);
    burst_channels = (uint32_t)str_toul(argv[5], NULL, 10);
    burst_chanmodes = (uint32_t)str_toul(argv[6], NULL, 10);

    if(cptr->server->in.servers != burst_servers)
      log(server_log, L_warning, 
          "Server count doesn't match in burst from %N (r:%u/l:%u).",
          cptr, burst_servers, cptr->server->in.servers);
    if(cptr->server->in.clients != burst_clients)
      log(server_log, L_warning, 
          "Client count doesn't match in burst from %N (r:%u/l:%u).",
          cptr, burst_clients, cptr->server->in.clients);
    if(cptr->server->in.channels != burst_channels)
      log(server_log, L_warning, 
          "Channel count doesn't match in burst from %N (r:%u/l:%u).",
          cptr, burst_channels, cptr->server->in.channels);
    if(cptr->server->in.chanmodes != burst_chanmodes)
      log(server_log, L_warning, 
          "Chanmode count doesn't match in burst from %N (r:%u/l:%u).",
          cptr, burst_chanmodes, cptr->server->in.chanmodes);
    
    if(client_is_local(cptr))
    {
      float    burst_rate  = (float)((burst_sendq * 1000) / burst_time) / 1024;
      uint32_t burst_kb = (uint32_t)(burst_rate);
      uint32_t burst_db;
      
      burst_db = (uint32_t)(((burst_rate - (float)burst_kb) * 100) + 0.5);
      burst_kb = (uint32_t)(burst_rate + 0.5);
      
      log(server_log, L_status, 
          "Burst from %N done in %llu msecs (%u.%02ukb/s)",
          cptr, burst_time, burst_kb, burst_db);

      log(server_log, L_status, 
          "Synched %u servers, %u clients, %u channels, %u channelmodes from %N.",
          cptr->server->in.servers,
          cptr->server->in.clients,
          cptr->server->in.channels,
          cptr->server->in.chanmodes,
          cptr);
    }
    else
    {
      log(server_log, L_status, 
          "Burst from %N via %s done in %llu msecs",
          cptr, lcptr->name, burst_time);
      
      log(server_log, L_status, 
          "Synched %u servers, %u clients, %u channels, %u channelmodes from %N via %s.",
          cptr->server->in.servers,
          cptr->server->in.clients,
          cptr->server->in.channels,
          cptr->server->in.chanmodes,
          cptr, lcptr->name);
    }
    
    server_send(lcptr, NULL, CAP_NONE, CAP_NONE,
                ":%N BURST %u %u %u %u %u",
                cptr,
                burst_sendq,
                cptr->server->in.servers,
                cptr->server->in.clients,
                cptr->server->in.channels,
                cptr->server->in.chanmodes);                
  }  
}
