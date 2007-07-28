/* chaosircd - pi-networks irc server
 *
 * Copyright (C) 2004  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: st_traffic.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/io.h>
#include <libchaos/timer.h>
#include <libchaos/hook.h>
#include <libchaos/log.h>
#include <libchaos/mem.h>
#include <libchaos/str.h>
#include <libchaos/graph.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/lclient.h>
#include <chaosircd/client.h>
#include <chaosircd/class.h>
#include <chaosircd/msg.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- *
 * Local variables                                                            *
 * -------------------------------------------------------------------------- */
static struct graph *st_traffic_client;
static struct graph *st_traffic_server;

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static struct graph *st_traffic_setup(const char    *name, unsigned long *out, 
                                      unsigned long *in)
{
  struct graph *graph = NULL;
  
/*  graph = graph_new(name, 400, 160, GRAPH_TYPE_OPS);
  
  graph_colorize(graph, GRAPH_COLOR_DARK);
  
  graph_source_add(graph, GRAPH_MEASURE_DIFFTIME, GRAPH_SOURCE_ULONG, out, "out");
  graph_source_add(graph, GRAPH_MEASURE_DIFFTIME, GRAPH_SOURCE_ULONG, in, "in");
  
  graph_drain_add(graph, GRAPH_DATA_HOURLY);*/
/*  graph_drain_add(graph, GRAPH_DATA_DAILY);
  graph_drain_add(graph, GRAPH_DATA_WEEKLY);
  graph_drain_add(graph, GRAPH_DATA_MONTHLY);
  graph_drain_add(graph, GRAPH_DATA_YEARLY);*/
  
/*  graph_drain_render(graph, GRAPH_DATA_HOURLY);
  graph_drain_save(graph, GRAPH_DATA_HOURLY);*/
  
  return graph;
}
                               
/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int st_traffic_load(void)
{
  st_traffic_client = st_traffic_setup("client", 
                                       &lclient_sendb[CLIENT_USER],
                                       &lclient_recvb[CLIENT_USER]);
  st_traffic_server = st_traffic_setup("server", 
                                       &lclient_sendb[CLIENT_SERVER],
                                       &lclient_recvb[CLIENT_SERVER]);
  return 0;
}

void st_traffic_unload(void)
{
  graph_delete(st_traffic_client);
  graph_delete(st_traffic_server);
}

