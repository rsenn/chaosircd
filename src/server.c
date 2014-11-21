/* chaosircd - CrowdGuard IRC daemon
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
 * $Id: server.c,v 1.4 2006/09/28 09:44:11 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include "libchaos/connect.h"
#include "libchaos/timer.h"
#include "libchaos/net.h"
#include "libchaos/str.h"
#include "libchaos/log.h"
#include "libchaos/hook.h"
#include "libchaos/ssl.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/conf.h"
#include "ircd/user.h"
#include "ircd/ircd.h"
#include "ircd/chars.h"
#include "ircd/class.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/channel.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int               server_log;
struct sheap      server_heap;
struct timer     *server_timer;
struct list       server_list;
struct list       server_lists[2];
struct stats      server_stats[64];
struct server    *server_me;
uint32_t          server_id;
int               server_default_caps   = CAP_DEFAULT;
int               server_default_cipher = CAP_ENC_AES_256;
uint32_t          server_serial;

struct capab server_caps[] = {
  { "HUB", CAP_HUB },
  { "EOB", CAP_EOB },
  { "UID", CAP_UID },
  { "KLN", CAP_KLN },
  { "GLN", CAP_GLN },
  { "HOP", CAP_HOP },
  { "PAR", CAP_PAR },
  { "SPF", CAP_SPF },
  { "RW",  CAP_RW  },
  { "DE",  CAP_DE  },
  { "AM",  CAP_AM  },
  { "TS",  CAP_TS  },
  { "NSV", CAP_NSV },
  { "CLK", CAP_CLK },
  { "SVC", CAP_SVC },
  { "SSL", CAP_SSL },
  { NULL,  0       },
};

/* ------------------------------------------------------------------------ */
int server_get_log() { return server_log; }

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct cryptcap server_ciphers[] = {
  { "AES/128",   CAP_ENC_AES_128,   16, CIPHER_AES_128 },
  { "AES/192",   CAP_ENC_AES_192,   24, CIPHER_AES_192 },
  { "AES/256",   CAP_ENC_AES_256,   32, CIPHER_AES_256 },
  { "3DES/168",  CAP_ENC_3DES_168,  24, CIPHER_3DES    },
  { "IDEA/128",  CAP_ENC_IDEA_128,  16, CIPHER_IDEA    },
  { 0,           0,                  0, 0              }
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void server_format_cb(char **pptr, size_t *bptr, size_t n,
                             int padding, int left, void *arg)
{
  struct server *asptr = arg;
  char          *name;

  if(asptr)
    name = asptr->name;
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
static void server_shift(int64_t delta)
{
  struct server *sptr;

  dlink_foreach(&server_list, sptr)
    sptr->bstart += delta;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_init(void)
{
  uint32_t i;

  server_log = log_source_register("server");

  dlink_list_zero(&server_list);
  dlink_list_zero(&server_lists[SERVER_LOCAL]);
  dlink_list_zero(&server_lists[SERVER_REMOTE]);

  mem_static_create(&server_heap, sizeof(struct server), SERVER_BLOCK_SIZE);
  mem_static_note(&server_heap, "server heap");

  for(i = 0; i < 0x40; i++)
  {
    server_stats[i].letter = 0x40 + i;
    server_stats[i].cb = NULL;
  }

  server_id = 0;
  server_serial = 0;
  server_me = server_new(lclient_me->name, SERVER_LOCAL);
  server_me->lclient = lclient_me;
  server_me->connect = connect_pop(lclient_me->connect);
  lclient_me->server = server_me;

  str_register('S', server_format_cb);

  timer_shift_register(server_shift);

  log(server_log, L_status, "Initialised [server] module.");
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_shutdown(void)
{
  struct server *sptr;

  log(server_log, L_status, "Shutting down [server] module.");

  timer_shift_unregister(server_shift);

  str_unregister('S');

  dlink_foreach(&server_list, sptr)
    server_delete(sptr);

  mem_static_destroy(&server_heap);

  log_source_unregister(server_log);

  server_me = NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct server *server_new(const char *name, int location)
{
  struct server *sptr;

  sptr = mem_static_alloc(&server_heap);

  memset(sptr, 0, sizeof(struct server));

  strlcpy(sptr->name, name, sizeof(sptr->name));

  sptr->hash = str_ihash(sptr->name);
  sptr->id = server_id++;
  sptr->refcount = 1;
  sptr->location = location & 0x01;
  sptr->bstart = timer_mtime;
  sptr->lclient = NULL;
  sptr->client = NULL;
  sptr->progress = 0;

  dlink_list_zero(&sptr->deps[CLIENT_USER]);
  dlink_list_zero(&sptr->deps[CLIENT_SERVER]);
  dlink_list_zero(&sptr->deps[CLIENT_SERVICE]);

  dlink_add_tail(&server_list, &sptr->node, sptr);
  dlink_add_tail(&server_lists[sptr->location], &sptr->lnode, sptr);

  return sptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct server *server_new_local(struct lclient *lcptr)
{
  struct server *sptr;

  sptr = server_new(lcptr->name, SERVER_LOCAL);

  sptr->client = NULL;
  sptr->lclient = lclient_pop(lcptr);
  sptr->connect = connect_pop(lcptr->connect);

  return sptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct server *server_new_remote(struct lclient *lcptr, struct client *cptr,
                                 const char     *name,  const char    *info)
{
  struct server *sptr;

  sptr = server_new(name, SERVER_REMOTE);

  sptr->client = client_new_remote(CLIENT_SERVER, lcptr, NULL, cptr, sptr->name,
                                   cptr->hops + 1,
                                   sptr->name, sptr->name, info);
  sptr->client->server = sptr;

  return sptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_delete(struct server *sptr)
{
  log(server_log, L_verbose, "Removing server block: %s", sptr->name);

  server_release(sptr);

  dlink_delete(&server_list, &sptr->node);

  dlink_delete(&server_lists[sptr->location], &sptr->lnode);

  mem_static_free(&server_heap, sptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_set_name(struct server *sptr, const char *name)
{
  strlcpy(sptr->name, name, sizeof(sptr->name));

  sptr->hash = str_ihash(sptr->name);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct server *server_find_id(uint32_t id)
{
  struct server *sptr;

  dlink_foreach(&server_list, sptr)
    if(sptr->id == id)
      return sptr;

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct server *server_find_name(const char *name)
{
  struct server *sptr;
  hash_t         hash;
  
  hash = str_ihash(name);

  dlink_foreach(&server_list, sptr)
  {
    if(sptr->hash == hash)
      if(!str_icmp(sptr->name, name))
        return sptr;
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct server *server_find_namew(struct client *cptr, const char *name)
{
  struct server *sptr;
  hash_t         hash;
  
  hash = str_ihash(name);

  dlink_foreach(&server_list, sptr)
  {
    if(sptr->hash == hash)
      if(!str_icmp(sptr->name, name))
        return sptr;
  }

  numeric_send(cptr, ERR_NOSUCHSERVER, name);

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int server_connect(struct client *sptr, struct connect *cnptr)
{
  return connect_start(cnptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_vsend(struct lclient *one,    struct channel *chptr,
                  uint64_t        caps,   uint64_t        nocaps,
                  const char     *format, va_list         args)
{
  struct node     *node;
  struct lclient  *lcptr = NULL;
  struct chanuser *cuptr;
  size_t           n;
  struct fqueue    multi;
  char             buf[IRCD_LINELEN + 1];

  /* Formatted print */
  n = str_vsnprintf(buf, sizeof(buf) - 2, format, args);

  /* Add line separator */
  buf[n++] = '\r';
  buf[n++] = '\n';

  /* Start a multicast queue */
  io_multi_start(&multi);
  io_multi_write(&multi, buf, n);

  if(chptr == NULL)
  {
    dlink_foreach_data(&lclient_lists[LCLIENT_SERVER], node, lcptr)
    {
      if(lcptr == one)
        continue;

      if(lcptr == lclient_me)
        continue;

      if((lcptr->caps & caps) != caps)
        continue;

      if((lcptr->caps & nocaps) != 0)
        continue;

/*      if(lcptr->silent)
        continue;*/

      if(io_valid(lcptr->fds[1]))
      {
        io_multi_link(&multi, lcptr->fds[1]);
        lclient_update_sendb(lcptr, n);
#ifdef DEBUG
        buf[n - 2] = '\0';
        debug(ircd_log_out, "To %s: %s", lcptr->name, buf);
#endif /* DEBUG */
      }
      else
      {
        debug(server_log, "invalid fd: %i", lcptr->fds[1]);
      }
    }
  }
  else
  {
    lclient_serial++;

    dlink_foreach(&chptr->rchanusers, node)
    {
      cuptr = node->data;
      lcptr = cuptr->client->source;

/*      if(lcptr->silent)
        continue;*/

      if(lcptr->serial == lclient_serial)
        continue;

      if(lcptr == one)
        continue;

      if((lcptr->caps & caps) != caps)
        continue;

      if((lcptr->caps & nocaps) != 0)
        continue;

      io_multi_link(&multi, lcptr->fds[1]);
      lclient_update_sendb(lcptr, n);
#ifdef DEBUG
      buf[n - 2] = '\0';
      debug(ircd_log_out, "To %s: %s", lcptr->name, buf);
#endif /* DEBUG */
      lcptr->serial = lclient_serial;
    }
  }

  io_multi_end(&multi);
}

void server_send(struct lclient *one,    struct channel *chptr,
                 uint64_t        caps,   uint64_t        nocaps,
                 const char     *format, ...)
{
  va_list args;

  va_start(args, format);
  server_vsend(one, chptr, caps, nocaps, format, args);
  va_end(args);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int server_relay(struct lclient *lcptr, struct client *cptr,
                 struct server  *sptr,  const char    *format,
                 char          **argv)
{
  if(sptr == server_me)
    return 0;

  client_send(sptr->client, format, cptr,
              argv[2], argv[3], argv[4], argv[5],
              argv[6], argv[7], argv[8]);
  return 1;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int server_relay_always(struct lclient *lcptr,  struct client *cptr,
                        uint32_t        sindex, const char    *format,
                        int            *argc,   char         **argv)
{
  struct server *sptr;
  uint32_t       i;

  if((sptr = server_find_namew(cptr, argv[sindex])) == NULL)
    return -1;

  if(sptr == server_me)
  {
    for(i = sindex; argv[i]; i++)
      argv[i] = argv[i + 1];

    (*argc)--;

    return 0;
  }

  return server_relay(lcptr, cptr, sptr, format, argv);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int server_relay_maybe(struct lclient *lcptr,  struct client *cptr,
                       uint32_t        sindex, const char    *format,
                       int            *argc,   char         **argv)
{
  struct server *sptr;
  uint32_t       i;

  if((sptr = server_find_name(argv[sindex])) == NULL)
    return 0;

  if(sptr == server_me)
  {
    for(i = sindex; argv[i]; i++)
      argv[i] = argv[i + 1];

    (*argc)--;

    return 0;
  }

  return server_relay(lcptr, cptr, sptr, format, argv);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int server_find_capab(const char *capstr)
{
  size_t i;

  for(i = 0; server_caps[i].name; i++)
  {
    if(!str_icmp(server_caps[i].name, capstr))
      return i;
  }

  return -1;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int server_find_cipher(const char *cipher)
{
  size_t i = 0;

  for(i = 0; server_ciphers[i].name; i++)
  {
    if(!str_icmp(server_ciphers[i].name, cipher))
      return i + 1;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_send_pass(struct lclient *lcptr)
{
  struct conf_connect *ccptr;

  ccptr = lcptr->connect->args;

  lclient_send(lcptr, "PASS %s :TS", ccptr->passwd);

  log(server_log, L_debug, "Sending password '%s' to %s.",
      ccptr->passwd, lcptr->connect->name);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_send_capabs(struct lclient *lcptr)
{
  struct conf_connect *ccptr;
  uint32_t             cipherbit;
  uint32_t             i;
  uint32_t             n;
  int                  cipher = 0;
  int                  cap_can_send;
  int                  enc_can_send;
  char                 capbuf[IRCD_LINELEN];

  cap_can_send = server_default_caps |
    (conf_current.global.hub ? CAP_HUB : 0);
  enc_can_send = 0;

  ccptr = lcptr->connect->args;

  capbuf[0] = '\0';

  n = 0;

  for(i = 0; server_caps[i].name; i++)
  {
    if(server_caps[i].cap == CAP_SSL)
    {
      char ciphername[64];

      if(io_list[lcptr->fds[0]].ssl == NULL)
        continue;

      ssl_cipher(lcptr->fds[0], ciphername, sizeof(ciphername));

      n += strlcpy(&capbuf[n], ciphername, IRCD_LINELEN - n);
      capbuf[n++] = ' ';
      continue;
    }

    if(server_caps[i].cap & cap_can_send)
    {
      n += strlcpy(&capbuf[n], server_caps[i].name, IRCD_LINELEN - n);
      capbuf[n++] = ' ';
    }
  }

  if(ccptr->cipher[0])
    cipher = server_find_cipher(ccptr->cipher);

  if(cipher == 0)
    cipher = server_default_cipher;

  if(cipher)
  {
    cipherbit = 1 << (cipher - 1);

    if(cipherbit & enc_can_send)
    {
      strlcat(capbuf, "ENC:", IRCD_LINELEN);
      strlcat(capbuf, server_ciphers[i].name, IRCD_LINELEN);
      strlcat(capbuf, " ", IRCD_LINELEN);
    }
  }

  if(n)
    capbuf[n - 1] = '\0';
  else
    capbuf[n] = '\0';

  lclient_send(lcptr, "CAPAB :%s", capbuf);

  log(server_log, L_debug, "Sending capabs '%s' to %s.",
      capbuf, lcptr->connect->name);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_send_server(struct lclient *lcptr)
{
  lclient_send(lcptr, "SERVER %s :%s",
               client_me->name, client_me->info);

  log(server_log, L_debug, "Sending server '%s' (%s) to %s.",
      client_me->name, client_me->info, lcptr->connect->name);
}

/* -------------------------------------------------------------------------- *
 * After valid PASS/CAPAB/SERVER has been received                            *
 * -------------------------------------------------------------------------- */
void server_login(struct lclient *lcptr)
{
  struct connect      *cnptr;
  struct conf_connect *ccptr;
  struct class        *clptr;

  /* Cancel timeout */
  timer_push(&lcptr->ptimer);

  /* So the peer will get the error messages */
  if(server_find_name(lcptr->name))
  {
    log(server_log, L_warning, "Server %s already exists.",
        lcptr->name);

    lclient_exit(lcptr, "already exists");
    return;
  }

  /* Find connect{} block */
  if((cnptr = connect_find_name(lcptr->name)) == NULL)
  {
    log(server_log, L_warning, "No connect{} block for servername %s.",
        lcptr->name);

    lclient_exit(lcptr, "no connect{} block");
    return;
  }

  cnptr->fd = -1;
  connect_cancel(cnptr);
  cnptr->fd = lcptr->fds[0];
  cnptr->status = CONNECT_DONE;

  ccptr = cnptr->args;

  /* Check connection class */
  if((clptr = class_find_name(ccptr->class)) == NULL)
  {
    log(server_log, L_warning, "Invalid connection class: %s",
        ccptr->class);

    lclient_exit(lcptr, "invalid connection class");
    return;
  }

  /* Check the password */
  if(str_cmp(ccptr->passwd, lcptr->pass))
  {
    log(server_log, L_warning, "Password mismatch for %s.",
        lcptr->name);

    lclient_exit(lcptr, "password mismatch");
    return;
  }

  /* Link the connect{} */
  lcptr->connect = connect_pop(cnptr);

  /* Got listener? -> Its passive connection, we
     need to send handshake after we got SERVER */
  if(lcptr->listen)
  {
    /* Maybe we've to change the connection class */
    if(lcptr->class != clptr)
    {
      class_push(&lcptr->class);
      lcptr->class = class_pop(clptr);
    }

    /* Send handshake */
    server_send_pass(lcptr);
    server_send_capabs(lcptr);
    server_send_server(lcptr);
  }

  /* Register the server */
  lcptr->server = server_new_local(lcptr);
  lcptr->client = client_new_local(CLIENT_SERVER, lcptr);
  lcptr->server->client = lcptr->client;
  lcptr->client->server = lcptr->server;
  lcptr->silent = 0;
  lcptr->shut = 0;

  io_note(lcptr->fds[0], "server %s", lcptr->name);

  /* Hook for nifty stuff */
  if(hooks_call(server_login, HOOK_DEFAULT, lcptr, clptr) == 0)
    server_register(lcptr, clptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_progress(int fd, struct lclient *lcptr)
{
  uint32_t progress = lcptr->server->out.sendq - io_list[fd].sendq.size;

  if(io_list[fd].sendq.size)
  {
    uint32_t percent;
    uint32_t step;

    if(lcptr->server->out.sendq < 16384)
      step = 20;
    else if(lcptr->server->out.sendq < 131072)
      step = 10;
    else
      step = 5;

    percent = ((progress * 100) / lcptr->server->out.sendq);
    percent -= percent % step;

    if(percent > lcptr->server->progress)
      log(server_log, L_status, "Burst to %s progress: %2u%% (%u/%u)",
          lcptr->name, percent, progress, lcptr->server->out.sendq);

    lcptr->server->progress = percent;
  }
  else
  {
    log(server_log, L_status,
      "Bursted %u servers, %u clients, %u channels, %u channelmodes to %s.",
      lcptr->server->out.servers,
      lcptr->server->out.clients,
      lcptr->server->out.channels,
      lcptr->server->out.chanmodes,
      lcptr->name);

    io_register(lcptr->fds[1], IO_CB_WRITE, NULL);
    io_register(lcptr->fds[0], IO_CB_READ, lclient_recv, lcptr);

    /* Send a ping after netjoin burst */
    lcptr->ping = timer_mtime;
    server_ping(lcptr);

    /* Schedule the cyclic ping timer */
    lcptr->ptimer = timer_start(server_ping, lcptr->class->ping_freq, lcptr);

    timer_note(lcptr->ptimer, "ping timer for server %s",
               lcptr->server->name);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_register(struct lclient *lcptr, struct class *clptr)
{
  lcptr->silent = 0;

  /* Introduce the new server to other servers */
  server_send(lcptr, NULL, CAP_NSV, CAP_NONE,
              "NSERVER %S %s :%s",
              server_me, lcptr->name, lcptr->info);
  server_send(lcptr, NULL, CAP_NONE, CAP_NSV,
              "SERVER %s :%s",
              lcptr->name, lcptr->info);

  /* Start bursting */
  server_burst(lcptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int server_ping(struct lclient *lcptr)
{
  if(lcptr->ping == 0LLU)
  {
/*    int fds[2];

    fds[0] = lcptr->fds[0];
    fds[1] = lcptr->fds[1];*/

    lclient_exit(lcptr, "ping timeout: %u seconds",
                 (uint32_t)((lcptr->class->ping_freq + 500LL) / 1000LL));

/*    io_shutup(fds[0]);
    io_shutup(fds[1]);
    io_destroy(fds[0]);

    if(fds[0] != fds[1])
      io_destroy(fds[1]);*/

    return 0;
  }

  lcptr->ping = 0LLU;
  lclient_send(lcptr, "PING :%llu", timer_mtime);
  
  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_introduce(struct lclient *lcptr, struct server *sptr)
{
  struct client *cptr = NULL;
  struct node   *nptr;

  if(sptr != server_me && sptr->client != client_me &&
     lcptr->server != sptr && sptr->client->source != lcptr &&
     lcptr != sptr->client->origin->source)
  {
    if(lcptr->caps & CAP_NSV)
    {
      lclient_send(lcptr, "NSERVER %N %S :%s",
                   sptr->client->origin,
                   sptr,
                   sptr->client->info);
    }
    else
    {
      if(server_is_local(sptr))
        lclient_send(lcptr, "SERVER %s :%s", sptr->name, sptr->client->info);
      else
        lclient_send(lcptr, ":%s SERVER %s :%s",
                     sptr->client->origin->name,
                     sptr->name, sptr->client->info);
    }
  }

  dlink_foreach_data(&sptr->deps[CLIENT_SERVER], nptr, cptr)
    server_introduce(lcptr, cptr->server);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_links(struct client *cptr, struct server *sptr)
{
  struct client *acptr = NULL;
  struct node   *nptr;

  dlink_foreach_data(&sptr->deps[CLIENT_SERVER], nptr, acptr)
  {
    if(acptr->server == NULL)
    {
      log(server_log, L_warning, "invalid recursion while server_links on %s.",
          sptr->name);
      return;
    }

    if(acptr->server != sptr)
      server_links(cptr, acptr->server);
  }

  numeric_send(cptr, RPL_LINKS,
               sptr->name, sptr->client->origin->name,
               sptr->client->hops, sptr->client->info);


  if(sptr == server_me)
    numeric_send(cptr, RPL_ENDOFLINKS, sptr->name);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_burst(struct lclient *lcptr)
{
  struct channel *chptr;
  struct client  *acptr = NULL;
  struct node    *node;

  /* send start of burst */
  log(server_log, L_verbose, "Bursting %s to %s.", client_me->name, lcptr->name);

  lclient_send(lcptr, "BURST");

  /* burst all servers */
  server_introduce(lcptr, server_me);

  lcptr->server->out.servers = server_list.size - 2;

  /* burst channels and channel members of each channel */
  client_serial++;
  lcptr->server->out.chanmodes = 0;

  dlink_foreach(&channel_list, chptr)
    lcptr->server->out.chanmodes += channel_burst(lcptr, chptr);

  lcptr->server->out.channels = channel_list.size;

  /* burst remaining clients */
  dlink_foreach_data(&client_lists[CLIENT_GLOBAL][CLIENT_USER], node, acptr)
  {
    if(acptr->serial != client_serial)
    {
      client_burst(lcptr, acptr);
      acptr->serial = client_serial;
    }
  }

  lcptr->server->out.clients = client_lists[CLIENT_GLOBAL][CLIENT_USER].size;

  /* custom burst */
  hooks_call(server_burst, HOOK_DEFAULT, lcptr);

  lcptr->server->out.sendq = io_list[lcptr->fds[1]].sendq.size;

  lclient_send(lcptr, "BURST %u %u %u %u %u",
               lcptr->server->out.sendq,
               lcptr->server->out.servers,
               lcptr->server->out.clients,
               lcptr->server->out.channels,
               lcptr->server->out.chanmodes);

  io_register(lcptr->fds[1], IO_CB_WRITE, server_progress, lcptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_split(struct server *sptr, const char *uplink, const char *victim)
{
  struct chanuser *cuptr;
  struct client   *cptr;
  struct node     *node;
  struct node     *next;

  dlink_foreach_safe(&sptr->deps[CLIENT_USER], node, next)
  {
    cptr = node->data;

    log(server_log, L_verbose, "splitting client %s", cptr->name);

    chanuser_send(NULL, cptr, ":%s!%s@%s QUIT :%s %s",
                  cptr->name, cptr->user->name, cptr->host,
                  uplink, victim);

    dlink_foreach_safe(&cptr->user->channels, cuptr, node)
    {
      chanuser_delete(cuptr);

      if(cuptr->channel->chanusers.size == 0)
        channel_delete(cuptr->channel);
    }

    client_delete(cptr);
  }

  dlink_foreach_safe(&sptr->deps[CLIENT_SERVER], node, next)
  {
    cptr = node->data;

/*    if(cptr == sptr->client)
      continue;

    if(cptr->server == NULL)
    {
      log(server_log, L_warning, "split recursion error from %s", sptr->name);
      return;
    }*/

    log(server_log, L_verbose, "splitting server %s", cptr->name);

    server_split(cptr->server, uplink, victim);
    client_delete(cptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_exit(struct lclient *lcptr, struct client *cptr,
                 struct server  *sptr,  const char    *reason)
{
  size_t split_servers;
  size_t split_clients;
  size_t split_channels;

  if(client_is_local(sptr->client))
    log(lclient_log, L_status, "Local server %s exited: %s (send: %u.%03ukb recv: %u.%03ukb)",
        sptr->client->lclient->name, reason,
        sptr->client->lclient->sendk,
        (uint32_t)((((double)sptr->client->lclient->sendb) * 1000) / 1024),
        sptr->client->lclient->recvk,
        (uint32_t)((((double)sptr->client->lclient->recvb) * 1000) / 1024));

  if(reason)
  {
    if(cptr)
      server_send(lcptr, NULL, CAP_NONE, CAP_NONE,
                  ":%N NQUIT %S :%s", cptr, sptr, reason);
    else
      server_send(lcptr, NULL, CAP_NONE, CAP_NONE,
                  "NQUIT %S :%s", sptr, reason);
  }
  else
  {
    if(cptr)
      server_send(lcptr, NULL, CAP_NONE, CAP_NONE,
                  ":%N NQUIT %S", cptr, sptr);
    else
      server_send(lcptr, NULL, CAP_NONE, CAP_NONE,
                  "NQUIT %S", sptr);
  }

  split_servers = server_list.size;
  split_clients = client_lists[CLIENT_GLOBAL][CLIENT_USER].size;
  split_channels = channel_list.size;

  if(cptr)
    server_split(sptr, cptr->name, sptr->name);
  else
    server_split(sptr, client_me->name, sptr->name);

  split_servers -= server_list.size;
  split_clients -= client_lists[CLIENT_GLOBAL][CLIENT_USER].size;
  split_channels -= channel_list.size;

  if(sptr->connect)
    connect_cancel(sptr->connect);

  if(cptr)
  {
    log(server_log, L_warning,
        "%s splitted from %s. Lost %u servers, %u clients, %u channels.",
        sptr->name, cptr->name, split_servers, split_clients, split_channels);
  }
  else
  {
    log(server_log, L_warning,
        "%s splitted. Lost %u servers, %u clients, %u channels.",
        sptr->name, split_servers, split_clients, split_channels);
  }

  server_delete(sptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_release(struct server *sptr)
{
  if(sptr->client)
  {
    if(sptr->client->server)
      sptr->client->server = NULL;

    sptr->client = NULL;
  }
  if(sptr->lclient)
  {
    if(sptr->lclient->server)
      sptr->lclient->server = NULL;

    sptr->lclient = NULL;
  }

  connect_push(&sptr->connect);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_stats_register(char letter, void (*cb)(struct client *))
{
  server_stats[(uint32_t)letter - 0x40].letter = letter;
  server_stats[(uint32_t)letter - 0x40].cb = cb;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_stats_unregister(char letter)
{
  server_stats[(uint32_t)letter - 0x40].letter = '\0';
  server_stats[(uint32_t)letter - 0x40].cb = NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_stats_show(struct client *cptr, char letter)
{
  if(!chars_isalpha(letter))
    return;

  if(server_stats[(uint32_t)letter - 0x40].cb)
  {
    server_stats[(uint32_t)letter - 0x40].cb(cptr);
    numeric_send(cptr, RPL_ENDOFSTATS, letter);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void server_dump(struct server *sptr)
{
  if(sptr == NULL)
  {
    struct node *node;

    dump(server_log, "[============== server summary ===============]");

    if(server_lists[SERVER_LOCAL].size)
    {
      dump(server_log, " -------------- local servers ---------------- ");

      dlink_foreach_data(&server_lists[SERVER_LOCAL], node, sptr)
        dump(server_log, " #%u: [%u] %-20s (%s)",
              sptr->id, sptr->refcount,
              sptr->name[0] ? sptr->name : "<unknown>",
              sptr->client ? sptr->client->host : "0.0.0.0");
    }
    if(server_lists[SERVER_REMOTE].size)
    {
      dump(server_log, " -------------- remote servers --------------- ");

      dlink_foreach_data(&server_lists[SERVER_REMOTE], node, sptr)
        dump(server_log, " #%u: [%u] %-20s (%s)",
              sptr->id, sptr->refcount,
              sptr->name[0] ? sptr->name : "<unknown>",
              sptr->client ? sptr->client->host : "0.0.0.0");
    }

    dump(server_log, "[=========== end of server summary ===========]");
  }
  else
  {
    dump(server_log, "[============== server dump ===============]");
    dump(server_log, "         id: #%u", sptr->id);
    dump(server_log, "   refcount: %u", sptr->refcount);
    dump(server_log, "       hash: %p", sptr->hash);
    dump(server_log, "   location: %s",
          sptr->location == CLIENT_LOCAL ? "local" : "remote");
    dump(server_log, "     client: %s [%i]",
          sptr->client ? sptr->client->name : "(nil)",
          sptr->client ? sptr->client->refcount : 0);
    dump(server_log, "    lclient: %s [%i]",
          sptr->lclient ? sptr->lclient->name : "(nil)",
          sptr->lclient ? sptr->lclient->refcount : 0);
    dump(server_log, "    connect: %s [%i]",
          sptr->connect ? sptr->connect->name : "(nil)",
          sptr->connect ? sptr->connect->refcount : 0);
    dump(server_log, "      users: %u links", sptr->deps[CLIENT_USER].size);
    dump(server_log, "    servers: %u links", sptr->deps[CLIENT_SERVER].size);
    dump(server_log, "     bstart: %llu msecs", sptr->bstart);
    dump(server_log, "       name: %s", sptr->name);

    dump(server_log, "[=========== end of server dump ===========]");
  }
}
