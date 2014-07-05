/* chaosircd - pi-networks irc server
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
 * $Id: lc_mflood.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/lclient.h>
#include <chaosircd/client.h>
#include <chaosircd/class.h>
#include <chaosircd/msg.h>

/* -------------------------------------------------------------------------- *
 * Types                                                                      *
 * -------------------------------------------------------------------------- */
struct lc_mflood {
  struct node     node;
  struct lclient *lclient;
  uint32_t        lines;
  uint64_t        lastrst;
  uint64_t        lastrstm;
  struct timer   *rtimer;
  struct timer   *ttimer;
};

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static int               lc_mflood_hook    (struct lclient   *lcptr);
static void              lc_mflood_release (struct lclient   *lcptr);
static void              lc_mflood_done    (struct lc_mflood *lcmfptr);
static int               lc_mflood_parse   (struct lclient   *lcptr,
                                            char             *s);
static int               lc_mflood_rtimer  (struct lc_mflood *lcmfptr);
static int               lc_mflood_ttimer  (struct lc_mflood *lcmfptr);

/* -------------------------------------------------------------------------- *
 * Local variables                                                            *
 * -------------------------------------------------------------------------- */
static struct sheap lc_mflood_heap;
static struct list  lc_mflood_list;

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int lc_mflood_load(void)
{
  if(hook_register(lclient_register, HOOK_2ND, lc_mflood_hook) == NULL)
    return -1;

  hook_register(lclient_parse, HOOK_DEFAULT, lc_mflood_parse);
  hook_register(lclient_release, HOOK_DEFAULT, lc_mflood_release);

  mem_static_create(&lc_mflood_heap, sizeof(struct lc_mflood),
                    LCLIENT_BLOCK_SIZE / 2);
  mem_static_note(&lc_mflood_heap, "message flood heap");

  return 0;
}

void lc_mflood_unload(void)
{

  struct lc_mflood *mflood = NULL;
  struct node      *nptr;
  struct node      *next;

  dlink_foreach_safe_data(&lc_mflood_list, nptr, next, mflood)
    lc_mflood_done(mflood);

  hook_unregister(lclient_release, HOOK_DEFAULT, lc_mflood_release);
  hook_unregister(lclient_parse, HOOK_DEFAULT, lc_mflood_parse);
  hook_unregister(lclient_register, HOOK_2ND, lc_mflood_hook);

  mem_static_destroy(&lc_mflood_heap);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int lc_mflood_hook(struct lclient *lcptr)
{
  struct lc_mflood *lcmfptr;
  struct class     *clptr;

  clptr = lcptr->class;

  if(clptr == NULL)
    return 0;

  if(lclient_is_user(lcptr) && clptr->flood_interval)
  {
    lcmfptr = mem_static_alloc(&lc_mflood_heap);

    /* Add to pending mflood list */
    if(lcptr->plugdata[LCLIENT_PLUGDATA_MFLOOD])
      mem_static_free(&lc_mflood_heap, lcptr->plugdata[LCLIENT_PLUGDATA_MFLOOD]);

    lcptr->plugdata[LCLIENT_PLUGDATA_MFLOOD] = lcmfptr;
    lcmfptr->lclient = lcptr;

    lcmfptr->lastrst = timer_mtime;
    lcmfptr->rtimer = timer_start(lc_mflood_rtimer, clptr->flood_interval,
                                  lcmfptr);

    timer_note(lcmfptr->rtimer, "message flood timer for client %s from %s:%u",
               lcptr->name, net_ntoa(lcptr->addr_remote), lcptr->port_remote);

    lcmfptr->ttimer = NULL;
    lcmfptr->lines = 0;

    dlink_add_tail(&lc_mflood_list, &lcmfptr->node, lcmfptr);
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int lc_mflood_parse(struct lclient *lcptr, char *s)
{
  struct lc_mflood *lcmfptr;
  struct class     *clptr;
  uint64_t          delta;

  lcmfptr = lcptr->plugdata[LCLIENT_PLUGDATA_MFLOOD];

  if(lcmfptr)
  {
    clptr = lcptr->class;

    if(lcmfptr->lines == 0)
      lcmfptr->lastrstm = timer_mtime;

    lcmfptr->lines++;

    if(lcmfptr->ttimer == NULL && clptr->throttle_trigger &&
       clptr->throttle_interval &&
       lcmfptr->lines >= clptr->throttle_trigger)
    {
      lcptr->shut = 1;
      lcmfptr->ttimer = timer_start(lc_mflood_ttimer,
                                    clptr->throttle_interval, lcmfptr);

      timer_note(lcmfptr->rtimer, "message throttling timer for client %s from %s:%u",
                 lcptr->name, net_ntoa(lcptr->addr_remote), lcptr->port_remote);

      return 0;
    }

    if(clptr->flood_trigger && lcmfptr->lines > clptr->flood_trigger)
    {
      int fd;

      delta = timer_mtime - lcmfptr->lastrstm;

      if(!lclient_is_oper(lcptr))
      {
        fd = lcptr->fds[0];
        lclient_exit(lcptr, "excess flood: %umsgs in %I64umsecs",
                     lcmfptr->lines + io_list[lcptr->fds[0]].recvq.lines, delta);
        io_close(fd);
        return 1;
      }
    }
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void lc_mflood_done(struct lc_mflood *lcmfptr)
{
  lc_mflood_release(lcmfptr->lclient);
}

/* -------------------------------------------------------------------------- *
 * Hook when a lclient is released.                                           *
 * -------------------------------------------------------------------------- */
static void lc_mflood_release(struct lclient *lcptr)
{
  struct lc_mflood *lcmfptr;

  lcmfptr = lcptr->plugdata[LCLIENT_PLUGDATA_MFLOOD];

  if(lcmfptr)
  {
    timer_push(&lcmfptr->rtimer);
    timer_push(&lcmfptr->ttimer);

    dlink_delete(&lc_mflood_list, &lcmfptr->node);
    mem_static_free(&lc_mflood_heap, lcmfptr);
    lcptr->plugdata[LCLIENT_PLUGDATA_MFLOOD] = NULL;
    lcptr->shut = 0;
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int lc_mflood_rtimer(struct lc_mflood *lcmfptr)
{
  if(lcmfptr->ttimer == NULL)
  {
    lcmfptr->lastrst = timer_mtime;
    lcmfptr->lines = 0;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int lc_mflood_ttimer(struct lc_mflood *lcmfptr)
{
  struct lclient *lcptr = lcmfptr->lclient;
  struct class   *clptr = lcptr->class;
  uint64_t        delta;

  if(io_list[lcptr->fds[0]].recvq.lines > clptr->flood_trigger)
  {
    int fd;

    lcptr->shut = 0;

    delta = timer_mtime - lcmfptr->lastrstm;

    if(!lclient_is_oper(lcptr))
    {
      fd = lcptr->fds[0];
      lclient_exit(lcptr, "excess flood: %umsgs in %I64umsecs",
                   lcmfptr->lines + io_list[lcptr->fds[0]].recvq.lines, delta);
      io_close(fd);
      return 0;
    }
  }

  if(io_list[lcptr->fds[0]].recvq.lines == 0)
  {
    lcptr->shut = 0;
    lcmfptr->ttimer = NULL;

    return 1;
  }

  lclient_process(lcptr->fds[0], lcptr);

  return 0;
}

