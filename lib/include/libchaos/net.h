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
 * $Id: net.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_NET_H
#define LIB_NET_H

/* ------------------------------------------------------------------------ *
 * Library headers                                                          *
 * ------------------------------------------------------------------------ */
#include "libchaos/config.h"
#include "libchaos/defs.h"
#include "libchaos/io.h"

/* ------------------------------------------------------------------------ *
 * Constants                                                                *
 * ------------------------------------------------------------------------ */
#define NET_SERVER 0
#define NET_CLIENT 1

#define NET_ADDR_ANY       0x00000000
#define NET_ADDR_BROADCAST 0xffffffff
#define NET_ADDR_LOOPBACK  0x7f000001

#define NET_CLASSC_NET     0xffffff00
#define NET_CLASSB_NET     0xffff0000
#define NET_CLASSA_NET     0xff000000

/* ------------------------------------------------------------------------ *
 * Types                                                                    *
 * ------------------------------------------------------------------------ */
enum net_socket
{
  NET_SOCKET_STREAM = 0,
  NET_SOCKET_DGRAM  = 1
};

enum net_address
{
  NET_ADDRESS_IPV4 = 0,
  NET_ADDRESS_IPV6 = 1
};

typedef enum net_socket net_socket_t;
typedef enum net_address net_address_t;

struct listen;

/* Protocol handler type */
typedef void (net_callback_t)(int, void *listenerorconnect, void *arg);

typedef uint32_t net_addr_t;  /* IPv4 address in network byte order */
typedef uint16_t net_port_t;  /* Port number in network byte order */

struct protocol
{
  struct node     node;
  uint32_t        id;
  int             refcount;
  hash_t          hash;
  int             type;
  char            name[PROTOLEN + 1];
  net_callback_t *handler;
};

typedef struct protocol protocol_t;

/* ------------------------------------------------------------------------ *
 * Global variables                                                         *
 * ------------------------------------------------------------------------ */
CHAOS_DATA(int           )net_log;
CHAOS_DATA(struct sheap  )net_heap;
CHAOS_DATA(struct timer *)net_timer;
CHAOS_DATA(struct list   )net_list;
CHAOS_DATA(uint32_t      )net_id;

CHAOS_DATA(net_addr_t    )net_addr_loopback;
CHAOS_DATA(net_addr_t    )net_addr_any;

/* ------------------------------------------------------------------------ */
CHAOS_API(int) net_get_log(void);

/* ------------------------------------------------------------------------ *
 * Convert a short from host to network byteorder                              *
 * ------------------------------------------------------------------------ */
static inline net_port_t net_htons(uint16_t n)
{
  union {
    uint8_t c[2];
    uint16_t i;
  } u;

  u.c[0] = (n >> 8) & 0xff;
  u.c[1] =  n       & 0xff;

  return u.i;
}

/* ------------------------------------------------------------------------ *
 * Convert a short from network to host byteorder                             *
 * ------------------------------------------------------------------------ */
static inline uint16_t net_ntohs(net_port_t n)
{
  union {
    uint16_t i;
    uint8_t c[2];
  } u;

  u.i = n;


  return ((uint16_t)(u.c[0] << 8)) | ((uint16_t)u.c[1]);
}

/* ------------------------------------------------------------------------ *
 * Convert a long from host to network byteorder                              *
 * ------------------------------------------------------------------------ */
static inline net_addr_t net_htonl(uint32_t n)
{
  union {
    uint8_t c[4];
    uint32_t i;
  } u;

  u.c[0] = (n >> 24) & 0xff;
  u.c[1] = (n >> 16) & 0xff;
  u.c[2] = (n >>  8) & 0xff;
  u.c[3] =  n        & 0xff;

  return u.i;
}

/* ------------------------------------------------------------------------ *
 * Convert a long from network to host byteorder                              *
 * ------------------------------------------------------------------------ */
static inline uint32_t net_ntohl(net_addr_t n)
{
  union {
    uint32_t i;
    uint8_t c[4];
  } u;

  u.i = n;

  return (u.c[0] << 24) |
         (u.c[1] << 16) |
         (u.c[2] <<  8) |
          u.c[3];
}

/* ------------------------------------------------------------------------ *
 * Convert from network address to string (re-entrant). (AF_INET)             *
 * ------------------------------------------------------------------------ */
CHAOS_API(char *)      net_ntoa_r   (net_addr_t      in,
                                     char           *buf);

/* ------------------------------------------------------------------------ *
 * Convert from network address to string. (AF_INET)                          *
 * ------------------------------------------------------------------------ */
CHAOS_API(char *)      net_ntoa     (net_addr_t      in);

/* ------------------------------------------------------------------------ *
 * Convert from string to network address. (AF_INET)                          *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)         net_aton     (const char     *cp,
                                     net_addr_t     *inp);

/* ------------------------------------------------------------------------ *
 * Convert from network address to string. (AF_INET and AF_INET6)             *
 * ------------------------------------------------------------------------ */
CHAOS_API(const char *)net_ntop     (int             af,
                                     const void     *cp,
                                     char           *buf,
                                     size_t          len);

/* ------------------------------------------------------------------------ *
 * Convert from string to network address. (AF_INET and AF_INET6)             *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)         net_pton     (int             af,
                                     const char     *cp,
                                     void           *buf);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#if 1 //def __GNUC__

CHAOS_API(char *)      net_ntoa    (net_addr_t in);
/*{
  static char buf[16];
  return net_ntoa_r(in, buf);
}*/

#endif /* __GNUC__ */

/* ------------------------------------------------------------------------ *
 * Initialize protocol heap.                                                  *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)             net_init       (void);

/* ------------------------------------------------------------------------ *
 * Destroy protocol heap.                                                     *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)             net_shutdown   (void);

/* ------------------------------------------------------------------------ *
 * Find a protocol.                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct protocol *)net_find       (int             type,
                                            const char     *name);

CHAOS_API(struct protocol *)net_find_id    (uint32_t        id);

/* ------------------------------------------------------------------------ *
 * Register a protocol.                                                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct protocol *)net_register   (int             type,
                                            const char     *name,
                                            void           *handler);

/* ------------------------------------------------------------------------ *
 * Unregister a protocol.                                                     *
 * ------------------------------------------------------------------------ */
CHAOS_API(void *)           net_unregister (int               type,
                                            const char       *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct protocol *)net_pop        (struct protocol  *pptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct protocol *)net_push       (struct protocol **pptrptr);


/* ------------------------------------------------------------------------ *
 * Create a streaming socket with desired protocol family and type            *
 * (SOCK_DGRAM or SOCK_STREAM)                                                *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)              net_socket     (net_address_t   at,
                                            net_socket_t    st);

/* ------------------------------------------------------------------------ *
 * Bind a socket to a specified address/port                                  *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)              net_bind       (int             fd,
                                            net_addr_t      addr,
                                            uint16_t        port);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)              net_vconnect   (int             fd,
                                            net_addr_t      addr,
                                            uint16_t        port,
                                            void           *cb_rd,
                                            void           *cb_wr,
                                            va_list         args);

CHAOS_API(int)              net_connect    (int             fd,
                                            net_addr_t      addr,
                                            uint16_t        port,
                                            void           *cb_rd,
                                            void           *cb_wr,
                                            ...);

/* ------------------------------------------------------------------------ *
 * Listen for incoming connections and register read callback.                *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)              net_vlisten    (int             fd,
                                            int             backlog,
                                            void           *callback,
                                            va_list         args);

CHAOS_API(int)              net_listen     (int             fd,
                                            int             backlog,
                                            void           *callback,
                                            ...);

/* ------------------------------------------------------------------------ *
 * Accept a pending connection.                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)              net_accept     (int             fd,
                                            net_addr_t     *addrptr,
                                            net_port_t     *portptr);

/* ------------------------------------------------------------------------ *
 * Get local socket address.                                                *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)              net_getsockname(int             fd,
                                            net_addr_t     *addrptr,
                                            net_port_t     *portptr);

/* ------------------------------------------------------------------------ *
 * Dump protocol stack.                                                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)             net_dump       (struct protocol  *nptr);

#endif /* LIB_NET_H */
