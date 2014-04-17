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
 * $Id: listen.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_LISTEN_H
#define LIB_LISTEN_H

/* ------------------------------------------------------------------------ *
 * Library headers                                                            *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/mem.h"
#include "libchaos/net.h"
#include "libchaos/dlink.h"
#include "libchaos/ssl.h"

/* ------------------------------------------------------------------------ *
 * Constants                                                                  *
 * ------------------------------------------------------------------------ */

#define LISTEN_IDLE        0x00          /* listen block just sitting there */
#define LISTEN_LISTENING   0x01          /* listening for incoming connects */
#define LISTEN_ERROR       0x02          /* error listening */
#define LISTEN_CONNECTION  0x03          /* incoming connection */

/* ------------------------------------------------------------------------ *
 * Listen block structure.                                                    *
 * ------------------------------------------------------------------------ */
struct filter;

struct listen 
{
  struct node         node;                 /* linking node for listen_list */
  uint32_t            id;
  uint32_t            refcount;             /* times this block is referenced */
  uint32_t            lhash;
  uint32_t            nhash;
  
  /* externally initialised */
  uint16_t            port;
  uint16_t            backlog;              /* backlog buffer size */
  int                 ssl;                  /* SSL encryption flag */
  struct protocol    *proto;                /* protocol handler */
  void               *args;                 /* user-defineable arguments */
  char                name[HOSTLEN + 1];    /* user-definable name */
  char                address[HOSTLEN + 1]; /* hostname to listen on */
  char                context[64];

  /* internally initialised */
  int                 fd;
  net_addr_t          addr_local;
  net_addr_t          addr_remote;
  net_port_t          port_local;
  net_port_t          port_remote;
  uint32_t            status;
  struct filter      *filter;
  struct ssl_context *ctxt;
};

/* ------------------------------------------------------------------------ *
 * Global variables                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)            listen_log;
CHAOS_API(struct sheap)   listen_heap;       /* heap containing listen blocks */
CHAOS_API(struct list)    listen_list;       /* list linking listen blocks */
CHAOS_API(uint32_t)       listen_id;
CHAOS_API(int)            listen_dirty;

/* ------------------------------------------------------------------------ */
CHAOS_API(int) listen_get_log(void);

/* ------------------------------------------------------------------------ *
 * Initialize listener heap and add garbage collect timer.                    *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)           listen_init          (void);

/* ------------------------------------------------------------------------ *
 * Destroy listener heap and cancel timer.                                    *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)           listen_shutdown      (void);

/* ------------------------------------------------------------------------ *
 * Garbage collect                                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)            listen_collect       (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)           listen_default       (struct listen  *liptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct listen *)listen_add           (const char     *address,
                                                uint16_t        port,
                                                int             backlog,
                                                int             ssl,
                                                const char     *context,
                                                const char     *protocol);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)            listen_update        (struct listen  *liptr,
                                                int             backlog,
                                                int             ssl,
                                                const char     *context,
                                                const char     *protocol);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)           listen_delete        (struct listen  *liptr);

/* ------------------------------------------------------------------------ *
 * Loose all references                                                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)           listen_release       (struct listen  *liptr);
  
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct listen *)listen_pop           (struct listen  *liptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct listen *)listen_push          (struct listen **liptrptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct listen *)listen_find          (const char     *address,
                                                uint16_t        port);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)           listen_set_args      (struct listen  *liptr, 
                                                const void     *argbuf,
                                                size_t          n);
 
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)           listen_get_args      (struct listen  *liptr, 
                                                void           *argbuf,
                                                size_t          n);
  
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)           listen_set_name      (struct listen  *liptr,
                                                const char     *name);
 
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(const char *)   listen_get_name      (struct listen  *liptr);
  
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct listen *)listen_find_name     (const char     *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct listen *)listen_find_id       (uint32_t        id);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)            listen_attach_filter (struct listen *lptr, 
                                                struct filter *fptr);  

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)            listen_detach_filter (struct listen *lptr);

/* ------------------------------------------------------------------------ *
 * Dump listeners and listen heap.                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)           listen_dump          (struct listen  *lptr);
  
#endif /* LIB_LISTEN_H */
