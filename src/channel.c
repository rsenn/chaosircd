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
 * $Id: channel.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#define _GNU_SOURCE

/* -------------------------------------------------------------------------- *
 * Library headers.                                                           *
 * -------------------------------------------------------------------------- */
#include <libchaos/defs.h>
#include <libchaos/io.h>
#include <libchaos/dlink.h>
#include <libchaos/timer.h>
#include <libchaos/hook.h>
#include <libchaos/log.h>
#include <libchaos/mem.h>
#include <libchaos/net.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * Core headers.                                                              *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/user.h>
#include <chaosircd/chanuser.h>
#include <chaosircd/chanmode.h>
#include <chaosircd/channel.h>
#include <chaosircd/lclient.h>
#include <chaosircd/client.h>
#include <chaosircd/numeric.h>
#include <chaosircd/server.h>
#include <chaosircd/service.h>
#include <chaosircd/msg.h>

/* -------------------------------------------------------------------------- *
 * Global variables.                                                          *
 * -------------------------------------------------------------------------- */
int           channel_log;
struct sheap  channel_heap;
struct sheap  channel_invite_heap;
struct list   channel_list;
struct list   channel_lists[CHANNEL_HASH_SIZE];
uint32_t      channel_id;
uint32_t      channel_serial;

/* ------------------------------------------------------------------------ */
int channel_get_log() { return channel_log; }

/* -------------------------------------------------------------------------- *
 * Initialize channel heaps and add garbage collect timer.                    *
 * -------------------------------------------------------------------------- */
void channel_init(void)
{
  size_t i;
  
  channel_log = log_source_register("channel");

  channel_id = 0;
  channel_serial = 0;
  
  dlink_list_zero(&channel_list);
  
  for(i = 0; i < CHANNEL_HASH_SIZE; i++)
    dlink_list_zero(&channel_lists[i]);
  
  mem_static_create(&channel_heap, sizeof(struct channel), 
                    CHANNEL_BLOCK_SIZE);
  mem_static_note(&channel_heap, "channel heap");

  mem_static_create(&channel_invite_heap, sizeof(struct invite),
                    CHANUSER_BLOCK_SIZE);
  mem_static_note(&channel_invite_heap, "channel invite heap");

  ircd_support_set("CHANTYPES", "#");
  ircd_support_set("TOPICLEN", "%u", IRCD_TOPICLEN);
  ircd_support_set("KICKLEN", "%u", IRCD_KICKLEN);
  
  log(channel_log, L_status, "Initialised [channel] module.");
}

/* -------------------------------------------------------------------------- *
 * Destroy channel heaps and cancel timer.                                    *
 * -------------------------------------------------------------------------- */
void channel_shutdown(void)
{
  struct channel *chptr;
  struct channel *next;
  
  log(channel_log, L_status, "Shutting down [channel] module...");
  
  ircd_support_unset("KICKLEN");
  ircd_support_unset("TOPICLEN");
  ircd_support_unset("CHANTYPES");
  
  dlink_foreach_safe(&channel_list, chptr, next)
    channel_delete(chptr);

  mem_static_destroy(&channel_invite_heap);
  
  mem_static_destroy(&channel_heap);
  
  log_source_unregister(channel_log);
}

/* -------------------------------------------------------------------------- *
 * Create a channel.                                                          *
 * -------------------------------------------------------------------------- */
struct channel *channel_new(const char *name)
{
  struct channel *chptr;
  
  /* allocate, zero and link channel struct */
  chptr = mem_static_alloc(&channel_heap);
  
  memset(chptr, 0, sizeof(struct channel));
  
  /* initialize the struct */
  strlcpy(chptr->name, name, sizeof(chptr->name));
  
  chptr->ts = timer_systime;
  chptr->hash = str_ihash(chptr->name);
  chptr->refcount = 1;
  chptr->id = channel_id++;
  chptr->serial = channel_serial;
  chptr->modes = 0LLU;
  
  dlink_list_zero(&chptr->invites);
  
  dlink_add_tail(&channel_list, &chptr->node, chptr);
  dlink_add_tail(&channel_lists[chptr->hash % CHANNEL_HASH_SIZE], 
                 &chptr->hnode, chptr);

  debug(channel_log, "Created channel block: %s", chptr->name);
  
  return chptr;
}     
     
/* -------------------------------------------------------------------------- *
 * Delete a channel.                                                          *
 * -------------------------------------------------------------------------- */
void channel_delete(struct channel *chptr)
{
  debug(channel_log, "Destroying channel block: %s", chptr->name);
  
  dlink_delete(&channel_list, &chptr->node);
  dlink_delete(&channel_lists[chptr->hash % CHANNEL_HASH_SIZE], &chptr->hnode);
  
  channel_release(chptr);
  
  mem_static_free(&channel_heap, chptr);
}

/* -------------------------------------------------------------------------- *
 * Loose all references of a channel block.                                   *
 * -------------------------------------------------------------------------- */
void channel_release(struct channel *chptr)
{
  struct node *node;
  struct node *next;
  
  dlink_foreach_safe(&chptr->chanusers, node, next)
    chanuser_delete(node->data);
  
  dlink_foreach_safe(&chptr->invites, node, next)
    user_uninvite(node->data);
  
  dlink_list_zero(&chptr->lchanusers);
  dlink_list_zero(&chptr->chanusers);
} 

/* -------------------------------------------------------------------------- *
 * Find a channel by name.                                                    *
 * -------------------------------------------------------------------------- */
struct channel *channel_find_name(const char *name)
{
  struct channel *chptr;
  struct node    *node;
  uint32_t        hash;
  
  hash = str_ihash(name);
  
  /* Walk through a hashed list */
  dlink_foreach(&channel_lists[hash % CHANNEL_HASH_SIZE], node)
  {
    chptr = node->data;
    
    /* Hash matches */
    if(hash == chptr->hash)
    {
      /* Name matches */
      if(!str_icmp(chptr->name, name))
        return chptr;
    }
  }
  
  return NULL;
}

/* -------------------------------------------------------------------------- *
 * Find a channel by name.                                                    *
 * -------------------------------------------------------------------------- */
struct channel *channel_find_id(uint32_t id)
{
  struct channel *chptr;
  
  dlink_foreach(&channel_list, chptr)
  {
    if(chptr->id == id)
      return chptr;
  }
  
  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct channel *channel_find_warn(struct client *cptr, const char *name)
{
  struct channel *chptr;
  
  chptr = channel_find_name(name);
  
  if(chptr)
    return chptr;
  
  if(client_is_user(cptr))
    numeric_send(cptr, ERR_NOSUCHCHANNEL, name);
   
  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void channel_topic(struct lclient *lcptr, struct client   *cptr,
                   struct channel *chptr, struct chanuser *cuptr,
                   const char     *topic)
{
  if(hooks_call(channel_topic, HOOK_DEFAULT, lcptr, cptr, chptr, cuptr, topic))
    return;
  
  /* Did the topic change? */
  if(str_ncmp(chptr->topic, topic, sizeof(chptr->topic) - 1))
  {
    /* Yes it did, actualise it */
    strlcpy(chptr->topic, topic, sizeof(chptr->topic));
    
    /* Set topic info */
    if(client_is_user(cptr))
      str_snprintf(chptr->topic_info, sizeof(chptr->topic_info), "%s!%s@%s",
               cptr->name, cptr->user->name, cptr->host);
    else
      strlcpy(chptr->topic_info, cptr->name, sizeof(chptr->topic_info));
    
    chptr->topic_ts = timer_systime;
    
    /* Send out to channel */
    channel_send(NULL, chptr, CHFLG(NONE), CHFLG(NONE),
                 ":%s TOPIC %s :%s", 
                 chptr->topic_info, chptr->name, chptr->topic);
    
    /* Send to servers */
    if(client_is_user(cptr))
    {
      server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                  ":%s TOPIC %s :%s",
                  cptr->user->uid, chptr->name, chptr->topic);
      server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                  ":%s TOPIC %s :%s",
                  cptr->name, chptr->name, chptr->topic);
    }
    else
    {
      server_send(lcptr, NULL, CAP_NONE, CAP_NONE,
                  ":%s TOPIC %s :%s", 
                  cptr->name, chptr->name, chptr->topic);
    }
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void channel_vsend(struct lclient *lcptr,  struct channel *chptr,
                   uint64_t        flag,   uint64_t        noflag, 
                   const char     *format, va_list         args)
{
  struct chanuser *cuptr;
  struct node     *node;
  struct fqueue    multi;
  size_t           n;
  char             buf[IRCD_LINELEN + 1];
  
  /* Formatted print */
  n = str_vsnprintf(buf, sizeof(buf) - 2, format, args);
  
/*  debug(channel_log, "Sending to channel %s: %s", chptr->name, buf);*/
  
  /* Add line separator */
  buf[n++] = '\r';
  buf[n++] = '\n';
  
  /* Start a multicast queue */
  io_multi_start(&multi);
  
  io_multi_write(&multi, buf, n);
  
  /* Loop through local chanuseres */
  dlink_foreach(&chptr->lchanusers, node)
  {
    cuptr = node->data;
    
    /* Huh? What the hell does a remote user on local chanuser list? */
    if(!client_is_local(cuptr->client))
      continue;
     
    if(client_is_service(cuptr->client))
      continue;
    
    /* The one we shouldn't send to */
    if(cuptr->client->source == lcptr)
      continue;
    
    /* Required flags check */
    if((cuptr->flags & flag) != flag)
      continue;
    
    /* Refused flags check */
    if((cuptr->flags & noflag) != 0)
      continue;
   
    /* Link it to the local queue */
    io_multi_link(&multi, cuptr->client->lclient->fds[1]);
    lclient_update_sendb(cuptr->client->lclient, n);
#ifdef DEBUG
    buf[n - 2] = '\0';
    debug(ircd_log_out, "To %s: %s", cuptr->client->lclient->name, buf);
#endif /* DEBUG */
  }
                    
  /* End multicasting */
  io_multi_end(&multi);
}

void channel_send(struct lclient *lcptr, struct channel *chptr, 
                  uint64_t        flag,  uint64_t        noflag, 
                  const char     *format, ...)
{
  va_list args;
  
  va_start(args, format);
  channel_vsend(lcptr, chptr, flag, noflag, format, args);
  va_end(args);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
size_t channel_burst(struct lclient *lcptr, struct channel *chptr)
{
  struct chanuser *acuptr = NULL;
  struct node     *node;
  size_t           chanmodes;
  
  debug(channel_log, "Bursting channel %s to %s.",
        chptr->name, lcptr->name);
  
  dlink_foreach_data(&chptr->chanusers, node, acuptr)
  {
    if(acuptr->client->serial != client_serial)
    {
      client_burst(lcptr, acuptr->client);
      
      acuptr->client->serial = client_serial;
    }
  }
  
  chanmodes = 0;
  
  chanmodes += chanuser_burst(lcptr, chptr);
  chanmodes += chanmode_burst(lcptr, chptr);
  
  if(chptr->topic[0])
  {
    lclient_send(lcptr, "NTOPIC %s %lu %s %lu :%s",
                 chptr->name, chptr->ts, 
                 chptr->topic_info, chptr->topic_ts, chptr->topic);
  }
  else
  {
    lclient_send(lcptr, "NTOPIC %s %lu %S %lu :",
                 chptr->name, chptr->ts, 
                 server_me, timer_systime);
  }
  
  return chanmodes;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void channel_message(struct lclient *lcptr, struct client *cptr,
                     struct channel *chptr, int            type,
                     const char     *text)
{
  struct chanuser *cuptr;
  struct chanuser *acuptr = NULL;
  struct node     *node;
  const char      *cmd = (type == CHANNEL_PRIVMSG ? "PRIVMSG" : "NOTICE");
  
  cuptr = chanuser_find(chptr, cptr);
  
  if(hooks_call(channel_message, HOOK_DEFAULT, lcptr, cptr, chptr, cuptr))
    return;
    
  if(client_is_user(cptr) || client_is_service(cptr))
    channel_send(lcptr, chptr, CHFLG(NONE), CHFLG(NONE),
                     ":%N!%U@%H %s %s :%s",
                     cptr, cptr, cptr,
                     cmd, chptr->name, text);
  else
    channel_send(lcptr, chptr, CHFLG(NONE), CHFLG(NONE),
                     ":%N %s %s :%s",
                     cptr, cmd, chptr->name, text);

    
  //  debug(channel_log, "Message to channel %s from %s.", chptr->name, lcptr->name);
  
  dlink_foreach_data(&chptr->lchanusers, node, acuptr)
  {
    if(client_is_service(acuptr->client))
      service_handle(acuptr->client->service, lcptr, cptr, chptr, cmd, "%s", text);
  }
    
  server_send(lcptr, chptr, CAP_UID, CAP_NONE,
              ":%s %s %s :%s",
                cptr->user ? cptr->user->uid : cptr->name,
                cmd, chptr->name, text);
  server_send(lcptr, chptr, CAP_NONE, CAP_UID,
                ":%s %s %s :%s",
                cptr->name, cmd, chptr->name, text);
  
  /* Don't reset idle time when client messages
     to a channel where only he is member */
  if((cuptr && chptr->chanusers.size == 1))
    return;
    
  cptr->lastmsg = timer_systime;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void channel_join(struct lclient *lcptr, struct client *cptr,
                  const char     *name,  const char    *key)
{
  struct chanuser *cuptr;
  struct channel  *chptr;
  struct list      modes;
  int              reply = 0;

  dlink_list_zero(&modes);
  
  /* Try to find the channel */
  chptr = channel_find_name(name);
  
  if(chptr)
  {
    /* The user already joined, ignore the join */
    if(channel_is_member(chptr, cptr))
      return;

    /* Check the plugin hooks if we have permission to join */
    hooks_call(channel_join, HOOK_1ST, cptr, chptr, key, &reply);
    
    if(reply > 0)
    {
      numeric_send(cptr, reply, chptr->name);
      return;
    }

    /* Add the user to the channel */
    cuptr = chanuser_add(chptr, cptr);
    
    /* Initial mode hooks */
    hooks_call(channel_join, HOOK_3RD, &modes, cuptr);
  }
  else
  {
    /* Add to channel list */
    chptr = channel_new(name);
    
    /* Uh, error adding channel */
    if(chptr == NULL)
    {
      numeric_send(cptr, ERR_UNAVAILRESOURCE, name);
      return;
    }
    
    /* Add the user to the channel */
    cuptr = chanuser_add(chptr, cptr);
    
    /* Initial mode hooks */
    hooks_call(channel_join, HOOK_2ND, &modes, cuptr);
  }
  
  /* Send out netjoin */
  chanuser_introduce(NULL, NULL, &cuptr->gnode);
  
  /* Send the join command to the channel */
  chanuser_send_joins(NULL, &cuptr->gnode);
  
  /* Send out local shit */
  if(chptr->topic[0] == '\0')
  {
    numeric_send(cptr, RPL_NOTOPIC, chptr->name);
  }
  else
  {
    numeric_send(cptr, RPL_TOPIC, chptr->name, chptr->topic);
    numeric_send(cptr, RPL_TOPICWHOTIME, chptr->name,
                 chptr->topic_info, chptr->topic_ts);
  }
  
  /* Show /names list */
  chanuser_show(cptr, chptr, cuptr, 1);
  
  /* Send out modechanges for initial modes */
  if(modes.size)
  {
    chanmode_send_local(client_me, chptr, modes.head, CHANMODE_PER_LINE);
    chanmode_change_destroy(&modes);
  }
}            

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void channel_show(struct client *cptr)
{
  struct channel *chptr;
  
  client_send(cptr, numeric_format(RPL_LISTSTART),
              client_me->name, cptr->name);
  
  dlink_foreach(&channel_list, chptr)
  {
    if(hooks_call(channel_show, HOOK_DEFAULT, cptr, chptr))
      continue;
    
    client_send(cptr, numeric_format(RPL_LIST),
                client_me->name, cptr->name,
                chptr->name, chptr->chanusers.size,
                chptr->topic);
  }

  client_send(cptr, numeric_format(RPL_LISTEND),
              client_me->name, cptr->name);
}

/* -------------------------------------------------------------------------- *
 * Get a reference to a channel block                                         *
 * -------------------------------------------------------------------------- */
struct channel *channel_pop(struct channel *chptr)
{
  if(chptr)
  {
    if(!chptr->refcount)
    {
      debug(channel_log, "Poping deprecated channel %s",
          chptr->name);
    }
    
    chptr->refcount++;
  }
  
  return chptr;
}

/* -------------------------------------------------------------------------- *
 * Push back a reference to a channel block                                   *
 * -------------------------------------------------------------------------- */
struct channel *channel_push(struct channel **chptrptr)
{
  if(*chptrptr)
  {
    if((*chptrptr)->refcount == 0)
    {
      debug(channel_log, "Trying to push deprecated channel %s",
          (*chptrptr)->name);
    }
    else
    {
      if(--(*chptrptr)->refcount == 0)
        channel_release(*chptrptr);
      
      (*chptrptr) = NULL;
    }
  }
      
  return *chptrptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void channel_dump(struct channel *chptr)
{
  if(chptr == NULL)
  {
    struct node *node;
    
    dump(channel_log, "[============== channel summary ===============]");
    
    dlink_foreach_data(&channel_list, node, chptr)
      dump(channel_log, " #%03u: [%u] %-20s (%u users)",
            chptr->id,
            chptr->refcount,
            chptr->name,
            chptr->chanusers.size);
    
    dump(channel_log, "[========== end of channel summary ============]");
  }
  else
  {
    dump(channel_log, "[============== channel dump ===============]");
    dump(channel_log, "               id: #%u", chptr->id);
    dump(channel_log, "         refcount: %u", chptr->refcount);
    dump(channel_log, "             hash: %p", chptr->hash);
    dump(channel_log, "             name: %s", chptr->name);
    dump(channel_log, "               ts: %lu", chptr->ts);
    dump(channel_log, "         topic_ts: %lu", chptr->topic_ts);
    dump(channel_log, "        chanusers: %u nodes", chptr->chanusers.size);
    dump(channel_log, "       lchanusers: %u nodes", chptr->lchanusers.size);
    dump(channel_log, "       rchanusers: %u nodes", chptr->rchanusers.size);
    dump(channel_log, "          invites: %u nodes", chptr->rchanusers.size);
/*    dump(channel_log, "         modelists: %u nodes", chptr->modelists.size);*/
    dump(channel_log, "            modes: %llu", chptr->rchanusers.size);
    dump(channel_log, "           serial: %u", chptr->rchanusers.size);
    dump(channel_log, "            topic: %s", chptr->topic);
    dump(channel_log, "       topic_info: %s", chptr->topic_info);
    dump(channel_log, "          modebuf: %s", chptr->modebuf);
    dump(channel_log, "          parabuf: %s", chptr->parabuf);
    dump(channel_log, "[========== end of channel dump ============]");    
  }
}
