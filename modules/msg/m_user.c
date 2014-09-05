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
 * $Id: m_user.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "defs.h"
#include "io.h"
#include "log.h"
#include "str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/msg.h>
#include <ircd/user.h>
#include <ircd/chars.h>
#include <ircd/client.h>
#include <ircd/lclient.h>
#include <ircd/numeric.h>
#include <ircd/usermode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mr_user(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

static void ms_user(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

static int m_user_login_log;

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_user_help[] = {
  "USER <username> <mode> * <realname>",
  "",
  "Used by clients to give additional information about",
  "the connecting client.",
  "Usermodes will be initialised to the specified flags",
  "if possible.",
  NULL
};

static struct msg m_user_msg = {
  "USER", 4, 4, MFLG_CLIENT | MFLG_UNREG,
  { mr_user, m_registered, ms_user, m_registered },
  m_user_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_user_load(void)
{
  if(msg_register(&m_user_msg) == NULL)
    return -1;

  m_user_login_log = log_source_find("login");                                                                                                                     
  if(m_user_login_log == -1)                                                                                                                                       
    m_user_login_log = log_source_register("login");                                                                                                               
      
  return 0;
}

void m_user_unload(void)
{
  log_source_unregister(m_user_login_log);
  m_user_login_log = -1;

  msg_unregister(&m_user_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'user'                                                           *
 * argv[2] - username                                                         *
 * argv[3] - mode                                                             *
 * argv[4] - server                                                           *
 * argv[5] - realname                                                         *
 * -------------------------------------------------------------------------- */
static void mr_user(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  char username[IRCD_USERLEN + 1];

  if(lcptr->user == NULL)
  {
    username[0] = '~';
    strlcpy(&username[1], argv[2], sizeof(username) - 1);
    strlcpy(lcptr->info, argv[5], sizeof(lcptr->info));

    lcptr->user = user_new(username, NULL);

    if(argv[3][0] == '+')
      strlcpy(lcptr->user->mode, &argv[3][1], sizeof(lcptr->user->mode));
    else
      lcptr->user->mode[0] = '\0';

    /* usermode stuff has been moved to lclient_login,
       because we should do the usermode only when registered.

       (or else a unregistered user that sent this command with +i
        could end up on the invisible list) */
/*    usermode_make(lcptr->user, argv[3], NULL, USERMODE_NOFLAG);*/

    if(lcptr->name[0])
      lclient_login(lcptr);
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'user'                                                           *
 * argv[2] - username                                                         *
 * argv[3] - host                                                             *
 * argv[4] - server                                                           *
 * argv[5] - realname                                                         *
 * -------------------------------------------------------------------------- */
static void ms_user(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
}
