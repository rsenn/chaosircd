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
 * $Id: m_who.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/log.h"
#include "libchaos/str.h"
#include "libchaos/dlink.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/user.h"
#include "ircd/chars.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/channel.h"
#include "ircd/chanuser.h"
#include "ircd/chanmode.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_who  (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);
static void ms_who (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_who_help[] = {
  "WHO [-server <server>] [channel]",
  "",
  "Displays users visible to you. Can be limited to a channel and",
  "forwarded to a remote server (RFC violating BitchX extension).",
  NULL
};

static struct msg m_who_msg = {
  "WHO", 1, 3, MFLG_CLIENT,
  { NULL, m_who, ms_who, m_who },
  m_who_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_who_load(void)
{
  if(msg_register(&m_who_msg) == NULL)
    return -1;

  return 0;
}

void m_who_unload(void)
{
  msg_unregister(&m_who_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'who'                                                          *
 * -------------------------------------------------------------------------- */
static void ms_who (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  struct server *asptr;
  char          *av[5];

  if((asptr = server_find_name(argv[2])) == NULL)
  {
    log(server_log, L_warning, "Dropping WHO for unknown server %s.",
        argv[2]);
    return;
  }

  if(asptr == server_me)
  {
    av[0] = argv[0];
    av[1] = argv[1];
    av[2] = argv[3];
    av[4] = NULL;

    m_who(lcptr, cptr, argc - 1, av);
  }
  else
  {
    if(argv[3])
      client_send(asptr->client, ":%C WHO %S :%s",
                  cptr, asptr, argv[3]);
    else
      client_send(asptr->client, ":%C WHO %S",
                  cptr, asptr);
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'who'                                                          *
 * -------------------------------------------------------------------------- */
static void m_who(struct lclient *lcptr, struct client *cptr,
                  int             argc,  char         **argv)
{
  if(argc > 3 && !str_icmp(argv[2], "-server"))
  {
    struct server *asptr;
    char          *av[6];

    if((asptr = server_find_name(argv[3])) == NULL)
    {
      numeric_send(cptr, ERR_NOSUCHSERVER, argv[3]);
      return;
    }

    av[0] = argv[0];
    av[1] = argv[1];
    av[2] = argv[3];
    av[3] = argv[4];
    av[4] = argv[5];
    av[5] = NULL;

    ms_who(lcptr, cptr, argc - 1, av);
  }
  else if(chars_valid_chan(argv[2]))
  {
    struct chanuser *acuptr = NULL;
    struct chanuser *cuptr = NULL;
    struct channel  *chptr;
    struct node     *node;

    if((chptr = channel_find_name(argv[2])) == NULL)
    {
      numeric_send(cptr, ERR_NOSUCHCHANNEL, argv[2]);
      return;
    }

    client_serial++;

    if((cuptr = chanuser_find(chptr, cptr)))
    {
      dlink_foreach_data(&chptr->chanusers, node, acuptr)
      {
        if(acuptr->client->serial == client_serial)
          continue;

        numeric_send(cptr, RPL_WHOREPLY,
                     chptr->name,
                     acuptr->client->user->name,
                     acuptr->client->host,
                     acuptr->client->origin->name,
                     acuptr->client->name,
                     acuptr->prefix[0] ? acuptr->prefix : "*",
                     acuptr->client->hops,
                     acuptr->client->info);

        acuptr->client->serial = client_serial;
      }
    }
    else
    {
      if(!(chptr->modes & CHFLG(s)))
      {
        dlink_foreach_data(&chptr->chanusers, node, acuptr)
        {
          if(acuptr->client->serial == client_serial)
            continue;
/*          if(acuptr->client->user->modes & UFLG(i))
            continue;*/

          numeric_send(cptr, RPL_WHOREPLY,
                       chptr->name,
                       acuptr->client->user->name,
                       acuptr->client->host,
                       acuptr->client->origin->name,
                       acuptr->client->name,
                       acuptr->prefix[0] ? acuptr->prefix : "*",
                       acuptr->client->hops,
                       acuptr->client->info);

          acuptr->client->serial = client_serial;
        }
      }

    }

    numeric_send(cptr, RPL_ENDOFWHO, chptr->name);
  }
  else if(chars_valid_nick(argv[2]))
  {
    struct chanuser *cuptr;
    struct client   *acptr;

    if((acptr = client_find_nickhw(cptr, argv[2])) == NULL)
      return;

    dlink_foreach(&acptr->user->channels, cuptr)
    {
      /* FIXME: invisible check */
      if((cuptr->channel->modes & CHFLG(s)) &&
         !channel_is_member(cuptr->channel, cptr))
        continue;

      numeric_send(cptr, RPL_WHOREPLY,
                   cuptr->channel->name,
                   acptr->user->name,
                   acptr->host,
                   acptr->origin->name,
                   acptr->name,
                   cuptr->prefix[0] ? cuptr->prefix : "*",
                   acptr->hops,
                   acptr->info);
    }

    numeric_send(cptr, RPL_ENDOFWHO, acptr->name);
  }
  else
  {
    struct chanuser *cuptr;
    struct node     *node;
    struct client   *acptr = NULL;

    client_serial++;

    dlink_foreach(&cptr->user->channels, cuptr)
    {
      if(cuptr->client->serial == client_serial)
        continue;

      numeric_send(cptr, RPL_WHOREPLY,
                   "*",
                   cuptr->client->user->name,
                   cuptr->client->host,
                   cuptr->client->origin->name,
                   cuptr->client->name,
                   "*",
                   cuptr->client->hops,
                   cuptr->client->info);

      cuptr->client->serial = client_serial;
    }

    dlink_foreach_data(&client_lists[CLIENT_GLOBAL][CLIENT_USER], node, acptr)
    {
      if(acptr->serial == client_serial)
        continue;

      numeric_send(cptr, RPL_WHOREPLY,
                   "*",
                   acptr->user->name,
                   acptr->host,
                   acptr->origin->name,
                   acptr->name,
                   "*",
                   acptr->hops,
                   acptr->info);

      acptr->serial = client_serial;
    }

    numeric_send(cptr, RPL_ENDOFWHO, "*");
  }
}

