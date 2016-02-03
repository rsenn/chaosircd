/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003,2004  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: m_spoof.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/io.h"
#include "libchaos/ini.h"
#include "libchaos/log.h"
#include "libchaos/net.h"
#include "libchaos/str.h"
#include "libchaos/hook.h"
#include "libchaos/module.h"
#include "libchaos/timer.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/user.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/channel.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define M_SPOOF_INTERVAL  (5 * 60 * 1000ull)
#define M_SPOOF_SPOOFTIME (24 * 3600 * 1000ull)
#define M_SPOOF_LIFETIME  (M_SPOOF_SPOOFTIME * 7)
#define M_SPOOF_BLOCKSIZE 32
#define M_SPOOF_INI       "spoof.ini"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void                  m_spoof            (struct lclient       *lcptr,
                                                 struct client        *cptr,
                                                 int                   argc,
                                                 char                **argv);
static void                  ms_spoof           (struct lclient       *lcptr,
                                                 struct client        *cptr,
                                                 int                   argc,
                                                 char                **argv);
static int                   m_spoof_cleanup    (void);
static void                  m_spoof_callback   (struct ini           *ini);
static struct m_spoof_entry *m_spoof_new        (net_addr_t            ip,
                                                 uint64_t              ts,
                                                 const char           *host);
static struct m_spoof_entry *m_spoof_add        (net_addr_t            ip,
                                                 uint64_t              ts,
                                                 const char           *host);
static void                  m_spoof_delete     (struct m_spoof_entry *mseptr);
static int                   m_spoof_loaddb     (void);
static int                   m_spoof_savedb     (void);
static struct m_spoof_entry *m_spoof_find_ip    (net_addr_t            ip);
static struct m_spoof_entry *m_spoof_find_host  (const char           *host);
static void                  m_spoof_hook_exit  (struct lclient       *lcptr,
                                                 struct client        *cptr,
                                                 const char           *comment);
static void                  m_spoof_hook_burst (struct lclient       *lcptr,
                                                 struct client        *acptr);
static void                  m_spoof_hook_local (struct client        *cptr,
                                                 struct user          *uptr);
static void                  m_spoof_hook_remote(struct client        *cptr,
                                                 struct user          *uptr);
static void                  m_spoof_stats      (struct client        *cptr);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_spoof_help[] =
{
  "SPOOF <hostname>",
  "",
  "Spoofs your hostname to a freeform string.",
  "This hostname will persist for at least 24 hours.",
  NULL
};

static struct msg m_spoof_msg =
{
  "SPOOF", 1, 3, MFLG_CLIENT,
  { NULL, m_spoof, ms_spoof, m_spoof },
  m_spoof_help
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct m_spoof_entry
{
  struct node    node;
  net_addr_t     ip;
  uint64_t       ts;
  char           host[IRCD_HOSTLEN];
  hash_t           hash;
  int            waslame;
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static struct list   m_spoof_list;
static struct timer *m_spoof_timer;
static struct sheap  m_spoof_heap;
static struct ini   *m_spoof_ini;

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_spoof_load(void)
{
  dlink_list_zero(&m_spoof_list);

  if(msg_register(&m_spoof_msg) == NULL)
    return -1;

  m_spoof_timer = timer_start(m_spoof_cleanup, M_SPOOF_INTERVAL);

  timer_note(m_spoof_timer, "m_spoof timer");

  mem_static_create(&m_spoof_heap, sizeof(struct m_spoof_entry),
                    M_SPOOF_BLOCKSIZE);
  mem_static_note(&m_spoof_heap, "spoof heap");

  hook_register(client_exit, HOOK_DEFAULT, m_spoof_hook_exit);
  hook_register(client_burst, HOOK_DEFAULT, m_spoof_hook_burst);
  hook_register(client_new_local, HOOK_DEFAULT, m_spoof_hook_local);
  hook_register(client_new_remote, HOOK_DEFAULT, m_spoof_hook_remote);

  server_stats_register('s', m_spoof_stats);

  if((m_spoof_ini = ini_find_name(M_SPOOF_INI)) == NULL)
  {
    log(client_log, L_status,
        "Could not find spoof database, add '"M_SPOOF_INI
        "' to your config file.");
  }

  if(m_spoof_ini)
    ini_callback(m_spoof_ini, m_spoof_callback);

  server_default_caps |= CAP_SPF;

  return 0;
}

void m_spoof_unload(void)
{
  server_default_caps &= ~CAP_SPF;

  m_spoof_cleanup();

  hook_unregister(client_new_remote, HOOK_DEFAULT, m_spoof_hook_remote);
  hook_unregister(client_new_local, HOOK_DEFAULT, m_spoof_hook_local);
  hook_unregister(client_burst, HOOK_DEFAULT, m_spoof_hook_burst);
  hook_unregister(client_exit, HOOK_DEFAULT, m_spoof_hook_exit);

  server_stats_unregister('s');

  mem_static_destroy(&m_spoof_heap);

  timer_push(&m_spoof_timer);

  msg_unregister(&m_spoof_msg);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static struct m_spoof_entry *
m_spoof_new(net_addr_t ip, uint64_t ts, const char *host)
{
  struct m_spoof_entry *mseptr;

  mseptr = mem_static_alloc(&m_spoof_heap);

  dlink_add_tail(&m_spoof_list, &mseptr->node, mseptr);

  mseptr->ip = ip;
  mseptr->ts = ts;
  strlcpy(mseptr->host, host, sizeof(mseptr->host));

  debug(client_log, "new spoof entry: %s %s", net_ntoa(ip), host);

  return mseptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static struct m_spoof_entry *
m_spoof_add(net_addr_t ip, uint64_t ts, const char *host)
{
  if(m_spoof_ini)
  {
    struct ini_section *isptr;
    char                hostip[IRCD_HOSTIPLEN];

    net_ntoa_r(ip, hostip);

    if((isptr = ini_section_find(m_spoof_ini, hostip)) == NULL)
      isptr = ini_section_new(m_spoof_ini, hostip);

    ini_write_ulong_long(isptr, "ts", ts);
    ini_write_str(isptr, "host", host);
  }

  return m_spoof_new(ip, ts, host);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_spoof_delete(struct m_spoof_entry *mseptr)
{
  char hostip[IRCD_HOSTIPLEN + 1];

  net_ntoa_r(mseptr->ip, hostip);

  dlink_delete(&m_spoof_list, &mseptr->node);
  mem_static_free(&m_spoof_heap, mseptr);

  if(m_spoof_ini)
  {
    struct ini_section *isptr;

    if((isptr = ini_section_find(m_spoof_ini, hostip)))
      ini_section_remove(m_spoof_ini, isptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int m_spoof_cleanup(void)
{
  struct m_spoof_entry *mseptr;
  struct node          *next;

  dlink_foreach_safe(&m_spoof_list, mseptr, next)
  {
    if(mseptr->ts + M_SPOOF_LIFETIME < timer_mtime)
      m_spoof_delete(mseptr);
  }

  if(m_spoof_ini)
  {
    if(m_spoof_savedb())
    {
      log(client_log, L_warning, "Failed saving spoof database '%s'.",
          m_spoof_ini->name);
    }
  }
  else
  {
    if((m_spoof_ini = ini_find_name(M_SPOOF_INI)) == NULL)
      log(client_log, L_status,
          "Could not find spoof database, add '"
          M_SPOOF_INI"' to your config file.");
    else
      log(client_log, L_status, "Found spoof database: %s", m_spoof_ini->path);
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_spoof_callback(struct ini *ini)
{
  if(m_spoof_loaddb())
  {
    log(client_log, L_warning, "Failed loading spoof database '%s'.",
        m_spoof_ini->name);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static struct m_spoof_entry *m_spoof_find_ip(net_addr_t ip)
{
  struct m_spoof_entry *mseptr;
  char buf[16];

  net_ntoa_r(ip, buf);

  dlink_foreach(&m_spoof_list, mseptr)
  {

    debug(client_log, "Spoof check: %s %s",
          buf, net_ntoa(mseptr->ip));

    if(mseptr->ip == ip)
      return mseptr;
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static struct m_spoof_entry *m_spoof_find_host(const char *host)
{
  struct m_spoof_entry *mseptr;
  hash_t                  hash;
  
  hash = str_ihash(host);

  dlink_foreach(&m_spoof_list, mseptr)
  {
    if(mseptr->hash == hash)
    {
      if(!str_icmp(mseptr->host, host))
        return mseptr;
    }
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_spoof_hook_exit(struct lclient *lcptr, struct client *cptr,
                              const char     *comment)
{
  struct m_spoof_entry *mseptr;

  if(!client_is_user(cptr))
    return;

  if((mseptr = m_spoof_find_ip(cptr->ip)))
  {
    mseptr->waslame = 0;
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_spoof_hook_burst(struct lclient *lcptr,
                               struct client  *acptr)
{
  if(acptr->hhash != acptr->rhash)
    lclient_send(lcptr, "SPOOF %C %llu :%s",
                 acptr, 0ULL, acptr->host);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_spoof_hook_local(struct client *cptr, struct user *uptr)
{
  struct m_spoof_entry *mseptr;

  if(!client_is_user(cptr))
    return;

  if((mseptr = m_spoof_find_ip(cptr->ip)))
  {
    strncpy(cptr->host, mseptr->host, sizeof(cptr->host));
    cptr->hhash = mseptr->hash;

    server_send(NULL, NULL, CAP_UID, CAP_NONE,
                ":%S SPOOF %s %llu :%s",
                server_me, uptr->uid, mseptr->ts, cptr->host);
    server_send(NULL, NULL, CAP_NONE, CAP_UID,
                ":%S SPOOF %s %llu :%s",
                server_me, cptr->name, mseptr->ts, cptr->host);

    client_send(cptr, ":%S NOTICE %C :(m_spoof) spoofed your hostname to '%s'.",
                server_me, cptr, cptr->host);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_spoof_hook_remote(struct client *cptr, struct user *uptr)
{
  struct m_spoof_entry *mseptr;

  if(!client_is_user(cptr))
    return;

  if((mseptr = m_spoof_find_ip(cptr->ip)))
  {
    strncpy(cptr->host, mseptr->host, sizeof(cptr->host));
    cptr->hhash = mseptr->hash;

    server_send(NULL, NULL, CAP_UID, CAP_NONE,
                ":%S SPOOF %s %llu :%s",
                server_me, uptr->uid, mseptr->ts, cptr->host);
    server_send(NULL, NULL, CAP_NONE, CAP_UID,
                ":%S SPOOF %s %llu :%s",
                server_me, cptr->name, mseptr->ts, cptr->host);

    m_spoof_delete(mseptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int m_spoof_loaddb(void)
{
  struct m_spoof_entry *mseptr;
  struct ini_section   *isptr;
  char                  host[IRCD_HOSTLEN];
  uint64_t              ts;
  net_addr_t            ip;

  if(m_spoof_ini == NULL)
    return -1;

  if((isptr = ini_section_first(m_spoof_ini)))
  {
    do
    {
      ip = net_addr_any;

      net_aton(isptr->name, &ip);

      if(!ini_get_str(isptr, "host", host, IRCD_HOSTLEN) &&
         !ini_read_ulong_long(isptr, "ts", &ts))
      {
        if((mseptr = m_spoof_find_ip(ip)))
        {
          strlcpy(mseptr->host, host, sizeof(mseptr->host));
          mseptr->hash = str_ihash(mseptr->host);
        }
        else
        {
          m_spoof_new(ip, ts, host);
        }
      }
    }
    while((isptr = ini_section_next(m_spoof_ini)));
  }

  ini_close(m_spoof_ini);

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int m_spoof_savedb(void)
{
  ini_open(m_spoof_ini, INI_WRITE);
  io_queue_control(m_spoof_ini->fd, OFF, OFF, OFF);
  ini_save(m_spoof_ini);
  ini_close(m_spoof_ini);

  return 0;
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'spoof'                                                          *
 * -------------------------------------------------------------------------- */
static void m_spoof(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  struct m_spoof_entry *mseptr;
  char                  host[IRCD_HOSTLEN];
  hash_t                  hash;

  strlcpy(host, argv[2], sizeof(host));

  /* Validate argument */
  if(!chars_valid_host(host))
  {
    client_send(cptr, ":%S NOTICE %C :(m_spoof) invalid hostname '%s'.",
                server_me, cptr, host);
    return;
  }

  /* Hash the argument, used later when it is actually spoofed */
  hash = str_ihash(host);

  /* Return if the argument is exactly the same as the hostname */
  if(cptr->hhash == hash && !str_cmp(cptr->host, host))
    return;

  /* Find spoof entry for this client */
  mseptr = m_spoof_find_ip(cptr->ip);

  if(mseptr)
  {
    log(client_log, L_warning,
        "Found spoof entry for IP %s: %s", net_ntoa(cptr->ip), mseptr->host);

    /* If there is already a spoof entry and it hasn't expired then
       the client can only spoof back to his real host or real ip */
    if(mseptr->ts + M_SPOOF_SPOOFTIME > timer_mtime)
    {
      if(hash != cptr->ihash && hash != cptr->rhash && hash != mseptr->hash &&
         str_cmp(host, cptr->hostip) && str_cmp(host, cptr->hostreal) &&
         str_cmp(host, mseptr->host))
      {
        if(!mseptr->waslame)
        {
          client_send(cptr,
                      ":%S NOTICE %C :(m_spoof) you already spoofed, "
                      "you can only reset to your real hostname "
                      "within the next %u hours.",
                      server_me, cptr, (uint32_t)((M_SPOOF_SPOOFTIME -
                      (timer_mtime - mseptr->ts) + (30 * 60 * 1000L)) / (60 * 60 * 1000L)));

          mseptr->waslame++;
        }

        return;
      }

      log(client_log, L_warning,
          "spoof time hasn't expired but host for '%s' will be changed to '%s'.",
          cptr->name, host);
    }
    /* There is already a spoof entry but it has expired, update
       spoofed host if the argument isn't hostreal or hostip */
    else
    {
      mseptr->ts = timer_mtime;
      mseptr->waslame = 0;
    }

    if(hash != cptr->ihash && hash != cptr->rhash && hash != mseptr->hash &&
       str_cmp(host, cptr->hostip) && str_cmp(host, cptr->hostreal) &&
       str_cmp(host, mseptr->host))
    {
      mseptr->hash = hash;
      strlcpy(mseptr->host, host, sizeof(mseptr->host));
    }
  }
  else
  {
    /* First time spoofer */
    if(hash != cptr->ihash && hash != cptr->rhash &&
       str_cmp(host, cptr->hostip) && str_cmp(host, cptr->hostreal))
    {
      struct client *acptr;

      if((((acptr = client_find_host(host)) && acptr != cptr) ||
         m_spoof_find_host(host)) && cptr->ip != acptr->ip)
      {
        client_send(cptr,
                    ":%S NOTICE %C :(m_spoof) Someone already took the hostname '%s'",
                    server_me, cptr, host);
        return;
      }
    }

    mseptr = m_spoof_add(cptr->ip, timer_mtime, host);
  }

  /* Update host information */
  strlcpy(cptr->host, host, sizeof(cptr->host));
  cptr->hhash = hash;

  client_send(cptr, ":%S NOTICE %C :(m_spoof) spoofed your hostname to '%s'.",
              server_me, cptr, cptr->host);

  server_send(NULL, NULL, CAP_UID, CAP_NONE,
              ":%S SPOOF %s %llu :%s",
              server_me, cptr->user->uid, mseptr->ts, host);
  server_send(NULL, NULL, CAP_NONE, CAP_UID,
              ":%S SPOOF %s %llu :%s",
              server_me, cptr->name, mseptr->ts, host);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'spoof'                                                          *
 * argv[2] - 'spoof'                                                          *
 * argv[3] - 'spoof'                                                          *
 * argv[4] - 'spoof'                                                          *
 * -------------------------------------------------------------------------- */
static void ms_spoof(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  struct client *acptr;
  uint64_t       ts;

  if(argc < 5)
    return;

  if(lcptr->caps & CAP_UID)
    acptr = client_find_uid(argv[2]);
  else
    acptr = client_find_nick(argv[2]);

  if(acptr == NULL)
  {
    log(client_log, L_warning, "Dropping SPOOF for invalid client '%s'.",
        argv[2]);
    return;
  }

  strlcpy(acptr->host, argv[4], sizeof(acptr->host));
  acptr->hhash = str_ihash(acptr->host);

  ts = str_toull(argv[3], NULL, 10);

  server_send(lcptr, NULL, CAP_UID, CAP_NONE,
              ":%C SPOOF %s %llu :%s",
              cptr, acptr->user->uid, ts, acptr->host);
  server_send(lcptr, NULL, CAP_NONE, CAP_UID,
              ":%C SPOOF %s %llu :%s",
              cptr, acptr->name, ts, acptr->host);

  if(client_is_local(acptr))
  {
    struct m_spoof_entry *mseptr;

    /* Move spoof entry to this server */
    if((mseptr = m_spoof_find_ip(acptr->ip)) == NULL)
    {
      mseptr = m_spoof_add(acptr->ip, ts, argv[4]);
    }
    else
    {
      mseptr->hash = acptr->hhash;
      mseptr->ts = ts;
      strlcpy(mseptr->host, argv[4], sizeof(mseptr->host));
    }

    client_send(acptr, ":%S NOTICE %C :(m_spoof) spoofing your hostname to '%s'.",
                server_me, acptr, mseptr->host);
  }
}
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_spoof_stats(struct client *cptr)
{
  struct m_spoof_entry *mseptr;

  dlink_foreach(&m_spoof_list, mseptr)
  {
    numeric_send(cptr, RPL_STATSSLINE, 's',
                 net_ntoa(mseptr->ip), mseptr->ts, mseptr->host);
  }
}

