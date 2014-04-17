/* chaosircd - pi-networks irc server
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
 * $Id: connect.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_CONNECT_H
#define LIB_CONNECT_H

/* ------------------------------------------------------------------------ *
 * Library headers                                                            *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/dlink.h"
#include "libchaos/timer.h"
#include "libchaos/net.h"
#include "libchaos/ssl.h"

/* ------------------------------------------------------------------------ *
 * Constants                                                                  *
 * ------------------------------------------------------------------------ */

/* connect->status value */
#define CONNECT_IDLE       0x00          /* Connect block just sitting there */
#define CONNECT_RESOLVING  0x01          /* Doing a DNS query */
#define CONNECT_CONNECTING 0x02          /* Waiting for connection */
#define CONNECT_TIMEOUT    0x03          /* Connection timed out */
#define CONNECT_ERROR      0x04          /* Socket error */
#define CONNECT_DONE       0x05          /* Connection succeeded */

#define CONNECT_DEFAULT_ADDR     "127.0.0.1"
#define CONNECT_DEFAULT_PORT     1024
#define CONNECT_DEFAULT_NAME     "127.0.0.1:1024"
#define CONNECT_DEFAULT_LADDR    INADDR_ANY
#define CONNECT_DEFAULT_RADDR    INADDR_LOOPBACK
#define CONNECT_DEFAULT_TIMEOUT  (30 * 1000LLU)     /* 30 seconds */
#define CONNECT_DEFAULT_INTERVAL (5 * 60 * 1000LLU) /* 5 minutes */

/* ------------------------------------------------------------------------ *
 * Connect block structure.                                                   *
 * ------------------------------------------------------------------------ */
struct connect 
{
  /* Block info */
  struct node      node;                 /* Linking node for connect_list */
  uint32_t         id;                   /* Serial number */
  uint32_t         refcount;             /* Times this block is referenced */
  uint32_t         chash;                /* Hash based on address:port */
  uint32_t         nhash;                /* Hash based on name */
  uint32_t         status;               /* Status of the connect block */ 
  
  /* References to other blocks */
  struct timer    *timer;                /* Connect timeout timer */
  struct protocol *proto;                /* Protocol block */
  
  /* Internally initialised */
  int              fd;                   /* File descriptor */
  int              active;
  int              silent;
  net_addr_t       addr_remote;          /* Inet address we connect to */
  net_addr_t       addr_local;           /* Inet address we connect from */
  net_port_t       port_remote;          /* Port we connect to */
  net_port_t       port_local;           /* Port we connect from */
  uint64_t         start;                /* Time at which the connect started */
  struct sauth    *sauth;
  int              resolve;

  /* Externally initialised */
  uint64_t         timeout;              /* Connect times out after n msecs */
  uint64_t         interval;             /* Try to connect every n msecs */
  int              autoconn;             /* Auto-initiate connect? */
  int              ssl;                  /* SSL encryption flag */
  char             context[64];
  void            *args;                 /* User-defineable block */
  char             address[HOSTLEN];     /* Hostname to connect to */
  char             name[HOSTLEN];        /* User-definable name */

  struct ssl_context *ctxt;
};

/* ------------------------------------------------------------------------ *
 * Global variables                                                         *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)             connect_log;        /* Log source */
CHAOS_API(struct sheap)    connect_heap;       /* Heap containing connect blocks */
CHAOS_API(struct timer *)  connect_timer;      /* Garbage collect timer */
CHAOS_API(struct list)     connect_list;       /* List linking connect blocks */
CHAOS_API(uint32_t)        connect_id;         /* Next serial number */

/* ------------------------------------------------------------------------ */
CHAOS_API(int) connect_get_log(void);

/* ------------------------------------------------------------------------ *
 * Initialize the connect module.                                           *
 *                                                                          *
 * - register connect_log                                                   *
 * - create connect_heap                                                    *
 * - start connect_timer                                                    *
 * - zero connect_list                                                      *
 * - report success                                                         *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            connect_init           (void);

/* ------------------------------------------------------------------------ *
 * Shut down the connect module.                                            *
 *                                                                          *
 * - report status                                                          *
 * - remove all connect blocks                                              *
 * - cancel connect_timer                                                   *
 * - destroy connect_heap                                                   *
 * - unregister connect_log                                                 *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            connect_shutdown       (void);

/* ------------------------------------------------------------------------ *
 * Collect connect block garbage.                                           *
 *                                                                          *
 * - report verbose                                                         *
 * - free all connect blocks with a zero refcount                           *
 * - collect garbage on connect_heap                                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)             connect_collect        (void);
  
/* ------------------------------------------------------------------------ *
 * Fill a connect block with default values.                                *
 *                                                                          *
 * <cnptr>           pointer to connect block                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            connect_default        (struct connect  *cnptr);

/* ------------------------------------------------------------------------ *
 * Add a new connect block if we got a valid address and a valid protocol   *
 * handler. Initialise all the user-supplied (externally initialised) and   *
 * the internal stuff. If the autoconn flag is set the connect will be      * 
 * initiated immediately.                                                   *
 *                                                                          *
 * <address>           A valid hostname or address                          *
 * <port>              A valid port                                         *
 * <pptr>              Pointer to a protocol block                          *
 * <timeout>           Connect timeout in msecs                             *
 * <interval>          Connect interval for autoconnect                     *
 * <autoconn>          Flag for autoconnect                                 *
 * <ssl>               SSL connection?                                      *
 *                                                                          *
 * Will return a pointer to the connect block or NULL on failure.           *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct connect *)connect_add          (const char      *address,
                                                 uint16_t         port,
                                                 struct protocol *pptr,
                                                 uint64_t         timeout,
                                                 uint64_t         interval,
                                                 int              autoconn,
                                                 int              ssl,
                                                 const char      *context);

/* ------------------------------------------------------------------------ *
 * Update the externally initialised stuff of a connect block.              *
 *                                                                          *
 * <cnptr>             The connect block to update                          *
 * <address>           A valid hostname or address                          *
 * <port>              A valid port                                         *
 * <pptr>              Pointer to a protocol block                          *
 * <timeout>           Connect timeout in msecs                             *
 * <interval>          Connect interval for autoconnect                     *
 * <autoconn>          Flag for autoconnect                                 *
 * <ssl>               SSL connection?                                      * 
 * ------------------------------------------------------------------------ */
CHAOS_API(int)             connect_update       (struct connect  *cnptr, 
                                                 const char      *address,
                                                 uint16_t         port,
                                                 struct protocol *pptr,
                                                 uint64_t         timeout,
                                                 uint64_t         interval,
                                                 int              autoconn,
                                                 int              ssl,
                                                 const char      *context);

/* ------------------------------------------------------------------------ *
 * Remove and free a connect block.                                         *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            connect_delete       (struct connect  *cnptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct connect *)connect_pop          (struct connect  *cnptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct connect *)connect_push         (struct connect **cnptrptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)             connect_resolve      (struct connect  *cnptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)             connect_start        (struct connect  *cnptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)             connect_initiate     (struct connect  *cnptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            connect_cancel       (struct connect  *cnptr);

/* ------------------------------------------------------------------------ *
 * If the connect block has an interval value then schedule a connection    *
 * retry. The retry will occurr in the remaining time from last connection  *
 * initiation (cnptr->start) 'til next interval (cnptr->start + interval)   *
 * Returns 0 if a retry was scheduled.                                      *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)             connect_retry        (struct connect  *cnptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            connect_set_arg      (struct connect  *cnptr, 
                                                 void            *arg);
  
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            connect_set_args     (struct connect  *cnptr,
                                                 const void      *argbuf,
                                                 size_t           n);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void *)          connect_get_args     (struct connect  *cnptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            connect_set_name     (struct connect   *cnptr,
                                                 const char       *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(const char *)    connect_get_name     (struct connect  *cnptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct connect *)connect_find_name    (const char      *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct connect *)connect_find_id      (uint32_t         id);

/* ------------------------------------------------------------------------ *
 * Dump connects and connect heap.                                          *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            connect_dump         (struct connect  *cnptr);
  
#endif /* LIB_CONNECT_H */
