/* chaosircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: lc_clones.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/io.h"
#include "libchaos/timer.h"
#include "libchaos/hook.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/lclient.h"
#include "ircd/client.h"
#include "ircd/class.h"
#include "ircd/msg.h"

#define LC_CLONES_MAX_HOST_LOCAL  4
#define LC_CLONES_MAX_HOST_REMOTE 8
#define LC_CLONES_MAX_NET_LOCAL   8
#define LC_CLONES_MAX_NET_REMOTE  16

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static int               lc_clones_hook        (struct lclient   *lcptr);

static int               lc_clones_check_local (struct lclient   *lcptr);
static int               lc_clones_check_remote(struct client    *cptr);

/* -------------------------------------------------------------------------- *
 * Local variables                                                            *
 * -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int lc_clones_load(void)
{
  if(hook_register(lclient_register, HOOK_2ND, lc_clones_hook) == NULL)
    return -1;

  return 0;
}

void lc_clones_unload(void)
{
  hook_unregister(lclient_register, HOOK_2ND, lc_clones_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static uint32_t lc_clones_count_local (uint32_t addr, uint32_t mask)
{
  struct lclient *alcptr = NULL;
  struct node    *nptr;
  uint32_t        n = 0;

  dlink_foreach_data(&lclient_lists[LCLIENT_USER], nptr, alcptr)
  {
    if((alcptr->addr_remote & mask) == (addr & mask))
      n++;
  }

  return n;
}

static uint32_t lc_clones_count_remote (uint32_t addr, uint32_t mask)
{
  struct client *acptr = NULL;
  struct node   *nptr;
  uint32_t       n = 0;

  dlink_foreach_data(&client_lists[CLIENT_GLOBAL][CLIENT_USER], nptr, acptr)
  {
    if((acptr->ip & mask) == (addr & mask))
      n++;
  }

  return n;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int lc_clones_check_local (struct lclient *lcptr)
{
  uint32_t       host;
  uint32_t       net;
  struct msg    *mptr;
  uint32_t       addr;

  host = lc_clones_count_local(lcptr->addr_remote, NET_ADDR_BROADCAST);
  net = lc_clones_count_local(lcptr->addr_remote, NET_CLASSC_NET);

  mptr = msg_find("KLINE");

  if(net > LC_CLONES_MAX_NET_LOCAL)
  {
    addr = lcptr->addr_remote & NET_CLASSC_NET;

    log(lclient_log, L_warning, "Local clone alert: %u clones from %s/24",
        net, net_ntoa(addr));

    if(mptr)
    {
      char mask[32];
      char *argv[] = { client_me->name, "KLINE", mask, "clones", NULL };

      str_snprintf(mask, sizeof(mask), "*@%s/24", net_ntoa(addr));

      mptr->handlers[MSG_OPER](lclient_me, client_me, 4, argv);
    }

    return 1;
  }
  else if(host > LC_CLONES_MAX_HOST_LOCAL)
  {
    addr = lcptr->addr_remote;

    log(lclient_log, L_warning, "Local clone alert: %u clones from %s",
        net, net_ntoa(addr));

    if(mptr)
    {
      char mask[32];
      char *argv[] = { client_me->name, "KLINE", mask, "clones", NULL };

      str_snprintf(mask, sizeof(mask), "*@%s", net_ntoa(addr));

      mptr->handlers[MSG_OPER](lclient_me, client_me, 4, argv);
    }

    return 1;
  }

  return 0;
}

static int lc_clones_check_remote(struct client *cptr)
{
  uint32_t    host;
  uint32_t    net;
  struct msg *mptr;
  uint32_t    addr;

  host = lc_clones_count_remote(cptr->ip, NET_ADDR_BROADCAST);
  net = lc_clones_count_remote(cptr->ip, NET_CLASSC_NET);

  mptr = msg_find("GLINE");

  if(net > LC_CLONES_MAX_NET_REMOTE)
  {
    addr = cptr->ip & NET_CLASSC_NET;

    log(lclient_log, L_warning, "Remote clone alert: %u clones from %s/24",
        net, net_ntoa(addr));

    if(mptr)
    {
      char mask[32];
      char *argv[] = { client_me->name, "KLINE", mask, "clones", NULL };

      str_snprintf(mask, sizeof(mask), "*@%s/24", net_ntoa(addr));

      mptr->handlers[MSG_OPER](lclient_me, client_me, 4, argv);
    }

    return 1;
  }
  else if(host > LC_CLONES_MAX_HOST_REMOTE)
  {
    addr = cptr->ip;

    log(lclient_log, L_warning, "Remote clone alert: %u clones from %s",
        net, net_ntoa(addr));

    if(mptr)
    {
      char mask[32];
      char *argv[] = { client_me->name, "KLINE", mask, "clones", NULL };

      str_snprintf(mask, sizeof(mask), "*@%s", net_ntoa(addr));

      mptr->handlers[MSG_OPER](lclient_me, client_me, 4, argv);
    }

    return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int lc_clones_hook(struct lclient *lcptr)
{
  if(lclient_is_user(lcptr))
  {
    if(!lc_clones_check_remote(lcptr->client))
      lc_clones_check_local(lcptr);
  }

  return 0;
}
