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
 * $Id: listen.c,v 1.5 2006/09/28 08:38:31 roman Exp $
 */

#define _GNU_SOURCE

/* ------------------------------------------------------------------------ *
 * Library headers                                                          *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/listen.h"
#include "libchaos/filter.h"
#include "libchaos/timer.h"
#include "libchaos/hook.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/net.h"
#include "libchaos/str.h"
#include "libchaos/ssl.h"
#include "libchaos/io.h"

/* ------------------------------------------------------------------------ *
 * System headers                                                           *
 * ------------------------------------------------------------------------ */
#include "../config.h"

#ifdef HAVE_CYGWIN_IN_H
#include <cygwin/in.h>
#endif /* HAVE_CYGWIN_IN_H */

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

/* ------------------------------------------------------------------------ *
 * Global variables                                                         *
 * ------------------------------------------------------------------------ */
int           listen_log;
struct sheap  listen_heap;       /* heap containing listen blocks */
struct list   listen_list;       /* list linking listen blocks */
uint32_t      listen_id;
int           listen_dirty;

/* ------------------------------------------------------------------------ */
int listen_get_log() { return listen_log; }

/* ------------------------------------------------------------------------ *
 * Accept incoming connection find the listener and call the protocol handler *
 * ------------------------------------------------------------------------ */
static void listen_accept(int fd, void *ptr)
{
  struct listen *listen;
  int            newfd;
  net_addr_t     addr;
  net_port_t     port;

  listen = ptr;
  newfd = net_accept(fd, &addr, &port);

  /* Accept was successful, call protocol handler */
  if(newfd >= 0)
  {
    listen->status = LISTEN_CONNECTION;

    listen->addr_remote = addr;
    listen->port_remote = port;

    /* If the listener is SSL enabled, then try to create
       a new SSL instance and initiate the SSL handshake. */
    if(listen->ssl)
    {
      if(ssl_new(newfd, listen->ctxt))
      {
        /* Perhaps we don't support SSL or we are out of memory */
        listen->status = LISTEN_ERROR;
        io_destroy(newfd);
        return;
      }

      if(ssl_accept(newfd))
      {
        log(listen_log, L_warning, "SSL handshake error on %s: %s",
            listen->name, ssl_strerror(newfd));

        ssl_close(newfd);
        io_destroy(newfd);
				return;
      }
    }
  }
  else
  {
    if(syscall_errno == EAGAIN)
      return;

    listen->status = LISTEN_ERROR;

    listen->addr_remote = NET_ADDR_ANY;
    listen->port_remote = 0;
  }

  if(listen->proto)
  {
    if(listen->proto->handler)
    {
      listen->proto->handler(newfd, listen, listen->args);
      return;
    }
  }
}

/* ------------------------------------------------------------------------ *
 * Initialize listener heap and add garbage collect timer.                  *
 * ------------------------------------------------------------------------ */
void listen_init(void)
{
  listen_log = log_source_register("listen");

  dlink_list_zero(&listen_list);

  listen_id = 0;
  listen_dirty = 0;

  mem_static_create(&listen_heap, sizeof(struct listen), LISTEN_BLOCK_SIZE);
  mem_static_note(&listen_heap, "listen block heap");
}

/* ------------------------------------------------------------------------ *
 * Destroy listener heap and cancel timer.                                  *
 * ------------------------------------------------------------------------ */
void listen_shutdown(void)
{
  struct listen *liptr;
  struct listen *next;

  /* Remove all listen blocks */
  dlink_foreach_safe(&listen_list, liptr, next)
  {
    if(liptr->refcount)
      liptr->refcount--;

    listen_delete(liptr);
  }

  /* Destroy listen block heap */
  mem_static_destroy(&listen_heap);

  /* Unregister log source */
  log_source_unregister(listen_log);
}

/* ------------------------------------------------------------------------ *
 * Collect listen block garbage.                                            *
 * ------------------------------------------------------------------------ */
int listen_collect(void)
{
  struct listen *cnptr;
  struct listen *next;
  size_t         n = 0;

  if(listen_dirty)
  {
    /* Report verbose */
    log(listen_log, L_verbose, "Doing garbage collect for [listen] module.");

    /* Free all listen blocks with a zero refcount */
    dlink_foreach_safe(&listen_list, cnptr, next)
    {
      if(!cnptr->refcount)
      {
        listen_delete(cnptr);

        n++;
      }
    }

    /* Collect garbage on listen_heap */
    mem_static_collect(&listen_heap);

    listen_dirty = 0;
  }

  return 0;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void listen_default(struct listen *listen)
{
  dlink_node_zero(&listen->node);

  strcpy(listen->address, "0.0.0.0");
  listen->port = 1024;
  listen->backlog = 5;
  listen->ssl = 0;
  listen->proto = NULL;
  listen->args = NULL;
  strcpy(listen->name, "0.0.0.0:1024");

  listen->fd = -1;
  listen->id = 0;
  listen->status = LISTEN_IDLE;
  listen->refcount = 0;
  listen->lhash = 0;
  listen->nhash = 0;
  listen->addr_local = NET_ADDR_ANY;
  listen->addr_remote = NET_ADDR_ANY;
  listen->port_local = 0;
  listen->port_remote = 0;
  listen->filter = NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct listen *listen_add(const char *address, uint16_t    port,
                          int         backlog, int         ssl,
                          const char *context, const char *protocol)
{
  struct protocol *p;
  net_addr_t       addr;
  struct listen   *listen;
  int              fd;

  if(!net_aton(address, &addr))
    return NULL;

  if((p = net_find(NET_SERVER, protocol)) == NULL)
    return NULL;

  fd = net_socket(NET_ADDRESS_IPV4, NET_SOCKET_STREAM);

  if(fd == -1)
    return NULL;

  if(net_bind(fd, addr, port))
  {
    io_destroy(fd);
    hooks_call(listen_add, HOOK_DEFAULT, address, port,
               syscall_strerror(syscall_errno));
    return NULL;
  }

  listen = mem_static_alloc(&listen_heap);

  if(net_listen(fd, backlog, listen_accept, listen))
  {
    io_destroy(fd);
    mem_static_free(&listen_heap, listen);
    return NULL;
  }

  listen->fd = fd;
  listen->backlog = backlog;
  listen->proto = net_pop(p);
  listen->lhash = str_ihash(address) ^ port;
  strlcpy(listen->address, address, sizeof(listen->address));
  listen->port = port;
  listen->id = listen_id++;
  listen->refcount = 1;

  listen->port_local = port;
  listen->addr_local = addr;
  listen->port_remote = 0;
  listen->addr_remote = NET_ADDR_ANY;
  listen->filter = NULL;

  listen->ssl = 0;

  if(ssl)
  {
    if(context == NULL || context[0] == '\0')
    {
      log(listen_log, L_warning, "SSL listeners need a context!",
          listen->context);
    }
    else
    {
      strlcpy(listen->context, context, sizeof(listen->context));

      if((listen->ctxt = ssl_find_name(listen->context)))
        listen->ssl = 1;
      else
        log(listen_log, L_warning, "Could not find SSL context '%s'.",
            listen->context);
    }
  }

  str_snprintf(listen->name, sizeof(listen->name), "%s:%u", address, port);

  listen->nhash = str_ihash(listen->name);

  io_note(fd, "listener on %s", listen->name);

  dlink_add_tail(&listen_list, &listen->node, listen);

  log(listen_log, L_status, "Added listen block: %s", listen->name);

  hooks_call(listen_add, HOOK_2ND, listen);

  return listen;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int listen_update(struct listen *listen,  int         backlog, int ssl,
                  const char    *context, const char *protocol)
{
  struct protocol *p;

  if((p = net_find(NET_SERVER, protocol)) == NULL)
    return -1;

  listen->backlog = backlog;
  listen->ssl = ssl;

  if(context)
  {
    strlcpy(listen->context, context, sizeof(listen->context));
#ifdef HAVE_SSL
    if((listen->ctxt = ssl_find_name(listen->context)))
      listen->ssl = ssl;
#endif /* HAVE_SSL */
  }

  listen->proto = p;

  log(listen_log, L_status, "Updated listen block: %s", listen->name);

  return 0;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void listen_delete(struct listen *listen)
{
  log(listen_log, L_status, "Deleting listen block: %s", listen->name);

  net_push(&listen->proto);

  if(listen->fd > -1)
    io_destroy(listen->fd);

  if(listen->args)
    free(listen->args);

  dlink_delete(&listen_list, (struct node *)listen);

  mem_static_free(&listen_heap, listen);
}

 /* ------------------------------------------------------------------------ *
  * Loose all references                                                     *
  * ------------------------------------------------------------------------ */
void listen_release(struct listen *liptr)
{
  io_push(&liptr->fd);
  net_push(&liptr->proto);

  listen_dirty = 1;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct listen *listen_pop(struct listen *liptr)
{
  if(liptr)
  {
    if(!liptr->refcount)
      log(listen_log, L_warning, "Poping deprecated listen: %s",
          liptr->name);

    liptr->refcount++;
  }

  return liptr;
}


/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct listen *listen_push(struct listen **liptrptr)
{
  if(*liptrptr)
  {
    if(!(*liptrptr)->refcount)
    {
      log(listen_log, L_warning, "Trying to push deprecated listen %s",
          (*liptrptr)->name);
    }
    else
    {
      if(--(*liptrptr)->refcount == 0)
        listen_release(*liptrptr);
    }

    (*liptrptr) = NULL;
  }

  return *liptrptr;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct listen *listen_find(const char *address, uint16_t port)
{
  struct listen *lptr;
  hash_t         lhash;
  
  lhash = str_ihash(address) ^ port;

  dlink_foreach(&listen_list, lptr)
  {
    if(lptr->lhash == lhash)
    {
      if(lptr->port == port && !str_icmp(lptr->address, address))
        return lptr;
    }
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void listen_set_args(struct listen *listen, const void *argbuf, size_t n)
{
  if(!listen->args)
    listen->args = malloc(n);

  memcpy(listen->args, argbuf, n);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void listen_get_args(struct listen *listen, void *argbuf, size_t n)
{
  if(listen->args)
    memcpy(argbuf, listen->args, n);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void listen_set_name(struct listen *listen, const char *name)
{
  strlcpy(listen->name, name, sizeof(listen->name));

  listen->nhash = str_ihash(listen->name);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
const char *listen_get_name(struct listen *listen)
{
  return listen->name;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct listen *listen_find_name(const char *name)
{
  struct node   *node;
  struct listen *listen;
  hash_t         nhash;
  
  nhash = str_ihash(name);

  dlink_foreach(&listen_list, node)
  {
    listen = node->data;

    if(listen->nhash == nhash)
    {
      if(!str_icmp(listen->name, name))
        return listen;
    }
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct listen *listen_find_id(uint32_t id)
{
  struct listen *lptr;

  dlink_foreach(&listen_list, lptr)
  {
    if(lptr->id == id)
      return lptr;
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int listen_attach_filter(struct listen *lptr, struct filter *fptr)
{
  lptr->filter = fptr;

  return filter_attach_socket(fptr, lptr->fd);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int listen_detach_filter(struct listen *lptr)
{
  struct filter *fptr;

  if(lptr->filter)
  {
    fptr = lptr->filter;

    lptr->filter = NULL;

    return filter_detach_socket(fptr, lptr->fd);
  }

  return -1;
}

/* ------------------------------------------------------------------------ *
 * Dump listeners and listen heap.                                          *
 * ------------------------------------------------------------------------ */
void listen_dump(struct listen *lptr)
{
  if(lptr == NULL)
  {
    dump(listen_log, "[============== listen summary ===============]");

    dlink_foreach(&listen_list, lptr)
      dump(listen_log, " #%03u: [%u] %-20s (%s:%u) (%s)",
            lptr->id,
            lptr->refcount,
            lptr->name,
            net_ntoa(lptr->addr_local),
            (uint32_t)lptr->port_local,
            lptr->proto ? lptr->proto->name : "<none>");

    dump(listen_log, "[========== end of listen summary ============]");
  }
  else
  {
    dump(listen_log, "[============== listen dump ===============]");
    dump(listen_log, "         id: #%u", lptr->id);
    dump(listen_log, "   refcount: %u", lptr->refcount);
    dump(listen_log, "      lhash: %p", lptr->lhash);
    dump(listen_log, "      nhash: %p", lptr->nhash);
    dump(listen_log, "       port: %u", (uint32_t)lptr->port);
    dump(listen_log, "        ssl: %s", lptr->ssl ? "yes" : "no");
    dump(listen_log, "      proto: %s", lptr->proto ?
                                         lptr->proto->name : "<none>");
    dump(listen_log, "       args: %p", lptr->args);
    dump(listen_log, "       name: %s", lptr->name);
    dump(listen_log, "    address: %s", lptr->address);
    dump(listen_log, "    context: %s", lptr->context);
    dump(listen_log, "         fd: %i", lptr->fd);
    dump(listen_log, "      local: %s:%u", net_ntoa(lptr->addr_local),
                                            (uint32_t)lptr->port_local);
    dump(listen_log, "     remote: %s:%u", net_ntoa(lptr->addr_remote),
                                            (uint32_t)lptr->port_remote);
    dump(listen_log, "     status: %i", lptr->status);
    dump(listen_log, "       ctxt: %p", lptr->ctxt);

    dump(listen_log, "[========== end of listen dump ============]");
  }
}
