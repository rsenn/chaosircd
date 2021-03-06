/* 
 * Copyright (C) 2013-2014  CrowdGuard organisation
 * All rights reserved.
 *
 * Author: Roman Senn <rls@crowdguard.org>
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/log.h"
#include "libchaos/str.h"
#include "libchaos/timer.h"
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
#include "ircd/crowdguard.h"
#include "ircd/usermode.h"

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
  "GEOLOCATION", 1, 0, MFLG_CLIENT,
  { NULL, m_geolocation, ms_geolocation, m_geolocation },
  m_geolocation_help
};

static int m_geolocation_log;

static const char base32[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8',
  '9', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'j',
  'k', 'm', 'n', 'p', 'q', 'r', 's', 't', 'u',
  'v', 'w', 'x', 'y', 'z',
  0
};

/* -------------------------------------------------------------------------- */
static int valid_base32(const char *s)
{
  while(*s)
  {
    if(!strchr(base32, *s))
      return 0;

    s++;
  }
  return 1;
}

/* -------------------------------------------------------------------------- */
static int check_hashes(int nhashes, char *hasharray[], const char *hash)
{
  int i;

  for(i = 0; i < nhashes; i++)
  {
    size_t len = str_len(hasharray[i]);

    /*debug(ircd_log, "checking hash[%d](%s) against %s", i, hasharray[i], hash);*/

    if(!str_ncmp(hasharray[i], hash, len))
      return 1;
  }
  return 0;
}

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int m_geolocation_load(void)
{
  if(msg_register(&m_geolocation_msg) == NULL)
    return -1;

  if((m_geolocation_log = log_source_register("geolocation")) == -1)
    return -1;

  return 0;
}

void m_geolocation_unload(void)
{
  log_source_unregister(m_geolocation_log);

  msg_unregister(&m_geolocation_msg);
}

/* -------------------------------------------------------------------------- */
static int
have_common_channel(struct user* u1, struct user* u2) {
	struct chanuser *cu1, *cu2;
	struct node *n1, *n2;

	dlink_foreach_data(&u1->channels, n1, cu1) {
		struct channel* chptr = cu1->channel;
		dlink_foreach_data(&u2->channels, n2, cu2) {
			if(chptr == cu2->channel)
				return 1;
		}
	}
	return 0;
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
  /*  char channame[IRCD_CHANNELLEN+1];*/

    if(client_is_user(cptr))
    {
      int first_location = (cptr->user->name[0] == '~');
      int changed_location = strcmp(cptr->user->name, argv[3]);
      int do_log = 0;
      struct chanuser *cuptr;
      struct channel *chptr = NULL;
           struct node *nptr;

      strlcpy(cptr->user->name, argv[3], IRCD_USERLEN);

      cptr->lastmsg = timer_systime;

      /*channame[0] = '#';
      strlcpy(&channame[1], cptr->name, sizeof(channame)-1);
*/
      dlink_foreach_data(&cptr->user->channels, nptr, cuptr)
      {
        chptr = cuptr->channel;

        /* if this server is responsible for that channel and the channel is persistent, then log*/
        if(chptr->server == server_me && (chptr->modes & CHFLG(P)))
        {
          do_log = 1;
          break;
        }
      }  
   
      if(!(cptr->user->modes & (1ll << ((int)'g' - 0x40)))) 
      {
        char umodestr[] = "+g"; 
        char *umodelist[] = { umodestr, NULL };

        if(usermode_make(cptr->user, umodelist, cptr, USERMODE_OPTION_PERMISSION))                                                                                                                           
        {                                                                                                                                                                       
          /* let the user know his changes */                                                                                                                                   
          usermode_change_send(lcptr, cptr, USERMODE_SEND_LOCAL);                                                                                                               
                                                                                                                                                                                
          /* and the whole network */                                                                                                                                           
          usermode_change_send(lcptr, cptr, USERMODE_SEND_REMOTE);                                                                                                              
        }                                                                   
      } 

      /* Log the geolocation if there's a channel for this user (user has been set to sharp)*/
    /*  if((chptr = channel_find_name(channame)))*/
      if(do_log && changed_location)
      {
        /* CrowdGuard functionality requires a persistent (+P) channel*/
    /*    if(chptr->modes & CHFLG(P))*/
          log(m_geolocation_log, L_verbose, "%s geolocation for %s to %s", (first_location ? "Set" : "Changed"), cptr->name, cptr->user->name);
      }
    }

    server_send(lcptr, NULL, CAP_NONE, CAP_NONE, ":%C GEOLOCATION SET :%s", cptr, argv[3]);
  }
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'GEOLOCATION'                                                    *
 * -------------------------------------------------------------------------- */
static void m_geolocation(struct lclient *lcptr, struct client *cptr,
                          int             argc,  char         **argv)
{
  enum { SET, SEARCH } mode;
  size_t minlen = 9, i, len,  hashcount;
  char **hasharray;

  if(argc <= 3)
  {
    numeric_lsend(lcptr, ERR_NEEDMOREPARAMS, argv[1]);
    return;
  }

  if(!str_icmp(argv[2], "SEARCH"))
    mode = SEARCH;
  else
    mode = SET;

  if(mode == SEARCH)
    hashcount = argc - 3;
  else
    hashcount = 1;

  hasharray = &argv[3];

  /* check every hash for valid base32h */
  for(i = 0; i < hashcount; i++)
  {
    if(!valid_base32(hasharray[i]))
    {
      client_send(cptr, ":%S NOTICE %C :(m_geolocation) invalid geohash '%s': not base32", server_me, cptr, hasharray[i]);
      return;
    }
  }

  if(!str_icmp(argv[2], "SEARCH"))
  {
    minlen = 2;
  }

  /* check every hash for minimum length */
  for(i = 0; i < hashcount; i++)
  {
    len = str_len(hasharray[i]);

    if(len < minlen)
    {
      client_send(cptr, ":%S NOTICE %C :(m_geolocation) invalid geohash '%s': too short", server_me, cptr, hasharray[i]);
      return;
    }
  }

  if(!str_icmp(argv[2], "SET"))
  {
    ms_geolocation(lcptr, cptr, argc, argv);
  }
  else if(!str_icmp(argv[2], "SEARCH"))
  {
     struct user *user;
     struct node *node;
     char buffer[IRCD_LINELEN+1];
     int count;
     size_t di, dlen = 0;
     di = str_snprintf(buffer, sizeof(buffer), ":%S 600 %N", server_me, cptr);

     count = 0;

     dlink_foreach_data(&user_list, node, user) {
       if(!user->client) continue;
       if(user->client == cptr) continue;
       if(str_len(user->name) < len) continue;
       if(!valid_base32(user->name)) continue;

/*       if(!str_ncmp(user->name, argv[3], len))*/
       if(check_hashes(hashcount, hasharray, user->name))
       {
         size_t toklen = str_len(user->client->name);
         int show_geohash = have_common_channel(cptr->user, user);

         if(show_geohash)
        	 toklen += 1 + str_len(user->name);

         /* location reply only when the searching and the found user have at least 1 channel in common*/
         if(di+dlen+1+toklen > IRCD_LINELEN)
		 {
		   client_send(cptr, "%s", buffer);
		   dlen = 0;
		 }
		 dlen += str_snprintf(&buffer[di+dlen], sizeof(buffer)-(di+dlen), (show_geohash?" %N!%U":" %N"), user->client, user->client);

		 if(lclient_is_oper(lcptr)) /* && cptr->user->name[0] == '~')*/
		   lclient_send(lcptr, ":%S NOTICE %N :--- geolocation reply: %N!%U", server_me, cptr, user->client, user->client);

         count++;
       }
     }

     if(dlen)
       client_send(cptr, "%s", buffer);

     client_send(cptr, ":%S 601 %N %s %u :end of /GEOLOCATION query", server_me, cptr, argv[3], count);

     if(lclient_is_oper(lcptr)) /* && cptr->user->name[0] == '~')*/
       lclient_send(lcptr, ":%S NOTICE %N :--- end of /geolocation (%d replies)", server_me, cptr, count);
  }
}

