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
 * $Id: client.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#define _GNU_SOURCE

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/timer.h>
#include <libchaos/hook.h>
#include <libchaos/str.h>
#include <libchaos/log.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/user.h>
#include <chaosircd/oper.h>
#include <chaosircd/client.h>
#include <chaosircd/numeric.h>
#include <chaosircd/server.h>
#include <chaosircd/lclient.h>
#include <chaosircd/chanuser.h>
#include <chaosircd/chanmode.h>
#include <chaosircd/channel.h>
#include <chaosircd/service.h>

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
int             client_log;
struct sheap    client_heap;
struct sheap    client_history_heap;
struct timer   *client_timer;
struct client  *client_me;
struct client  *client_uplink;
uint32_t        client_id;
uint32_t        client_serial;
struct lclient *client_source;
uint32_t        client_seed;
uint32_t        client_max;
uint32_t        client_max_localusers;
uint32_t        client_max_globalusers;
char            client_randchars[] =
  "0123456789-abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* ------------------------------------------------------------------------ */
int client_get_log() { return client_log; }

/* -------------------------------------------------------------------------- *
 * Client lists                                                               *
 * -------------------------------------------------------------------------- */

struct list client_list;               /* contains all clients */
struct list client_lists[3][3];
struct list client_listsn[CLIENT_HASH_SIZE];
struct list client_history[CLIENT_HASH_SIZE];

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void client_format_cb(char **pptr, size_t *bptr, size_t n,
                             int padding, int left, void *arg)
{
  struct client *acptr = arg;
  char          *name;

  if(acptr)
  {
    if(client_is_user(acptr) && client_source &&
       (client_source->caps & CAP_UID))
      name = acptr->user->uid;
    else
      name = acptr->name;
  }
  else
  {
    name = "(nil)";
  }

  while(*name && *bptr < n)
  {
    *(*pptr)++ = *name++;
    (*bptr)++;
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void client_nformat_cb(char **pptr, size_t *bptr, size_t n,
                              int padding, int left, void *arg)
{
  struct client *acptr = arg;
  char          *name;

  if(acptr)
    name = acptr->name;
  else
    name = "(nil)";

  while(*name && *bptr < n)
  {
    *(*pptr)++ = *name++;
    (*bptr)++;
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void client_hformat_cb(char **pptr, size_t *bptr, size_t n,
                              int padding, int left, void *arg)
{
  struct client *acptr = arg;
  char          *host;

  if(acptr)
    host = acptr->host;
  else
    host = "(nil)";

  while(*host && *bptr < n)
  {
    *(*pptr)++ = *host++;
    (*bptr)++;
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void client_uformat_cb(char **pptr, size_t *bptr, size_t n,
                              int padding, int left, void *arg)
{
  struct client *acptr = arg;
  char          *user;

  if(acptr && acptr->user)
    user = acptr->user->name;
  else
    user = "(nil)";

  while(*user && *bptr < n)
  {
    *(*pptr)++ = *user++;
    (*bptr)++;
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
 #define ROR(v, n) ((v >> ((n) & 0x1f)) | (v << (32 - ((n) & 0x1f))))
 #define ROL(v, n) ((v >> ((n) & 0x1f)) | (v << (32 - ((n) & 0x1f))))
 static uint32_t client_random(void)
 {
   int      it;
   int      i;
   uint32_t ns = timer_mtime;

   it = (ns & 0x1f) + 0x20;

   for(i = 0; i < it; i++)
   {
     ns = ROL(ns, client_seed);

     if(ns & 0x01)
       client_seed ^= 0x7ea8de93;
     else
       client_seed ^= 0x4f034e1a;

     ns = ROR(ns, client_seed >> 14);

     if(client_seed & 0x02)
       ns += client_seed;
     else
       ns ^= client_seed;

     client_seed = ROL(client_seed, ns >> 18);
     if(ns & 0x04)
       client_seed ^= ns;
     else
       client_seed -= ns;

     ns = ROL(ns, client_seed >> 11);

     if(client_seed & 0x08)
       ns -= client_seed;
     else
       client_seed += ns;
   }

   return ns;
}
#undef ROR
#undef ROL

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_init(void)
{
  uint32_t i;

  client_log = log_source_register("client");

  dlink_list_zero(&client_lists[CLIENT_LOCAL][CLIENT_USER]);
  dlink_list_zero(&client_lists[CLIENT_LOCAL][CLIENT_SERVER]);
  dlink_list_zero(&client_lists[CLIENT_LOCAL][CLIENT_SERVICE]);
  dlink_list_zero(&client_lists[CLIENT_REMOTE][CLIENT_USER]);
  dlink_list_zero(&client_lists[CLIENT_REMOTE][CLIENT_SERVER]);
  dlink_list_zero(&client_lists[CLIENT_REMOTE][CLIENT_SERVICE]);
  dlink_list_zero(&client_lists[CLIENT_GLOBAL][CLIENT_USER]);
  dlink_list_zero(&client_lists[CLIENT_GLOBAL][CLIENT_SERVER]);
  dlink_list_zero(&client_lists[CLIENT_GLOBAL][CLIENT_SERVICE]);

  for(i = 0; i < CLIENT_HASH_SIZE; i++)
    dlink_list_zero(&client_listsn[i]);

  for(i = 0; i < CLIENT_HISTORY_SIZE; i++)
    dlink_list_zero(&client_history[i]);

  mem_static_create(&client_heap, sizeof(struct client), CLIENT_BLOCK_SIZE);
  mem_static_note(&client_heap, "client heap");
  mem_static_create(&client_history_heap, sizeof(struct history), CLIENT_BLOCK_SIZE);
  mem_static_note(&client_history_heap, "nickname history heap");

  str_register('C', client_format_cb);
  str_register('N', client_nformat_cb);
  str_register('H', client_hformat_cb);
  str_register('U', client_uformat_cb);

  client_id = 0;
  client_max = 0;
  client_seed = ~(timer_mtime >> 16);
  client_me = client_new_local(CLIENT_SERVER, lclient_me);
  client_source = NULL;
  client_me->hops = 0;
  client_me->origin = client_me;
  client_me->server = server_me;
  server_me->client = client_me;
  lclient_me->client = client_me;

  client_max_localusers = 0;
  client_max_globalusers = 0;

  ircd_support_set("MAXTARGETS", "%u", IRCD_MAXTARGETS);
  ircd_support_set("NICKLEN", "%u", IRCD_NICKLEN);

  log(client_log, L_status, "Initialised [client] module.");
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_shutdown(void)
{
  struct client *cptr;
  struct node   *next;

  log(client_log, L_status, "Shutting down [client] module...");

  ircd_support_unset("NICKLEN");
  ircd_support_unset("MAXTARGETS");

  str_unregister('U');
  str_unregister('H');
  str_unregister('N');
  str_unregister('C');

  dlink_foreach_safe(&client_list, cptr, next)
    client_delete(cptr);

  mem_static_destroy(&client_history_heap);
  mem_static_destroy(&client_heap);

  log_source_unregister(client_log);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client *client_new(int location, int type)
{
  struct client *cptr;

  /* Allocate a client block */
  cptr = mem_static_alloc(&client_heap);

  memset(cptr, 0, sizeof(struct client));

  cptr->id = client_id++;
  cptr->refcount = 1;
  cptr->type = type;
  cptr->location = location;
  cptr->hops = 0;
  cptr->name[0] = '\0';
  cptr->server = NULL;
  cptr->user = NULL;
  cptr->oper = NULL;
  cptr->service = NULL;

  /* Initialize timer values */
  cptr->created = timer_systime;
  cptr->lastmsg = timer_systime;
  cptr->lastread = timer_systime;

  /* Add it to the appropriate lists */
  dlink_add_tail(&client_list, &cptr->node, cptr);
  dlink_add_tail(&client_lists[location][type], &cptr->lnode, cptr);
  dlink_add_tail(&client_lists[CLIENT_GLOBAL][type], &cptr->gnode, cptr);

  if(client_lists[CLIENT_LOCAL][CLIENT_USER].size > client_max_localusers)
    client_max_localusers++;
  if(client_lists[CLIENT_GLOBAL][CLIENT_USER].size > client_max_globalusers)
    client_max_globalusers++;

  if(client_list.size > client_max)
    client_max = client_list.size;

  return cptr;
}

/* -------------------------------------------------------------------------- *
 * Create a new client coming from a local connection.                        *
 * -------------------------------------------------------------------------- */
struct client *client_new_local(int type, struct lclient *lcptr)
{
  struct client *cptr;

  /* Allocate a client block */
  cptr = client_new(CLIENT_LOCAL, type);

  /* Setup some references */
  cptr->lclient = lclient_pop(lcptr);
  cptr->source = lclient_pop(lcptr);
  cptr->origin = client_pop(client_me);
  cptr->hops = 1;

  strcpy(cptr->host, lcptr->host);
  strcpy(cptr->hostreal, lcptr->host);
  strcpy(cptr->hostip, lcptr->hostip);
  strcpy(cptr->info, lcptr->info);

  cptr->hhash = str_ihash(cptr->host);
  cptr->rhash = cptr->hhash;
  cptr->ihash = str_ihash(cptr->hostip);
  cptr->ip = lcptr->addr_remote;

  lclient_set_type(lcptr, type + 1);

  strlcpy(cptr->name, lcptr->name, sizeof(cptr->name));
  cptr->hash = str_ihash(cptr->name);
  cptr->ts = timer_systime;

  if(type == CLIENT_SERVER)
    cptr->server = lcptr->server;

  if(type == CLIENT_USER)
  {
    dlink_add_head(&client_listsn[cptr->hash % CLIENT_HASH_SIZE],
                   &cptr->nnode, cptr);

    cptr->user = lcptr->user;

    client_introduce(lcptr, NULL, cptr);
  }

  log(client_log, L_verbose, "New client block: %s",
      cptr->name[0] ? cptr->name : cptr->host);

  if(cptr->origin)
  {
    if(lcptr != lclient_me)
      dlink_add_tail(&server_me->deps[type], &cptr->dnode, cptr);
  }

  hooks_call(client_new_local, HOOK_DEFAULT, cptr, lcptr->user);

  return cptr;
}

/* -------------------------------------------------------------------------- *
 * Create a new client coming from a remote connection                        *
 * -------------------------------------------------------------------------- */
struct client *client_new_remote(int            type,   struct lclient *lcptr,
                                 struct user   *uptr,   struct client  *origin,
                                 const char    *name,   int             hops,
                                 const char    *host,   const char     *hostip,
                                 const char    *info)
{
  struct client *cptr;

  /* Allocate a client block */
  cptr = client_new(CLIENT_REMOTE, type);

  cptr->lclient = NULL;
  cptr->source = lclient_pop(lcptr);
  cptr->origin = client_pop(origin);
  cptr->hops = cptr->origin->hops + 1;

  if(name)
  {
    strlcpy(cptr->name, name, sizeof(cptr->name));
    cptr->hash = str_ihash(cptr->name);

    if(type == CLIENT_USER)
    {
      dlink_add_head(&client_listsn[cptr->hash % CLIENT_HASH_SIZE],
                     &cptr->nnode, cptr);
    }
  }

  cptr->hops = hops;

  if(info)
    strlcpy(cptr->info, info, sizeof(cptr->info));

  if(host)
  {
    strlcpy(cptr->host, host, sizeof(cptr->host));
    strlcpy(cptr->hostreal, host, sizeof(cptr->host));
    cptr->hhash = str_ihash(cptr->host);
    cptr->rhash = cptr->hhash;
  }

  if(hostip)
  {
    strlcpy(cptr->hostip, hostip, sizeof(cptr->hostip));
    net_aton(cptr->hostip, &cptr->ip);
    cptr->ihash = str_ihash(cptr->host);
  }

  if(info)
    strlcpy(cptr->info, info, sizeof(cptr->info));

  log(client_log, L_verbose, "New remote client block: %s",
      cptr->name[0] ? cptr->name : cptr->host);

  dlink_add_tail(&cptr->origin->server->deps[type], &cptr->dnode, cptr);

  hooks_call(client_new_remote, HOOK_DEFAULT, cptr, uptr);

  return cptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client *client_new_service(struct user *uptr, const char *name,
                                  const char *host, const char *hostip,
                                  const char *info)
{
  struct client *cptr;

  /* Allocate a client block */
  cptr = client_new(CLIENT_LOCAL, CLIENT_SERVICE);

  cptr->user = uptr;
  cptr->origin = client_pop(client_me);

  strlcpy(cptr->name, name, sizeof(cptr->name));
  strlcpy(cptr->info, info, sizeof(cptr->info));
  strlcpy(cptr->host, host, sizeof(cptr->host));
  strlcpy(cptr->hostip, hostip, sizeof(cptr->hostip));

  cptr->hhash = str_ihash(cptr->host);
  cptr->rhash = cptr->hhash;
  cptr->ihash = str_ihash(cptr->hostip);
  cptr->hash = str_ihash(cptr->name);
  cptr->ip = net_addr_any;

  dlink_add_head(&client_listsn[cptr->hash % CLIENT_HASH_SIZE],
                 &cptr->nnode, cptr);

  log(client_log, L_verbose, "New service client block: %s",
      cptr->name[0] ? cptr->name : cptr->host);

  if(cptr->origin)
    dlink_add_tail(&server_me->deps[CLIENT_SERVICE], &cptr->dnode, cptr);

  return cptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_delete(struct client *cptr)
{
  if(cptr->location == CLIENT_LOCAL)
  {
    if(cptr->lclient != lclient_me && server_me)
      dlink_delete(&server_me->deps[cptr->type], &cptr->dnode);
  }
  else if(cptr->origin && cptr->origin->server)
  {
    dlink_delete(&cptr->origin->server->deps[cptr->type], &cptr->dnode);
  }

  if(cptr->server)
  {
    struct client *acptr = NULL;
    struct node   *nptr;

    dlink_foreach_data(&cptr->server->deps[CLIENT_USER], nptr, acptr)
      acptr->origin = NULL;
    dlink_foreach_data(&cptr->server->deps[CLIENT_SERVER], nptr, acptr)
      acptr->origin = NULL;
  }

  client_release(cptr);

  dlink_delete(&client_list, &cptr->node);

  dlink_delete(&client_lists[CLIENT_GLOBAL][cptr->type], &cptr->gnode);
  dlink_delete(&client_lists[cptr->location][cptr->type], &cptr->lnode);

  if(cptr->type == CLIENT_USER)
  {
    dlink_delete(&client_listsn[cptr->hash % CLIENT_HASH_SIZE], &cptr->nnode);
  }

  mem_static_free(&client_heap, cptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_release(struct client *cptr)
{
  cptr->source = NULL;

  if(cptr->lclient)
  {
    cptr->lclient->client = NULL;
    cptr->lclient->user = NULL;
    cptr->lclient = NULL;
  }

  if(cptr->user)
  {
    user_delete(cptr->user);
    cptr->user = NULL;
  }

  cptr->origin = NULL;

  if(cptr->oper)
  {
    oper_down(cptr->oper, cptr);
    cptr->oper = NULL;
  }

  if(cptr->server)
  {
    server_delete(cptr->server);
    cptr->server = NULL;
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client *client_find_id(uint32_t id)
{
  struct client *cptr;

  dlink_foreach(&client_list, cptr)
    if(cptr->id == id)
      return cptr;

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client *client_find_name(const char *name)
{
  struct client *cptr;
  hash_t         hash;
  
  hash = str_ihash(name);

  dlink_foreach(&client_list, cptr)
  {
    if(cptr->hash == hash)
      if(!str_icmp(cptr->name, name))
        return cptr;
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client *client_find_nick(const char *nick)
{
  struct client *cptr = NULL;
  struct node   *node;
  hash_t         hash;
  
  hash = str_ihash(nick);

  dlink_foreach_data(&client_listsn[hash % CLIENT_HASH_SIZE], node, cptr)
  {
    if(cptr->name[0] && hash == cptr->hash)
    {
      if(!str_icmp(cptr->name, nick))
        return cptr;
    }
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct history *client_history_find(const char *nick)
{
  struct history *hptr = NULL;
  struct node    *node;
  hash_t          hash;
  
  hash = str_ihash(nick);

  dlink_foreach_data(&client_history[hash % CLIENT_HASH_SIZE], node, hptr)
  {
    if(hptr->nick[0] && hash == hptr->hash)
    {
      if(!str_icmp(hptr->nick, nick))
        return hptr;
    }
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct history *client_history_add(struct client *cptr, const char *nick)
{
  struct history *hptr;

  if((hptr = client_history_find(nick)) == NULL)
  {
    hptr = mem_static_alloc(&client_history_heap);
    hptr->hash = str_ihash(nick);

    if(client_history[hptr->hash % CLIENT_HASH_SIZE].size == CLIENT_HISTORY_SIZE)
      client_history_delete(client_history[hptr->hash % CLIENT_HASH_SIZE].tail->data);

    dlink_add_head(&client_history[hptr->hash % CLIENT_HASH_SIZE], &hptr->node, hptr);
  }

  hptr->client = cptr;
  strlcpy(hptr->nick, nick, sizeof(hptr->nick));

  return hptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_history_delete(struct history *hptr)
{
  dlink_delete(&client_history[hptr->hash % CLIENT_HASH_SIZE], &hptr->node);
  mem_static_free(&client_history_heap, hptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_history_clean(struct client *cptr)
{
  struct history *hptr;
  uint32_t        i;

  for(i = 0; i < CLIENT_HASH_SIZE; i++)
  {
    dlink_foreach(&client_history[i], hptr)
      if(hptr->client == cptr)
        client_history_delete(hptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client *client_find_host(const char *host)
{
  struct client *cptr = NULL;
  struct node   *node;
  hash_t         hash;
  
  hash = str_ihash(host);

  dlink_foreach_data(&client_lists[CLIENT_GLOBAL][CLIENT_USER], node, cptr)
  {
    if(hash == cptr->hhash || hash == cptr->ihash || hash == cptr->rhash)
    {
      if(!str_icmp(cptr->host, host) || !str_icmp(cptr->hostreal, host) ||
         !str_icmp(cptr->hostip, host))
        return cptr;
    }
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client *client_find_uid(const char *uid)
{
  struct user *uptr;

  uptr = user_find_uid(uid);

  return uptr ? uptr->client : NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client *client_find_nickw(struct client *cptr, const char *nick)
{
  struct client *acptr;

  acptr = client_find_nick(nick);

  if(acptr)
    return acptr;

  if(client_is_user(cptr))
    numeric_send(cptr, ERR_NOSUCHNICK, nick);

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client *client_find_nickh(const char *nick)
{
  struct client  *acptr;
  struct history *hptr;

  acptr = client_find_nick(nick);

  if(acptr)
    return acptr;

  hptr = client_history_find(nick);

  if(hptr)
    return hptr->client;

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct client *client_find_nickhw(struct client *cptr, const char *nick)
{
  struct client  *acptr;
  struct history *hptr;

  acptr = client_find_nick(nick);

  if(acptr)
    return acptr;

  hptr = client_history_find(nick);

  if(hptr)
    return hptr->client;

  if(client_is_user(cptr))
    numeric_send(cptr, ERR_NOSUCHNICK, nick);

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_set_name(struct client *cptr, const char *name)
{
  if(cptr->type == CLIENT_USER)
  {
    dlink_delete(&client_listsn[cptr->hash % CLIENT_HASH_SIZE], &cptr->nnode);

    if(cptr->name[0])
      client_history_add(cptr, cptr->name);
  }

  strlcpy(cptr->name, name, sizeof(cptr->name));

  cptr->hash = str_ihash(cptr->name);
  cptr->ts = timer_systime;

  if(cptr->lclient)
    lclient_set_name(cptr->lclient, name);

  if(cptr->type == CLIENT_USER)
  {
    dlink_add_head(&client_listsn[cptr->hash % CLIENT_HASH_SIZE],
                   &cptr->nnode, cptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_send(struct client *cptr, const char *format, ...)
{
  va_list args;

  va_start(args, format);
  lclient_vsend(cptr->source, format, args);
  va_end(args);
}

void client_vsend(struct client *cptr, const char *format, va_list args)
{
  client_source = cptr->source;

  if(client_source)
    lclient_vsend(client_source, format, args);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_lusers(struct client *client)
{
  numeric_send(client, RPL_LUSERCLIENT,
               client_lists[CLIENT_GLOBAL][CLIENT_USER].size,
               0,
               client_lists[CLIENT_GLOBAL][CLIENT_SERVER].size);

/*  numeric_send(client, RPL_LUSEROP, );*/

  if(lclient_lists[LCLIENT_UNKNOWN].size)
    numeric_send(client, RPL_LUSERUNKNOWN, lclient_lists[LCLIENT_UNKNOWN].size);

  if(channel_list.size)
    numeric_send(client, RPL_LUSERCHANNELS, channel_list.size);

  numeric_send(client, RPL_LUSERME,
               client_lists[CLIENT_LOCAL][CLIENT_USER].size,
               client_lists[CLIENT_LOCAL][CLIENT_SERVER].size);

  numeric_send(client, RPL_LOCALUSERS,
               client_lists[CLIENT_LOCAL][CLIENT_USER].size,
               client_max_localusers);
  numeric_send(client, RPL_GLOBALUSERS,
               client_lists[CLIENT_GLOBAL][CLIENT_USER].size,
               client_max_globalusers);

  numeric_send(client, RPL_STATSCONN,
               lclient_max - 1, client_max - 1, lclient_id - 1);
}

/* -------------------------------------------------------------------------- *
 * Exit a client of any type source this server.                                *
 * This also generates all necessary protocol messages that this exit may     *
 * cause.                                                                     *
 * -------------------------------------------------------------------------- */
void client_vexit(struct lclient *lcptr,   struct client *cptr,
                  const char     *comment, va_list        args)
{
  struct chanuser *cuptr;
  struct node     *node;
  struct node     *next;
  char             cstr[IRCD_TOPICLEN + 1];

  if(comment != NULL)
    str_vsnprintf(cstr, sizeof(cstr), comment, args);
  else if(comment)
    strcpy(cstr, comment);
  else
    strcpy(cstr, "client quit");

  hooks_call(client_exit, HOOK_DEFAULT, lcptr, cptr, cstr);

  log(client_log, L_verbose, "Exiting client: %s (%s)", cptr->name, cstr);

  if(client_is_user(cptr))
  {
    server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                ":%s QUIT :%s",
                cptr->user->uid, cstr);
    server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                ":%s QUIT :%s",
                cptr->name, cstr);

    chanuser_send(lcptr, cptr, ":%s!%s@%s QUIT :%s",
                  cptr->name, cptr->user->name, cptr->host, cstr);

    /* bug? */
    dlink_foreach_safe(&cptr->user->channels, node, next)
    {
      cuptr = node->data;

      chanuser_delete(cuptr);

      if(cuptr->channel->chanusers.size == 0 && !(cuptr->channel->modes & CHFLG(P)))
        channel_delete(cuptr->channel);
    }

    client_history_clean(cptr);
  }

  if(cptr->user)
  {
    cptr->user->client = NULL;
    user_delete(cptr->user);
    cptr->user = NULL;

    if(cptr->lclient)
      cptr->lclient->user = NULL;
  }

  if(cptr->server)
  {
    server_exit(lcptr, NULL, cptr->server, cstr);
    cptr->server = NULL;

    if(cptr->lclient)
      cptr->lclient->server = NULL;
  }

  if(cptr->lclient)
  {
    cptr->lclient->client = NULL;
    cptr->lclient->user = NULL;
    lclient_exit(cptr->lclient, "%s", cstr);

    if(cptr->server)
      cptr->server->lclient = NULL;

    cptr->lclient = NULL;
  }

  client_delete(cptr);
}

void client_exit(struct lclient *lcptr,   struct client *cptr,
                 const char     *comment, ...)
{
  va_list args;

  va_start(args, comment);
  client_vexit(lcptr, cptr, comment, args);
  va_end(args);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int client_nick(struct lclient *lcptr, struct client *cptr, char *nick)
{
  cptr->ts = timer_systime;

  if(cptr->user)
  {
    chanuser_send(NULL, cptr, ":%N!%U@%H NICK :%s",
                  cptr, cptr, cptr, nick);

    server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                ":%s NICK %s :%lu", cptr->user->uid,
                nick, (unsigned long)cptr->ts);
    server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                ":%s NICK %s :%lu", cptr->name,
                nick, (unsigned long)cptr->ts);
  }

  client_set_name(cptr, nick);

  if(client_is_local(cptr))
    io_note(cptr->lclient->fds[0], "client: %s", cptr->name);

  return 1;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_kill(struct client *cptr, struct client *acptr, char *format, ...)
{
  char comment[IRCD_TOPICLEN + 1];
  va_list args;

  va_start(args, format);
  str_vsnprintf(comment, sizeof(comment), format, args);
  va_end (args);

/*  if(HasID(acptr) && cptr->lclient && IsCapable(cptr, CAP_UID))
    client_send(cptr, ":%s KILL %s :%s", client_me->name, ID(acptr));
  else
    client_send(cptr, ":%s KILL %s :%s", client_me->name, acptr->name);*/
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_nick_random(char *buf)
{
  size_t len;
  size_t i;

  do
  {
    len = (client_random() % (IRCD_NICKLEN - 1)) + 1;
    i = 0;

    buf[i++] =
      client_randchars[(client_random() % (sizeof(client_randchars - 12))) + 11];

    while(i < len)
    {
      buf[i++] =
        client_randchars[client_random() % (sizeof(client_randchars) - 1)];
    }

    buf[i] = '\0';
  }
  while((client_find_nick(buf)) != NULL);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int client_nick_rotate(const char *name, char *buf)
{
  uint32_t       i;
  uint32_t       j;
  uint32_t       len;
  struct client *acptr = NULL;

  len = str_len(name);

  if(len < 2)
    return -1;

  for(i = 0; i + 1 < len; i++)
  {
    for(j = 0; j + 1 < len; j++)
      buf[j] = name[j + 1];

    buf[j] = name[0];
    buf[j + 1] = '\0';

    if((acptr = client_find_nick(buf)) == NULL)
      break;
  }

  return (acptr != NULL ? -1 : 0);
}

/* -------------------------------------------------------------------------- *
 * introduce acptr to the rest of the net                                     *
 *                                                                            *
 * <lcptr>         the local connection the new client is coming from         *
 * <acptr>         the new client                                             *
 * -------------------------------------------------------------------------- */
void client_introduce(struct lclient *lcptr, struct client *cptr,
                      struct client  *acptr)
{
  if(cptr)
  {
    server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                ":%N NUSER %N %d %lu %s %U %s %s %N %s :%s",
                cptr,
                acptr,
                acptr->hops + 1,
                (unsigned long)acptr->ts,
                acptr->user->mode[0] ? acptr->user->mode : "-",
                acptr,
                acptr->hostreal,
                acptr->hostip,
                acptr->origin,
                acptr->user->uid,
                acptr->info);

    server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                ":%N NICK %N %d %lu %s %U %s %N :%s",
                cptr,
                acptr,
                acptr->hops + 1,
                (unsigned long)acptr->ts,
                acptr->user->mode[0] ? acptr->user->mode : "-",
                acptr,
                acptr->hostreal,
                acptr->origin,
                acptr->info);
  }
  else
  {
    server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                "NUSER %N %d %lu %s %U %s %s %N %s :%s",
                acptr,
                acptr->hops + 1,
                (unsigned long)acptr->ts,
                acptr->user->mode[0] ? acptr->user->mode : "-",
                acptr,
                acptr->hostreal,
                acptr->hostip,
                acptr->origin,
                acptr->user->uid,
                acptr->info);

    //log(server_log, L_debug, "%s: mode: %s", __func__, acptr->user->mode);

    server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                "NICK %N %d %lu %s %U %s %N :%s",
                acptr,
                acptr->hops + 1,
                (unsigned long)acptr->ts,
                acptr->user->mode[0] ? acptr->user->mode : "-",
                acptr,
                acptr->hostreal,
                acptr->origin,
                acptr->info);
  }
}

/* -------------------------------------------------------------------------- *
 * burst acptr to cptr                                                        *
 * -------------------------------------------------------------------------- */
void client_burst(struct lclient *lcptr, struct client *acptr)
{
  /* Sanity check */
  if(!client_is_user(acptr))
    return;

  log(client_log, L_verbose, "Bursting client %s to server %s",
      acptr->name, lcptr->name);

  if(lcptr->caps & CAP_UID)
  {
    lclient_send(lcptr, "NUSER %N %d %lu %s %U %s %s %s %s :%s",
                 acptr,
                 acptr->hops + 1,
                 (unsigned long)acptr->ts,
                 acptr->user->mode[0] ? acptr->user->mode : "-",
                 acptr,
                 acptr->hostreal,
                 acptr->hostip,
                 acptr->origin->name,
                 acptr->user->uid,
                 acptr->info);
  }
  else
  {
    lclient_send(lcptr, "NICK %N %d %lu %s %s %s %s :%s",
                 acptr,
                 acptr->hops + 1,
                 (unsigned long)acptr->ts,
                 acptr->user->mode[0] ? acptr->user->mode : "-",
                 acptr->user->name,
                 acptr->hostreal,
                 acptr->origin->name,
                 acptr->info);
  }

  hooks_call(client_burst, HOOK_DEFAULT, lcptr, acptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int client_relay(struct lclient *lcptr, struct client *cptr,
                 struct client  *acptr, const char    *format,
                 char          **argv)
{
  if(client_is_local(acptr))
    return 0;

  client_send(acptr, format, cptr,
              argv[2], argv[3], argv[4], argv[5],
              argv[6], argv[7], argv[8]);
  return 1;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int client_relay_always(struct lclient *lcptr,    struct client *cptr,
                        struct client **acptrptr, uint32_t       cindex,
                        const char     *format,   int           *argc,
                        char          **argv)
{
  struct client *acptr;

  if(lcptr->caps & CAP_UID)
  {
    if((acptr = client_find_uid(argv[cindex])) == NULL)
      return -1;
  }
  else
  {
    if((acptr = client_find_nickhw(cptr, argv[cindex])) == NULL)
      return -1;
  }

  if(acptr->source->caps & CAP_UID)
    argv[cindex] = acptr->user->uid;
  else
    argv[cindex] = acptr->name;

  if(acptrptr)
    *acptrptr = acptr;

  return client_relay(lcptr, cptr, acptr, format, argv);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_message(struct lclient *lcptr, struct client *cptr,
                    struct client  *acptr, int            type,
                    const char     *text)
{
  const char *cmd = (type == 0 ? "PRIVMSG" : "NOTICE");

  if(client_is_local(acptr) && client_is_user(acptr))
  {
    if(client_is_user(cptr))
      client_send(acptr, ":%N!%U@%H %s %N :%s",
                  cptr, cptr, cptr, cmd, acptr, text);
    else
      client_send(acptr, ":%N %s %N :%s",
                  cptr, cmd, acptr, text);
  }
  else if(client_is_service(acptr))
  {
    service_handle(acptr->service, lcptr, cptr, NULL, cmd, "%s", text);
  }
  else
  {
    client_send(acptr, ":%C %s %C :%s",
                cptr, cmd, acptr, text);
  }

  if(acptr != cptr)
        cptr->lastmsg = timer_systime;
}

/* -------------------------------------------------------------------------- *
 * Get a reference to a client block                                          *
 * -------------------------------------------------------------------------- */
struct client *client_pop(struct client *cptr)
{
  if(cptr)
  {
    if(!cptr->refcount)
      debug(client_log, "Poping deprecated client %N", cptr);

    cptr->refcount++;
  }

  return cptr;
}


/* -------------------------------------------------------------------------- *
 * Push back a reference to a client block                                    *
 * -------------------------------------------------------------------------- */
struct client *client_push(struct client **cptrptr)
{
  if(*cptrptr)
  {
    if((*cptrptr)->refcount == 0)
    {
      debug(client_log, "Trying to push deprecated client %N",
            *cptrptr);
    }
    else
    {
      if(--(*cptrptr)->refcount == 0)
        client_release(*cptrptr);

      (*cptrptr) = NULL;
    }
  }

  return *cptrptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void client_dump(struct client *cptr)
{
  if(cptr == NULL)
  {
    struct node *node;

    dump(client_log, "[============== client summary ===============]");

    dump(client_log, "------------------- servers -------------------");

    dlink_foreach_data(&client_lists[CLIENT_GLOBAL][CLIENT_SERVER], node, cptr)
      dump(client_log, " #%u: [%u] %-20s (%s)",
            cptr->id, cptr->refcount,
            cptr->name[0] ? cptr->name : "<unknown>",
            cptr->location == CLIENT_LOCAL ? "local" : "remote");

    dump(client_log, "-------------------- users --------------------");

    dlink_foreach_data(&client_lists[CLIENT_GLOBAL][CLIENT_USER], node, cptr)
      dump(client_log, " #%u: [%u] %-20s (%s)",
            cptr->id, cptr->refcount,
            cptr->name[0] ? cptr->name : "<unknown>",
            cptr->location == CLIENT_LOCAL ? "local" : "remote");

    dump(client_log, "[=========== end of client summary ===========]");
  }
  else
  {
    dump(client_log, "[============== client dump ===============]");
    dump(client_log, "         id: #%u", cptr->id);
    dump(client_log, "   refcount: %u", cptr->refcount);
    dump(client_log, "       hash: %p", cptr->hash);
    dump(client_log, "       type: %s",
          cptr->type == CLIENT_USER ? "user" : "server");
    dump(client_log, "   location: %s",
          cptr->location == CLIENT_LOCAL ? "local" : "remote");
    dump(client_log, "    lclient: %s [%i]",
          cptr->lclient ? cptr->lclient->name : "(nil)",
          cptr->lclient ? cptr->lclient->refcount : 0);
    dump(client_log, "     source: %s [%i]",
          cptr->source ? cptr->source->name : "(nil)",
          cptr->source ? cptr->source->refcount : 0);
    dump(client_log, "     origin: %C [%i]",
          cptr->origin,
          cptr->origin ? cptr->origin->refcount : 0);
    dump(client_log, "       user: %U [%i]",
          cptr,
          cptr->user ? cptr->user->refcount : 0);
    dump(client_log, "     server: %S [%i]",
          cptr->server,
          cptr->server ? cptr->server->refcount : 0);
    dump(client_log, "     serial: %u", cptr->serial);
    dump(client_log, "       hops: %u", cptr->hops);
    dump(client_log, "    created: %lu", cptr->created);
    dump(client_log, "    lastmsg: %lu", cptr->lastmsg);
    dump(client_log, "   lastread: %lu", cptr->lastread);
    dump(client_log, "         ts: %lu", cptr->ts);
    dump(client_log, "         ip: %s", net_ntoa(cptr->ip));
    dump(client_log, "       name: %N", cptr);
    dump(client_log, "       host: %H", cptr);
    dump(client_log, "   hostreal: %s", cptr->hostreal);
    dump(client_log, "     hostip: %s", cptr->hostip);
    dump(client_log, "       info: %s", cptr->info);
    dump(client_log, "[=========== end of client dump ===========]");
  }
}
