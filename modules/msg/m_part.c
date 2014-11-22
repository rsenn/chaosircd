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
 * $Id: m_part.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/timer.h"
#include "libchaos/log.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/user.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/channel.h"
#include "ircd/numeric.h"
#include "ircd/chanmode.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_part (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);
static void ms_part(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_part_help[] = {
  "PART <channel[,channel,...]> [text]",
  "",
  "This command causes you to leave the given channel",
  "or the given channels. The text can be shown to the",
  "channel when parting.",
  NULL
};

static struct msg m_part_msg = {
  "PART", 1, 2, MFLG_CLIENT,
  { m_unregistered, m_part, ms_part, m_part },
  m_part_help
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define channel_owner_flags ((cuptr->flags & CHFLG(o)) | \
                             (cuptr->flags & CHFLG(h)) | \
                             (cuptr->flags & CHFLG(v)))

#define chanuser_is_owner(cuptr) \
  ((cuptr->flags & channel_owner_flags) == channel_owner_flags)

#define channel_is_persistent(chptr) \
  ((chptr)->modes & CHFLG(P))

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_part_load(void)
{
  if(msg_register(&m_part_msg) == NULL)
    return -1;

  return 0;
}

void m_part_unload(void)
{
  msg_unregister(&m_part_msg);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_part_send(struct lclient *lcptr, struct client *cptr,
                        struct channel *chptr, const char    *reason);

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'part'                                                           *
 * argv[2] - channel                                                          *
 * argv[3] - reason                                                           *
 * -------------------------------------------------------------------------- */
static void m_part(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  char             reason[IRCD_TOPICLEN + 1];
  struct channel  *chptr;
  struct chanuser *cuptr;

  reason[0] = '\0';

  if(argc > 3)
    strlcpy(reason, argv[3], IRCD_TOPICLEN + 1);

  chptr = channel_find_name(argv[2]);

  if(chptr == NULL)
  {
    client_send(cptr, numeric_format(ERR_NOSUCHCHANNEL),
                client_me->name, cptr->name, argv[2]);
    return;
  }

  cuptr = chanuser_find(chptr, cptr);

  if(cuptr == NULL)
  {
    client_send(cptr, numeric_format(ERR_NOTONCHANNEL),
                client_me->name, cptr->name, argv[2]);
    return;
  }

  if(chanuser_is_owner(cuptr) && client_is_local(cptr) && channel_is_persistent(chptr))
//     !str_ncmp(cptr->name, &chptr->name[1], str_len(&chptr->name[1])))
  {
    client_send(cptr, ":%s NOTICE %N :*** You need to /OPART if you really want to leave %s.",
                server_me->name, cptr, chptr->name);
    return;
  }

  m_part_send(NULL, cptr, chptr, reason);

  /* send server PART */
  chanuser_discharge(NULL, cuptr, argv[3]);

  chanuser_delete(cuptr);

  if(chptr->chanusers.size == 0 && !channel_is_persistent(chptr))
    channel_delete(chptr);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'part'                                                           *
 * argv[2] - channel                                                          *
 * argv[3] - reason                                                           *
 * -------------------------------------------------------------------------- */
static void ms_part(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  struct channel  *chptr;
  struct chanuser *cuptr;

  if((chptr = channel_find_name(argv[2])) == NULL)
  {
    log(channel_log, L_warning, "Dropping PART for invalid channel %s.",
        argv[2]);
    return;
  }

  if((cuptr = chanuser_find(chptr, cptr)) == NULL)
  {
    log(channel_log, L_warning, "Dropping PART because user %s is not on %s.",
        cptr->name, chptr->name);
    return;
  }

  m_part_send(lcptr, cptr, chptr, argv[3]);

  /* send server PART */
  chanuser_discharge(lcptr, cuptr, argv[3]);

  chanuser_delete(cuptr);

  if(chptr->chanusers.size == 0)
    channel_delete(chptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void m_part_send(struct lclient *lcptr, struct client *cptr,
                        struct channel *chptr, const char    *reason)
{
  if(reason && reason[0])
    channel_send(NULL, chptr, CHFLG(NONE), CHFLG(NONE),
                 ":%s!%s@%s PART %s :%s",
                 cptr->name, cptr->user->name,
                 cptr->host, chptr->name, reason);
  else
    channel_send(NULL, chptr, CHFLG(NONE), CHFLG(NONE),
                 ":%s!%s@%s PART %s",
                 cptr->name, cptr->user->name,
                 cptr->host, chptr->name);
}
