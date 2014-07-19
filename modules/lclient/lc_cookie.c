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
 * $Id: lc_cookie.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/io.h>
#include <libchaos/dlink.h>
#include <libchaos/timer.h>
#include <libchaos/hook.h>
#include <libchaos/log.h>
#include <libchaos/mem.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/lclient.h>
#include <ircd/client.h>
#include <ircd/msg.h>

/* -------------------------------------------------------------------------- *
 * Types                                                                      *
 * -------------------------------------------------------------------------- */
struct lc_cookie {
  struct node     node;
  struct lclient *lclient;
  char            data[IRCD_COOKIELEN + 1];
};

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static uint32_t          lc_cookie_random  (void);
static void              lc_cookie_hook    (struct lclient   *lcptr);
static void              lc_cookie_release (struct lclient   *lcptr);
static void              lc_cookie_build   (struct lc_cookie *cookie);
static void              lc_cookie_done    (struct lc_cookie *cookie);

static void              mr_pong           (struct lclient *lcptr,
                                            struct client  *cptr,
                                            int             argc,
                                            char          **argv);

/* -------------------------------------------------------------------------- *
 * Local variables                                                            *
 * -------------------------------------------------------------------------- */
static uint32_t     lc_cookie_seed;
static struct sheap lc_cookie_heap;
static struct list  lc_cookie_list;
static struct msg  *lc_cookie_msg;
static char         lc_cookie_base[] = "0123456789abcdef";

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int lc_cookie_load(void)
{
  if(hook_register(lclient_login, HOOK_DEFAULT, lc_cookie_hook) == NULL)
    return -1;

  hook_register(lclient_release, HOOK_DEFAULT, lc_cookie_release);

  lc_cookie_seed = timer_mtime;

  lc_cookie_msg = msg_find("PONG");

  if(lc_cookie_msg == NULL)
  {
    log(lclient_log, L_warning, "You need to load m_pong.so before this module");

    return -1;
  }

  lc_cookie_msg->handlers[MSG_UNREGISTERED] = mr_pong;

  mem_static_create(&lc_cookie_heap, sizeof(struct lc_cookie),
                    LCLIENT_BLOCK_SIZE / 4);
  mem_static_note(&lc_cookie_heap, "cookie heap");

  return 0;
}

void lc_cookie_unload(void)
{
  struct lc_cookie *cookie = NULL;
  struct node      *nptr;
  struct node      *next;

  dlink_foreach_safe_data(&lc_cookie_list, nptr, next, cookie)
    lc_cookie_done(cookie);

  hook_unregister(lclient_release, HOOK_DEFAULT, lc_cookie_release);
  hook_unregister(lclient_login, HOOK_DEFAULT, lc_cookie_hook);

  lc_cookie_msg->handlers[MSG_UNREGISTERED] = m_unregistered;

  mem_static_destroy(&lc_cookie_heap);
}

/* -------------------------------------------------------------------------- *
 * A 32-bit PRNG for the ping cookies                                         *
 * -------------------------------------------------------------------------- */
#define ROR(v, n) ((v >> ((n) & 0x1f)) | (v << (32 - ((n) & 0x1f))))
#define ROL(v, n) ((v >> ((n) & 0x1f)) | (v << (32 - ((n) & 0x1f))))
static uint32_t lc_cookie_random(void)
{
  int      it;
  int      i;
  uint64_t ns = timer_mtime;

  it = (ns & 0x1f) + 0x20;

  for(i = 0; i < it; i++)
  {
    ns = ROL(ns, lc_cookie_seed);

    if(ns & 0x01)
      lc_cookie_seed += ns;
    else
      lc_cookie_seed -= ns;

    lc_cookie_seed = ROR(lc_cookie_seed, ns >> 6);

    if(lc_cookie_seed & 0x02LLU)
      lc_cookie_seed ^= ns;
    else
      ns ^= lc_cookie_seed;

    ns = ROL(ns, lc_cookie_seed >> 12);

    if(ns & 0x04LLU)
      lc_cookie_seed += 0xdeadbeef;
    else
      lc_cookie_seed -= 0xcafebabe;

    lc_cookie_seed = ROL(lc_cookie_seed, ns >> 16);

    if(ns & 0x08LLU)
      ns ^= lc_cookie_seed;
    else
      lc_cookie_seed ^= ns;
  }

  return ns;
}
#undef ROR
#undef ROL

/* -------------------------------------------------------------------------- *
 * Hook before user/nick is validated                                         *
 * -------------------------------------------------------------------------- */
static void lc_cookie_hook(struct lclient *lcptr)
{
  struct lc_cookie *cookie;

  cookie = mem_static_alloc(&lc_cookie_heap);

  /* Add to pending cookie list */
  if(lcptr->plugdata[LCLIENT_PLUGDATA_COOKIE])
    mem_static_free(&lc_cookie_heap, lcptr->plugdata);

  lcptr->plugdata[LCLIENT_PLUGDATA_COOKIE] = cookie;
  cookie->lclient = lcptr;

  dlink_add_tail(&lc_cookie_list, &cookie->node, cookie);

  /* Build cookie data */
  lc_cookie_build(cookie);

  /* Now send the cookie */
  lclient_send(cookie->lclient, "PING :%s", cookie->data);
}

/* -------------------------------------------------------------------------- *
 * Hook when a lclient is released.                                           *
 * -------------------------------------------------------------------------- */
static void lc_cookie_release(struct lclient *lcptr)
{
  struct lc_cookie *cookie;

  cookie = lcptr->plugdata[LCLIENT_PLUGDATA_COOKIE];

  if(cookie)
  {
    dlink_delete(&lc_cookie_list, &cookie->node);
    lcptr->plugdata[LCLIENT_PLUGDATA_COOKIE] = NULL;
    mem_static_free(&lc_cookie_heap, cookie);
    mem_static_collect(&lc_cookie_heap);
  }
}

/* -------------------------------------------------------------------------- *
 * Build ping cookie                                                          *
 * -------------------------------------------------------------------------- */
static void lc_cookie_build(struct lc_cookie *cookie)
{
  size_t   i;
  uint32_t val = 0;

  for(i = 0; i < IRCD_COOKIELEN; i++)
  {
    /* Fetch random value every 4 bytes */
    if(!(i & 0x03))
      val = lc_cookie_random();

    /* Write to cookie data */
    cookie->data[i] = lc_cookie_base[val & 0x0f];

    /* Get next nibble */
    val >>= 4;
  }

  /* Null-terminate cookie */
  cookie->data[i] = '\0';
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void lc_cookie_done(struct lc_cookie *cookie)
{
  lc_cookie_release(cookie->lclient);
  lclient_handshake(cookie->lclient);
}

/* -------------------------------------------------------------------------- *
 * Redirected PONG message handler                                            *
 * -------------------------------------------------------------------------- */
static void mr_pong (struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)

{
  struct lc_cookie *cookie;

  cookie = lcptr->plugdata[LCLIENT_PLUGDATA_COOKIE];

  if(cookie)
  {
    if(!str_cmp(cookie->data, argv[2]))
      lc_cookie_done(cookie);
  }
}
