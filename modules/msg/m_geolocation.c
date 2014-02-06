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
 * $Id: m_geolocation.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
#include <chaosircd/msg.h>
#include <chaosircd/user.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/lclient.h>
#include <chaosircd/numeric.h>
#include <chaosircd/channel.h>
#include <chaosircd/chanuser.h>
#include <chaosircd/chanmode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_geolocation  (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);
static void ms_geolocation (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_geolocation_help[] = {
  "GEOLOCATION [SET|SEARCH] [geohash]",
  "",
  "Sets the geohash or searches for geohashes",
  NULL
};

static struct msg m_geolocation_msg = {
  "GEOLOCATION", 2, 2, MFLG_CLIENT,
  { NULL, m_geolocation, ms_geolocation, m_geolocation },
  m_geolocation_help
};

static const char base32[] = { 
  '0', '1', '2', '3', '4', '5', '6', '7', '8',
  '9', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'j',
  'k', 'm', 'n', 'p', 'q', 'r', 's', 't', 'u',
  'v', 'w', 'x', 'y', 'z',
  0
};

static int valid_base32(const char *s)
{
  while(*s)
  {
    if(!str_chr(base32, *s))
      return 0;

    s++;
  }
  return 1;
}

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_geolocation_load(void)
{
  if(msg_register(&m_geolocation_msg) == NULL)
    return -1;
  
  return 0;
}

void m_geolocation_unload(void)
{
  msg_unregister(&m_geolocation_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'GEOLOCATION'                                                    *
 * argv[2] - 'SET'                                                            *
 * argv[3] - geohash                                                          *
 * -------------------------------------------------------------------------- */
static void ms_geolocation (struct lclient *lcptr, struct client *cptr,
                            int             argc,  char         **argv)
{
  if(!str_icmp(argv[2], "SET"))
  {
     

    if(client_is_user(cptr))
    {
      strlcpy(cptr->user->name, argv[3], IRCD_USERLEN);


    }
   
    server_send(lcptr, NULL, CAP_NONE, CAP_NONE, ":%C GEOLOCATION SET :%s", argv[3]);
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'GEOLOCATION'                                                    *
 * -------------------------------------------------------------------------- */
static void m_geolocation(struct lclient *lcptr, struct client *cptr,
                          int             argc,  char         **argv)
{
  if(!str_icmp(argv[2], "SET"))
  {
    if(str_len(argv[3]) < 9)
    {
      client_send(cptr, ":%S NOTICE %C :(m_geolocation) invalid geohash '%s': too short", server_me, cptr, argv[3]);
      return;
    }
    if(!valid_base32(argv[3]))
    {
      client_send(cptr, ":%S NOTICE %C :(m_geolocation) invalid geohash '%s': not base32", server_me, cptr, argv[3]);
      return;
    }

    ms_geolocation(lcptr, cptr, argc, argv);
  }
  else if(!str_icmp(argv[2], "SEARCH"))
  {
     
    
  }


  /*if(argc > 3 && !str_icmp(argv[2], "-server"))
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
    
    ms_geolocation(lcptr, cptr, argc - 1, av);
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
//          if(acuptr->client->user->modes & UFLG(i))
//            continue;

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
      // FIXME: invisible check 
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
  }*/
}

