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
 * $Id: m_dump.c,v 1.4 2006/09/28 09:56:24 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/io.h>
#include <libchaos/ini.h>
#include <libchaos/log.h>
#include <libchaos/mem.h>
#include <libchaos/str.h>
#include <libchaos/child.h>
#include <libchaos/htmlp.h>
#include <libchaos/httpc.h>
#include <libchaos/sauth.h>
#include <libchaos/timer.h>
#include <libchaos/mfile.h>
#include <libchaos/filter.h>
#include <libchaos/module.h>
#include <libchaos/listen.h>
#include <libchaos/connect.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/msg.h>
#include <ircd/user.h>
#include <ircd/chars.h>
#include <ircd/class.h>
#include <ircd/server.h>
#include <ircd/client.h>
#include <ircd/lclient.h>
#include <ircd/channel.h>
#include <ircd/usermode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mo_dump(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mo_dump_help[] = {
  "DUMP [server] <module> [handle]",
  "",
  "Dumps internal state information of a server.",
  "Use without the <module> argument for a list of valid modules.",
  NULL
};

static struct msg mo_dump_msg = {
  "DUMP", 0, 3, MFLG_OPER,
  { NULL, NULL, mo_dump, mo_dump },
  mo_dump_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_dump_load(void)
{
  if(msg_register(&mo_dump_msg) == NULL)
    return -1;

  return 0;
}

void m_dump_unload(void)
{
  msg_unregister(&mo_dump_msg);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#ifdef HAVE_SOCKET_FILTER
static void m_dump_filter(char *arg)
{
  struct filter *filter = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      filter = filter_find_id(atoi(arg));
    else
      filter = filter_find_name(arg);
  }

  filter_dump(filter);
}
#endif /* HAVE_SOCKET_FILTER */

static void m_dump_htmlp(char *arg)
{
  struct htmlp *htmlp = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      htmlp = htmlp_find_id(atoi(arg));
    else
      htmlp = htmlp_find_name(arg);
  }

  htmlp_dump(htmlp);
}

static void m_dump_httpc(char *arg)
{
  struct httpc *httpc = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      httpc = httpc_find_id(atoi(arg));
    else
      httpc = httpc_find_name(arg);
  }

  httpc_dump(httpc);
}

static void m_dump_dlink(char *arg)
{
  dlink_dump();
}

static void m_dump_slog(char *arg)
{
  int id = -1;

  if(arg)
  {
    if(chars_isdigit(*arg))
      id = atoi(arg);
    else
      id = log_source_find(arg);
  }

  log_source_dump(id);
}

static void m_dump_dlog(char *arg)
{
  struct dlog *dlptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
    {
      if(arg[1] == 'x')
        dlptr = log_drain_find_cb((void *)str_toul(arg, NULL, 0x10));
      else
        dlptr = log_drain_find_id(atoi(arg));
    }
    else
    {
      dlptr = log_drain_find_path(arg);
    }
  }

  log_drain_dump(dlptr);
}

static void m_dump_sheap(char *arg)
{
  struct sheap *shptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      shptr = mem_static_find(atoi(arg));
  }

  mem_static_dump(shptr);
}

static void m_dump_dheap(char *arg)
{
  struct dheap *dhptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      dhptr = mem_dynamic_find(atoi(arg));
  }

  mem_dynamic_dump(dhptr);
}

static void m_dump_io(char *arg)
{
  int fd = -1;

  if(arg)
  {
    if(chars_isdigit(*arg))
      fd = atoi(arg);
  }

  io_dump(fd);
}

static void m_dump_ini(char *arg)
{
  struct ini *ini = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      ini = ini_find_id(atoi(arg));
    else
      ini = ini_find_name(arg);
  }

  ini_dump(ini);
}

static void m_dump_timer(char *arg)
{
  struct timer *tptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      tptr = timer_find_id(atoi(arg));
  }

  timer_dump(tptr);
}

static void m_dump_user(char *arg)
{
  struct user *uptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      uptr = user_find_id(atoi(arg));
    else
      uptr = user_find_name(arg);

    if(uptr == NULL)
      uptr = user_find_uid(arg);
  }

  user_dump(uptr);
}

static void m_dump_umode(char *arg)
{
  struct usermode *umptr = NULL;

  if(arg != NULL)
    umptr = usermode_find(*arg);

  usermode_dump(umptr);
}

static void m_dump_child(char *arg)
{
  struct child *cdptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      cdptr = child_find_id(atoi(arg));
    else
      cdptr = child_find_name(arg);
  }

  child_dump(cdptr);
}

static void m_dump_mfile(char *arg)
{
  struct mfile *mfptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      mfptr = mfile_find_id(atoi(arg));
    else
      mfptr = mfile_find_name(arg);
  }

  mfile_dump(mfptr);
}

/* dump servers */
static void m_dump_server(char *arg)
{
  struct server *sptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      sptr = server_find_id(atoi(arg));
    else
      sptr = server_find_name(arg);
  }

  server_dump(sptr);
}

/* dump clients */
static void m_dump_client(char *arg)
{
  struct client *cptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      cptr = client_find_id(atoi(arg));
    else
      cptr = client_find_name(arg);
  }

  client_dump(cptr);
}

/* dump lclients */
static void m_dump_lclient(char *arg)
{
  struct lclient *lcptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      lcptr = lclient_find_id(atoi(arg));
    else
      lcptr = lclient_find_name(arg);
  }

  lclient_dump(lcptr);
}

/* dump listeners */
static void m_dump_listen(char *arg)
{
  struct listen *lptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      lptr = listen_find_id(atoi(arg));
    else
      lptr = listen_find_name(arg);
  }

  listen_dump(lptr);
}

/* dump classes */
static void m_dump_class(char *arg)
{
  struct class *clptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      clptr = class_find_id(atoi(arg));
    else
      clptr = class_find_name(arg);
  }

  class_dump(clptr);
}

/* dump channels */
static void m_dump_channel(char *arg)
{
  struct channel *chptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      chptr = channel_find_id(atoi(arg));
    else
      chptr = channel_find_name(arg);
  }

  channel_dump(chptr);
}

/* dump connects */
static void m_dump_connect(char *arg)
{
  struct connect *cnptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      cnptr = connect_find_id(atoi(arg));
    else
      cnptr = connect_find_name(arg);
  }

  connect_dump(cnptr);
}

/* dump modules */
static void m_dump_module(char *arg)
{
  struct module *mptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
    {
      mptr = module_find_id(atoi(arg));
    }
    else
    {
      if((mptr = module_find_name(arg)) == NULL)
        mptr = module_find_path(arg);
    }
  }

  module_dump(mptr);
}

/* dump msgs */
static void m_dump_msg(char *arg)
{
  struct msg *mptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      mptr = msg_find_id(atoi(arg));
    else
      mptr = msg_find(arg);
  }

  msg_dump(mptr);
}

/* dump net */
static void m_dump_net(char *arg)
{
  struct protocol *nptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      nptr = net_find_id(atoi(arg));
  }

  net_dump(nptr);
}

/* dump SSL */
static void m_dump_ssl(char *arg)
{
  struct ssl_context *scptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      scptr = ssl_find_id(atoi(arg));
    else
      scptr = ssl_find_name(arg);
  }

  ssl_dump(scptr);
}

/* dump sauth */
static void m_dump_sauth(char *arg)
{
  struct sauth *saptr = NULL;

  if(arg)
  {
    if(chars_isdigit(*arg))
      saptr = sauth_find(atoi(arg));
  }

  sauth_dump(saptr);
}

typedef void (dump_cb_t)(char *);

static struct {
  const char *name;
  dump_cb_t  *cb;
  int       (*sp)(void);
} m_dump_table[] = {
  { "io",      m_dump_io,      io_get_log       },
  { "ini",     m_dump_ini,     ini_get_log      },
  { "msg",     m_dump_msg,     msg_get_log      },
  { "net",     m_dump_net,     net_get_log      },
  { "ssl",     m_dump_ssl,     ssl_get_log      },
  { "slog",    m_dump_slog,    log_get_log      },
  { "dlog",    m_dump_dlog,    log_get_log      },
  { "user",    m_dump_user,    user_get_log     },
  { "umode",   m_dump_umode,   usermode_get_log },
  { "sheap",   m_dump_sheap,   mem_get_log      },
  { "dheap",   m_dump_dheap,   mem_get_log      },
  { "child",   m_dump_child,   child_get_log    },
  { "class",   m_dump_class,   class_get_log    },
  { "dlink",   m_dump_dlink,   dlink_get_log    },
  { "htmlp",   m_dump_htmlp,   htmlp_get_log    },
  { "httpc",   m_dump_httpc,   httpc_get_log    },
  { "mfile",   m_dump_mfile,   mfile_get_log    },
  { "sauth",   m_dump_sauth,   sauth_get_log    },
  { "timer",   m_dump_timer,   timer_get_log    },
  { "client",  m_dump_client,  client_get_log   },
#ifdef HAVE_SOCKET_FILTER
  { "filter",  m_dump_filter,  filter_get_log   },
#endif /* HAVE_SOCKET_FILTER */
  { "listen",  m_dump_listen,  listen_get_log   },
  { "module",  m_dump_module,  module_get_log   },
  { "server",  m_dump_server,  server_get_log   },
  { "channel", m_dump_channel, channel_get_log  },
  { "connect", m_dump_connect, connect_get_log  },
  { "lclient", m_dump_lclient, lclient_get_log  },
  { NULL,    NULL,         NULL              }
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_dump_callback(uint64_t        flag,  int         lvl,
                            const char     *level, const char *source,
                            const char     *date,  const char *msg,
                            struct client  *cptr)
{
  client_send(cptr, ":%S NOTICE %C :%s",
              server_me, cptr, msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'dump'                                                           *
 * argv[2] - [server]                                                         *
 * argv[3] - module                                                           *
 * argv[4] - handle                                                           *
 * -------------------------------------------------------------------------- */
static void mo_dump(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  size_t         i;
  struct dlog   *ldptr;

  if(argc > 2)
  {
    if(argc == 3)
    {
      if(server_relay_maybe(lcptr, cptr, 2, ":%C DUMP %s", &argc, argv))
        return;
    }
    else if(argc == 4)
    {
      if(server_relay_maybe(lcptr, cptr, 2, ":%C DUMP %s %s", &argc, argv))
        return;
    }
    else if(argc == 5)
    {
      if(server_relay_always(lcptr, cptr, 2, ":%C DUMP %s %s %s", &argc, argv))
        return;
    }
  }

  if(argc == 2)
  {
    uint32_t sz;

    client_send(cptr, ":%S NOTICE %C :modules available to dump:",
                server_me, cptr);

    sz = (sizeof(m_dump_table) / sizeof(m_dump_table[0])) - 1;

    for(i = 0; i + 4 < sz; i += 4)
      client_send(cptr, ":%S NOTICE %C :%-10s %-10s %-10s %-10s",
                  server_me, cptr,
                  m_dump_table[i + 0].name,
                  m_dump_table[i + 1].name,
                  m_dump_table[i + 2].name,
                  m_dump_table[i + 3].name);

    if(sz - i == 3)
      client_send(cptr, ":%S NOTICE %C :%-10s %-10s %-10s",
                  server_me, cptr,
                  m_dump_table[i + 0].name,
                  m_dump_table[i + 1].name,
                  m_dump_table[i + 2].name);
    else if(sz - i == 2)
      client_send(cptr, ":%S NOTICE %C :%-10s %-10s",
                  server_me, cptr,
                  m_dump_table[i + 0].name,
                  m_dump_table[i + 1].name);
    else if(sz - i == 1)
      client_send(cptr, ":%S NOTICE %C :%-10s",
                  server_me, cptr,
                  m_dump_table[i + 0].name);
    return;
  }

  for(i = 0; m_dump_table[i].name; i++)
  {
    if(!str_icmp(m_dump_table[i].name, argv[2]))
    {
      ldptr = log_drain_callback(m_dump_callback,
                                 log_sources[m_dump_table[i].sp()].flag,
                                 L_debug, cptr);

      m_dump_table[i].cb(argv[3]);

      log_drain_delete(ldptr);
    }
  }
}
