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
 * $Id: m_module.c,v 1.3 2006/09/28 09:56:24 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/log.h"
#include "libchaos/str.h"
#include "libchaos/module.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/lclient.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mo_module(struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static struct module *m_module_module;

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mo_module_help[] = {
  "MODULE [server] <list|load|unload|reload> [module]",
  "",
  "Manages dynamically loadable modules.",
  "",
  "list        List all currently loaded modules.",
  "load        Load a module.",
  "unload      Unload a module.",
  "reload      Reload a module.",
  NULL
};

static struct msg m_module_msg = {
  "MODULE", 1, 3, MFLG_OPER,
  { NULL, NULL, mo_module, mo_module },
  mo_module_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_module_load(struct module *module)
{
  if(msg_register(&m_module_msg) == NULL)
    return -1;

  m_module_module = module;

  return 0;
}

void m_module_unload(void)
{
  m_module_module = NULL;

  msg_unregister(&m_module_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'module'                                                         *
 * argv[2] - 'server'                                                         *
 * argv[3] - command                                                          *
 * argv[4] - module                                                           *
 * -------------------------------------------------------------------------- */
static void mo_module(struct lclient *lcptr, struct client *cptr,
                      int             argc,  char         **argv)
{
  struct module *module;

  if(argc > 3)
  {
    if(argv[4])
    {
      if(server_relay_maybe(lcptr, cptr, 2, ":%C MODULE %s %s :%s", &argc, argv))
        return;
    }
    else
    {
      if(server_relay_maybe(lcptr, cptr, 2, ":%C MODULE %s %s", &argc, argv))
        return;
    }
  }

  if(!str_icmp(argv[2], "list"))
  {
    client_send(cptr, ":%C NOTICE %C : ======= module list ======== ",
                client_me, cptr);

    client_send(cptr, ":%C NOTICE %C :  id name",
                client_me, cptr);
    client_send(cptr, ":%C NOTICE %C : ---------------------------- ",
                client_me, cptr);

    dlink_foreach_up(&module_list, module)
    {
      client_send(cptr, ":%C NOTICE %C : %3u %s",
                  client_me, cptr, module->id, module->name);
    }

    client_send(cptr, ":%C NOTICE %C : ==== end of module list ==== ",
                client_me, cptr);
  }
  else if(!str_icmp(argv[2], "load"))
  {
    if(argv[3] == NULL)
    {
      client_send(cptr, ":%C NOTICE %C : need path of the module",
                  client_me, cptr);
      return;
    }

    module = module_add(argv[3]);

    if(module)
    {
      client_send(cptr, ":%C NOTICE %C : loaded module '%s'.",
                  client_me, cptr, module->name);
    }
  }
  else if(!str_icmp(argv[2], "unload"))
  {
    if(argv[3] == NULL)
    {
      client_send(cptr, ":%C NOTICE %C : need name or id of the module",
                  client_me, cptr);
      return;
    }

    module = module_find_name(argv[3]);

    if(module == NULL && chars_isdigit(argv[3][0]))
      module = module_find_id(atoi(argv[3]));

    if(module == NULL)
    {
      client_send(cptr, ":%C NOTICE %C : module '%s' not found.",
                  client_me, cptr, argv[3]);
      return;
    }

    if(module == m_module_module)
    {
      client_send(cptr, ":%C NOTICE %C : can not unload myself!",
                  client_me, cptr);
      return;
    }

    client_send(cptr, ":%C NOTICE %C : unloaded module '%s'.",
                client_me, cptr, module->name);

    module_delete(module);
  }
  else if(!str_icmp(argv[2], "reload"))
  {
    if(argv[3] == NULL)
    {
      client_send(cptr, ":%C NOTICE %C : need name or id of the module",
                  client_me, cptr);
      return;
    }

    module = module_find_name(argv[3]);

    if(module == NULL && chars_isdigit(argv[3][0]))
      module = module_find_id(atoi(argv[3]));

    if(module == NULL)
    {
      client_send(cptr, ":%C NOTICE %C : module '%s' not found.",
                  client_me, cptr, argv[3]);
      return;
    }

    if(module == m_module_module)
    {
      client_send(cptr, ":%C NOTICE %C : can not reload myself!",
                  client_me, cptr);
      return;
    }

    if(module_reload(module))
      client_send(cptr, ":%C NOTICE %C : failed reloading module '%s'.",
                  client_me, cptr, module->name);
    else
      client_send(cptr, ":%C NOTICE %C : reloaded module '%s'.",
                  client_me, cptr, module->name);
  }
  else
  {
    client_send(cptr, ":%C NOTICE %C :invalid command '%s' for /module",
                client_me, cptr, argv[2]);
  }
}
