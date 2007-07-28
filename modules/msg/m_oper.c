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
 * $Id: m_oper.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/defs.h>
#include <libchaos/io.h>
#include <libchaos/log.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/msg.h>
#include <chaosircd/ircd.h>
#include <chaosircd/oper.h>
#include <chaosircd/user.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/channel.h>
#include <chaosircd/numeric.h>
#include <chaosircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_oper (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

static void ms_oper(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

static void mo_oper(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_oper_help[] = {
  "OPER <username> <password>",
  "",
  "If supplied with a correct username/password pair,",
  "you get oper'ed, and have elevated privileges.",
  NULL
};    

static struct msg m_oper_msg = {
  "OPER", 2, 2, MFLG_CLIENT | MFLG_UNREG,
  { m_unregistered, m_oper, ms_oper, mo_oper },
  m_oper_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_oper_load(void)
{
  if(msg_register(&m_oper_msg) == NULL)
    return -1;
  
  return 0;
}

void m_oper_unload(void)
{
  msg_unregister(&m_oper_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'oper'                                                           *
 * argv[2] - name                                                             *
 * argv[3] - password                                                         *
 * -------------------------------------------------------------------------- */
static void m_oper(struct lclient *lcptr, struct client *cptr, 
                   int             argc,  char         **argv)
{
  struct oper *oper;
  
  oper = oper_find(argv[2]);
  
  if(oper == NULL)
  {
    numeric_send(cptr, ERR_NOOPERHOST);
    log(oper_log, L_warning, 
        "failed oper attempt by %s (%s@%s) - "
        "no such oper entry: %s [%s]",
        cptr->name, cptr->user->name,
        cptr->host, argv[2], argv[3]);
    return;
  }
  
  if(strcmp(argv[3], oper->passwd))
  {    
    client_send(cptr, numeric_format(ERR_PASSWDMISMATCH), 
                client_me->name, cptr->name);    
    
    log(oper_log, L_warning, 
        "failed oper attempt by %s (%s@%s) - "
        "wrong password for %s: %s",
        cptr->name, cptr->user->name,
        cptr->host, argv[2], argv[3]);
    return;
  }
  
  if(cptr->oper == NULL)
  {
    oper_up(oper, cptr);
    cptr->oper = oper;
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'oper'                                                           *
 * argv[2] - name                                                             *
 * argv[3] - password                                                         *
 * -------------------------------------------------------------------------- */
static void ms_oper(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'oper'                                                           *
 * argv[2] - name                                                             *
 * argv[3] - password                                                         *
 * -------------------------------------------------------------------------- */
static void mo_oper(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  numeric_send(cptr, RPL_YOUREOPER);
}
