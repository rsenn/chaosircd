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
 * $Id: m_umode.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/log.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/usermode.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_umode         (struct lclient *lcptr, struct client *cptr,
                            int             argc,  char         **argv);
static void ms_umode        (struct lclient *lcptr, struct client *cptr,
                            int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_umode_help[] = {
  "UMODE <flags>",
  "",
  "Changes your usermodes. For a detailed",
  "description of the usermodes use the",
  "following command:",
  "/quote help usermodes",
  NULL
};

static struct msg m_umode_msg = {
  "UMODE", 0, 0, MFLG_CLIENT,
  { NULL, m_umode, ms_umode, m_umode },
  m_umode_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_umode_load(void)
{
  if(msg_register(&m_umode_msg) == NULL)
    return -1;

  return 0;
}

void m_umode_unload(void)
{
  msg_unregister(&m_umode_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'umode'                                                          *
 * argv[2] - [modebuf]                                                        *
 * -------------------------------------------------------------------------- */
static void m_umode(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  /* call usermode_show if it's a mode request */
  if(argc < 3)
  {
    usermode_show(cptr);
    return;
  }

  /* parse the string and make struct usermodechange */
  if(usermode_make(cptr->user, argv + 2, cptr,
                   USERMODE_OPTION_PERMISSION))
  {
    /* let the user know his changes */
    usermode_change_send(lcptr, cptr, USERMODE_SEND_LOCAL);

    /* and the whole network */
    usermode_change_send(lcptr, cptr, USERMODE_SEND_REMOTE);
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'umode'                                                          *
 * argv[2] - [modebuf]                                                        *
 * -------------------------------------------------------------------------- */
static void ms_umode(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  if(usermode_make(cptr->user, argv + 2, cptr, 0UL))
  {
    usermode_change_send(lcptr, cptr, USERMODE_SEND_REMOTE);

    return;
  }

  log(usermode_log, L_warning, "UMODE from server with no change");
}
