/* chaosircd - pi-networks irc server
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
 * $Id: m_gline.c,v 1.5 2006/09/28 09:56:24 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/io.h>
#include <libchaos/ini.h>
#include <libchaos/timer.h>
#include <libchaos/hook.h>
#include <libchaos/log.h>
#include <libchaos/str.h>
#include <libchaos/filter.h>
#include <libchaos/listen.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/msg.h>
#include <chaosircd/user.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/lclient.h>
#include <chaosircd/server.h>
#include <chaosircd/numeric.h>

/* -------------------------------------------------------------------------- *
 * Constants                                                                  *
 * -------------------------------------------------------------------------- */
#define M_GLINE_BLOCKSIZE 32
#define M_GLINE_INTERVAL  (5 * 60 * 1000ull)
#define M_GLINE_INI       "gline.ini"
#define M_GLINE_FILTER    "listen"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void                  mo_gline           (struct lclient *lcptr,
                                                 struct client  *cptr,
                                                 int             argc,
                                                 char          **argv);
static void                  mo_ungline         (struct lclient *lcptr,
                                                 struct client  *cptr,
                                                 int             argc,
                                                 char          **argv);
static void                  ms_gline           (struct lclient *lcptr,
                                                 struct client  *cptr,
                                                 int             argc,  
                                                 char          **argv);
static void                  ms_ungline         (struct lclient *lcptr,
                                                 struct client  *cptr, 
                                                 int             argc,
                                                 char          **argv);
static struct m_gline_entry *m_gline_add        (const char     *user, 
                                                 const char     *host,
                                                 const char     *info,
                                                 uint64_t        ts,
                                                 const char     *reason);
static int                   m_gline_cleanup    (void);
static struct m_gline_entry *m_gline_find       (const char     *user,
                                                 const char     *host,
                                                 net_addr_t      addr);
static void                  m_gline_callback   (struct ini     *ini);
static int                   m_gline_hook       (struct lclient *lcptr);
static int                   m_gline_loaddb     (void);
static int                   m_gline_savedb     (void);
static void                  m_gline_burst      (struct lclient *lcptr);
static void                  m_gline_stats      (struct client  *cptr);
#ifdef HAVE_SOCKET_FILTER
static void                  m_gline_listen     (struct listen  *lptr);
#endif /* HAVE_SOCKET_FILTER */
static void                  m_gline_mask       (const char     *host,
                                                 net_addr_t     *addr,
                                                 net_addr_t     *mask);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_gline_help[] = {
  "GLINE <user@host> <reason>",
  "",
  "Operator command banning a user from the whole network.",
  "Use '/STATS g' for a list of active g-lines.",
  NULL
};

static char *m_ungline_help[] = {
  "UNGLINE <user@host>",
  "",
  "Operator command removing a g-line. Use '/STATS g' for",
  "a list of active g-lines.",
  NULL
};

/* Message handler for /GLINE */
static struct msg m_gline_msg = {
  "GLINE", 2, 5, MFLG_OPER,
  { NULL, NULL, ms_gline, mo_gline },
  m_gline_help
};

/* Message handler for /UNGLINE */
static struct msg m_ungline_msg = {
  "UNGLINE", 1, 2, MFLG_OPER,
  { NULL, NULL, ms_ungline, mo_ungline },
  m_ungline_help
};

/* -------------------------------------------------------------------------- *
 * G-line entry structure                                                     *
 * -------------------------------------------------------------------------- */
struct m_gline_entry 
{
  struct node    node;
  uint64_t       ts;                      /* timestamp */
  char           user[IRCD_USERLEN];      /* user mask */
  char           host[IRCD_HOSTLEN];      /* host mask */
  char           info[IRCD_PREFIXLEN];    /* nick!user@host of the evil ircop */
  char           reason[IRCD_KICKLEN];    /* gline reason */
  net_addr_t     addr;
  net_addr_t     mask;
};

/* -------------------------------------------------------------------------- *
 * Local variables                                                            *
 * -------------------------------------------------------------------------- */
static struct list    m_gline_list;
static struct timer  *m_gline_timer;
static struct sheap   m_gline_heap;
static struct ini    *m_gline_ini;
#ifdef HAVE_SOCKET_FILTER
static struct filter *m_gline_filter;
#endif /* HAVE_SOCKET_FILTER */

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_gline_load(void)
{
  dlink_list_zero(&m_gline_list);
  
  /* Initialize message handlers */
  if(msg_register(&m_gline_msg) == NULL)
    return -1;
  
  if(msg_register(&m_ungline_msg) == NULL)
  {
    msg_unregister(&m_gline_msg);
    return -1;
  }

  /* Start timer for periodic cleanup of timed g-lines */
  m_gline_timer = timer_start(m_gline_cleanup, M_GLINE_INTERVAL);
  
  timer_note(m_gline_timer, "m_gline cleanup timer");
  
  /* Create heap for g-line storage */
  mem_static_create(&m_gline_heap, sizeof(struct m_gline_entry),
                    M_GLINE_BLOCKSIZE);
  mem_static_note(&m_gline_heap, "gline heap");

  /* Add some hooks to intercept the clients */
  hook_register(lclient_register, HOOK_DEFAULT, m_gline_hook);
  hook_register(server_burst, HOOK_DEFAULT, m_gline_burst);

  /* Add stats handler for g-line listing and the capability */
  server_stats_register('g', m_gline_stats);
  server_default_caps |= CAP_GLN;
  
  /* Initialize g-line database */
  if((m_gline_ini = ini_find_name(M_GLINE_INI)) == NULL)
  {
    log(client_log, L_status,
        "Could not find g-line database, add '"M_GLINE_INI
        "' to your config file.");
  }
  
  /* Callback called on a successful ini read */
  if(m_gline_ini)
    ini_callback(m_gline_ini, m_gline_callback);
  
  /* Add socket filter */
#ifdef HAVE_SOCKET_FILTER
  if((m_gline_filter = filter_find_name(M_GLINE_FILTER)) == NULL)
    m_gline_filter = filter_add(M_GLINE_FILTER);
  
  filter_rule_compile(m_gline_filter);
  
  filter_reattach_all(m_gline_filter);
  
  hook_register(listen_add, HOOK_2ND, m_gline_listen);
  
  filter_dump(m_gline_filter);
#endif /* HAVE_SOCKET_FILTER */
  
  return 0;
}

void m_gline_unload(void)
{
#ifdef HAVE_SOCKET_FILTER
  if((m_gline_filter = filter_find_name(M_GLINE_FILTER)))
    filter_delete(m_gline_filter);
#endif /* HAVE_SOCKET_FILTER */
  
  /* Clean the g-line list */
  m_gline_cleanup();
  
  /* Unregister stats handler and capability */
  server_stats_unregister('g');
  server_default_caps &= ~CAP_GLN;
  
  /* Unregister client hooks */
  hook_unregister(server_burst, HOOK_DEFAULT, m_gline_burst);
  hook_unregister(lclient_register, HOOK_DEFAULT, m_gline_hook);
  
  /* Free g-line memory */
  mem_static_destroy(&m_gline_heap);
  
  /* Kill periodic timer */
  timer_push(&m_gline_timer);
  
  /* Unregister message handlers */
  msg_unregister(&m_ungline_msg);
  msg_unregister(&m_gline_msg);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_gline_mask(const char *host, net_addr_t *addr,
                         net_addr_t *mask)
{
  char     netmask[32];
  char    *s;
  uint32_t mbc = 32;

  *addr = net_addr_any;
  *mask = net_addr_any;
  
  /* Parse CIDR netmask */
  strlcpy(netmask, host, sizeof(netmask));

  if((s = str_chr(netmask, '/')))
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
  
  if(net_aton(netmask, addr) && *addr != NET_ADDR_ANY)
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
 * Add a new g-line entry                                                     *
 * -------------------------------------------------------------------------- */
static struct m_gline_entry *m_gline_new(const char *user, const char *host,
                                         const char *info, uint64_t    ts,
                                         const char *reason)
{   
  struct m_gline_entry *mgeptr;
  
  /* Alloc entry block */
  mgeptr = mem_static_alloc(&m_gline_heap);
  
  /* Link to g-line list */
  dlink_add_tail(&m_gline_list, &mgeptr->node, mgeptr);
  
  /* Initialize fields */
  mgeptr->ts = ts;
  strlcpy(mgeptr->user, user, sizeof(mgeptr->user));
  strlcpy(mgeptr->host, host, sizeof(mgeptr->host));
  strlcpy(mgeptr->info, info, sizeof(mgeptr->info));

  m_gline_mask(host, &mgeptr->addr, &mgeptr->mask);
  
#ifdef HAVE_SOCKET_FILTER
  if(mgeptr->user[0] == '*' && mgeptr->user[1] == '\0' &&
     mgeptr->addr != NET_ADDR_ANY)
  {
    filter_rule_insert(m_gline_filter,
                       FILTER_SRCNET, FILTER_DENY,
                       mgeptr->addr,
                       mgeptr->mask, 0ull);
    
    filter_rule_compile(m_gline_filter);
    
    filter_reattach_all(m_gline_filter);
  }
#endif /* HAVE_SOCKET_FILTER */
  
  /* Copy reason and report status */
  if(reason)
  {
    strlcpy(mgeptr->reason, reason, sizeof(mgeptr->reason));
    debug(client_log, "new gline entry: %s@%s (%s)", 
          mgeptr->user, mgeptr->host, mgeptr->reason);
  }
  else
  {
    mgeptr->reason[0] = '\0';
    debug(client_log, "new gline entry: %s@%s", 
          mgeptr->user, mgeptr->host);
  }
  
  return mgeptr;
}

/* -------------------------------------------------------------------------- *
 * Add g-line to INI file and create an entry                                 *
 * -------------------------------------------------------------------------- */
static struct m_gline_entry *m_gline_add(const char *user, const char *host,
                                         const char *info, uint64_t    ts,
                                         const char *reason)
{
  /* Skip INI thing if the file is not loaded */
  if(m_gline_ini)
  {
    struct ini_section *isptr;
    char                mask[IRCD_PREFIXLEN];
    
    str_snprintf(mask, sizeof(mask), "%s@%s", user, host);
    
    /* Maybe that g-line already exists, then just modify the section */
    if((isptr = ini_section_find(m_gline_ini, mask)) == NULL)
      isptr = ini_section_new(m_gline_ini, mask);
    
    /* Write properties */ 
    ini_write_ulong_long(isptr, "ts", ts);
    ini_write_str(isptr, "info", info);
    
    if(reason)
      ini_write_str(isptr, "reason", reason);
  }
    
  /* Now create a g-line entry */
  return m_gline_new(user, host, info, ts, reason);
}

/* -------------------------------------------------------------------------- *
 * Delete a g-line entry                                                      *
 * -------------------------------------------------------------------------- */
static void m_gline_delete(struct m_gline_entry *mgeptr)
{
  char mask[IRCD_PREFIXLEN];
    
  /* Assemble the mask */
  str_snprintf(mask, sizeof(mask), "%s@%s", mgeptr->user, mgeptr->host);

  /* Kill the entry */
  dlink_delete(&m_gline_list, &mgeptr->node);
  mem_static_free(&m_gline_heap, mgeptr);
  
  /* Find the corresponding INI section */
  if(m_gline_ini)
  {
    struct ini_section *isptr;

    if((isptr = ini_section_find(m_gline_ini, mask)))
      ini_section_remove(m_gline_ini, isptr);
  }
}

/* -------------------------------------------------------------------------- *
 * Periodic g-line timer                                                      *
 * -------------------------------------------------------------------------- */
static int m_gline_cleanup(void)
{
  /* If the INI file is open then save it */
  if(m_gline_ini)
  {
    if(m_gline_savedb())
    {
      log(client_log, L_warning, "Failed saving gline database '%s'.",
          m_gline_ini->name);
    }
  }
  /* Otherwise try to create the INI */
  else
  {
    if((m_gline_ini = ini_find_name(M_GLINE_INI)) == NULL)
      log(client_log, L_status,
          "Could not find gline database, add '"
          M_GLINE_INI"' to your config file.");
    else
      log(client_log, L_status, "Found gline database: %s", m_gline_ini->path);
  }
  
  return 0;
}

/* -------------------------------------------------------------------------- *
 * Handler for INI data                                                       *
 * -------------------------------------------------------------------------- */
static void m_gline_callback(struct ini *ini)
{
  /* We got INI data, so parse it */
  if(m_gline_loaddb())
  {
    log(client_log, L_warning, "Failed loading gline database '%s'.",
        m_gline_ini->name);
  }
}

/* -------------------------------------------------------------------------- *
 * Find a g-line by user@host mask                                            *
 * -------------------------------------------------------------------------- */
static struct m_gline_entry *m_gline_find(const char *user, const char *host,
                                          net_addr_t addr)
{
  struct m_gline_entry *mkeptr;
  
  /* Cycle k-lines */
  dlink_foreach(&m_gline_list, mkeptr)
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
 * Load g-lines from INI database                                             *
 * -------------------------------------------------------------------------- */
static int m_gline_loaddb(void)
{
  struct m_gline_entry *mgeptr;
  struct ini_section   *isptr;
  char                  mask[IRCD_PREFIXLEN];
  char                 *host = NULL;
  char                 *info = NULL;
  char                 *reason = NULL;
  uint64_t              ts;
  net_addr_t            address = net_addr_any;
  net_addr_t            netmask = net_addr_any;
  
  /* Damn, we have no INI */
  if(m_gline_ini == NULL)
    return -1;
  
  /* Loop through all sections */
  for(isptr = ini_section_first(m_gline_ini); isptr; 
      isptr = ini_section_next(m_gline_ini))
  {
    /* The section name is the mask */
    strlcpy(mask, isptr->name, sizeof(mask));
    
    /* Invalid section name, skip it */
    if((host = str_chr(mask, '@')) == NULL)
      continue;
    
    /* Null-terminate user mask */
    *host++ = '\0';
    
    /* Read the properties */
    if(!ini_read_str(isptr, "info", &info) &&
       !ini_read_ulong_long(isptr, "ts", &ts))
    {
      ini_read_str(isptr, "reason", &reason);

      m_gline_mask(host, &address, &netmask);
      
      /* Maybe there is already an entry for this section */
      if((mgeptr = m_gline_find(mask, host, address)))
      {
        /* ....so just update the entry */
        mgeptr->ts = ts;
        strlcpy(mgeptr->info, info, sizeof(mgeptr->info));
        
        if(reason)
          strlcpy(mgeptr->reason, reason, sizeof(mgeptr->reason));
      }
      else
      {
        /* ...or create a new one */
        m_gline_new(mask, host, info, ts, reason);
      }
    }
  }
  
  /* Close the INI file */
  ini_close(m_gline_ini);
  
  return 0;
}

/* -------------------------------------------------------------------------- *
 * Write INI database to disc                                                 *
 * -------------------------------------------------------------------------- */
static int m_gline_savedb(void)
{
  /* Open the file */
  ini_open(m_gline_ini, INI_WRITE);
  
  /* Write immediately (unqueued) */
  io_queue_control(m_gline_ini->fd, OFF, OFF, OFF);
  
  /* Now write */
  ini_save(m_gline_ini);
  
  /* Close the file */
  ini_close(m_gline_ini);
  
  return 0;
}

/* -------------------------------------------------------------------------- *
 * Split mask string into user/host                                           *
 * -------------------------------------------------------------------------- */
static int m_gline_split(char  user[IRCD_USERLEN], 
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
  if((p = str_chr(mask, '@')))
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
static int m_gline_hook(struct lclient *lcptr)
{
  struct m_gline_entry *mgeptr;
  
  /* Look for a g-line entry matching the client */
  if((mgeptr = m_gline_find(lcptr->user->name, lcptr->host, lcptr->addr_remote)) == NULL)
    mgeptr = m_gline_find(lcptr->user->name, lcptr->hostip, lcptr->addr_remote);

  /* Found some? */
  if(mgeptr)
  {
    /* Yes, wipe the user :P */
    lclient_set_type(lcptr, LCLIENT_USER);
    
    /* Fear this, scriptkids! */
    if(mgeptr->reason[0])
      lclient_exit(lcptr, "g-lined: %s", mgeptr->reason);
    else
      lclient_exit(lcptr, "g-lined");
    
#ifdef HAVE_SOCKET_FILTER
    if(mgeptr->addr != NET_ADDR_ANY)
      filter_rule_insert(m_gline_filter,
                         FILTER_SRCNET, FILTER_DENY,
                         mgeptr->addr,
                         mgeptr->mask, 0ull);
    else
      filter_rule_insert(m_gline_filter,
                         FILTER_SRCIP, FILTER_DENY,
                         lcptr->addr_remote, 0,
                         FILTER_LIFETIME);
    
    filter_rule_compile(m_gline_filter);
    
    filter_reattach_all(m_gline_filter);
#endif /* HAVE_SOCKET_FILTER */

    return 1;
  }
  
  /* Lucky dude got through! :D */
  return 0;
}

/* -------------------------------------------------------------------------- *
 * Match a g-line entry against all local users                               *
 * -------------------------------------------------------------------------- */
static void m_gline_match(struct m_gline_entry *mgeptr)
{
  struct lclient *lcptr = NULL;
  struct node    *nptr;
  struct node    *next;
  
  /* Loop through list of local and registered users */
  dlink_foreach_safe_data(&lclient_lists[LCLIENT_USER], nptr, next, lcptr)
  {
    if(lcptr->user == NULL || lcptr->client == NULL)
      continue;
    
    /* Match the masks */
    if(str_match(lcptr->user->name, mgeptr->user)) 
    {
      if(((mgeptr->addr != net_addr_any) && 
          (mgeptr->addr & mgeptr->mask) == (lcptr->addr_remote & mgeptr->mask)) ||
         str_imatch(lcptr->host, mgeptr->host) || str_imatch(lcptr->hostip, mgeptr->host))
      {
        /* G-line seems active, so we exit the client */
        if(mgeptr->reason[0])
        {
          log(server_log, L_status, "G-line active for %s (%s@%s): %s",
              lcptr->client->name, lcptr->user->name, lcptr->host, mgeptr->reason);
        
          lclient_exit(lcptr, "g-lined: %s", mgeptr->reason);
        }
        else
        {
          /* Reasonless g-line shouldn't be possible, but maybe we get such remote */
          log(server_log, L_status, "G-line active for %s (%s@%s)", 
              lcptr->client->name, lcptr->user->name, lcptr->host);
          
          lclient_exit(lcptr, "g-lined");
        }
      }
    }
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'gline'                                                          *
 * argv[2] - user@host                                                        *
 * argv[3] - reason                                                           *
 * -------------------------------------------------------------------------- */
static void mo_gline(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  char                  user[IRCD_USERLEN];
  char                  host[IRCD_HOSTLEN];
  char                  mask[IRCD_PREFIXLEN];
  struct m_gline_entry *mgeptr;
  net_addr_t            addr = net_addr_any;
  net_addr_t            netmask = net_addr_any;
  
  strlcpy(mask, argv[2], sizeof(mask));
  
  /* Split and verify the supplied mask */
  if(m_gline_split(user, host, argv[2]))
  {
    if(client_is_user(cptr))
      client_send(cptr, ":%S NOTICE %C :*** Invalid mask '%s@%s'", 
                  server_me, cptr, user, host);
    return;
  }

  m_gline_mask(host, &addr, &netmask);
  
  /* Look whether a g-line already matches this mask */
  if(m_gline_find(user, host, addr))
  {
    if(client_is_user(cptr))
      client_send(cptr, 
                  ":%S NOTICE %C :*** There is already a g-line matching the mask '%s@%s'",
                  server_me, cptr, user, host);
    return;
  }
  
  /* Create info string */
  if(client_is_user(cptr))
    str_snprintf(mask, sizeof(mask), "%s!%s@%s", cptr->name, cptr->user->name, cptr->host);
  else
    strlcpy(mask, cptr->name, sizeof(mask));
  
  /* Add the g-line entry */
  mgeptr = m_gline_add(user, host, mask, timer_mtime, argv[3]);
  
  if(client_is_user(cptr))
    log(server_log, L_status, "%s (%s@%s) added a g-line for %s@%s (%s).",
        cptr->name, cptr->user->name, cptr->host,
        user, host, argv[3]);
  else
    log(server_log, L_status, "adding g-line for %s@%s (%s).",
        user, host, argv[3]);
    
  /* Enforce it (bye kiddies!) */
  m_gline_match(mgeptr);
  
  /* Sync it with remote servers */
  if(argv[3])
  {
    server_send(NULL, NULL, 
                CAP_GLN, CAP_NONE,
                ":%C GLINE %s %s %s %llu :%s",
                cptr, mgeptr->user, mgeptr->host,
                mgeptr->info, mgeptr->ts, mgeptr->reason);
  }
  else
  {
    server_send(NULL, NULL,
                CAP_GLN, CAP_NONE,
                ":%C GLINE %s %s %s %llu",
                cptr, mgeptr->user, mgeptr->host, mgeptr->info, mgeptr->ts);
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'gline'                                                          *
 * argv[2] - user                                                             *
 * argv[3] - host                                                             *
 * argv[4] - info                                                             *
 * argv[5] - ts                                                               *
 * argv[6] - reason                                                           *
 * -------------------------------------------------------------------------- */
static void ms_gline(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  struct m_gline_entry *mgeptr;
  uint64_t              ts;
  net_addr_t            addr = net_addr_any;
  net_addr_t            netmask = net_addr_any;
  
  /* Drop remote messages with too few args */
  if(argc < 6)
    return;
  
  /* Parse the timestamp */
  ts = str_toull(argv[5], NULL, 10);

  m_gline_mask(argv[3], &addr, &netmask);
  
  /* Check if a g-line already matches the mask */
  if((mgeptr = m_gline_find(argv[2], argv[3], addr)) == NULL)
  {
    /* Nope, create new one */
    mgeptr = m_gline_add(argv[2], argv[3], argv[4], ts, argv[6]);
  }
  else
  {
    /* Yes... update it! */
    strlcpy(mgeptr->info, argv[4], sizeof(mgeptr->info));
    
    mgeptr->ts = ts;
    
    if(argv[6])
      strlcpy(mgeptr->reason, argv[6], sizeof(mgeptr->reason));
  }
  
  /* If the the source is a server then its a burst */
  if(client_is_server(cptr))
    log(server_log, L_verbose, "%s is bursting g-line for %s@%s.",
        cptr->name, mgeptr->user, mgeptr->host);
  /* Otherwise it's been added by a client */
  else
    log(server_log, L_status, "%s (%s@%s) added a g-line for %s@%s (%s).",
        cptr->name, cptr->user->name, cptr->host, 
        mgeptr->user, mgeptr->host, mgeptr->reason);
  
  /* Enforce it (bye kiddies!) */
  m_gline_match(mgeptr);
  
  /* Relay it further */
  server_send(lcptr, NULL, CAP_GLN, CAP_NONE, ":%C GLINE %s %s %s %llu",
              cptr, mgeptr->user, mgeptr->host, mgeptr->info, mgeptr->ts);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'ungline'                                                        *
 * argv[2] - user@host                                                        *
 * -------------------------------------------------------------------------- */
static void mo_ungline(struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv)
{
  char                  user[IRCD_USERLEN];
  char                  host[IRCD_HOSTLEN];
  struct m_gline_entry *mgeptr;
  net_addr_t            addr = net_addr_any;
  net_addr_t            netmask = net_addr_any;
  
  /* Split and verify the supplied mask */
  if(m_gline_split(user, host, argv[2]))
  {
    client_send(cptr, ":%S NOTICE %C :*** Invalid mask '%s@%s'",
                server_me, cptr, user, host);
    return;
  }
  
  m_gline_mask(host, &addr, &netmask);
  
  /* Check if we have such a g-line */
  if((mgeptr = m_gline_find(user, host, addr)) == NULL)
  {
    client_send(cptr,
                ":%S NOTICE %C :*** There is no g-line matching the mask '%s@%s'",
                server_me, cptr, user, host);
    return;
  }

#ifdef HAVE_SOCKET_FILTER
  if(addr != NET_ADDR_ANY)
  {
    filter_rule_delete(m_gline_filter,
                       FILTER_SRCNET, FILTER_DENY,
                       addr,
                       netmask);
    
    filter_rule_compile(m_gline_filter);
    
    filter_reattach_all(m_gline_filter);
  }
#endif /* HAVE_SOCKET_FILTER */
  
  /* Remove the g-line and report status */
  m_gline_delete(mgeptr);
  
  log(server_log, L_status, "%s (%s@%s) removed g-line for %s@%s.",
      cptr->name, cptr->user->name, cptr->host, user, host);
  
  /* Relay it to all servers on the net */
  server_send(NULL, NULL, CAP_GLN, CAP_NONE, ":%C UNGLINE %s %s",
              cptr, mgeptr->user, mgeptr->host);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'ungline'                                                        *
 * argv[2] - user                                                             *
 * argv[3] - host                                                             *
 * -------------------------------------------------------------------------- */
static void ms_ungline(struct lclient *lcptr, struct client *cptr,
                       int             argc,  char         **argv)
{
  struct m_gline_entry *mgeptr;
  net_addr_t            addr = net_addr_any;
  net_addr_t            netmask = net_addr_any;
  
  /* Drop remote messages with too few args */
  if(argc < 4)
    return;

  m_gline_mask(argv[3], &addr, &netmask);
  
  /* Check if we have such a g-line */
  if((mgeptr = m_gline_find(argv[2], argv[3], addr)) == NULL)
  {
    if(client_is_user(cptr))
      client_send(cptr,
                  ":%S NOTICE %C :*** There is no g-line matching the mask '%s@%s'",
                  server_me, cptr, argv[2], argv[3]);
    return;
  }
  
#ifdef HAVE_SOCKET_FILTER
  if(addr != NET_ADDR_ANY)
  {
    filter_rule_delete(m_gline_filter,
                       FILTER_SRCNET, FILTER_DENY,
                       addr,
                       netmask);
    
    filter_rule_compile(m_gline_filter);
    
    filter_reattach_all(m_gline_filter);
  }
#endif /* HAVE_SOCKET_FILTER */
  
  /* Remove g-line */
  m_gline_delete(mgeptr);
  
  /* If the the source is a server then its a burst */
  if(client_is_server(cptr))
    log(server_log, L_status, "%s removed g-line for %s@%s.",
        cptr->name, mgeptr->user, mgeptr->host);
  else
    log(server_log, L_status, "%s (%s@%s) removed g-line for %s@%s.",
        cptr->name, cptr->user->name, cptr->host, mgeptr->user, mgeptr->host);

  /* Relay it further */
  server_send(lcptr, NULL, CAP_GLN, CAP_NONE, ":%C UNGLINE %s %s",
              cptr, mgeptr->user, mgeptr->host);
}

/* -------------------------------------------------------------------------- *
 * Introduce all my g-lines to a local server                                 *
 * -------------------------------------------------------------------------- */
static void m_gline_burst(struct lclient *lcptr)
{
  struct m_gline_entry *mgeptr;
  
  /* Server doesn't support g-line */
  if(!(lcptr->caps & CAP_GLN))
    return;
  
  /* Cycle g-line list */
  dlink_foreach(&m_gline_list, mgeptr)
  {
    /* Burst a g-line */
    if(mgeptr->reason[0])
      lclient_send(lcptr, "GLINE %s %s %s %llu :%s",
                   mgeptr->user, mgeptr->host, mgeptr->info, mgeptr->ts, mgeptr->reason);
    else
      lclient_send(lcptr, "GLINE %s %s %s %llu",
                   mgeptr->user, mgeptr->host, mgeptr->info, mgeptr->ts);
  }
}

/* -------------------------------------------------------------------------- *
 * Show all my g-lines to a client                                            *
 * -------------------------------------------------------------------------- */
static void m_gline_stats(struct client *cptr)
{
  struct m_gline_entry *mgeptr;
  
  /* Cycle g-line list */
  dlink_foreach(&m_gline_list, mgeptr)
  {
    /* Send an entry */
    numeric_send(cptr, RPL_STATSGLINE, 'g', 
                 mgeptr->user, mgeptr->host, (unsigned long)(mgeptr->ts / 1000L),
                 mgeptr->info, mgeptr->reason);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#ifdef HAVE_SOCKET_FILTER
static void m_gline_listen(struct listen *lptr)
{
  if(m_gline_filter)
  {
    filter_attach_listener(m_gline_filter, lptr);
  }
}
#endif /* HAVE_SOCKET_FILTER */
