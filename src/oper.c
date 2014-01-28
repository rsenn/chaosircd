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
 * $Id: oper.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#define _GNU_SOURCE

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/defs.h>
#include <libchaos/io.h>
#include <libchaos/timer.h>
#include <libchaos/hook.h>
#include <libchaos/log.h>
#include <libchaos/mem.h>
#include <libchaos/net.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/numeric.h>
#include <chaosircd/lclient.h>
#include <chaosircd/client.h>
#include <chaosircd/ircd.h>
#include <chaosircd/oper.h>
#include <chaosircd/user.h>
#include <chaosircd/msg.h>

/* -------------------------------------------------------------------------- *
 * Heap for the opers.                                                        *
 * -------------------------------------------------------------------------- */
int           oper_log;
struct sheap  oper_heap;
struct timer *oper_timer;
struct list   oper_list;
struct dlog  *oper_drain;

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void oper_drain_cb(uint64_t    src,   int         lvl,
                          const char *level, const char *source, 
                          const char *date,  const char *msg)
{
  struct client *cptr = NULL;
  struct oper   *optr;
  struct node   *node;
  
  dlink_foreach(&oper_list, optr)
  {
    if((optr->sources & src) && (optr->level >= lvl))
    {
      dlink_foreach_data(&optr->online, node, cptr)
      {
        client_send(cptr, ":%s NOTICE %s :<%s> - %s",
                    client_me->name, cptr->name, source, msg);
      }
    }
  }
}

/* -------------------------------------------------------------------------- *
 * Initialize oper heaps and add garbage collect timer.                       *
 * -------------------------------------------------------------------------- */
void oper_init(void)
{
  oper_log = log_source_register("oper");
  
  dlink_list_zero(&oper_list);
  
  mem_static_create(&oper_heap, sizeof(struct oper), OPER_BLOCK_SIZE);
  mem_static_note(&oper_heap, "oper heap");

  oper_drain = log_drain_callback(oper_drain_cb, LOG_ALL, L_debug);
  
  log(oper_log, L_status, "Initialised [oper] module.");
}

/* -------------------------------------------------------------------------- *
 * Destroy class heap and cancel timer.                                       *
 * -------------------------------------------------------------------------- */
void oper_shutdown(void)
{
  struct node *node;
  struct node *next;
  
  log(oper_log, L_status, "Shutting down [oper] module...");
  
  dlink_foreach_safe(&oper_list, node, next)
    oper_delete((struct oper *)node);
  
  mem_static_destroy(&oper_heap);
  
  log_drain_delete(oper_drain);
  log_source_unregister(oper_log);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void oper_default(struct oper *optr)
{
  strcpy(optr->name, "default");
  optr->passwd[0] = 0;
  optr->flags = 0;
  optr->sources = 0LLU;
  optr->level = 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct oper *oper_add(const char   *name,    const char *passwd,
                      struct class *clptr,   int         level,
                      uint64_t      sources, uint32_t    flags)
{
  struct oper *optr;
  
  /* allocate, zero and link oper struct */
  optr = mem_static_alloc(&oper_heap);
  
  memset(optr, 0, sizeof(struct oper));
  
  dlink_add_tail(&oper_list, &optr->node, optr);

  strlcpy(optr->name, name, sizeof(optr->name));
  
  optr->hash = str_ihash(optr->name);

  if(passwd[0])
    strlcpy(optr->passwd, passwd, sizeof(optr->passwd));

  optr->level = level;
  optr->sources = sources;
  optr->flags = flags;
  optr->clptr = class_pop(clptr);
  optr->refcount = 1;
  
  log(oper_log, L_status, "Added oper block: %s", name);
  
  return optr;
}     
     
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int oper_update(struct oper  *optr,    const char *passwd,
                struct class *clptr,   int         level,
                uint64_t      sources, uint32_t    flags)
{
  strlcpy(optr->passwd, passwd, sizeof(optr->passwd));

  if(optr->clptr != clptr)
  {
    class_push(&optr->clptr);
    optr->clptr = class_pop(clptr);
  }

  optr->flags = flags;
  optr->level = level;
  optr->sources = sources;
  
  log(oper_log, L_status, "Updated oper block: %s", optr->name);
  
  return 0;
}     
     
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void oper_delete(struct oper *optr)
{
  log(oper_log, L_status, "Removed oper block: %s", optr->name);
  
  dlink_delete(&oper_list, &optr->node);
  
  mem_static_free(&oper_heap, optr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct oper *oper_find(const char *name)
{
  struct node *node;
  struct oper *optr;
  uint32_t     hash;
  
  hash = str_ihash(name);
  
  dlink_foreach(&oper_list, node)
  {
    optr = (struct oper *)node;
    
    if(optr->name[0] && hash == optr->hash)
    {
      if(!str_icmp(optr->name, name))
        return optr;
    }
  }
  
  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void oper_release(struct oper *optr)
{
  
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct oper *oper_pop(struct oper *optr)
{
  if(optr)
  {
    if(!optr->refcount)
      log(oper_log, L_warning, "Poping deprecated oper: %s",
          optr->name);
    
    optr->refcount++;
  }
  
  return optr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct oper *oper_push(struct oper **optrptr)
{
  if(*optrptr)
  {
    if((*optrptr)->refcount == 0)
    {
      log(oper_log, L_warning, "Trying to push deprecated user: %s",
          (*optrptr)->name);
    }
    else
    {
      if(--(*optrptr)->refcount == 0)
        oper_release(*optrptr);
      
      (*optrptr) = NULL;
    }
  }
  
  return *optrptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void oper_up(struct oper *optr, struct client *cptr)
{
  if(client_is_local(cptr))
  {
    struct node *nptr;
    
    lclient_set_type(cptr->lclient, LCLIENT_OPER);

    numeric_send(cptr, RPL_YOUREOPER);
    
    nptr = dlink_node_new();
    dlink_add_head(&optr->online, nptr, cptr);
    
    if(cptr->user)
      cptr->user->oper = oper_pop(optr);
    
    hooks_call(oper_up, HOOK_DEFAULT, cptr->user);
  }
  
  log(oper_log, L_status, "%s (%s@%s) has become a k-rad whore.",
      cptr->name, cptr->user->name, cptr->host);
}
  
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void oper_down(struct oper *optr, struct client *cptr)
{
  if(client_is_local(cptr))
  {
    dlink_find_delete(&optr->online, cptr);
    
    if(cptr->lclient)
      lclient_set_type(cptr->lclient, LCLIENT_OPER);
    
    if(cptr->source)
      numeric_send(cptr, RPL_NOTOPERANYMORE);
    
    if(cptr->user)
      oper_push(&cptr->user->oper);
  }
}
  
/* -------------------------------------------------------------------------- *
 * Dump opers and oper heap.                                                  *
 * -------------------------------------------------------------------------- */
#ifdef DEBUG
void oper_dump(void)
{
  struct node *node;
  struct oper *c;
  size_t       i = 1;
  
  debug(oper_log, "--- oper dump ---");
  
  dlink_foreach(&oper_list, node)
  {
    c = (struct oper *)node;
    
/*    debug("--------- oper entry %02u ---------", i);
    debug("name: %s", c->name);*/
    
    i++;
  }
  
  debug(oper_log, "--- end of oper dump ---");
  
  mem_static_dump(&oper_heap);
}
#endif /* DEBUG */
