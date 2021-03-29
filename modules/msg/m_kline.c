/* chaosircd - Chaoz's IRC daemon daemon
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
 * $Id: m_kline.c,v 1.4 2006/09/28 09:56:24 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/io.h"
#include "libchaos/ini.h"
#include "libchaos/dlink.h"
#include "libchaos/filter.h"
#include "libchaos/listen.h"
#include "libchaos/timer.h"
#include "libchaos/hook.h"
#include "libchaos/log.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/msg.h"
#include "ircd/user.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/lclient.h"
#include "ircd/server.h"
#include "ircd/numeric.h"

/* -------------------------------------------------------------------------- *
 * Constants                                                                  *
 * -------------------------------------------------------------------------- */
#define M_KLINE_BLOCKSIZE 32
#define M_KLINE_INTERVAL  (5 * 60 * 1000LLU)
#define M_KLINE_INI       "kline.ini"
#define M_KLINE_FILTER    "listen"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void                  mo_kline           (struct lclient *lcptr,
                                                 struct client  *cptr,
                                                 int             argc,
                                                 char          **argv);
static void                  mo_unkline         (struct lclient *lcptr,
                                                 struct client  *cptr,
                                                 int             argc,
                                                 char          **argv);
static struct m_kline_entry *m_kline_add        (const char     *user,
                                                 const char     *host,
                                                 const char     *info,
                                                 uint64_t        ts,
                                                 const char     *reason);
static int                   m_kline_cleanup    (void);
static struct m_kline_entry *m_kline_find       (const char     *user,
                                                 const char     *host,
                                                 net_addr_t      addr);
static void                  m_kline_callback   (struct ini     *ini);
static int                   m_kline_hook       (struct lclient *lcptr);
static int                   m_kline_loaddb     (void);
static int                   m_kline_savedb     (void);
static void                  m_kline_stats      (struct client  *cptr);
#ifdef HAVE_SOCKET_FILTER
static void                  m_kline_listen     (struct listen  *lptr);
#endif
static void                  m_kline_mask       (const char     *host,
                                                 net_addr_t     *addr,
                                                 net_addr_t     *mask);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_kline_help[] =
{
  "KLINE [server] <user@host> <reason>",
  "",
  "Operator command banning a user from the server.",
  "Use '/STATS k' for a list of active k-lines.",
  NULL
};

static char *m_unkline_help[] =
{
  "UNKLINE [server] <user@host>",
  "",
  "Operator command removing a k-line. Use '/STATS g' for",
  "a list of active k-lines.",
  NULL
};

/* Message handler for /KLINE */
static struct msg m_kline_msg = {
  "KLINE", 2, 3, MFLG_OPER,
  { NULL, NULL, mo_kline, mo_kline },
  m_kline_help
};

/* Message handler for /UNKLINE */
static struct msg m_unkline_msg = {
  "UNKLINE", 1, 2, MFLG_OPER,
  { NULL, NULL, mo_unkline, mo_unkline },
  m_unkline_help
};

/* -------------------------------------------------------------------------- *
 * K-line entry structure                                                     *
 * -------------------------------------------------------------------------- */
struct m_kline_entry
{
  struct node    node;
  uint64_t       ts;                      /* timestamp */
  char           user[IRCD_USERLEN];      /* user mask */
  char           host[IRCD_HOSTLEN];      /* host mask */
  char           info[IRCD_PREFIXLEN];    /* nick!user@host of the evil ircop */
  char           reason[IRCD_KICKLEN];    /* kline reason */
  net_addr_t     addr;
  net_addr_t     mask;
};

/* -------------------------------------------------------------------------- *
 * Local variables                                                            *
 * -------------------------------------------------------------------------- */
static struct list    m_kline_list;
static struct timer  *m_kline_timer;
static struct sheap   m_kline_heap;
static struct ini    *m_kline_ini;
#ifdef HAVE_SOCKET_FILTER
static struct filter *m_kline_filter;
#endif

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_kline_load(void)
{
  dlink_list_zero(&m_kline_list);

  /* Initialize message handlers */
  if(msg_register(&m_kline_msg) == NULL)
    return -1;

  if(msg_register(&m_unkline_msg) == NULL)
  {
    msg_unregister(&m_kline_msg);
    return -1;
  }

  /* Start timer for periodic cleanup of timed k-lines */
  m_kline_timer = timer_start(m_kline_cleanup, M_KLINE_INTERVAL);

  timer_note(m_kline_timer, "m_kline cleanup timer");

  /* Create heap for k-line storage */
  mem_static_create(&m_kline_heap, sizeof(struct m_kline_entry),
                    M_KLINE_BLOCKSIZE);
  mem_static_note(&m_kline_heap, "kline heap");

  /* Add a hook to intercept the clients */
  hook_register(lclient_register, HOOK_DEFAULT, m_kline_hook);

  /* Add stats handler for k-line listing and the capability */
  server_stats_register('k', m_kline_stats);
  server_default_caps |= CAP_KLN;

  /* Initialize k-line database */
  if((m_kline_ini = ini_find_name(M_KLINE_INI)) == NULL)
  {
    log(client_log, L_status,
        "Could not find k-line database, add '"M_KLINE_INI
        "' to your config file.");
  }

  /* Callback called on a successful ini read */
  if(m_kline_ini)
    ini_callback(m_kline_ini, m_kline_callback);

  /* Add socket filter */
#ifdef HAVE_SOCKET_FILTER
  if((m_kline_filter = filter_find_name(M_KLINE_FILTER)) == NULL)
    m_kline_filter = filter_add(M_KLINE_FILTER);

  filter_rule_compile(m_kline_filter);

  filter_reattach_all(m_kline_filter);

  hook_register(listen_add, HOOK_2ND, m_kline_listen);

  filter_dump(m_kline_filter);
#endif

  return 0;
}

void m_kline_unload(void)
{
#ifdef HAVE_SOCKET_FILTER
  if((m_kline_filter = filter_find_name(M_KLINE_FILTER)))
    filter_delete(m_kline_filter);
#endif

  /* Clean the k-line list */
  m_kline_cleanup();

  /* Unregister stats handler and capability */
  server_stats_unregister('k');
  server_default_caps &= ~CAP_KLN;

  /* Unregister client hook */
  hook_unregister(lclient_register, HOOK_DEFAULT, m_kline_hook);

  /* Free k-line memory */
  mem_static_destroy(&m_kline_heap);

  /* Kill periodic timer */
  timer_push(&m_kline_timer);

  /* Unregister message handlers */
  msg_unregister(&m_unkline_msg);
  msg_unregister(&m_kline_msg);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_kline_mask(const char *host, net_addr_t *addr,
                         net_addr_t *mask)
{
  char     netmask[32];
  char    *s;
  uint32_t mbc = 32;

  *addr = net_addr_any;
  *mask = net_addr_any;

  /* Parse CIDR netmask */
  strlcpy(netmask, host, sizeof(netmask));

  if((s = strchr(netmask, '/')))
  {
    *s++ = '\0';
    mbc = str_toul(s, NULL, 10);

    while(mbc > 32)
      mbc -= 32;
  }
  else
  {
    s = NULL;
    mbc = 32;
  }

  if(net_aton(netmask, addr) && *addr != net_addr_any)
  {
    uint32_t i;

    *mask = 0xffffffff;

    mbc = 32 - mbc;

    for(i = 0; i < mbc; i++)
      *mask &= ~net_htonl(1 << i);

    *addr &= *mask;
  }
}

/* -------------------------------------------------------------------------- *
 * Add a new k-line entry                                                     *
 * -------------------------------------------------------------------------- */
static struct m_kline_entry *m_kline_new(const char *user, const char *host,
                                         const char *info, uint64_t    ts,
                                         const char *reason)
{
  struct m_kline_entry *mkeptr;

  /* Alloc entry block */
  mkeptr = mem_static_alloc(&m_kline_heap);

  /* Link to k-line list */
  dlink_add_tail(&m_kline_list, &mkeptr->node, mkeptr);

  /* Initialize fields */
  mkeptr->ts = ts;
  strlcpy(mkeptr->user, user, sizeof(mkeptr->user));
  strlcpy(mkeptr->host, host, sizeof(mkeptr->host));
  strlcpy(mkeptr->info, info, sizeof(mkeptr->info));

  m_kline_mask(host, &mkeptr->addr, &mkeptr->mask);

#ifdef HAVE_SOCKET_FILTER
  if(mkeptr->user[0] == '*' && mkeptr->user[1] == '\0' &&
     mkeptr->addr != net_addr_any)
  {
    filter_rule_insert(m_kline_filter,
                       FILTER_SRCNET, FILTER_DENY,
                       mkeptr->addr,
                       mkeptr->mask, 0LLU);

    filter_rule_compile(m_kline_filter);

    filter_reattach_all(m_kline_filter);

    log(filter_log, L_warning, "Filter active for %s", mkeptr->host);
  }
#endif

  /* Copy reason and report status */
  if(reason)
  {
    strlcpy(mkeptr->reason, reason, sizeof(mkeptr->reason));
    debug(client_log, "new kline entry: %s@%s (%s)",
          mkeptr->user, mkeptr->host, mkeptr->reason);
  }
  else
  {
    mkeptr->reason[0] = '\0';
    debug(client_log, "new kline entry: %s@%s",
          mkeptr->user, mkeptr->host);
  }


  return mkeptr;
}

/* -------------------------------------------------------------------------- *
 * Add k-line to INI file and create an entry                                 *
 * -------------------------------------------------------------------------- */
static struct m_kline_entry *m_kline_add(const char *user, const char *host,
                                         const char *info, uint64_t    ts,
                                         const char *reason)
{
  /* Skip INI thing if the file is not loaded */
  if(m_kline_ini)
  {
    struct ini_section *isptr;
    char                mask[IRCD_PREFIXLEN];

    str_snprintf(mask, sizeof(mask), "%s@%s", user, host);

    /* Maybe that k-line already exists, then just modify the section */
    if((isptr = ini_section_find(m_kline_ini, mask)) == NULL)
      isptr = ini_section_new(m_kline_ini, mask);

    /* Write properties */
    ini_write_ulong_long(isptr, "ts", ts);
    ini_write_str(isptr, "info", info);

    if(reason)
      ini_write_str(isptr, "reason", reason);
  }

  /* Now create a k-line entry */
  return m_kline_new(user, host, info, ts, reason);
}

/* -------------------------------------------------------------------------- *
 * Delete a k-line entry                                                      *
 * -------------------------------------------------------------------------- */
static void m_kline_delete(struct m_kline_entry *mkeptr)
{
  char mask[IRCD_PREFIXLEN];

  /* Assemble the mask */
  str_snprintf(mask, sizeof(mask), "%s@%s", mkeptr->user, mkeptr->host);

#ifdef HAVE_SOCKET_FILTER
  if(mkeptr->addr != NET_ADDR_ANY &&
     mkeptr->user[0] == '*' && mkeptr->user[1] == '\0')
  {
    filter_rule_delete(m_kline_filter,
                       FILTER_SRCNET, FILTER_DENY,
                       mkeptr->addr,
                       mkeptr->mask);

    filter_rule_compile(m_kline_filter);

    filter_reattach_all(m_kline_filter);

    log(filter_log, L_warning, "Filter deactivated for %s", mkeptr->host);
  }
#endif

  /* Kill the entry */
  dlink_delete(&m_kline_list, &mkeptr->node);
  mem_static_free(&m_kline_heap, mkeptr);

  /* Find the corresponding INI section */
  if(m_kline_ini)
  {
    struct ini_section *isptr;

    if((isptr = ini_section_find(m_kline_ini, mask)))
      ini_section_remove(m_kline_ini, isptr);
  }
}

/* -------------------------------------------------------------------------- *
 * Periodic k-line timer                                                      *
 * -------------------------------------------------------------------------- */
static int m_kline_cleanup(void)
{
  /* If the INI file is open then save it */
  if(m_kline_ini)
  {
    if(m_kline_savedb())
    {
      log(client_log, L_warning, "Failed saving kline database '%s'.",
          m_kline_ini->name);
    }
  }
  /* Otherwise try to create the INI */
  else
  {
    if((m_kline_ini = ini_find_name(M_KLINE_INI)) == NULL)
      log(client_log, L_status,
          "Could not find kline database, add '"
          M_KLINE_INI"' to your config file.");
    else
      log(client_log, L_status, "Found kline database: %s", m_kline_ini->path);
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Handler for INI data                                                       *
 * -------------------------------------------------------------------------- */
static void m_kline_callback(struct ini *ini)
{
  /* We got INI data, so parse it */
  if(m_kline_loaddb())
  {
    log(client_log, L_warning, "Failed loading kline database '%s'.",
        m_kline_ini->name);
  }
}

/* -------------------------------------------------------------------------- *
 * Find a k-line by user@host mask                                            *
 * -------------------------------------------------------------------------- */
static struct m_kline_entry *m_kline_find(const char *user, const char *host,
                                          net_addr_t addr)
{
  struct m_kline_entry *mkeptr;

  /* Cycle k-lines */
  dlink_foreach(&m_kline_list, mkeptr)
  {
    /* When matching return the entry */
    if(str_match(user, mkeptr->user))
    {
      if(mkeptr->addr != NET_ADDR_ANY)
      {
        if((addr & mkeptr->mask) == mkeptr->addr)
          return mkeptr;
      }
      else
      {
        if(str_imatch(host, mkeptr->host))
          return mkeptr;
      }
    }
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * Load k-lines from INI database                                             *
 * -------------------------------------------------------------------------- */
static int m_kline_loaddb(void)
{
  struct m_kline_entry *mkeptr;
  struct ini_section   *isptr;
  char                  mask[IRCD_PREFIXLEN];
  char                 *host = NULL;
  char                 *info = NULL;
  char                 *reason = NULL;
  uint64_t              ts;
  net_addr_t            address = net_addr_any;
  net_addr_t            netmask = net_addr_any;

  /* Damn, we have no INI */
  if(m_kline_ini == NULL)
    return -1;

  /* Loop through all sections */
  for(isptr = ini_section_first(m_kline_ini); isptr;
      isptr = ini_section_next(m_kline_ini))
  {
    /* The section name is the mask */
    strlcpy(mask, isptr->name, sizeof(mask));

    /* Invalid section name, skip it */
    if((host = strchr(mask, '@')) == NULL)
      continue;

    /* Null-terminate user mask */
    *host++ = '\0';

    /* Read the properties */
    if(!ini_read_str(isptr, "info", &info) &&
       !ini_read_ulong_long(isptr, "ts", &ts))
    {
      ini_read_str(isptr, "reason", &reason);

      m_kline_mask(host, &address, &netmask);

      /* Maybe there is already an entry for this section */
      if((mkeptr = m_kline_find(mask, host, address)))
      {
        /* ....so just update the entry */
        mkeptr->ts = ts;
        strlcpy(mkeptr->info, info, sizeof(mkeptr->info));

        if(reason)
          strlcpy(mkeptr->reason, reason, sizeof(mkeptr->reason));
      }
      else
      {
        /* ...or create a new one */
        m_kline_new(mask, host, info, ts, reason);
      }
    }
  }

  /* Close the INI file */
  ini_close(m_kline_ini);

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Write INI database to disc                                                 *
 * -------------------------------------------------------------------------- */
static int m_kline_savedb(void)
{
  /* Open the file */
  ini_open(m_kline_ini, INI_WRITE);

  /* Write immediately (unqueued) */
  io_queue_control(m_kline_ini->fd, OFF, OFF, OFF);

  /* Now write */
  ini_save(m_kline_ini);

  /* Close the file */
  ini_close(m_kline_ini);

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Split mask string into user/host                                           *
 * -------------------------------------------------------------------------- */
static int m_kline_split(char  user[IRCD_USERLEN],
                         char  host[IRCD_HOSTLEN],
                         char *mask)
{
  char  *p;
  size_t nonwild = 0;
  size_t wild = 0;
  size_t i;

  /* Default is *@* if we haven't got any parseable data */
  user[0] = '*';
  host[0] = '*';
  user[1] = '\0';
  host[1] = '\0';

  /* Split up the mask */
  if((p = strchr(mask, '@')))
  {
    *p++ = '\0';

    if(mask[0])
      strlcpy(user, mask, IRCD_USERLEN);

    if(p[0])
      strlcpy(host, p, IRCD_HOSTLEN);
  }
  else
  {
    if(mask[0])
      strlcpy(host, mask, IRCD_HOSTLEN);
  }

  /* Filter out any invalid chars */
  for(i = 0; user[i]; i++)
  {
    if(!chars_iskwildchar(user[i]) && !chars_isuserchar(user[i]))
    {
      if(i == 0)
        user[i++] = '*';
      else
        user[i] = '\0';

      break;
    }

    if(chars_iskwildchar(user[i]))
      wild++;
    else
      nonwild++;
  }

  /* Terminate the user mask */
  user[i] = '\0';

  /* Filter out any invalid chars */
  for(i = 0; host[i]; i++)
  {
    if(!chars_iskwildchar(host[i]) && !chars_ishostchar(host[i]) && host[i] != '/')
    {
      if(i == 0)
        host[i++] = '*';
      else
        host[i] = '\0';

      break;
    }

    if(chars_iskwildchar(host[i]))
      wild++;
    else
      nonwild++;
  }

  /* Terminate the host mask */
  host[i] = '\0';

  /* We must have at least some non-wildchars */
  if(wild >= nonwild)
    return 1;

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Hooks all local clients                                                    *
 * -------------------------------------------------------------------------- */
static int m_kline_hook(struct lclient *lcptr)
{
  struct m_kline_entry *mkeptr;

  /* Look for a k-line entry matching the client */
  if((mkeptr = m_kline_find(lcptr->user->name, lcptr->host, lcptr->addr_remote)) == NULL)
    mkeptr = m_kline_find(lcptr->user->name, lcptr->hostip, lcptr->addr_remote);

  /* Found some? */
  if(mkeptr)
  {
    /* Yes, wipe the user :P */
    lclient_set_type(lcptr, LCLIENT_USER);

    /* Fear this, scriptkids! */
    if(mkeptr->reason[0])
      lclient_exit(lcptr, "k-lined: %s", mkeptr->reason);
    else
      lclient_exit(lcptr, "k-lined");

#ifdef HAVE_SOCKET_FILTER
    if(mkeptr->addr != NET_ADDR_ANY)
      filter_rule_insert(m_kline_filter,
                         FILTER_SRCNET, FILTER_DENY,
                         mkeptr->addr,
                         mkeptr->mask, 0LLU);
    else
      filter_rule_insert(m_kline_filter,
                         FILTER_SRCIP, FILTER_DENY,
                         lcptr->addr_remote, 0,
                         FILTER_LIFETIME);

    filter_rule_compile(m_kline_filter);

    filter_reattach_all(m_kline_filter);

    log(filter_log, L_warning, "Filter active for %s", mkeptr->host);
#endif

    return 1;
  }

  /* Lucky dude got through! :D */
  return 0;
}

/* -------------------------------------------------------------------------- *
 * Match a k-line entry against all local users                               *
 * -------------------------------------------------------------------------- */
static void m_kline_match(struct m_kline_entry *mkeptr)
{
  struct lclient *lcptr = NULL;
  struct node    *nptr;
  struct node    *next;

  if(mkeptr == NULL)
    return;

  /* Loop through list of local and registered users */
  dlink_foreach_safe_data(&lclient_lists[LCLIENT_USER], nptr, next, lcptr)
  {
    if(lcptr->user == NULL || lcptr->client == NULL)
      continue;

    /* Match the masks */
    if(str_match(lcptr->user->name, mkeptr->user))
    {
      if(((mkeptr->addr != NET_ADDR_ANY) &&
          (mkeptr->addr & mkeptr->mask) ==
          (lcptr->addr_remote & mkeptr->mask)) ||
         str_imatch(lcptr->host, mkeptr->host) ||
         str_imatch(lcptr->hostip, mkeptr->host))
      {
        /* k-line seems active, so we exit the client */
        if(mkeptr->reason[0])
        {
          log(server_log, L_status, "K-line active for %s (%s@%s): %s",
              lcptr->client->name, lcptr->user->name, lcptr->host, mkeptr->reason);

          lclient_exit(lcptr, "k-lined: %s", mkeptr->reason);
        }
        else
        {
          /* Reasonless k-line shouldn't be possible, but maybe we get such remote */
          log(server_log, L_status, "K-line active for %s (%s@%s)",
              lcptr->client->name, lcptr->user->name, lcptr->host);

          lclient_exit(lcptr, "k-lined");
        }
      }
    }
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'kline'                                                          *
 * argv[2] - user@host                                                        *
 * argv[3] - reason                                                           *
 * -------------------------------------------------------------------------- */
static void mo_kline(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  char                  user[IRCD_USERLEN];
  char                  host[IRCD_HOSTLEN];
  char                  mask[IRCD_PREFIXLEN];
  struct m_kline_entry *mkeptr;
  net_addr_t            address = net_addr_any;
  net_addr_t            netmask = net_addr_any;

  /* Relay to remote server if the 2nd argument is a valid server */
  if(argc > 3)
  {
    if(argc > 4)
    {
      if(server_relay_maybe(lcptr, cptr, 2, ":%C KLINE %s %s :%s", &argc, argv))
        return;
    }
    else
    {
      if(server_relay_maybe(lcptr, cptr, 2, ":%C KLINE %s %s", &argc, argv))
        return;
    }
  }

  /* This check is here because server_relay_maybe() may swallow an argument  */
  if(argc < 4)
  {
    if(client_is_user(cptr))
      numeric_send(cptr, ERR_NEEDMOREPARAMS, argv[1]);

    return;
  }

  /* Split and verify the supplied mask */
  if(m_kline_split(user, host, argv[2]))
  {
    if(client_is_user(cptr))
      client_send(cptr, ":%S NOTICE %C :*** Invalid mask '%s@%s'",
                  server_me, cptr, user, host);
    return;
  }

  m_kline_mask(host, &address, &netmask);

  /* Look whether a k-line already matches this mask */
  if(m_kline_find(user, host, address))
  {
    if(client_is_user(cptr))
      client_send(cptr,
                  ":%S NOTICE %C :*** There is already a k-line matching the mask '%s@%s'",
                  server_me, cptr, user, host);
    return;
  }

  /* Create info string */
  if(client_is_user(cptr))
    str_snprintf(mask, sizeof(mask), "%s!%s@%s", cptr->name, cptr->user->name, cptr->host);
  else
    strlcpy(mask, cptr->name, sizeof(mask));

  /* Add the k-line entry */
  mkeptr = m_kline_add(user, host, mask, timer_mtime, argv[3]);

  if(client_is_user(cptr))
    log(server_log, L_status, "%s (%s@%s) added a k-line for %s@%s.",
        cptr->name, cptr->user->name, cptr->host,
        user, host);
  else
    log(server_log, L_status, "Adding a k-line for %s@%s.",
        user, host);

  /* Enforce it (bye kiddies!) */
  m_kline_match(mkeptr);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'unkline'                                                        *
 * argv[2] - user@host                                                        *
 * -------------------------------------------------------------------------- */
static void mo_unkline(struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv)
{
  char                  user[IRCD_USERLEN];
  char                  host[IRCD_HOSTLEN];
  char                  mask[IRCD_PREFIXLEN];
  struct m_kline_entry *mkeptr;
  net_addr_t            address;
  net_addr_t            netmask;

  if(argc > 3)
  {
    if(server_relay_maybe(lcptr, cptr, 2, ":%C UNKLINE %s %s", &argc, argv))
      return;
  }

  strlcpy(mask, argv[2], sizeof(mask));

  if(m_kline_split(user, host, mask))
  {
    client_send(cptr, ":%S NOTICE %C :*** Invalid mask '%s@%s'",
                server_me, cptr, user, host);
    return;
  }

  m_kline_mask(host, &address, &netmask);

  if((mkeptr = m_kline_find(user, host, address)) == NULL)
  {
    client_send(cptr,
                ":%S NOTICE %C :*** There is no k-line matching the mask '%s@%s'",
                server_me, cptr, user, host);
    return;
  }

  log(server_log, L_status, "%s (%s@%s) removed k-line for %s@%s.",
      cptr->name, cptr->user->name, cptr->host, user, host);

#ifdef HAVE_SOCKET_FILTER
  if(address != NET_ADDR_ANY)
  {
    filter_rule_delete(m_kline_filter,
                       FILTER_SRCNET, FILTER_DENY,
                       address,
                       netmask);

    filter_rule_compile(m_kline_filter);

    filter_reattach_all(m_kline_filter);
  }
#endif

  m_kline_delete(mkeptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_kline_stats(struct client *cptr)
{
  struct m_kline_entry *mkeptr;

  dlink_foreach(&m_kline_list, mkeptr)
  {
    numeric_send(cptr, RPL_STATSKLINE, 'k',
                 mkeptr->user, mkeptr->host, (unsigned long)(mkeptr->ts / 1000L),
                 mkeptr->info, mkeptr->reason);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#ifdef HAVE_SOCKET_FILTER
static void m_kline_listen(struct listen  *lptr)
{
  if(m_kline_filter)
  {
    filter_attach_listener(m_kline_filter, lptr);
  }
}
#endif
