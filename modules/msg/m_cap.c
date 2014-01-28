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
 * $Id: m_cap.c,v 1.3 2006/09/28 09:56:24 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/io.h>
#include <libchaos/timer.h>
#include <libchaos/log.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/msg.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/lclient.h>
#include <chaosircd/server.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mr_cap (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mr_cap_help[] = {
  "CAP <LS|LIST|REQ|ACK|NAK|CLEAR|END> [args...]",
  "",
  "Used in the beginning of a irc session, to allow you",
  "to connect to the server. Also user on server connection",
  "for link authentication.",
  NULL
};

static struct msg mr_cap_msg = {
  "CAP", 1, 2, MFLG_CLIENT | MFLG_UNREG,
  { mr_cap, m_registered, NULL, m_registered },
  mr_cap_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_cap_load(void)
{
  if(msg_register(&mr_cap_msg) == NULL)
    return -1;
  
  return 0;
}

void m_cap_unload(void)
{
  msg_unregister(&mr_cap_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'CAP'                                                            *
 * argv[2] - subcommand                                                       *
 * argv[3] - arguments                                                        *
 * -------------------------------------------------------------------------- */
static void mr_cap(struct lclient *lcptr, struct client *cptr, 
                   int             argc,  char         **argv)
{
/*  Mitchell, et al.       Expires September 5, 2005               [Page 16]
 *
 *   
 *  Internet-Draft                  IRC CAP                       March 2005
 *
 *  Appendix A. Examples
 *
 *
 *     In the following examples, lines preceded by "CLIENT:" indicate
 *     protocol messages sent by the client, and lines preceded by "SERVER:"
 *     indicate protocol messages sent by the server.  For clarity, the
 *     origin field for server-originated protocol messages has been
 *     omitted.  This field would consist of a colon (':') followed by the
 *     full server name, and would be the first field in the command.
 *
 *     A client communicating with a server not supporting CAP.
 *
 *            CLIENT: CAP LS
 *            CLIENT: NICK nickname
 *            CLIENT: USER username ignored ignored :real name
 *            SERVER: 001 [...]
 */


  log(lclient_log, L_warning, "Unsupported CAP command: %s %s", argv[1], argv[2]);
}
