/* chaosircd - pi-networks irc server
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
 * $Id: service.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#define _GNU_SOURCE

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/defs.h>
#include <libchaos/io.h>
#include <libchaos/log.h>
#include <libchaos/mem.h>
#include <libchaos/net.h>
#include <libchaos/str.h>
#include <libchaos/hook.h>
#include <libchaos/timer.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/user.h>
#include <chaosircd/service.h>
#include <chaosircd/usermode.h>
#include <chaosircd/channel.h>
#include <chaosircd/numeric.h>

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
int           service_log;                   /* user log source */
struct sheap  service_heap;                  /* heap for struct service */
struct sheap  service_handler_heap;          /* heap for struct service_handler */
int           service_dirty;                 /* we need a garbage collect */
struct list   service_list;                  /* list with all of them */
uint32_t      service_seed;
uint32_t      service_id;

/* -------------------------------------------------------------------------- *
 * Initialize user module.                                                    *
 * -------------------------------------------------------------------------- */
void service_init(void)
{
  service_log = log_source_register("service");

  /* Zero all user lists */
  dlink_list_zero(&service_list);

  /* Setup user heap & timer */
  mem_static_create(&service_heap, sizeof(struct service), SERVICE_BLOCK_SIZE);
  mem_static_note(&service_heap, "service heap");
  mem_static_create(&service_handler_heap, sizeof(struct service_handler),
                    SERVICE_BLOCK_SIZE * 4);
  mem_static_note(&service_handler_heap, "service handler heap");

  service_seed = ~timer_mtime;
  service_dirty = 0;
  service_id = 0;

  log(service_log, L_status, "Initialised [service] module.");
}

/* -------------------------------------------------------------------------- *
 * Shutdown service module.                                                   *
 * -------------------------------------------------------------------------- */
void service_shutdown(void)
{
  struct service *svptr;
  struct service *next;

  log(service_log, L_status, "Shutting down [service] module.");

  /* Push all users */
  dlink_foreach_safe(&service_list, svptr, next)
     service_delete(svptr);

  /* Destroy static heap */
  mem_static_destroy(&service_handler_heap);
  mem_static_destroy(&service_heap);

  /* Unregister log source */
  log_source_unregister(service_log);
}

/* -------------------------------------------------------------------------- *
 * Create a new service block                                                 *
 * -------------------------------------------------------------------------- */
struct service *service_new(const char *name, const char *user,
                            const char *host, const char *info)
{
  struct service *svptr;

  /* Allocate and zero service block */
  svptr = mem_static_alloc(&service_heap);

  /* Initialise the block */

  svptr->refcount = 1;
  svptr->client = NULL;
  svptr->user = user_new(user, NULL);
  svptr->client = client_new_service(svptr->user, name, host, "*", info);
  svptr->client->service = svptr;
  svptr->user->client = svptr->client;
  svptr->id = service_id++;

  svptr->name[0] = '\0';

  dlink_list_zero(&svptr->handlers);

  service_set_name(svptr, name);

  if(name)
    strlcpy(svptr->name, name, sizeof(svptr->name));

  svptr->nhash = str_hash(svptr->name);

  /* Inform about the new service */
  debug(service_log, "New service block: %s", name);

  dlink_add_tail(&service_list, &svptr->node, svptr);

  return svptr;
}

/* -------------------------------------------------------------------------- *
 * Delete a service block                                                     *
 * -------------------------------------------------------------------------- */
void service_delete(struct service *svptr)
{
  service_release(svptr);

  /* Unlink from main list and typed list */
  dlink_delete(&service_list, &svptr->node);

  debug(service_log, "Deleted service block: %s", svptr->name);

  /* Free the block */
  mem_static_free(&service_heap, svptr);
  mem_static_collect(&service_heap);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct service *service_find_id(uint32_t id)
{
  struct service *svptr;

  dlink_foreach(&service_list, svptr)
    if(svptr->id == id)
      return svptr;

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct service *service_find_name(const char *name)
{
  struct service *svptr;
  uint32_t        hash;

  hash = str_ihash(name);

  dlink_foreach(&service_list, svptr)
  {
    if(svptr->nhash == hash)
    {
      if(!str_icmp(svptr->name, name))
        return svptr;
    }
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void service_set_name(struct service *svptr, const char *name)
{
  if(name && name[0])
  {
    strlcpy(svptr->name, name, sizeof(svptr->name));
    svptr->nhash = str_ihash(svptr->name);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct service_handler *service_register(struct service *svptr, const char *msg,
                                         service_callback_t *callback)
{
  struct service_handler *svhptr;

  svhptr = mem_static_alloc(&service_handler_heap);

  strlcpy(svhptr->name, msg, sizeof(svhptr->name));
  svhptr->hash = str_ihash(svhptr->name);
  svhptr->callback = callback;

  dlink_add_tail(&svptr->handlers, &svhptr->node, svhptr);

  return svhptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct service_handler *service_handler_find(struct service *svptr, const char *cmd)
{
  struct service_handler *svhptr;
  uint32_t                hash;

  hash = str_ihash(cmd);

  dlink_foreach(&svptr->handlers, svhptr)
    if(svhptr->hash == hash)
    {
      if(!str_icmp(svhptr->name, cmd))
        return svhptr;
    }

  return NULL;
}


/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void service_vhandle(struct service *svptr, struct lclient *lcptr,
                     struct client  *cptr,  struct channel *chptr,
                     const char     *cmd,   const char     *format,
                     va_list         args)
{
  struct service_handler *svhptr;
  char                    buffer[IRCD_LINELEN + 1];

  if((svhptr = service_handler_find(svptr, cmd)) == NULL)
    return;

  str_vsnprintf(buffer, sizeof(buffer), format, args);

  if(svhptr->callback)
    svhptr->callback(lcptr, cptr, chptr, buffer);
}

void service_handle(struct service *svptr, struct lclient *lcptr,
                    struct client  *cptr,  struct channel *chptr,
                    const char     *cmd,   const char     *format, ...)
{
  va_list args;

  va_start(args, format);
  service_vhandle(svptr, lcptr, cptr, chptr, cmd, format, args);
  va_end(args);
}

/* -------------------------------------------------------------------------- *
 * Loose all references                                                       *
 * -------------------------------------------------------------------------- */
void service_release(struct service *svptr)
{
  if(svptr->client)
  {
    client_exit(NULL, svptr->client, "service released");
    svptr->client = NULL;
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct service *service_pop(struct service *svptr)
{
  if(svptr)
  {
    if(!svptr->refcount)
      log(service_log, L_warning, "Poping deprecated service: %s",
          svptr->name);

    svptr->refcount++;
  }

  return svptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct service *service_push(struct service **svptrptr)
{
  if(*svptrptr)
  {
    if((*svptrptr)->refcount == 0)
    {
      log(service_log, L_warning, "Trying to push deprecated service: %s",
          (*svptrptr)->name);
    }
    else
    {
      if(--(*svptrptr)->refcount == 0)
        service_release(*svptrptr);

      (*svptrptr) = NULL;
    }
  }

  return *svptrptr;
}


/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void service_dump(struct service *svptr)
{
  if(svptr == NULL)
  {
    struct node *node;

    dump(service_log, "[============== service summary ===============]");

    dlink_foreach_data(&service_list, node, svptr)
      dump(service_log, " #%u: [%u] %-20s (%s)",
            svptr->id, svptr->refcount,
            svptr->name[0] ? svptr->name : "<unknown>",
            svptr->client ? svptr->client->name : "?");

    dump(service_log, "[=========== end of service summary ===========]");
  }
  else
  {
    dump(service_log, "[============== service dump ===============]");
    dump(service_log, "         id: #%u", svptr->id);
    dump(service_log, "   refcount: %u", svptr->refcount);
    dump(service_log, "      uhash: %p", svptr->uhash);
    dump(service_log, "      nhash: %p", svptr->nhash);
    dump(service_log, "     client: %s [%i]",
          svptr->client ? svptr->client->name : "(nil)",
          svptr->client ? svptr->client->refcount : 0);
    dump(service_log, "       name: %s", svptr->name);
    dump(service_log, "[=========== end of service dump ===========]");
  }
}
