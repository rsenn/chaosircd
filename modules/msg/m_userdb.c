/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: m_userdb.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/dlink.h"
#include "libchaos/io.h"
#include "libchaos/ini.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/str.h"
#include "libchaos/hook.h"
#include "libchaos/timer.h"
#include "libchaos/sauth.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/user.h"
#include "ircd/msg.h"
#include "ircd/lclient.h"
#include "ircd/server.h"
#include "ircd/client.h"
#include "ircd/numeric.h"

#include "stralloc.h"

/* -------------------------------------------------------------------------- *
 * Constants                                                                  *
 * -------------------------------------------------------------------------- */

#define M_USERDB_HASH_SIZE 16

/* -------------------------------------------------------------------------- *
 * Types                                                                      *
 * -------------------------------------------------------------------------- */
struct m_userdb_s {
  struct node     node;
  struct lclient *lclient;
  struct timer   *timer;
  struct sauth   *sauth;
  int             done;
};

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_userdb_release(struct lclient *lcptr);
static void m_userdb(struct lclient* lcptr, struct client* cptr,
                     int             argc,  char         **argv);

static void m_userdb_done(struct sauth* saptr, struct m_userdb_s *arg);
/* -------------------------------------------------------------------------- *
 * Local variables                                                            *
 * -------------------------------------------------------------------------- */

static struct list   m_userdb_list;
static struct sheap  m_userdb_heap;

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_userdb_help[] = {
  "USERDB [VERIFY|REGISTER|SEARCH|MUTATE] <UID> [ARGUMENTS...]",
  "",
  "VERIFY <uid> <password>",
  "REGISTER <uid> <key=value pairs...>",
  "SEARCH < uid | key-value-pairs... >",
  "MUTATE <uid> <key-value-pairs...>",
  NULL
};

static struct msg m_userdb_msg = {
  "USERDB", 1, 0, MFLG_CLIENT,
  { NULL, m_userdb, m_userdb, m_userdb },
  m_userdb_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int
m_userdb_load(void) {
/*  if(hook_register(lclient_handshake, HOOK_DEFAULT, m_userdb_handshake) == NULL)*/
/*    return -1;*/

  hook_register(lclient_release, HOOK_DEFAULT, m_userdb_release);
/*  hook_register(lclient_register, HOOK_DEFAULT, m_userdb_register);*/

  mem_static_create(&m_userdb_heap, sizeof(struct m_userdb_s),
                    SAUTH_BLOCK_SIZE / 2);
  mem_static_note(&m_userdb_heap, "m_userdb client heap");

  dlink_list_zero(&m_userdb_list);

  msg_register(&m_userdb_msg);

  return 0;
}

void
m_userdb_unload(void) {
  struct m_userdb_s *arg = NULL;
  struct node     *node;
  struct node     *next;

  msg_unregister(&m_userdb_msg);

  /* Remove all pending udb stuff */
  dlink_foreach_safe_data(&m_userdb_list, node, next, arg)
    m_userdb_done(arg->sauth, arg);

 /* hook_unregister(lclient_register, HOOK_DEFAULT, m_userdb_register);*/
  hook_unregister(lclient_release, HOOK_DEFAULT, m_userdb_release);
 /* hook_unregister(lclient_handshake, HOOK_DEFAULT, m_userdb_handshake);*/

  mem_static_destroy(&m_userdb_heap);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static struct m_userdb_s*
m_userdb_new(struct lclient *lcptr) {
  struct m_userdb_s *arg;

	  /* Keep track of the lclient if the module gets unloaded */
	  arg = mem_static_alloc(&m_userdb_heap);

	  arg->lclient = lclient_pop(lcptr);

  dlink_add_tail(&m_userdb_list, &arg->node, arg);

  return arg;
}


/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void
m_userdb_release(struct lclient *lcptr) {

	struct m_userdb_s* udb;

	dlink_foreach(&m_userdb_list, udb) {
     if(udb->lclient == lcptr) {
       m_userdb_done(udb->sauth, udb);
       return;
		 }
	}
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
/*static int
m_userdb_register(struct lclient *lcptr) {
  struct m_userdb_s *udb;

  udb = lcptr->plugdata[LCLIENT_PLUGDATA_USERDB];

  if(udb)
  {
    m_userdb_done(udb);
    lcptr->plugdata[LCLIENT_PLUGDATA_USERDB] = NULL;
  }

  return 0;
}
*/

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void
m_userdb_done(struct sauth* saptr, struct m_userdb_s *arg) {
  struct node *nptr;

  /*if(arg->timer) { timer_cancel(&arg->timer);  }*/

	log(sauth_log, L_status, "userdb query (#%u) done: %s",
			arg->sauth->id, arg->sauth->userargs);

	lclient_send(arg->lclient, ":%S NOTICE %s :*** USERDB reply: %s %s",
			server_me, arg->lclient->name, arg->sauth->usercmd, arg->sauth->userargs);

  /*arg->sauth->reply*/

  if(arg->sauth) {
    sauth_delete(arg->sauth);
    arg->sauth = NULL;
  }

  dlink_delete(&m_userdb_list, &arg->node);

  mem_static_free(&m_userdb_heap, arg);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void
m_userdb_callback(struct sauth* saptr) {

}

/* -------------------------------------------------------------------------- *
 * Start an AUTH lookup for a local client                                    *
 * -------------------------------------------------------------------------- */
static struct m_userdb_s*
m_userdb_query(struct lclient *lcptr, const char* cmd, const char *args) {

  struct m_userdb_s* arg = m_userdb_new(lcptr);

  arg->sauth = sauth_userdb(cmd, args, m_userdb_done, arg);

  /* Report start of the dns lookup */
  if(arg->sauth)
  {
    lclient_send(arg->lclient,
                 ":%s NOTICE %s :(userdb) query: %s", 
                 lclient_me->name, arg->lclient->name, cmd);
  }
  else
  {
    /* Huh, dns failed, maybe servauth done, let's try the auth query */
    lclient_send(arg->lclient, ":%s NOTICE %s :(userdb) servauth down.",
                 lclient_me->name, arg->lclient->name);
    /*m_userdb_lookup_auth(arg);*/
  }
  return arg;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void
m_userdb(struct lclient* lcptr, struct client* cptr, int argc, char** argv) {

  if(!str_icmp(argv[2],"verify")) {
  } else if(!str_icmp(argv[2],"register"))  {
  } else if(!str_icmp(argv[2],"mutate"))  {
  } else if(!str_icmp(argv[2],"search"))  {
  } else {
    return;
  }

	if(client_is_user(cptr)) {
		if(!client_is_local(cptr))
			return;

	 if(!lclient_is_oper(lcptr)) {
				numeric_lsend(lcptr, ERR_NOPRIVILEGES, argv[1]);
       return;
	 }
	}


  stralloc args;
  stralloc_init(&args);

  int i;
  for(i = 3; i < argc; ++i) {
		if(args.len) stralloc_catb(&args, " ", 1);
    stralloc_catb(&args, argv[i], strlen(argv[i]));
  }

  stralloc_catb(&args, "\0", 1);

  struct m_userdb_s* s = 
		m_userdb_query(lcptr, argv[2], args.s);
}
