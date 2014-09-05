/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: lc_sauth.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "dlink.h"
#include "io.h"
#include "ini.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "hook.h"
#include "sauth.h"
#include "timer.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/user.h>
#include <ircd/msg.h>
#include <ircd/lclient.h>
#include <ircd/server.h>
#include <ircd/client.h>
#include <ircd/numeric.h>

/* -------------------------------------------------------------------------- *
 * Constants                                                                  *
 * -------------------------------------------------------------------------- */

#define LC_SAUTH_HASH_SIZE 16

#define M_PROXY_INI        "proxy.ini"
#define M_PROXY_INTERVAL   (5 * 60 * 1000)

/* -------------------------------------------------------------------------- *
 * Types                                                                      *
 * -------------------------------------------------------------------------- */
struct lc_sauth {
  struct node     node;
  struct lclient *lclient;
  struct timer   *timer_reg;
  struct timer   *timer_auth;
  struct sauth   *sauth_dns;
  struct sauth   *sauth_auth;
  struct list     sauth_proxy;
  int             done_dns;
  int             done_auth;
  int             done_proxy;
};

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void             lc_sauth_handshake   (struct lclient  *lcptr);
static void             lc_sauth_release     (struct lclient  *lcptr);
static int              lc_sauth_register    (struct lclient  *lcptr);

static void             lc_sauth_done        (struct lc_sauth *arg);
static void             lc_sauth_lookup_dns  (struct lc_sauth *arg);
static int              lc_sauth_lookup_auth (struct lc_sauth *arg);
static int              lc_sauth_check_proxy (struct lc_sauth *arg);
static void             lc_sauth_dns         (struct sauth    *sauth,
                                              struct lc_sauth *arg);
static void             lc_sauth_auth        (struct sauth    *sauth,
                                              struct lc_sauth *arg);
static void             lc_sauth_proxy       (struct sauth    *sauth,
                                              struct lc_sauth *arg);
static struct node     *m_proxy_add          (uint16_t         port,
                                              int              service);
static struct node     *m_proxy_find         (uint16_t         port,
                                              int              service);
static void             mo_proxy             (struct lclient  *lcptr,
                                              struct client   *cptr,
                                              int              argc,
                                              char           **argv);
static int              m_proxy_cleanup      (void);
static int              m_proxy_save         (void);
static void             m_proxy_callback     (struct ini      *ini);

/* -------------------------------------------------------------------------- *
 * Local variables                                                            *
 * -------------------------------------------------------------------------- */

static struct list   lc_sauth_list;
static struct sheap  lc_sauth_heap;

static struct list   m_proxy_list;
static struct ini   *m_proxy_ini;
static struct timer *m_proxy_timer;

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mo_proxy_help[] = {
  "PROXY [server] <list|add|delete> [port] [type]",
  "",
  "Manages ports for the proxy scanner tests.",
  "",
  "list        Lists all scanned ports.",
  "add         Adds a port/service pair.",
  "delete      Deletes a port/service pair.",
  "",
  "Valid types are: http, socks4, socks5, wingate, cisco",
  "",
  "Examples:",
  "/PROXY add 8080 http",
  "/PROXY add 1080 socks4",
  "/PROXY add 23 wingate",
  "/PROXY add 23 cisco",
  NULL
};

static struct msg m_proxy_msg = {
  "PROXY", 1, 4, MFLG_OPER,
  { NULL, NULL, mo_proxy, mo_proxy },
  mo_proxy_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int lc_sauth_load(void)
{
  if(hook_register(lclient_handshake, HOOK_DEFAULT, lc_sauth_handshake) == NULL)
    return -1;

  hook_register(lclient_release, HOOK_DEFAULT, lc_sauth_release);
  hook_register(lclient_register, HOOK_DEFAULT, lc_sauth_register);

  mem_static_create(&lc_sauth_heap, sizeof(struct lc_sauth),
                    SAUTH_BLOCK_SIZE / 2);
  mem_static_note(&lc_sauth_heap, "lclient resolver heap");

  dlink_list_zero(&lc_sauth_list);
  dlink_list_zero(&m_proxy_list);

  msg_register(&m_proxy_msg);

  if((m_proxy_ini = ini_find_name(M_PROXY_INI)) == NULL)
  {
    log(lclient_log, L_warning,
        "Could not find proxy checklist, add '"M_PROXY_INI
        "' to your config file.");
  }

  if(m_proxy_ini)
    ini_callback(m_proxy_ini, m_proxy_callback);

  m_proxy_timer = timer_start(m_proxy_cleanup, M_PROXY_INTERVAL);

  timer_note(m_proxy_timer, "m_proxy cleaup timer");

  return 0;
}

void lc_sauth_unload(void)
{
  struct lc_sauth *arg = NULL;
  struct node     *node;
  struct node     *next;

  msg_unregister(&m_proxy_msg);

  /* Remove all pending sauth stuff */
  dlink_foreach_safe_data(&lc_sauth_list, node, next, arg)
    lc_sauth_done(arg);

  hook_unregister(lclient_register, HOOK_DEFAULT, lc_sauth_register);
  hook_unregister(lclient_release, HOOK_DEFAULT, lc_sauth_release);
  hook_unregister(lclient_handshake, HOOK_DEFAULT, lc_sauth_handshake);

  mem_static_destroy(&lc_sauth_heap);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void lc_sauth_handshake(struct lclient *lcptr)
{
  struct lc_sauth *arg;

  /* Keep track of the lclient if the module gets unloaded */
  arg = mem_static_alloc(&lc_sauth_heap);

  arg->lclient = lclient_pop(lcptr);
  arg->done_dns = 0;
  arg->done_auth = 0;
  arg->sauth_dns = NULL;
  arg->sauth_auth = NULL;

  dlink_add_tail(&lc_sauth_list, &arg->node, arg);

  lcptr->shut = 1;

  /* Start DNS lookup */
  lc_sauth_lookup_dns(arg);
}


/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void lc_sauth_release(struct lclient *lcptr)
{
  struct lc_sauth *sauth;

  sauth = lcptr->plugdata[LCLIENT_PLUGDATA_SAUTH];

  if(sauth)
  {
    timer_cancel(&sauth->timer_reg);

    lc_sauth_done(sauth);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int lc_sauth_register(struct lclient *lcptr)
{
  struct lc_sauth *sauth;

  sauth = lcptr->plugdata[LCLIENT_PLUGDATA_SAUTH];

  if(sauth)
  {
    lc_sauth_done(sauth);
    lcptr->plugdata[LCLIENT_PLUGDATA_SAUTH] = NULL;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void lc_sauth_done(struct lc_sauth *arg)
{
  struct node *nptr;

  if(arg->timer_reg)
  {
    lclient_register(arg->lclient);
    timer_cancel(&arg->timer_reg);
  }

  timer_cancel(&arg->timer_auth);

  if(arg->sauth_dns)
  {
    sauth_delete(arg->sauth_dns);
    arg->sauth_dns = NULL;
  }

  if(arg->sauth_auth)
  {
    sauth_delete(arg->sauth_auth);
    arg->sauth_auth = NULL;
  }

  if(arg->sauth_proxy.size)
  {
    dlink_foreach(&arg->sauth_proxy, nptr)
      sauth_delete(nptr->data);

    dlink_destroy(&arg->sauth_proxy);
  }

  dlink_delete(&lc_sauth_list, &arg->node);

  mem_static_free(&lc_sauth_heap, arg);
}

/* -------------------------------------------------------------------------- *
 * Start a DNS lookup for a local client                                      *
 * -------------------------------------------------------------------------- */
static void lc_sauth_lookup_dns(struct lc_sauth *arg)
{
  uint8_t *ip = (uint8_t *)&arg->lclient->addr_remote;

  /* Start reverse lookup */
  arg->sauth_dns = sauth_dns_reverse(arg->lclient->addr_remote,
                                     lc_sauth_dns, arg);

  /* Report start of the dns lookup */
  if(arg->sauth_dns)
  {
    lclient_send(arg->lclient,
                 ":%s NOTICE %s :(dns) looking up %u.%u.%u.%u.in-addr.arpa.",
                 lclient_me->name, arg->lclient->name,
                 (uint32_t)ip[3], (uint32_t)ip[2],
                 (uint32_t)ip[1], (uint32_t)ip[0]);
  }
  else
  {
    /* Huh, dns failed, maybe servauth done, let's try the auth query */
    lclient_send(arg->lclient, ":%s NOTICE %s :(dns) servauth down.",
                 lclient_me->name, arg->lclient->name);
    lc_sauth_lookup_auth(arg);
  }

}

/* -------------------------------------------------------------------------- *
 * Start an AUTH lookup for a local client                                    *
 * -------------------------------------------------------------------------- */
static int lc_sauth_lookup_auth(struct lc_sauth *arg)
{
  /* Start AUTH query */
  arg->sauth_auth = sauth_auth(arg->lclient->addr_remote,
                               arg->lclient->port_remote,
                               arg->lclient->port_local,
                               lc_sauth_auth, arg);

  /* Report start of the auth lookup */
  if(arg->sauth_auth)
  {
    lclient_send(arg->lclient, ":%s NOTICE %s :(auth) checking %s:%u -> %s:%u.",
                 lclient_me->name, arg->lclient->name,
                 arg->lclient->host, arg->lclient->port_remote,
                 lclient_me->name, arg->lclient->port_local);
  }
  else
  {
    /* Huh, dns failed, maybe servauth done, let's register */
    lclient_send(arg->lclient, ":%s NOTICE %s :(auth) servauth down.",
                 lclient_me->name, arg->lclient->name);

    lclient_register(arg->lclient);
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Start a proxy check for a local client                                     *
 * -------------------------------------------------------------------------- */
static int lc_sauth_check_proxy(struct lc_sauth *arg)
{
  struct node *nptr;
  struct node *node;
  uint16_t     port;
  int          type;

  dlink_foreach(&m_proxy_list, nptr)
  {
    size_t val = (size_t)nptr->data & 0xfffffffflu;
    port = val >> 16;
    type = val & 0xffff;

    node = dlink_node_new();

    node->data = sauth_proxy(type,
                             arg->lclient->addr_remote,
                             port,
                             arg->lclient->addr_local,
                             arg->lclient->port_local,
                             lc_sauth_proxy, arg);

    if(node->data)
      dlink_add_tail(&arg->sauth_proxy, node, node->data);
    else
      dlink_node_free(node);
  }

  /* Report start of the proxy scan */
  if(arg->sauth_proxy.size)
  {
    lclient_send(arg->lclient, ":%s NOTICE %s :(proxy) initiating proxy check %s -> %s",
                 lclient_me->name, arg->lclient->name,
                 arg->lclient->host,
                 net_ntoa(arg->lclient->addr_local));
  }
  else
  {
    /* Huh, scan failed, maybe servauth done, let's register */
    lclient_send(arg->lclient, ":%s NOTICE %s :(proxy) servauth down.",
                 lclient_me->name, arg->lclient->name);

    lclient_register(arg->lclient);
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * DNS lookup callback, called upon DNS failure or completion                 *
 * -------------------------------------------------------------------------- */
void lc_sauth_dns(struct sauth *sauth, struct lc_sauth *arg)
{
  if(sauth->host[0])
  {
    /* We got a reply, copy it to lclient struct and report success */
    strlcpy(arg->lclient->host, sauth->host, sizeof(arg->lclient->host));

    lclient_send(arg->lclient,
                 ":%s NOTICE %s :(dns) resolved your address to %s.",
                 lclient_me->name, arg->lclient->name,
                 arg->lclient->host);
  }
  else
  {
    /* Report failure */
    lclient_send(arg->lclient,
                 ":%s NOTICE %s :(dns) could not resolve your address.",
                 lclient_me->name, arg->lclient->name,
                 arg->lclient->host);
  }

  lc_sauth_lookup_auth(arg);

  /* Destroy DNS sauth instance */
  sauth_push(&arg->sauth_dns);
}


/* -------------------------------------------------------------------------- *
 * AUTH lookup callback, called upon AUTH failure or completion               *
 * -------------------------------------------------------------------------- */
void lc_sauth_auth(struct sauth *sauth, struct lc_sauth *arg)
{
  if(sauth->ident[0])
  {
    /* We got a reply, copy it to lclient struct and report success */
    strlcpy(arg->lclient->user->name, sauth->ident, sizeof(arg->lclient->user->name));

    lclient_send(arg->lclient, ":%s NOTICE %s :(auth) got ident response: %s",
                 lclient_me->name,
                 arg->lclient->name,
                 arg->lclient->user->name);
  }
  else
  {
    /* Report failure */
    lclient_send(arg->lclient, ":%s NOTICE %s :(auth) no ident response.",
                 lclient_me->name, arg->lclient->name);
  }

  lc_sauth_check_proxy(arg);

  sauth_push(&arg->sauth_auth);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void lc_sauth_deny(struct lc_sauth *arg, uint16_t port, int type)
{
  struct msg *mptr;
  char        mask[32];
  char        msg[128];
  char *argv[] = { client_me->name, "GLINE", mask, msg, NULL };

  str_snprintf(mask, sizeof(mask), "*@%s", net_ntoa(arg->lclient->addr_remote));

  str_snprintf(msg, sizeof(msg), "open %s proxy on port %u",
           sauth_types[type], (uint32_t)port);

  lclient_set_type(arg->lclient, LCLIENT_USER);
  lclient_exit(arg->lclient, "%s", msg);

  mptr = msg_find("GLINE");

  mptr->handlers[MSG_OPER](lclient_me, client_me, 4, argv);

  arg->lclient = NULL;
}

/* -------------------------------------------------------------------------- *
 * AUTH lookup callback, called upon AUTH failure or completion               *
 * -------------------------------------------------------------------------- */
void lc_sauth_proxy(struct sauth *sauth, struct lc_sauth *arg)
{
  struct node *nptr;

  lclient_send(arg->lclient, ":%s NOTICE %s :(proxy) %u/%s : %s",
               lclient_me->name,
               arg->lclient->name,
               sauth->remote,
               sauth_types[sauth->ptype],
               sauth_replies[sauth->reply]);

  nptr = dlink_find_delete(&arg->sauth_proxy, sauth);

  if(sauth->reply == SAUTH_PROXY_OPEN)
  {
    lc_sauth_done(arg);
    lc_sauth_deny(arg, sauth->remote, sauth->ptype);
  }
  else if(arg->sauth_proxy.size == 0)
  {
    lclient_register(arg->lclient);
    lc_sauth_done(arg);
  }

  if(nptr)
  {
    sauth_push((struct sauth **)(void *)&nptr->data);
    dlink_node_free(nptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static struct node *m_proxy_find(uint16_t port, int service)
{
  struct node *node;

  dlink_foreach(&m_proxy_list, node)
  {
    size_t val = (size_t)node->data & 0xfffffffflu;

    if((val >> 16) == port && (val & 0xffff) == service)
      return node;
  }

  return NULL;
}

static struct node *m_proxy_add(uint16_t port, int service)
{
  struct node *node;
  struct node *nptr;
  uint32_t     data;

  data = (uint32_t)port << 16;
  data |= service & 0xffff;

  nptr = dlink_node_new();

  dlink_foreach(&m_proxy_list, node)
  {
    if((size_t)node->data > (size_t)data)
    {
      dlink_add_before(&m_proxy_list, nptr, node, (void *)(size_t)data);


      return nptr;
    }
  }

  dlink_add_tail(&m_proxy_list, nptr, (void *)(size_t)data);

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int m_proxy_save(void)
{
  struct ini_section *isptr;
  struct node        *nptr;
  uint16_t            port;
  int                 type;
  char                namebuf[32];

  ini_clear(m_proxy_ini);

  dlink_foreach(&m_proxy_list, nptr)
  {
    size_t val = (size_t)nptr->data & 0xfffffffflu;
    port = val >> 16;
    type = val & 0xffff;

    str_snprintf(namebuf, sizeof(namebuf), "%u", (uint32_t)port);

    isptr = ini_section_new(m_proxy_ini, namebuf);

    ini_write_str(isptr, "type", sauth_types[type]);
  }

  ini_open(m_proxy_ini, INI_WRITE);
  io_queue_control(m_proxy_ini->fd, OFF, OFF, OFF);
  ini_save(m_proxy_ini);
  ini_close(m_proxy_ini);

  return 0;
}

static int m_proxy_load(void)
{
  struct ini_section *isptr;
  uint16_t            port;
  int                 type;
  char                typestr[32];

  if(m_proxy_ini == NULL)
    return -1;

  if((isptr = ini_section_first(m_proxy_ini)))
  {
    do
    {
      port = str_toul(isptr->name, NULL, 10);

      if(!ini_get_str(isptr, "type", typestr, 32))
      {
        type = sauth_proxy_type(typestr);

        if(type >= 0)
        {
          if(m_proxy_find(port, type) == NULL)
            m_proxy_add(port, type);
        }
      }
    }
    while((isptr = ini_section_next(m_proxy_ini)));
  }

  /* Close the INI file */
  ini_close(m_proxy_ini);

  return 0;
}

int m_proxy_cleanup(void)
{
  if(m_proxy_ini)
  {
    if(m_proxy_save())
    {
      log(lclient_log, L_warning, "Failed saving proxy checklist '%s'.",
          m_proxy_ini->name);
    }
  }
  else
  {
    if((m_proxy_ini = ini_find_name(M_PROXY_INI)) == NULL)
      log(lclient_log, L_status,
          "Could not find proxy database, add '"
          M_PROXY_INI"' to your config file.");
    else
      log(lclient_log, L_status, "Found proxy checklist: %s", m_proxy_ini->path);
  }

  return 0;
}

void m_proxy_callback(struct ini *ini)
{
  log(lclient_log, L_warning, "parsing proxy ini!!!!!!!!!!");

  if(m_proxy_load())
  {
    log(client_log, L_warning, "Failed loading proxy checklist '%s'.",
        m_proxy_ini->name);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void mo_proxy(struct lclient *lcptr, struct client *cptr, int argc, char **argv)
{
  uint16_t     port = 0;
  int          type = 0;
  struct node *nptr;

  if(argc > 3)
  {
    if(argv[5])
    {
      if(server_relay_maybe(lcptr, cptr, 2, ":%C PROXY %s %s %s :%s", &argc, argv))
        return;
    }
    else if(argv[4])
    {
      if(server_relay_maybe(lcptr, cptr, 2, ":%C PROXY %s %s :%s", &argc, argv))
        return;
    }
    else
    {
      if(server_relay_maybe(lcptr, cptr, 1, ":%C PROXY %s :%s", &argc, argv))
        return;
    }
  }

  if(argv[3])
  {
    port = (uint16_t)str_toul(argv[3], NULL, 10);

    if(argv[4])
    {
      type = sauth_proxy_type(argv[4]);

      if(type == -1)
      {
        client_send(cptr, ":%C NOTICE %C :*** invalid proxy type: %s",
                    client_me, cptr, argv[4]);
        return;
      }
    }
  }

  if(!str_icmp(argv[2], "list"))
  {
    struct node *node;

    client_send(cptr, ":%C NOTICE %C : ======= proxy checklist ======== ",
                client_me, cptr);
    client_send(cptr, ":%C NOTICE %C :  port service",
                client_me, cptr);
    client_send(cptr, ":%C NOTICE %C : ---------------------------- ",
                client_me, cptr);

    dlink_foreach(&m_proxy_list, node)
    {
      client_send(cptr, ":%C NOTICE %C : %5u %s",
                  client_me, cptr,
                  ((uint32_t)(size_t)node->data) >> 16,
                  sauth_types[((uint32_t)(size_t)node->data) & 0xffff]);
    }

    client_send(cptr, ":%C NOTICE %C : ==== end of proxy checklist ==== ",
                client_me, cptr);

    return;
  }

  if(argc < 5)
  {
    numeric_send(cptr, ERR_NEEDMOREPARAMS, "PROXY");
    return;
  }

  if(!str_icmp(argv[2], "add")) {

    if(m_proxy_find(port, type))
      return;

    m_proxy_add(port, type);

    client_send(cptr, ":%C NOTICE %C :*** added proxy check for %u/%s",
                client_me, cptr,
                (uint32_t)port, sauth_types[type]);

  } else if(!str_nicmp(argv[2], "del", 3)) {

    if(argc < 5)
    {
      numeric_send(cptr, ERR_NEEDMOREPARAMS, "PROXY");
      return;
    }

    nptr = m_proxy_find(port, type);

    if(nptr == NULL)
    {
      client_send(cptr, ":%C NOTICE %C :*** no such proxy check: %u/%s",
                  client_me, cptr,
                  (uint32_t)port, sauth_types[type]);
    }
    else
    {
      dlink_delete(&m_proxy_list, nptr);
      dlink_node_free(nptr);

      client_send(cptr, ":%C NOTICE %C :*** deleted proxy check for: %u/%s",
                  client_me, cptr,
                  (uint32_t)port, sauth_types[type]);
    }

  } else {
    client_send(cptr, ":%C NOTICE %C :*** invalid command: %s",
                client_me, cptr, argv[2]);
  }
}

