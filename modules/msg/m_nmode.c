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
 * $Id: m_nmode.c,v 1.3 2006/09/28 09:56:24 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/log.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/msg.h>
#include <ircd/chars.h>
#include <ircd/client.h>
#include <ircd/server.h>
#include <ircd/channel.h>
#include <ircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void ms_nmode(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *ms_nmode_help[] = {
  "NMODE <channel> <ts> <flags> <info[:info:...]> <ts[:ts:...]> <arg[:arg:...]>",
  "",
  "Synchronizes channel modes with a remote server.",
  NULL
};

static struct msg ms_nmode_msg = {
  "NMODE", 5, 6, MFLG_SERVER,
  { NULL, NULL, ms_nmode, NULL },
  ms_nmode_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_nmode_load(void)
{
  if(msg_register(&ms_nmode_msg) == NULL)
    return -1;

  return 0;
}

void m_nmode_unload(void)
{
  msg_unregister(&ms_nmode_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'nmode'                                                          *
 * argv[2] - channel                                                          *
 * argv[3] - channel ts                                                       *
 * argv[4] - mode flags                                                       *
 * argv[5] - mode infos                                                       *
 * argv[6] - mode timestamps                                                  *
 * argv[7] - mode args                                                        *
 * -------------------------------------------------------------------------- */
static void ms_nmode(struct lclient *lcptr, struct client *cptr,
                     int             argc,  char         **argv)
{
  struct channel *chptr;
  struct list     modelist;
  time_t          ts;
  char           *infos[CHANMODE_PER_LINE + 1] = { NULL };
  char           *timestamps[CHANMODE_PER_LINE + 1];
  char           *args[CHANMODE_PER_LINE + 1];
  size_t          len;
  size_t          i;
  char           *lastinfo = infos[0];
  hash_t          lasthash = 0;
  
  if((chptr = channel_find_name(argv[2])) == NULL)
  {
    log(client_log, L_warning, "Dropping NMODE for unknown channel %s.",
        argv[2]);
    return;
  }

  ts = str_toul(argv[3], NULL, 10);

  if(ts > chptr->ts)
  {
    log(server_log, L_warning, "Dropping NMODE for %s with too recent TS",
        chptr->name);

    cptr->server->in.chanmodes += str_len(argv[4]);

    return;
  }

  if(chptr->ts != ts)
  {
    log(channel_log, L_warning, "TS for channel %s changed from %lu to %lu.",
        chptr->name, chptr->ts, ts);

    chptr->ts = ts;
  }

  len = str_len(argv[4]);

  if(str_tokenize_s(argv[5], infos, CHANMODE_PER_LINE, ';') != len ||
     str_tokenize_s(argv[6], timestamps, CHANMODE_PER_LINE, ';') != len ||
     str_tokenize_s(argv[7], args, CHANMODE_PER_LINE, ';') != len)
  {
    log(chanmode_log, L_warning,
        "Argument count does not match on NMODE for %s.", chptr->name);
    return;
  }

  dlink_list_zero(&modelist);

  for(i = 0; i < len; i++)
  {
    struct chanmodechange *cmcptr;

    if(!chars_isalpha(argv[4][i]))
      continue;

    cmcptr = chanmode_change_add(&modelist, CHANMODE_ADD,
                                 argv[4][i], args[i], NULL);

    if(cmcptr == NULL)
    {
      log(chanmode_log, L_warning, "Unknown flag '%c' in NMODE for %s.",
          argv[4][i], chptr->name);
      continue;
    }

    if(infos[i][0] != '-')
    {
      strlcpy(cmcptr->info, (infos[i][0] != '*' ? infos[i] : lastinfo), sizeof(cmcptr->info));
      cmcptr->ihash = (infos[i][0] != '*' ? str_ihash(cmcptr->info) : lasthash);
      lastinfo = cmcptr->info;
      lasthash = cmcptr->ihash;
    }

    if(i)
      ts += str_tol(timestamps[i], NULL, 10);
    else
      ts = str_toul(timestamps[0], NULL, 10);

    cmcptr->ts = ts;
  }

  cptr->server->in.chanmodes += modelist.size;

  chanmode_apply(lcptr, cptr, chptr, NULL, &modelist);

  chanmode_send_local(cptr, chptr, modelist.head, IRCD_MODESPERLINE);

  chanmode_introduce(lcptr, cptr, chptr, modelist.head);

  chanmode_change_destroy(&modelist);
}
