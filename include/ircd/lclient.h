/* chaosircd - Chaoz's IRC daemon daemon
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
#include "libchaos/net.h"

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
#define LCLIENT_PLUGDATA_LWS    5
#define LCLIENT_PLUGDATA_LWS_SESSION 6

/* Client-facing IRCv3 capabilities (CAP LS/REQ/ACK/LIST) - separate from
 * the `caps' field above (server-to-server CAPAB link negotiation,
 * ircd/server.h's CAP_* bits - different mechanism, different namespace).
 * Stored directly as a bitmask in plugdata[LCLIENT_PLUGDATA_CLICAPS]
 * (pointer-sized, so it fits without a separate allocation) rather than as
 * a named struct field, to avoid an ABI break for every other already-built
 * module: adding a field shifts every field declared after it, silently
 * breaking any module not rebuilt in lockstep, whereas plugdata is already
 * part of every module's existing view of the struct. Use
 * lclient_clicaps()/lclient_set_clicaps() (lclient.h) rather than the slot
 * directly. Lives on lclient, not client, because CAP negotiation (and
 * real clients routinely do this) can happen before NICK/USER complete
 * registration and struct client is even allocated - struct lclient,
 * unlike struct client, exists from the moment the connection is
 * accepted. */
#define LCLIENT_PLUGDATA_CLICAPS 7

#define CLICAP_ECHO_MESSAGE 0x0000000000000001ULL

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
IRCD_DATA(struct lclient*) lclient_me;       /* my local client info */
IRCD_DATA(struct lclient*) lclient_uplink;   /* the uplink if we're a leaf */
extern struct sheap        lclient_heap;     /* heap for lclient_t */
extern struct timer       *lclient_timer;    /* timer for heap gc */
IRCD_DATA(int)             lclient_log;      /* lclient log source */
IRCD_DATA(uint32_t)        lclient_id;
IRCD_DATA(uint32_t)        lclient_serial;
IRCD_DATA(uint32_t)        lclient_max;
IRCD_DATA(struct list)     lclient_list;     /* list with all of them */
IRCD_DATA(struct list)     lclient_lists[4]; /* unreg, clients, servers, opers */
IRCD_DATA(unsigned long)   lclient_recvb[2];
IRCD_DATA(unsigned long)   lclient_sendb[2];
IRCD_DATA(unsigned long)   lclient_recvm[2];
IRCD_DATA(unsigned long)   lclient_sendm[2];

/* ------------------------------------------------------------------------ */
IRCD_DATA(int) lclient_get_log(void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define lclient_is_unknown(x) (((struct lclient *)(x))->type == LCLIENT_UNKNOWN)
#define lclient_is_user(x)    (((struct lclient *)(x))->type == LCLIENT_USER)
#define lclient_is_server(x)  (((struct lclient *)(x))->type == LCLIENT_SERVER)
#define lclient_is_oper(x)    (((struct lclient *)(x))->type == LCLIENT_OPER)

/* -------------------------------------------------------------------------- *
 * Client-facing IRCv3 capabilities - see LCLIENT_PLUGDATA_CLICAPS above.    *
 * -------------------------------------------------------------------------- */
#define lclient_clicaps(x)          ((uint64_t)(uintptr_t)(x)->plugdata[LCLIENT_PLUGDATA_CLICAPS])
#define lclient_set_clicaps(x, v)   ((x)->plugdata[LCLIENT_PLUGDATA_CLICAPS] = (void *)(uintptr_t)(v))

/* -------------------------------------------------------------------------- *
 * Initialize lclient module                                                  *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_init         (void);

/* -------------------------------------------------------------------------- *
 * Shutdown lclient module                                                    *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_shutdown     (void);

/* -------------------------------------------------------------------------- *
 * Garbage collect lclient data                                               *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_collect      (void);

/* -------------------------------------------------------------------------- *
 * A 32-bit PRNG for the ping cookies                                         *
 * -------------------------------------------------------------------------- */
IRCD_API(uint32_t)     lclient_random       (void);

/* -------------------------------------------------------------------------- *
 * Create a new lclient block                                                 *
 * -------------------------------------------------------------------------- */
IRCD_API(struct lclient*)lclient_new          (int              fd,
                                             net_addr_t       addr,
                                             net_port_t       port);

/* -------------------------------------------------------------------------- *
 * Delete a  lclient block                                                    *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_delete       (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Loose all references of an lclient block                                    *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_release      (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Get a reference to an lclient block                                        *
 * -------------------------------------------------------------------------- */
IRCD_API(struct lclient*)lclient_pop          (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Push back a reference to an lclient block                                  *
 * -------------------------------------------------------------------------- */
IRCD_API(struct lclient*)lclient_push         (struct lclient **lcptrptr);

/* -------------------------------------------------------------------------- *
 * Set the type of an lclient and move it to the appropriate list             *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_set_type     (struct lclient  *lcptr,
                                             uint32_t         type);

/* -------------------------------------------------------------------------- *
 * Set the name of an lclient block                                           *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_set_name     (struct lclient  *lcptr,
                                             const char      *name);

/* -------------------------------------------------------------------------- *
 * Accept a local client                                                      *
 *                                                                            *
 * <fd>                     - filedescriptor of the new connection            *
 *                            (may be invalid?)                               *
 * <listen>                 - the corresponding listen{} block                *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_accept       (int              fd,
                                             struct listen   *listen);

/* -------------------------------------------------------------------------- *
 * Whether the listen{} block <lcptr> was accepted through has "lws = yes;"   *
 * set (the default). Returns 0 for a client with no listen{} block at all    *
 * (e.g. one attached by lc_lws itself after a completed WS upgrade).         *
 * -------------------------------------------------------------------------- */
IRCD_API(int)          lclient_listen_has_lws(struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_connect      (int              fd,
                                             struct connect  *connect);

/* -------------------------------------------------------------------------- *
 * Read data from a local connection and process it                           *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_recv         (int              fd,
                                             struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Read a line from queue and process it                                      *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_process      (int              fd,
                                             struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Parse the prefix and the command                                           *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_parse        (struct lclient  *lcptr,
                                             char            *s,
                                             size_t           n);

/* -------------------------------------------------------------------------- *
 * Decide whether a message is numeric or not and call the appropriate        *
 * message handler.                                                           *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_message      (struct lclient  *client,
                                             char           **argv,
                                             char            *arg,
                                             size_t           n);

/* -------------------------------------------------------------------------- *
 * Process a numeric message                                                  *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_numeric      (struct lclient  *lcptr,
                                             char           **argv,
                                             char            *arg);

/* -------------------------------------------------------------------------- *
 * Process a command                                                          *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_command      (struct lclient  *lcptr,
                                             char           **argv,
                                             char            *arg,
                                             size_t           n);

/* -------------------------------------------------------------------------- *
 * Parse the prefix and find the appropriate client                           *
 * -------------------------------------------------------------------------- */
IRCD_API(struct client*)  lclient_prefix       (struct lclient  *lcptr,
                                             const char      *pfx);


/* -------------------------------------------------------------------------- *
 * Exit a local client and leave him an error message if he has registered.   *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_vexit        (struct lclient  *lcptr,
                                             char            *format,
                                             va_list          args);

IRCD_API(int)          lclient_exit         (struct lclient  *lcptr,
                                             char            *format,
                                             ...);

/* -------------------------------------------------------------------------- *
 * Update client message/byte counters                                        *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_update_recvb (struct lclient  *lcptr,
                                             size_t           n);

IRCD_API(void)         lclient_update_sendb (struct lclient  *lcptr,
                                             size_t           n);

/* -------------------------------------------------------------------------- *
 * Send a line to a local client                                              *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_send_raw     (struct lclient  *lcptr,
                                             const void      *buf,
                                             size_t           n);

IRCD_API(void)         lclient_vsend        (struct lclient  *lcptr,
                                             const char      *format,
                                             va_list          args);

IRCD_API(void)         lclient_send         (struct lclient  *lcptr,
                                             const char      *format,
                                             ...);

/* -------------------------------------------------------------------------- *
 * Send a line to a client list but one                                       *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_vsend_list   (struct lclient  *one,
                                             struct list     *list,
                                             const char      *format,
                                             va_list          args);

IRCD_API(void)         lclient_send_list    (struct lclient  *one,
                                             struct list     *list,
                                             const char      *format,
                                             ...);

/* -------------------------------------------------------------------------- *
 * Check for valid USER/NICK and start handshake                              *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_handshake    (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Register a local client to the global client pool                          *
 * -------------------------------------------------------------------------- */
IRCD_API(int)          lclient_register     (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Check if we got a PONG, if not exit the client otherwise send another PING *
 * -------------------------------------------------------------------------- */
IRCD_API(int)          lclient_ping         (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * USER/NICK has been sent but not yet validated                              *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_login        (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Send welcome messages to the client                                        *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_welcome      (struct lclient  *lcptr);

/* -------------------------------------------------------------------------- *
 * Find a lclient by its id                                                   *
 * -------------------------------------------------------------------------- */
IRCD_API(struct lclient*)lclient_find_id      (int              id);

/* -------------------------------------------------------------------------- *
 * Find a lclient by its name                                                 *
 * -------------------------------------------------------------------------- */
IRCD_API(struct lclient*)lclient_find_name    (const char      *name);

/* -------------------------------------------------------------------------- *
 * Dump lclients and lclient heap.                                            *
 * -------------------------------------------------------------------------- */
IRCD_API(void)         lclient_dump         (struct lclient  *lcptr);

#endif
