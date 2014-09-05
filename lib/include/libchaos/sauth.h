/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 *
 * $Id: sauth.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_SAUTH_H
#define LIB_SAUTH_H

/* ------------------------------------------------------------------------ *
 * Library headers                                                            *
 * ------------------------------------------------------------------------ */
#include "defs.h"
#include "dlink.h"
#include "timer.h"
#include "net.h"

/* ------------------------------------------------------------------------ *
 * Constants                                                                  *
 * ------------------------------------------------------------------------ */

#define SAUTH_DONE       0x00
#define SAUTH_ERROR      0x01
#define SAUTH_TIMEDOUT   0x02

#define SAUTH_TYPE_DNSF   0x00
#define SAUTH_TYPE_DNSR   0x01
#define SAUTH_TYPE_AUTH   0x02
#define SAUTH_TYPE_PROXY  0x03

#define SAUTH_TIMEOUT    20000
#define SAUTH_RELAUNCH   15000        /* 15 secs relaunch interval */

#define SAUTH_PROXY_TIMEOUT  1
#define SAUTH_PROXY_FILTERED 2
#define SAUTH_PROXY_CLOSED   3
#define SAUTH_PROXY_DENIED   4
#define SAUTH_PROXY_NA       5
#define SAUTH_PROXY_OPEN     6

#define PROXY_HTTP    0
#define PROXY_SOCKS4  1
#define PROXY_SOCKS5  2
#define PROXY_WINGATE 3
#define PROXY_CISCO   4

/* ------------------------------------------------------------------------ *
 * Types                                                                      *
 * ------------------------------------------------------------------------ */

struct sauth;
typedef void (sauth_callback_t)(struct sauth *, void *, void *, void *, void *);

/* ------------------------------------------------------------------------ *
 * Sauth block structure.                                                     *
 * ------------------------------------------------------------------------ */
typedef struct sauth {
  struct node       node;        /* linking node for sauth block list */
  uint32_t          id;
  uint32_t          refcount;

  /* externally initialised */
  int               type;
  int               status;
  char              host[HOSTLEN + 1];
  char              ident[USERLEN + 1];
  net_addr_t        addr;
  net_addr_t        connect;
  net_port_t        remote;
  net_port_t        local;
  struct timer     *timer;
  void             *args[4];
  int               reply;
  int               ptype;
  sauth_callback_t *callback;
} sauth_t;

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct sheap)  sauth_heap;
CHAOS_API(struct list)   sauth_list;
CHAOS_API(uint32_t)      sauth_serial;
CHAOS_API(struct timer *)sauth_timer;
CHAOS_API(struct timer *)sauth_rtimer;
CHAOS_API(struct child *)sauth_child;
CHAOS_API(int)           sauth_log;
CHAOS_API(int)           sauth_fds[2];
CHAOS_API(char)          sauth_readbuf[BUFSIZE];
CHAOS_API(const char *)  sauth_types[6];
CHAOS_API(const char *)  sauth_replies[8];

/* ------------------------------------------------------------------------ */
CHAOS_API(int) sauth_get_log(void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          sauth_init         (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          sauth_shutdown     (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          sauth_collect      (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct sauth *)sauth_dns_forward  (const char    *address,
                                             void          *callback,
                                             ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct sauth *)sauth_dns_reverse  (net_addr_t     address,
                                             void          *callback,
                                             ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)           sauth_launch       (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct sauth *)sauth_auth         (net_addr_t     address,
                                             net_port_t     local,
                                             net_port_t     remote,
                                             void          *callback,
                                             ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct sauth *)sauth_proxy        (int            type,
                                             net_addr_t     remote_addr,
                                             net_port_t     remote_port,
                                             net_addr_t     local_addr,
                                             net_port_t     local_port,
                                             void          *callback,
                                             ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)           sauth_proxy_reply  (const char    *reply);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)           sauth_proxy_type   (const char    *type);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          sauth_delete       (struct sauth  *sauth);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct sauth *)sauth_find         (uint32_t       id);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct sauth *)sauth_pop          (struct sauth  *sauth);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct sauth *)sauth_push         (struct sauth **sauth);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          sauth_cancel       (struct sauth  *sauth);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          sauth_vset_args    (struct sauth  *sauth,
                                             va_list        args);

CHAOS_API(void)          sauth_set_args     (sauth_t       *sauth,
                                             ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)          sauth_dump         (struct sauth  *saptr);

#endif /* LIB_SAUTH_H */
