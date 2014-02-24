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
 * $Id: user.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
#include <chaosircd/oper.h>
#include <chaosircd/usermode.h>
#include <chaosircd/channel.h>
#include <chaosircd/chanuser.h>
#include <chaosircd/numeric.h>

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
int           user_log;                   /* user log source */
struct sheap  user_heap;                  /* heap for struct user */
int           user_dirty;                 /* we need a garbage collect */
struct list   user_list;                  /* list with all of them */
struct list   user_lists[USER_HASH_SIZE]; /* hashed list */
uint32_t      user_seed;
uint32_t      user_id;
char          user_readbuf[IRCD_BUFSIZE];
const char    user_base[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV"
                            "WXYZabcdefghijklmnopqrstuvwxyzΩæ"
                            "ø¿¡¬√ƒ≈∆«»… ÀÃÕŒœ–—“”‘’÷◊ÿŸ⁄€‹›ﬁ"
                            "ﬂ‡·‚„‰ÂÊÁËÈÍÎÏÌÓÔÒÚÛÙıˆ˜¯˘˙˚¸˝˛";

/* ------------------------------------------------------------------------ */
int user_get_log() { return user_log; }

/* -------------------------------------------------------------------------- *
 * Initialize user module.                                                    *
 * -------------------------------------------------------------------------- */
void user_init(void)
{
  uint32_t i;
  
  user_log = log_source_register("user");
  
  /* Zero all user lists */
  dlink_list_zero(&user_list);
  
  for(i = 0; i < USER_HASH_SIZE; i++)
    dlink_list_zero(&user_lists[i]);
  
  /* Setup user heap & timer */
  mem_static_create(&user_heap, sizeof(struct user), USER_BLOCK_SIZE);
  mem_static_note(&user_heap, "user heap");
  
  user_seed = ~timer_mtime;
  user_dirty = 0;
  user_id = 0;
  
  log(user_log, L_status, "Initialised [user] module.");
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define ROR(v, n) ((v >> ((n) & 0x1f)) | (v << (32 - ((n) & 0x1f))))
#define ROL(v, n) ((v >> ((n) & 0x1f)) | (v << (32 - ((n) & 0x1f))))
static uint32_t user_random(void)
{
  int      it;
  int      i;
  uint32_t ns = timer_mtime;
  
  it = (ns & 0x1f) + 0x20;

  for(i = 0; i < it; i++)
  {
    ns = ROL(ns, user_seed);
   
    if(ns & 0x01)
      user_seed -= 0x35014541;
    else
      user_seed += 0x21524110;
    
    ns = ROR(ns, user_seed >> 27);
    
    if(user_seed & 0x02)
      ns ^= user_seed;
    else
      ns -= user_seed;
    
    user_seed = ROL(user_seed, ns >> 5);
    
    if(ns & 0x04)
      user_seed += ns;
    else
      user_seed ^= ns;

    ns = ROL(ns, user_seed >> 13);
  
    if(user_seed & 0x08)
      ns += user_seed;
    else
      user_seed -= ns;
  }
  
  return ns;
}
#undef ROR
#undef ROL

/* -------------------------------------------------------------------------- *
 * Shutdown user module.                                                      *
 * -------------------------------------------------------------------------- */
void user_shutdown(void)
{
  struct user *uptr;
  struct user *next;
  
  log(user_log, L_status, "Shutting down [user] module.");
  
  /* Push all users */
  dlink_foreach_safe(&user_list, uptr, next)
    user_delete(uptr);
  
  /* Destroy static heap */
  mem_static_destroy(&user_heap);
  
  /* Unregister log source */
  log_source_unregister(user_log);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void user_uid(struct user *uptr)
{
  uint64_t val = 0;
  size_t   i;

  do
  {
    /* Create ID */
    for(i = 0; i < IRCD_IDLEN; i++)
    {
      if(!(i & 0x03))
        val = user_random();
      
      uptr->uid[i] = user_base[(val & 0x7Full)];
      
      val >>= 7;
    }
    
    uptr->uid[i] = '\0';
  }
  while(user_find_uid(uptr->uid));
  
  /* Hash it */
  uptr->uhash = str_hash(uptr->uid);
}

/* -------------------------------------------------------------------------- *
 * Create a new user block                                                    *
 * -------------------------------------------------------------------------- */
struct user *user_new(const char *name, const char *uid)
{
  struct user *uptr;
  
  /* Allocate and zero user block */
  uptr = mem_static_alloc(&user_heap);
  
  /* Initialise the block */  
  
  uptr->refcount = 1;
  uptr->client = NULL;
  uptr->oper = NULL;
  uptr->id = user_id++;
  uptr->modes = 0ULL;
  
  uptr->name[0] = '\0';
  uptr->uid [0] = '\0';
  uptr->away[0] = '\0';
  uptr->mode[0] = '\0';
  
  dlink_list_zero(&uptr->channels);
  dlink_list_zero(&uptr->invites);
  
  user_set_name(uptr, name);
  
  if(name)
    strlcpy(uptr->name, name, sizeof(uptr->name));
  
  uptr->nhash = str_hash(uptr->name);
  
  if(uid)
  {
    strlcpy(uptr->uid, uid, sizeof(uptr->uid));
    
    uptr->uhash = str_hash(uptr->uid);
  }
  else
  {
    user_uid(uptr);
  }
  
  /* Inform about the new user */
  debug(user_log, "New user block: %s (%s)", name, uptr->uid);

  dlink_add_tail(&user_list, &uptr->node, uptr);

  dlink_add_tail(&user_lists[uptr->uhash % USER_HASH_SIZE], &uptr->hnode, uptr);
  
  return uptr;
}

/* -------------------------------------------------------------------------- *
 * Delete a user block                                                        *
 * -------------------------------------------------------------------------- */
void user_delete(struct user *uptr)
{
  user_release(uptr);
  
  /* Unlink from main list and typed list */
  dlink_delete(&user_lists[uptr->uhash % USER_HASH_SIZE], &uptr->hnode);
  dlink_delete(&user_list, &uptr->node);
  
  debug(user_log, "Deleted user block: %s", uptr->name);
  
  /* Free the block */
  mem_static_free(&user_heap, uptr);
  mem_static_collect(&user_heap);
}  

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct user *user_find_id(uint32_t id)
{
  struct user *uptr;
  
  dlink_foreach(&user_list, uptr)
    if(uptr->id == id)
      return uptr;
  
  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct user *user_find_uid(const char *uid)
{
  struct user *uptr = NULL;
  struct node *node;
  uint32_t     hash;
  char         uidbuf[IRCD_IDLEN + 1];
  
  strlcpy(uidbuf, uid, sizeof(uidbuf));
  
  hash = str_hash(uidbuf);

  dlink_foreach_data(&user_lists[hash % USER_HASH_SIZE], node, uptr)
  {
    if(uptr->uhash == hash)
    {
      if(!str_cmp(uptr->uid, uidbuf))
        return uptr;
    }
  }
  
  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct user *user_find_name(const char *name)
{
  struct user *uptr;
  uint32_t     hash;
  
  hash = str_ihash(name);
  
  dlink_foreach(&user_list, uptr)
  {
    if(uptr->nhash == hash)
    {
      if(!str_icmp(uptr->name, name))
        return uptr;
    }
  }
    
  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void user_set_name(struct user *uptr, const char *name)
{
  if(name && name[0])
  {
    strlcpy(uptr->name, name, sizeof(uptr->name));
    uptr->nhash = str_ihash(uptr->name);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void user_whois(struct client *cptr, struct user *auptr)
{
  struct client *acptr = auptr->client;
  
  numeric_send(cptr, RPL_WHOISUSER, acptr->name, 
               auptr->name, acptr->host, acptr->info);

  if(auptr->channels.size)
    chanuser_whois(cptr, auptr);
  
  numeric_send(cptr, RPL_WHOISSERVER, acptr->name, 
               client_me->name, client_me->info);
  
  hooks_call(user_whois, HOOK_DEFAULT, cptr, auptr);
  
  if(auptr->away[0])
    numeric_send(cptr, RPL_AWAY, acptr->name,
                 auptr->away);
  
  if(client_is_local(acptr) && !client_is_service(acptr))
    numeric_send(cptr, RPL_WHOISIDLE, acptr->name,
                 (timer_systime - acptr->lastmsg), acptr->created);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void user_invite(struct user *uptr, struct channel *chptr)
{
  struct invite *ivptr;
  
  dlink_foreach(&uptr->invites, ivptr)
  {
    if(ivptr->channel == chptr)
      return;
  }
  
  ivptr = mem_static_alloc(&channel_invite_heap);
  
  ivptr->user = uptr;
  ivptr->channel = chptr;
  
  dlink_add_head(&chptr->invites, &ivptr->cnode, ivptr);
  dlink_add_tail(&uptr->invites, &ivptr->unode, ivptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void user_uninvite(struct invite *ivptr)
{
  dlink_delete(&ivptr->channel->invites, &ivptr->cnode);
  dlink_delete(&ivptr->user->invites, &ivptr->unode);
  mem_static_free(&channel_invite_heap, ivptr);
}

/* -------------------------------------------------------------------------- *
 * Loose all references                                                       *
 * -------------------------------------------------------------------------- */
void user_release(struct user *uptr)
{
  struct chanuser *cuptr;
  struct invite   *ivptr;
  struct node     *nptr;
  
  dlink_foreach_safe(&uptr->channels, cuptr, nptr)
    chanuser_delete(cuptr);
  
  dlink_foreach_safe(&uptr->invites, ivptr, nptr)
    user_uninvite(ivptr);
  
  usermode_unlinkall_user(uptr);
  
  oper_push(&uptr->oper);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct user *user_pop(struct user *uptr)
{
  if(uptr)
  {
    if(!uptr->refcount)
      log(user_log, L_warning, "Poping deprecated user: %s",
          uptr->name);

    uptr->refcount++;
  }
  
  return uptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct user *user_push(struct user **uptrptr)
{
  if(*uptrptr)
  {
    if((*uptrptr)->refcount == 0)
    {
      log(user_log, L_warning, "Trying to push deprecated user: %s",
          (*uptrptr)->name);
    }
    else
    {
      if(--(*uptrptr)->refcount == 0)
        user_release(*uptrptr);
      
      (*uptrptr) = NULL;
    }
  }
  
  return *uptrptr;
}


/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void user_dump(struct user *uptr)
{
  if(uptr == NULL)
  {
    struct node *node;
    
    dump(user_log, "[============== user summary ===============]");
    
    dlink_foreach_data(&user_list, node, uptr)
      dump(user_log, " #%u: [%u] %-20s %-8s (%s)",
            uptr->id, uptr->refcount,
            uptr->name[0] ? uptr->name : "<unknown>",
            uptr->uid, 
            uptr->client ? uptr->client->name : "?");
    
    dump(user_log, "[=========== end of user summary ===========]");
  }
  else
  {
    dump(user_log, "[============== user dump ===============]");
    dump(user_log, "         id: #%u", uptr->id);
    dump(user_log, "   refcount: %u", uptr->refcount);
    dump(user_log, "      uhash: %p", uptr->uhash);
    dump(user_log, "      nhash: %p", uptr->nhash);
    dump(user_log, "     client: %s [%i]",
          uptr->client ? uptr->client->name : "(nil)",
          uptr->client ? uptr->client->refcount : 0);
    dump(user_log, "       oper: %s [%i]",
          uptr->oper ? uptr->oper->name : "(nil)",
          uptr->oper ? uptr->oper->refcount : 0);
    dump(user_log, "   channels: %u links", uptr->channels.size);
    dump(user_log, "    invites: %u links", uptr->invites.size);
    dump(user_log, "      modes: %llu", uptr->modes);
    dump(user_log, "       name: %s", uptr->name);
    dump(user_log, "        uid: %s", uptr->uid);
    dump(user_log, "       away: %s", uptr->away);
    dump(user_log, "       mode: %s", uptr->mode);
    dump(user_log, "[=========== end of user dump ===========]");
  }
}
