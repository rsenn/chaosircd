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
 * $Id: m_cap.c,v 1.3 2006/09/28 09:56:24 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/io.h"
#include "libchaos/log.h"
#include "libchaos/str.h"
#include "libchaos/timer.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/ircd.h"
#include "ircd/lclient.h"
#include "ircd/msg.h"
#include "ircd/server.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void mr_cap(struct lclient *lcptr, struct client *cptr, int argc,
                   char **argv);

/* -------------------------------------------------------------------------- *
 * Supported client-facing capabilities. Add a row here (and implement the   *
 * actual behaviour wherever it belongs - see CLICAP_ECHO_MESSAGE's use in   *
 * channel_message()/client_message()) to support a new one.                *
 * -------------------------------------------------------------------------- */
struct clicap {
  char    *name;
  uint64_t cap;
};

static struct clicap m_cap_table[] = {
    {"echo-message", CLICAP_ECHO_MESSAGE},
    {NULL, 0},
};

static int m_cap_find(const char *name) {
  size_t i;

  for (i = 0; m_cap_table[i].name; i++) {
    if (!str_icmp(m_cap_table[i].name, name))
      return i;
  }

  return -1;
}

/* Space-joined names of every capability in `mask'; NULs `buf' if none. */
static void m_cap_names(uint64_t mask, char *buf, size_t n) {
  size_t i;

  buf[0] = '\0';

  for (i = 0; m_cap_table[i].name; i++) {
    if (!(mask & m_cap_table[i].cap))
      continue;

    if (buf[0])
      strlcat(buf, " ", n);

    strlcat(buf, m_cap_table[i].name, n);
  }
}

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *mr_cap_help[] = {
    "CAP <LS|LIST|REQ|ACK|NAK|CLEAR|END> [args...]",
    "",
    "Used in the beginning of a irc session, to allow you",
    "to connect to the server. Also user on server connection",
    "for link authentication.",
    NULL};

static struct msg mr_cap_msg = {"CAP",
                                1,
                                2,
                                MFLG_CLIENT | MFLG_UNREG,
                                {mr_cap, mr_cap, NULL, mr_cap},
                                mr_cap_help};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_cap_load(void) {
  if (msg_register(&mr_cap_msg) == NULL)
    return -1;

  return 0;
}

void m_cap_unload(void) { msg_unregister(&mr_cap_msg); }

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'CAP'                                                            *
 * argv[2] - subcommand (LS, LIST, REQ, END, ...)                            *
 * argv[3] - arguments (capability list for REQ, version for LS)             *
 *                                                                            *
 * Registration doesn't wait for CAP END in this server (it completes as     *
 * soon as NICK+USER are both in, same as a client that never sent CAP at    *
 * all) - so a client is free to send CAP LS/REQ/END either before or after  *
 * NICK/USER, in any order, and this handler works the same either way. In   *
 * particular CAP REQ before NICK/USER (which is how most real IRCv3        *
 * clients sequence it) means `cptr' - struct client, only allocated once   *
 * registration completes - can still be NULL here; negotiated capabilities *
 * are therefore stored on `lcptr' (struct lclient), which always exists.   *
 * The target name in every reply is "*" pre-registration (no cptr, or      *
 * cptr->name isn't set yet) and the real nick afterwards, per the CAP      *
 * spec.                                                                     *
 * -------------------------------------------------------------------------- */
static void mr_cap(struct lclient *lcptr, struct client *cptr, int argc,
                   char **argv) {
  const char *target = (cptr && cptr->name[0]) ? cptr->name : "*";
  char        names[IRCD_LINELEN + 1];

  if (!str_icmp(argv[2], "LS")) {
    m_cap_names(~0ULL, names, sizeof(names));
    lclient_send(lcptr, "CAP %s LS :%s", target, names);
  } else if (!str_icmp(argv[2], "LIST")) {
    m_cap_names(lclient_clicaps(lcptr), names, sizeof(names));
    lclient_send(lcptr, "CAP %s LIST :%s", target, names);
  } else if (!str_icmp(argv[2], "REQ")) {
    char    *capv[64];
    ssize_t  capc, i;
    uint64_t enable = 0, disable = 0;
    int      unknown = 0;

    if (!argv[3]) {
      lclient_send(lcptr, "CAP %s NAK :", target);
      return;
    }

    capc = str_tokenize(argv[3], capv, 63);

    for (i = 0; i < capc; i++) {
      int drop = capv[i][0] == '-';
      const char *name = drop ? capv[i] + 1 : capv[i];
      int         capi = m_cap_find(name);

      if (capi < 0) {
        unknown = 1;
        break;
      }

      if (drop)
        disable |= m_cap_table[capi].cap;
      else
        enable |= m_cap_table[capi].cap;
    }

    if (unknown) {
      lclient_send(lcptr, "CAP %s NAK :%s", target, argv[3]);
      return;
    }

    lclient_set_clicaps(lcptr, (lclient_clicaps(lcptr) & ~disable) | enable);

    lclient_send(lcptr, "CAP %s ACK :%s", target, argv[3]);
  } else if (!str_icmp(argv[2], "END")) {
    /* No-op: registration never waited on this to begin with. */
  } else {
    lclient_send(lcptr, "CAP %s LS :", target);
  }
}
