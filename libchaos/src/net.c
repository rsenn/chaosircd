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
 * $Id: net.c,v 1.7 2006/09/28 08:38:31 roman Exp $
 */

#define _GNU_SOURCE

/* ------------------------------------------------------------------------ *
 * Library headers                                                          *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/syscall.h"
#include "libchaos/dlink.h"
#include "libchaos/timer.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/net.h"
#include "libchaos/str.h"

/* ------------------------------------------------------------------------ *
 * System headers                                                           *
 * ------------------------------------------------------------------------ */
#include "../config.h"

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif /* HAVE_WINSOCK2_H */

#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif /* HAVE_WS2TCPIP_H */

#ifdef HAVE_CYGWIN_IN_H
#include <cygwin/in.h>
#endif /* HAVE_CYGWIN_IN_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#ifdef WIN32
#include <winsock2.h>
#define EINPROGRESS WSAEWOULDBLOCK
#endif /* WIN32 */

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int           net_log;
struct sheap  net_heap;
struct timer *net_timer;
struct list   net_list;
uint32_t      net_id;

net_addr_t    net_addr_any = INADDR_ANY;
net_addr_t    net_addr_loopback = INADDR_ANY;

/* ------------------------------------------------------------------------ */
int net_get_log() { return net_log; }

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
static inline char tohex(char hexdigit)
{
  return hexdigit > 9 ? hexdigit + 'a' - 10 : hexdigit + '0';
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
static unsigned int i2a(char *dest, unsigned int x)
{
  register unsigned int tmp = x;
  register unsigned int len = 0;

  if(x >= 100)
  {
    *dest++ = tmp / 100 + '0';
    tmp = tmp % 100;
    len++;
  }

  if(x >= 10)
  {
    *dest++ = tmp / 10 + '0';
    tmp = tmp % 10;
    len++;
  }

  *dest++ = tmp + '0';

  return len + 1;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
/*static int fmt_xlong(char* s, unsigned int i)
{
  char *bak = s;

  *s = tohex((i >> 12) & 0x0f);
  if(s != bak || *s != '0') s++;
  *s = tohex((i >> 8) & 0x0f);
  if(s != bak || *s != '0') s++;
  *s = tohex((i >> 4) & 0x0f);
  if(s != bak || *s != '0') s++;
  *s = tohex(i & 0x0f);

  return s - bak + 1;
}*/

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
char *net_ntoa_r(net_addr_t in, char *buf)
{
  unsigned int len;
  unsigned char *ip = (unsigned char*)&in;

  len = i2a(buf, ip[0]);
  buf[len++] = '.';
  len += i2a(&buf[len], ip[1]);
  buf[len++]='.';
  len += i2a(&buf[len], ip[2]);
  buf[len++]='.';
  len += i2a(&buf[len], ip[3]);
  buf[len] = 0;

  return buf;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
char *net_ntoa(net_addr_t in)
{
  static char buf[16];
  return net_ntoa_r(in, buf);
}

/* ------------------------------------------------------------------------ *
 * Converts the Internet host address cp from the standard                  *
 * numbers-and-dots notation into binary data and stores it in the          *
 * structure that inp points to. net_aton returns nonzero if the           *
 * address is valid, zero if not.                                           *
 *                                                                          *
 * problem is, net_aton is historically quite, uh, lenient.                *
 * the following are all acceptable:                                        *
 *   0x7f000001 == 127.1 == 127.0.0.1.0 == 127.0.0.1                        *
 * btw: 127.0.0.x.y == 127.0.0.(x|y)                                        *
 * and: 10.1.1 == 10.1.0.1 (huh?!)                                          *
 * and: 10 == 0.0.0.10 (?!?!?)                                              *
 * The Berkeley people must have been so stoned that they are still high.   *
 *                                                                          *
 * I hereby disclaim that I wrote this                                      *
 * ------------------------------------------------------------------------ */
int net_aton(const char *cp, net_addr_t *inp)
{
  int i;
  unsigned int ip = 0;

  char *tmp = (char *)cp;

  for(i=24; ;)
  {
    long j;

    j = str_toul(tmp, &tmp, 0);

    if(*tmp == 0)
    {
      ip |= j;
      break;
    }

    if(*tmp == '.')
    {
      if(j > 255)
        return 0;

      ip |= (j << i);

      if(i > 0)
        i -= 8;

      tmp++;
      continue;
    }

    return 0;
  }

  *inp = net_htonl(ip);

  return 1;
}

/* ------------------------------------------------------------------------ *
 * Initialize protocol heap.                                                *
 * ------------------------------------------------------------------------ */
void net_init(void)
{
  net_log = log_source_register("net");

  mem_static_create(&net_heap, sizeof(struct protocol), NET_BLOCK_SIZE);
  mem_static_note(&net_heap, "protocol heap");

  net_id = 0;

/*  net_timer = timer_start(mem_static_collect, GC_INTERVAL, &net_heap);
  timer_note(net_timer, "garbage collect: net heap");*/
#ifdef WIN32
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2, 2);
  WSAStartup(wVersionRequested, &wsaData);
#endif /* WIN32 */
}

/* ------------------------------------------------------------------------ *
 * Destroy protocol heap.                                                   *
 * ------------------------------------------------------------------------ */
void net_shutdown(void)
{
  struct node *node;
  struct node *next;

  dlink_foreach_safe(&net_list, node, next)
  {
    dlink_delete(&net_list, node);
    mem_static_free(&net_heap, node);
  }

  mem_static_destroy(&net_heap);

  log_source_unregister(net_log);
}

/* ------------------------------------------------------------------------ *
 * Find a protocol.                                                         *
 * ------------------------------------------------------------------------ */
struct protocol *net_find(int type, const char *name)
{
  struct protocol *p;
  struct node     *node;
  hash_t           hash;
  
  hash = str_hash(name);

  dlink_foreach(&net_list, node)
  {
    p = (struct protocol *)node;

    if(p->hash == hash && p->type == type)
      return p;
  }

  return NULL;
}

struct protocol *net_find_id(uint32_t id)
{
  struct protocol *p;

  dlink_foreach(&net_list, p)
  {
    if(p->id == id)
      return p;
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * Register a protocol.                                                     *
 * ------------------------------------------------------------------------ */
struct protocol *net_register(int type, const char *name,
                              void *handler)
{
  struct protocol *p;

  if(net_find(type, name))
  {
    log(net_log, L_warning, "Protocol %s was already registered.", name);
    return NULL;
  }

  p = mem_static_alloc(&net_heap);

  strlcpy(p->name, name, sizeof(p->name));

  p->hash = str_ihash(p->name);
  p->type = type;
  p->refcount = 1;
  p->id = net_id++;

  p->handler = handler;

  dlink_add_tail(&net_list, &p->node, p);

  log(net_log, L_status, "Registered %s protocol %s.",
      (type == NET_CLIENT ? "client" : "server"), p->name);

  return p;
}

/* ------------------------------------------------------------------------ *
 * Unregister a protocol.                                                   *
 * ------------------------------------------------------------------------ */
void *net_unregister(int type, const char *name)
{
  void *ret = NULL;
  struct protocol *p;

  p = net_find(type, name);

  if(p)
  {
    log(net_log, L_status, "Unregistered %s protocol %s.",
        (type == NET_CLIENT ? "client" : "server"), name);

    dlink_delete(&net_list, &p->node);
    mem_static_free(&net_heap, p);
    mem_static_collect(&net_heap);
  }

  return ret;
}
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct protocol *net_pop(struct protocol *pptr)
{
  if(pptr)
  {
    if(!pptr->refcount)
      log(net_log, L_warning, "Poping deprecated %s proto: %s",
          (pptr->type == NET_CLIENT ? "client" : "server"), pptr->name);

    pptr->refcount++;
  }

  return pptr;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct protocol *net_push(struct protocol **pptrptr)
{
  if(*pptrptr)
  {
    if(!(*pptrptr)->refcount)
      log(net_log, L_warning, "Trying to push deprecated %s proto: %s",
          ((*pptrptr)->type == NET_CLIENT ? "client" : "server"),
          (*pptrptr)->name);
    else
      (*pptrptr)->refcount--;

    (*pptrptr) = NULL;
  }

  return *pptrptr;
}

/* ------------------------------------------------------------------------ *
 * Create a streaming socket with desired protocol family and type          *
 * (SOCK_DGRAM or SOCK_STREAM)                                              *
 * ------------------------------------------------------------------------ */
int net_socket(net_address_t at, net_socket_t st)
{
  int fd;
  int pf = 0;
  int type = 0;

  switch(at)
  {
    case NET_ADDRESS_IPV4: pf = AF_INET; break;
#ifdef AF_INET6
    case NET_ADDRESS_IPV6: pf = AF_INET6; break;
#else
    default: return -1;
#endif
  }

  switch(st)
  {
    case NET_SOCKET_DGRAM: type = SOCK_DGRAM; break;
    case NET_SOCKET_STREAM: type = SOCK_STREAM; break;
  }

  /* Try to create TCP/UDP socket */
  fd = syscall_socket(pf, type, IPPROTO_IP);

  if(fd < 0)
    return -1;

  /* Set O_NONBLOCK flag */
  io_nonblock(fd);

  /* Some info */
/*  debug(net_log, "new socket: fd = %i, family: %s, proto = %s",
        fd,
        pf == AF_INET ? "INET" : NULL,
        type == SOCK_DGRAM ? "UDP" : "TCP");*/

  /* Register it in the io_list */
  return io_new(fd, FD_SOCKET);
}

/* ------------------------------------------------------------------------ *
 * Bind a socket to a specified address/port                                *
 * ------------------------------------------------------------------------ */
int net_bind(int fd, net_addr_t addr, uint16_t port)
{
  int opt = 1;
  struct sockaddr_in local;
  /* Validate fd */
/*  if(!io_valid(fd))
    return -1;*/

  local.sin_family = AF_INET;
  local.sin_addr.s_addr = addr;
  local.sin_port = net_htons(port);

  if(syscall_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)))
  {
    io_close(fd);
    return -1;
  }

  if(syscall_bind(fd, (struct sockaddr *)&local, sizeof(struct sockaddr_in)) != 0)
  {
    io_list[fd].error = syscall_errno;
    return -1;
  }

/*  debug(net_log, "bound socket %i to %s:%u",
        fd, net_ntoa(addr), port);*/

  return 0;
}

/* ------------------------------------------------------------------------ *
 * Listen for incoming connections and register read callback.              *
 * ------------------------------------------------------------------------ */
int net_vlisten(int fd, int backlog, void *callback, va_list args)
{
/*  if(!io_valid(fd))
    return -1;*/

  if(syscall_listen(fd, backlog))
  {
    io_close(fd);
    return -1;
  }

/*  io_list[fd].status.listening = 1;*/

  if(io_vregister(fd, IO_CB_READ, callback, args))
    return -1;

  return 0;
}

int net_listen(int fd, int backlog, void *callback, ...)
{
  int     ret;
  va_list args;

  va_start(args, callback);

  ret = net_vlisten(fd, backlog, callback, args);

  va_end(args);

  return ret;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int net_vconnect(int   fd,    net_addr_t addr,  uint16_t port,
                 void *cb_rd, void      *cb_wr, va_list args)
{
  struct sockaddr_in sina;
  void              *argp[4];

/*  if(!io_valid(fd))
    return -1;*/

  sina.sin_family = AF_INET;
  sina.sin_addr.s_addr = addr;
  sina.sin_port = net_htons(port);

  if(syscall_connect(fd, (struct sockaddr *)&sina, sizeof(struct sockaddr_in)))
  {
    if((syscall_errno != EINPROGRESS))
    {
      io_close(fd);
      return -1;
    }
  }

/*  io_list[fd].status.connecting = 1;*/

  if(cb_rd && cb_wr)
  {
    argp[0] = va_arg(args, void *);
    argp[1] = va_arg(args, void *);
    argp[2] = va_arg(args, void *);
    argp[3] = va_arg(args, void *);

    if(io_register(fd, IO_CB_READ, cb_rd, argp[0], argp[1], argp[2], argp[3]) ||
       io_register(fd, IO_CB_WRITE, cb_wr, argp[0], argp[1], argp[2], argp[3]))
      return -1;
  }

  return 0;
}

int net_connect(int   fd,    net_addr_t addr,  uint16_t port,
                void *cb_rd, void      *cb_wr, ...)
{
  va_list args;
  int     ret;

  va_start(args, cb_wr);

  ret = net_vconnect(fd, addr, port, cb_rd, cb_wr, args);

  va_end(args);

  return ret;
}

/* ------------------------------------------------------------------------ *
 * Accept a pending connection.                                             *
 * ------------------------------------------------------------------------ */
int net_accept(int fd, net_addr_t *addrptr, net_port_t *portptr)
{
  struct sockaddr_in addr;
  struct sockaddr_in local;
  int                ret;
  socklen_t          addrlen = sizeof(struct sockaddr_in);

  ret = syscall_accept(fd, (struct sockaddr *)&addr, (unsigned int *)&addrlen);

  if(ret == -1)
    return -1;

#ifdef SO_PEERNAME
  syscall_getsockopt(ret, SOL_SOCKET, SO_PEERNAME, (void *)&addr, &addrlen);
#else
  addrlen = sizeof(struct sockaddr_in);
  syscall_getpeername(ret, (void *)&addr, (unsigned int *)&addrlen);
#endif
  addrlen = sizeof(struct sockaddr_in);
  syscall_getsockname(ret, (void *)&local, (unsigned int *)&addrlen);

  if(io_new(ret, FD_SOCKET) != ret)
  {
#ifdef WIN32
    closesocket(ret);
#else
    close(ret);
#endif /* WIN32 */
    return -1;
  }

  if(addrptr) *addrptr = addr.sin_addr.s_addr;
  if(portptr) *portptr = addr.sin_port;

  return ret;
}

/* ------------------------------------------------------------------------ *
 * Get local socket address.                                                *
 * ------------------------------------------------------------------------ */
int net_getsockname(int fd, net_addr_t *addrptr, net_port_t *portptr)
{
  struct sockaddr_in addr;
  int                ret;
  int                addrlen = sizeof(struct sockaddr_in);

  addrlen = sizeof(struct sockaddr_in);
  ret = syscall_getsockname(fd, (struct sockaddr *)&addr, (unsigned int *)&addrlen);

  if(ret == -1)
    return -1;

  if(addrptr) *addrptr = addr.sin_addr.s_addr;
  if(portptr) *portptr = addr.sin_port;

  return ret;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void net_dump(struct protocol *nptr)
{
  if(nptr == NULL)
  {
    dump(net_log, "[================ net summary ================]");

    dlink_foreach(&net_list, nptr)
    {
      dump(net_log, " #%u: [%u] %-20s (%s)",
           nptr->id, nptr->refcount, nptr->name,
           nptr->type == NET_SERVER ? "server" : "client");
    }

    dump(net_log, "[============= end of net summary ============]");
  }
  else
  {
    dump(net_log, "[================= net dump =================]");

    dump(net_log, "         id: #%u", nptr->id);
    dump(net_log, "   refcount: %u", nptr->refcount);
    dump(net_log, "       hash: %p", nptr->hash);
    dump(net_log, "       type: %s",
         nptr->type == NET_SERVER ? "server" : "client");
    dump(net_log, "       name: %s", nptr->name);
    dump(net_log, "    handler: %p", nptr->handler);

    dump(net_log, "[============== end of net dump =============]");
  }
}
