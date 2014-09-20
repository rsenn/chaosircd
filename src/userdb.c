/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 *
 * $Id: userdb.c,v 1.6 2006/09/28 08:38:31 roman Exp $
 */

#define _GNU_SOURCE

/* ------------------------------------------------------------------------ *
 * Library headers                                                          *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/child.h"
#include "libchaos/timer.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/net.h"
#include "libchaos/str.h"
#include "libchaos/io.h"

/* ------------------------------------------------------------------------ *
 * System headers                                                           *
 * ------------------------------------------------------------------------ */
#include "../config.h"

#include "ircd/ircd.h"
#include "ircd/userdb.h"
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct sheap      userdb_heap;
struct list       userdb_list;
uint32_t          userdb_serial;
struct timer     *userdb_timer;
struct timer     *userdb_rtimer;
struct child     *userdb_child;
int               userdb_log;
int               userdb_fds[2];
char              userdb_readbuf[BUFSIZE];
const char*       userdb_query_names[] = { "verify", "register", "search", "mutate", NULL };

/* ------------------------------------------------------------------------ */
int userdb_get_log() {
	return userdb_log;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
static int userdb_recover(void);
int userdb_launch(void);
static void userdb_callback(struct userdb *userdb, int type);
static void userdb_read(int fd, void *arg);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
static int userdb_timeout(struct userdb *userdb) {
	userdb_callback(userdb, USERDB_TIMEOUT);

	timer_cancel(&userdb->timer);

	return 0;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int userdb_query_reply(const char *reply) {
	uint32_t i;
/*
	for(i = 0; userdb_replies[i]; i++) {
		if(!str_icmp(userdb_replies[i], reply))
			return i;
	}*/

	return -1;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int userdb_proxy_type(const char *type) {
	uint32_t i;
/*
	for(i = 0; userdb_types[i]; i++) {
		if(!str_icmp(userdb_types[i], type))
			return i;
	}
*/
	return -1;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
static void userdb_child_cb(struct child *child) {
//  log(userdb_log, L_status, "userdb child callback %u", userdb_child->status);

	switch(userdb_child->status) {
		case CHILD_IDLE: {
			userdb_launch();
			break;
		}
		case CHILD_DEAD: {
			userdb_recover();
			break;
		}
		case CHILD_RUNNING: {
			break;
		}
	}
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
static void userdb_callback(struct userdb *userdb, int type) {
	const char *what;

//  userdb->status = type;

	if(userdb->status == USERDB_TIMEDOUT)
		what = "timed out";
	else if(userdb->status == USERDB_DONE)
		what = "done";
	else
		what = "failed";

	timer_cancel(&userdb->timer);

	switch(userdb->query) {
		case USERDB_QUERY_VERIFY: {
			log(userdb_log, L_verbose, "userdb verify (#%u) %s. %s", userdb->id,
					what, userdb->uid);

			break;
		}
		case USERDB_QUERY_REGISTER: {
			log(userdb_log, L_verbose, "userdb register (#%u) %s. %s",
					userdb->id, what, userdb->uid);

			break;
		}
		case USERDB_QUERY_SEARCH: {
			log(userdb_log, L_verbose, "userdb search (#%u) %s. %s", userdb->id,
					what, userdb->uid);

			break;
		}
		case USERDB_QUERY_MUTATE: {
			log(userdb_log, L_verbose, "userdb mutate (#%u) %s. %s", userdb->id,
					what, userdb->uid);

			break;
		}
	}

	if(userdb->callback)
		userdb->callback(userdb, userdb->args[0], userdb->args[1],
				userdb->args[2], userdb->args[3]);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
static int userdb_parse(char **argv) {
	int serial;
	struct userdb *userdb;

	if(!str_icmp(argv[0], "dns")) {
		serial = str_toi(argv[2]);

		userdb = userdb_find(serial);

		if(userdb) {
			if(userdb->query == USERDB_QUERY_VERIFY) {
				if(!str_icmp(argv[1], "verify")) {
					if(argv[3])
						userdb->result = atoi(argv[3]);
					else
						userdb->result = 0;
					userdb_callback(userdb, USERDB_DONE);
					return 0;
				}
			}
			if(userdb->query == USERDB_QUERY_REGISTER) {
				if(!str_icmp(argv[1], "register")) {
					if(argv[3])
						userdb->result = atoi(argv[3]);
					else
						userdb->result = 0;
					userdb_callback(userdb, USERDB_DONE);
					return 0;
				}
			}
			if(userdb->query == USERDB_QUERY_SEARCH) {
				if(!str_icmp(argv[1], "search")) {
					if(argv[3])
						userdb->result = atoi(argv[3]);
					else
						userdb->result = 0;
					userdb_callback(userdb, USERDB_DONE);
					return 0;
				}
			}
			if(userdb->query == USERDB_QUERY_MUTATE) {
				if(!str_icmp(argv[1], "mutate")) {
					if(argv[3])
						userdb->result = atoi(argv[3]);
					else
						userdb->result = 0;
					userdb_callback(userdb, USERDB_DONE);
					return 0;
				}
			}

			userdb_callback(userdb, USERDB_ERROR);

			return -1;
		}

		return 0;
	}

	return -1;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
static int userdb_recover(void) {
	struct node *node;
	struct node *next;

	dlink_foreach_safe(&userdb_list, node, next)
		userdb_callback((struct userdb *) node, USERDB_ERROR);

	if(userdb_rtimer) {
		timer_remove(userdb_rtimer);
		userdb_rtimer = NULL;
	}

	userdb_fds[0] = -1;
	userdb_fds[1] = -1;

	userdb_rtimer = timer_start(userdb_launch, USERDB_RELAUNCH);

	timer_note(userdb_rtimer, "relaunch for userdb child");

	userdb_child = NULL;

	return 0;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
static void userdb_read(int fd, void *ptr) {
	int len;
	char *argv[6];

	while(io_list[fd].recvq.lines) {
		if((len = io_gets(fd, userdb_readbuf, BUFSIZE)) == 0)
			break;

		str_tokenize(userdb_readbuf, argv, 5);

		if(userdb_parse(argv)) {
			log(userdb_log, L_warning, "Invalid reply from userdb!");

			io_shutup(fd);
		}
	}

	if(io_list[fd].status.eof || io_list[fd].status.err) {
		child_kill(userdb_child);
		child_cancel(userdb_child);
		child_push(&userdb_child);

		log(userdb_log, L_warning, "userdb channel closed!");

		userdb_recover();
	}
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int userdb_launch(void) {
	if(userdb_child == NULL) {
		userdb_child = child_find_name("-userdb");

		if(userdb_child == NULL) {
			log(userdb_log, L_warning, "No servauth child block found!");

			return 0;
		}

		child_set_callback(userdb_child, userdb_child_cb);

//    if(userdb_child->status != CHILD_RUNNING)
		{
			child_launch(userdb_child);

			if(userdb_child->channels[0][CHILD_PARENT][CHILD_READ] > -1) {
				userdb_fds[CHILD_READ] =
						userdb_child->channels[0][CHILD_PARENT][CHILD_READ];

#ifdef HAVE_SOCKETPAIR
				userdb_fds[CHILD_WRITE] = userdb_child->channels[0][CHILD_PARENT][CHILD_READ];
				io_queue_control(userdb_fds[CHILD_READ], ON, ON, ON);
#else
				userdb_fds[CHILD_WRITE] =
						userdb_child->channels[0][CHILD_PARENT][CHILD_WRITE];
				io_queue_control(userdb_fds[CHILD_READ], ON, OFF, ON);
				io_queue_control(userdb_fds[CHILD_WRITE], OFF, ON, ON);
#endif /* HAVE_SOCKETPAIR */

				io_register(userdb_fds[CHILD_READ], IO_CB_READ, userdb_read,
				NULL);
			}

			userdb_rtimer = NULL;

			return 1;
		}

		return 0;
	}

	userdb_rtimer = NULL;

	return 1;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
static struct userdb *userdb_new(int type) {
	struct userdb *userdb;

	userdb = mem_static_alloc(&userdb_heap);

	userdb->id = userdb_serial++;
	userdb->query = type;
	userdb->uid[0] = '\0';
	userdb->params = NULL;
//  //userdb->status = 0;
	userdb->result = 0;
	userdb->timer = NULL;
	userdb->refcount = 1;

	dlink_add_head(&userdb_list, &userdb->node, userdb);

	return userdb;
}

/* ------------------------------------------------------------------------ *
 * Initialize userdb heap and add garbage collect timer.                     *
 * ------------------------------------------------------------------------ */
void userdb_init(void) {
	userdb_log = log_source_register("userdb");

	/* Zero userdb block list */
	dlink_list_zero(&userdb_list);

	userdb_serial = 0;
	userdb_fds[0] = -1;
	userdb_fds[1] = -1;

	/* Setup userdb heap & timer */
	mem_static_create(&userdb_heap, sizeof(userdb_t), USERDB_BLOCK_SIZE);
	mem_static_note(&userdb_heap, "userdb query heap");

	userdb_rtimer = timer_start(userdb_launch, USERDB_RELAUNCH);

	timer_note(userdb_rtimer, "servauth relaunch timer");
}

/* ------------------------------------------------------------------------ *
 * Destroy userdb heap and cancel timer.                                     *
 * ------------------------------------------------------------------------ */
void userdb_shutdown(void) {
	struct node *node;
	struct node *next;

	timer_cancel(&userdb_timer);

	/* Remove all userdb blocks */
	dlink_foreach_safe(&userdb_list, node, next)
		userdb_delete((struct userdb *) node);

	mem_static_destroy(&userdb_heap);

	log_source_unregister(userdb_log);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void userdb_collect(void) {
	struct node *node;
	struct userdb *udptr;

	dlink_foreach(&userdb_list, node)
	{
		udptr = node->data;

		if(!udptr->refcount)
			userdb_delete(udptr);
	}

	mem_static_collect(&userdb_heap);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct userdb *
userdb_query_register(const char* uid, const char* values, void *callback, ...) {
	struct userdb *userdb;
	va_list args;
	char ipbuf[HOSTIPLEN + 1];

	if(!userdb_launch())
		return NULL;

	userdb = userdb_new(USERDB_QUERY_REGISTER);

	userdb->callback = callback;
	strlcpy(userdb->uid, uid, sizeof(userdb->uid));

	va_start(args, callback);
	userdb_vset_args(userdb, args);
	va_end(args);

	userdb->params = str_dup(values ? values : "");

	io_puts(userdb_fds[CHILD_WRITE], "userdb register %s %s", userdb->uid,
			userdb->params);

	userdb->timer = timer_start(userdb_timeout, USERDB_TIMEOUT, userdb);

	timer_note(userdb->timer, "userdb register %s timeout", userdb->uid);

	log(userdb_log, L_verbose, "Started register query for %s (%s)",
			userdb->uid, userdb->params);

	return userdb;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct userdb *
userdb_vquery(int type, const char* uid, const char* params, void* callback,
		va_list args) {
	struct userdb *userdb;

	if(!userdb_launch())
		return NULL;

	userdb = userdb_new(type);

	strlcpy(userdb->uid, uid, sizeof(uid));
	userdb->callback = callback;
	userdb->params = str_dup(params);
	userdb_vset_args(userdb, args);

	io_puts(userdb_fds[CHILD_WRITE], "userdb %s %s %s",
			userdb_query_names[userdb->query], userdb->uid, userdb->params);

	userdb->timer = timer_start(userdb_timeout, USERDB_TIMEOUT, userdb);

	timer_note(userdb->timer, "userdb timeout (%s %s %s)",
			userdb_query_names[userdb->query], userdb->uid, userdb->params);

	return userdb;
}

struct userdb *
userdb_query(int type, const char* uid, const char* params, void* callback, ...) {
	va_list args;
	va_start(args, callback);
	struct userdb* ret = userdb_vquery(type, uid, params, callback, args);
	va_end(args);
	return ret;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct userdb *
userdb_query_search(const char* values, void *callback, ...) {
	va_list args;
	va_start(args, callback);
	struct userdb* ret = userdb_vquery(USERDB_QUERY_SEARCH, values, "",
			callback, args);
	va_end(args);
	return ret;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct userdb *
userdb_query_mutate(const char* uid, const char* values, void *callback, ...) {
	va_list args;
	va_start(args, callback);
	struct userdb* ret = userdb_vquery(USERDB_QUERY_MUTATE, uid, values,
			callback, args);
	va_end(args);
	return ret;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct userdb *
userdb_query_verify(const char* uid, const char* password, void *callback, ...) {
	va_list args;
	va_start(args, callback);
	struct userdb* ret = userdb_vquery(USERDB_QUERY_VERIFY, uid, password,
			callback, args);
	va_end(args);
	return ret;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct userdb *userdb_find(uint32_t id) {
	struct userdb *userdb;
	struct node *node;

	dlink_foreach(&userdb_list, node)
	{
		userdb = node->data;

		if(userdb->id == id)
			return userdb;
	}

	return NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct userdb *userdb_pop(struct userdb *userdb) {
	if(userdb) {
		if(!userdb->refcount)
			log(userdb_log, L_warning, "Poping deprecated userdb #%u",
					userdb->id);

		userdb->refcount++;
	}

	return userdb;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct userdb *userdb_push(struct userdb **userdb) {
	if(*userdb) {
		if((*userdb)->refcount == 0) {
			log(userdb_log, L_warning, "Trying to push deprecated userdb #%u",
					(*userdb)->id);
		} else {
			if(--(*userdb)->refcount == 0)
				userdb_delete(*userdb);

			(*userdb) = NULL;
		}
	}

	return *userdb;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void userdb_delete(struct userdb *userdb) {
	timer_cancel(&userdb->timer);

	dlink_delete(&userdb_list, &userdb->node);

	mem_static_free(&userdb_heap, userdb);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void userdb_vset_args(struct userdb *userdb, va_list args) {
	userdb->args[0] = va_arg(args, void *);
	userdb->args[1] = va_arg(args, void *);
	userdb->args[2] = va_arg(args, void *);
	userdb->args[3] = va_arg(args, void *);
}

void userdb_set_args(struct userdb *userdb, ...) {
	va_list args;

	va_start(args, userdb);
	userdb_vset_args(userdb, args);
	va_end(args);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void userdb_dump(struct userdb *udptr) {
	if(udptr == NULL) {
		dump(userdb_log, "[================ userdb summary ================]");

		dump(userdb_log, "------------------- verify -------------------");

		dlink_foreach(&userdb_list, udptr)
		{
			if(udptr->query == USERDB_QUERY_VERIFY)
				dump(userdb_log, " #%u: [%u] %-20s (%p)", udptr->id,
						udptr->refcount, udptr->uid, udptr->callback);
		}

		dump(userdb_log, "------------------ register ------------------");

		dlink_foreach(&userdb_list, udptr)
		{
			if(udptr->query == USERDB_QUERY_REGISTER)
				dump(userdb_log, " #%u: [%u] %-20s (%p)", udptr->id,
						udptr->refcount, udptr->uid, udptr->callback);
		}

		dump(userdb_log, "---------------------- search ---------------------");

		dlink_foreach(&userdb_list, udptr)
		{
			if(udptr->query == USERDB_QUERY_SEARCH)
				dump(userdb_log, " #%u: [%u] %-20s (%p)", udptr->id,
						udptr->refcount, udptr->uid, udptr->callback);
		}

		dump(userdb_log, "---------------------- mutate ---------------------");

		dlink_foreach(&userdb_list, udptr)
		{
			if(udptr->query == USERDB_QUERY_MUTATE)
				dump(userdb_log, " #%u: [%u] %-20s (%p)", udptr->id,
						udptr->refcount, udptr->uid, udptr->callback);
		}

		dump(userdb_log, "[============= end of userdb summary ============]");
	} else {
		dump(userdb_log, "[================= userdb dump ==================]");

		dump(userdb_log, "         id: #%u", udptr->id);
		dump(userdb_log, "   refcount: %u", udptr->refcount);
		dump(userdb_log, "        uid: %s", udptr->uid);
		dump(userdb_log, "       type: %s",
				udptr->query == USERDB_QUERY_VERIFY ? "verify" : udptr->query == USERDB_QUERY_REGISTER ? "register" : udptr->query == USERDB_QUERY_SEARCH ? "search" : "mutate");
		dump(userdb_log, "     status: %s",
				udptr->status == USERDB_DONE ? "done" : udptr->status == USERDB_ERROR ? "error" : "timed out");
		dump(userdb_log, "      timer: %i",
				udptr->timer ? udptr->timer->id : -1);
		dump(userdb_log, "       args: %p, %p, %p, %p", udptr->args[0],
				udptr->args[1], udptr->args[2], udptr->args[3]);
		dump(userdb_log, "   callback: %p", udptr->callback);

		dump(userdb_log, "[============== end of userdb dump ==============]");
	}
}
