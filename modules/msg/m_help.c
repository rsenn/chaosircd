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
 * $Id: m_help.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/log.h>
#include <libchaos/str.h>
#include <libchaos/dlink.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/msg.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/numeric.h>
#include <chaosircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define M_HELP_INDEX     "INDEX"
#define M_HELP_USERMODES "USERMODES"
#define M_HELP_CHANMODES "CHANMODES"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_help           (struct lclient *lcptr, struct client *cptr,
                              int             argc,  char         **argv);
static void m_help_index     (struct client  *cptr,  int            flags);
/*static void m_help_usermodes (struct client  *cptr);*/
static void m_help_chanmodes (struct client  *cptr);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_help_help[] = {
  "HELP [topic]",
  "",
  "Displays help about the given topic.",
  NULL
};

static struct msg m_help_msg = {
  "HELP", 0, 1, MFLG_CLIENT,
  { NULL, m_help, NULL, m_help },
  m_help_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_help_load(void)
{
  if(msg_register(&m_help_msg) == NULL)
    return -1;

  return 0;
}

void m_help_unload(void)
{
  msg_unregister(&m_help_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'help'                                                           *
 * argv[2] - command                                                          *
 * -------------------------------------------------------------------------- */
static void m_help(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  struct msg *mptr;
  uint32_t    i;

  if(!client_is_user(cptr))
    return;

  if(argv[2] == NULL || !str_icmp(argv[2], M_HELP_INDEX))
  {
    numeric_send(cptr, RPL_HELPSTART, M_HELP_INDEX,
                 "---------------- Help topics ----------------");
    numeric_send(cptr, RPL_HELPTXT, M_HELP_INDEX, "Client commands:");
    m_help_index(cptr, MFLG_UNREG | MFLG_CLIENT);
    numeric_send(cptr, RPL_HELPTXT, M_HELP_INDEX, "Operator commands:");
    m_help_index(cptr, MFLG_OPER);
    numeric_send(cptr, RPL_HELPTXT, M_HELP_INDEX, "Server commands:");
    m_help_index(cptr, MFLG_SERVER);
    numeric_send(cptr, RPL_HELPTXT, M_HELP_INDEX, "Others:");
    numeric_send(cptr, RPL_HELPTXT, M_HELP_INDEX, "CHANMODES    USERMODES");
    numeric_send(cptr, RPL_ENDOFHELP, M_HELP_INDEX);
    return;
  }

/*  if(!str_icmp(argv[2], "usermodes") || !str_icmp(argv[2], "umodes"))
  {
    m_help_usermodes(cptr);
    return;
  }*/

  if(!str_icmp(argv[2], "channelmodes") || !str_icmp(argv[2], "chanmodes"))
  {
    m_help_chanmodes(cptr);
    return;
  }

  if((mptr = msg_find(argv[2])) == NULL)
  {
    numeric_send(cptr, ERR_HELPNOTFOUND, argv[2]);
    return;
  }

  if(mptr->help == NULL || mptr->help[0] == NULL)
  {
    log(msg_log, L_fatal,
        "The developer was too lazy to write a help for %s. This must be sued.",
        mptr->cmd);
    ircd_shutdown();
  }

  for(i = 0; mptr->help[i]; i++)
  {
    if(i == 0)
      numeric_send(cptr, RPL_HELPSTART, mptr->cmd, mptr->help[0]);
    else
      numeric_send(cptr, RPL_HELPTXT, mptr->cmd, mptr->help[i]);
  }

  numeric_send(cptr, RPL_ENDOFHELP, mptr->cmd);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_help_index(struct client *cptr, int flags)
{
  struct node *node;
  struct msg  *mptr = NULL;
  uint32_t     msgi = 0;
  char        *msgs[4];
  uint32_t     i;

  for(i = 0; i < MSG_HASH_SIZE; i++)
  {
    dlink_foreach_data(&msg_table[i], node, mptr)
    {
      if((mptr->flags & flags) == 0)
        continue;

      msgs[msgi++] = mptr->cmd;

      if(msgi == 4)
      {
        client_send(cptr, ":%S 705 %C %s :%-12s %-12s %-12s %s",
                    server_me, cptr, M_HELP_INDEX,
                    msgs[0], msgs[1], msgs[2], msgs[3]);
        msgi = 0;
      }
    }
  }

  if(msgi == 3)
  {
    client_send(cptr, ":%S 705 %C %s :%-12s %-12s %s",
                server_me, cptr, M_HELP_INDEX,
                msgs[0], msgs[1], msgs[2]);
  }
  else if(msgi == 2)
  {
    client_send(cptr, ":%S 705 %C %s :%-12s %s",
                server_me, cptr, M_HELP_INDEX,
                msgs[0], msgs[1]);
  }
  else if(msgi == 1)
  {
    client_send(cptr, ":%S 705 %C %s :%s",
                server_me, cptr, M_HELP_INDEX,
                msgs[0]);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
/*static void m_help_usermodes(struct client  *cptr)
{
}*/

static void m_help_chanmodes(struct client  *cptr)
{
  uint32_t i;
  uint32_t j;

  numeric_send(cptr, RPL_HELPSTART, M_HELP_CHANMODES, "Supported channel modes:");
  numeric_send(cptr, RPL_HELPTXT, M_HELP_CHANMODES, "");

  for(i = 0; i < 0x40; i++)
  {
    if(chanmode_table[i].type == 0)
      continue;

    for(j = 0; chanmode_table[i].help[j]; j++)
      numeric_send(cptr, RPL_HELPTXT, M_HELP_CHANMODES,
                   chanmode_table[i].help[j]);
  }
}

