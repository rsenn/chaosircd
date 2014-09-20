/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: lclient.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_LCLIENT_H
#define SRC_LCLIENT_H

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "net.h"

/* -------------------------------------------------------------------------- *
 * Constants                                                                  *
 * -------------------------------------------------------------------------- */
#define LCLIENT_UNKNOWN 0x00
#define LCLIENT_USER    0x01
#define LCLIENT_SERVER  0x02
#define LCLIENT_OPER    0x03

#define LCLIENT_PLUGDATA_SAUTH  0
#define LCLIENT_PLUGDATA_COOKIE 1
#define LCLIENT_PLUGDATA_MFLOOD 2
#define LCLIENT_PLUGDATA_CFLOOD 3
#define LCLIENT_PLUGDATA_USERDB 4

/* -------------------------------------------------------------------------- *
 * Types                                                                      *
 * -------------------------------------------------------------------------- */
struct lclient {
  struct node     node;          /* node for lclient_list */
  struct node     tnode;         /* node for typed lists */
  uint32_t        id;
  uint32_t        refcount;      /* how many times this block is referenced */
  uint32_t        type;          /* lclient type */
  hash_t          hash;
  struct user    *user;
  struct class   *class;         /* reference to connection class */
  struct client  *client;        /* reference to global client */
  struct server  *server;        /* reference to server */
  struct listen  *listen;        /* listener reference */
  struct connect *connect;       /* connect block reference */
  int             fds[2];        /* file descriptors for data connection */
  int             ctrlfds[2];    /* file descriptors for control connection */
  net_addr_t      addr_local;    /* local address */
  net_addr_t      addr_remote;   /* remote address */
  net_port_t      port_local;    /* local port */
  net_port_t      port_remote;   /* remote port */
  struct timer   *ptimer;        /* ping timer */
/*  struct timer   *atimer;         auth timer
  struct timer   *rtimer;         register timer */

  /* statistics */
  uint32_t        recvb;
  uint32_t        sendb;
  uint32_t        recvk;
  uint32_t        sendk;
  uint32_t        recvm;
  uint32_t        sendm;

  /* capabilities bit-field */
  uint64_t        caps;
  uint32_t        ts;
  uint32_t        serial;
  int64_t         lag;
  uint64_t        ping;
  int             hops;
  int             shut;          /* don't read (for flood throttling etc.) */
  int             silent;        /* don't send server broadcast (during handshake) */

  /* string fields */
  char            name   [IRCD_NICKLEN   + 1];
  char            host   [IRCD_HOSTLEN   + 1];
  char            hostip [IRCD_HOSTIPLEN + 1];
  char            pass   [IRCD_PASSWDLEN + 1];
  char            info   [IRCD_INFOLEN   + 1];

  void           *plugdata[32];
};

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
extern struct lclient *lclient_me;       /* my local client info */
extern struct lclient *lclient_uplink;   /* the uplink if we're a leaf */
extern struct sheap    lclient_heap;     /* heap for lclient_t */
extern struct timer   *lclient_timer;    /* timer for heap gc */
extern int             lclient_log;      /* lclient log source */
extern uint32_t        lclient_id;
extern uint32_t        lclient_serial;
extern uint32_t        lclient_max;
extern struct list     lclient_list;     /* list with all of them */
extern struct list     lclient_lists[4]; /* unreg, clients, servers, opers */
extern unsigned long   lclient_recvb[2];
extern unsigned long   lclient_sendb[2];
extern unsigned long   lclient_recvm[2];
extern unsigned long   lclient_sendm[2];

/* ------------------------------------------------------------------------ */
IRCD_API(int) lclient_get_log(void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define lclient_is_unknown(x) (((struct lclient *)(x))->type == LCLIENT_UNKNOWN)
#define lclient_is_user(x)    (((struct lclient *)(x))->type == LCLIENT_USER)
#define lclient_is_server(x)  (((struct lclient *)(x))->type == LCLIENT_SERVER)
#define lclient_is_oper(x)    (((struct lclient *)(x))->type == LCLIENT_OPER)

/* -------------------------------------------------------------------------- *
 * Initialize lclient module                                                  *
 * -------------------------------------------------------------------------- */
extern void            lclient_init         (void);

/* -------------------------------------------------------------------------- *
 * Shutdown lclient module                                                    *
 * -------------------------------------------------------------------------- */
extern void            lclient_shutdown     (void);

/* -------------------------------------------------------------------------- *
 * Garbage collect lclient data                                               *
 * -------------------------------------------------------------------------- */
extern void            lclient_collect      (void);

/* -------------------------------------------------------------------------- *
 * A 32-bit PRNG for the ping cookies                                         *
 * -------------------------------------------------------------------------- */
extern uint32_t        lclient_random       (void);

/* -------------------------------------------------------------------------- *
 * Create a new lclient block                                                 *
 * -------------------------------------------------------------------------- */
extern struct lclient *lclient_new          (int              fd,
                                             net_addr_t       addr,
                                             net_port_t       port);

/* -------------------------------------------------------------------------- *
 * Delete a  lclient block                                                    *
 * -------------------------------------------------------------------------- */
extern void            lclient_delete       (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Loose all references of an lclient block                                    *
 * -------------------------------------------------------------------------- */
extern void            lclient_release      (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Get a reference to an lclient block                                        *
 * -------------------------------------------------------------------------- */
extern struct lclient *lclient_pop          (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Push back a reference to an lclient block                                  *
 * -------------------------------------------------------------------------- */
extern struct lclient *lclient_push         (struct lclient **lcptrptr);

/* -------------------------------------------------------------------------- *
 * Set the type of an lclient and move it to the appropriate list             *
 * -------------------------------------------------------------------------- */
extern void            lclient_set_type     (struct lclient  *lcptr,
                                             uint32_t         type);

/* -------------------------------------------------------------------------- *
 * Set the name of an lclient block                                           *
 * -------------------------------------------------------------------------- */
extern void            lclient_set_name     (struct lclient  *lcptr,
                                             const char      *name);

/* -------------------------------------------------------------------------- *
 * Accept a local client                                                      *
 *                                                                            *
 * <fd>                     - filedescriptor of the new connection            *
 *                            (may be invalid?)                               *
 * <listen>                 - the corresponding listen{} block                *
 * -------------------------------------------------------------------------- */
extern void            lclient_accept       (int              fd,
                                             struct listen   *listen);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void            lclient_connect      (int              fd,
                                             struct connect  *connect);

/* -------------------------------------------------------------------------- *
 * Read data from a local connection and process it                           *
 * -------------------------------------------------------------------------- */
extern void            lclient_recv         (int              fd,
                                             struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Read a line from queue and process it                                      *
 * -------------------------------------------------------------------------- */
extern void            lclient_process      (int              fd,
                                             struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Parse the prefix and the command                                           *
 * -------------------------------------------------------------------------- */
extern void            lclient_parse        (struct lclient  *lcptr,
                                             char            *s,
                                             size_t           n);

/* -------------------------------------------------------------------------- *
 * Decide whether a message is numeric or not and call the appropriate        *
 * message handler.                                                           *
 * -------------------------------------------------------------------------- */
extern void            lclient_message      (struct lclient  *client,
                                             char           **argv,
                                             char            *arg,
                                             size_t           n);

/* -------------------------------------------------------------------------- *
 * Process a numeric message                                                  *
 * -------------------------------------------------------------------------- */
extern void            lclient_numeric      (struct lclient  *lcptr,
                                             char           **argv,
                                             char            *arg);

/* -------------------------------------------------------------------------- *
 * Process a command                                                          *
 * -------------------------------------------------------------------------- */
extern void            lclient_command      (struct lclient  *lcptr,
                                             char           **argv,
                                             char            *arg,
                                             size_t           n);

/* -------------------------------------------------------------------------- *
 * Parse the prefix and find the appropriate client                           *
 * -------------------------------------------------------------------------- */
extern struct client  *lclient_prefix       (struct lclient  *lcptr,
                                             const char      *pfx);


/* -------------------------------------------------------------------------- *
 * Exit a local client and leave him an error message if he has registered.   *
 * -------------------------------------------------------------------------- */
extern void            lclient_vexit        (struct lclient  *lcptr,
                                             char            *format,
                                             va_list          args);

extern int             lclient_exit         (struct lclient  *lcptr,
                                             char            *format,
                                             ...);

/* -------------------------------------------------------------------------- *
 * Update client message/byte counters                                        *
 * -------------------------------------------------------------------------- */
extern void            lclient_update_recvb (struct lclient  *lcptr,
                                             size_t           n);

extern void            lclient_update_sendb (struct lclient  *lcptr,
                                             size_t           n);

/* -------------------------------------------------------------------------- *
 * Send a line to a local client                                              *
 * -------------------------------------------------------------------------- */
extern void            lclient_vsend        (struct lclient  *lcptr,
                                             const char      *format,
                                             va_list          args);

extern void            lclient_send         (struct lclient  *lcptr,
                                             const char      *format,
                                             ...);

/* -------------------------------------------------------------------------- *
 * Send a line to a client list but one                                       *
 * -------------------------------------------------------------------------- */
extern void            lclient_vsend_list   (struct lclient  *one,
                                             struct list     *list,
                                             const char      *format,
                                             va_list          args);

extern void            lclient_send_list    (struct lclient  *one,
                                             struct list     *list,
                                             const char      *format,
                                             ...);

/* -------------------------------------------------------------------------- *
 * Check for valid USER/NICK and start handshake                              *
 * -------------------------------------------------------------------------- */
extern void            lclient_handshake    (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Register a local client to the global client pool                          *
 * -------------------------------------------------------------------------- */
extern int             lclient_register     (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Check if we got a PONG, if not exit the client otherwise send another PING *
 * -------------------------------------------------------------------------- */
extern int             lclient_ping         (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * USER/NICK has been sent but not yet validated                              *
 * -------------------------------------------------------------------------- */
extern void            lclient_login        (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Send welcome messages to the client                                        *
 * -------------------------------------------------------------------------- */
extern void            lclient_welcome      (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Find a lclient by its id                                                   *
 * -------------------------------------------------------------------------- */
extern struct lclient *lclient_find_id      (int              id);

/* -------------------------------------------------------------------------- *
 * Find a lclient by its name                                                 *
 * -------------------------------------------------------------------------- */
extern struct lclient *lclient_find_name    (const char      *name);

/* -------------------------------------------------------------------------- *
 * Dump lclients and lclient heap.                                            *
 * -------------------------------------------------------------------------- */
extern void            lclient_dump         (struct lclient  *lcptr);

#endif /* SRC_LCLIENT_H */
