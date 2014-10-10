/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: lclient.c,v 1.5 2006/09/28 09:44:11 roman Exp $
 */

#define _GNU_SOURCE

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/connect.h"
#include "libchaos/listen.h"
#include "libchaos/dlink.h"
#include "libchaos/timer.h"
#include "libchaos/sauth.h"
#include "libchaos/hook.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/str.h"
#include "libchaos/io.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/msg.h"
#include "ircd/ircd.h"
#include "ircd/conf.h"
#include "ircd/user.h"
#include "ircd/chars.h"
#include "ircd/class.h"
#include "ircd/client.h"
#include "ircd/server.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/chanmode.h"
#include "ircd/usermode.h"

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
int             lclient_log;                /* lclient log source */
struct sheap    lclient_heap;               /* heap for struct lclient */
struct timer   *lclient_timer;              /* timer for heap gc */
struct lclient *lclient_me;                 /* my local client info */
struct lclient *lclient_uplink;             /* the uplink if we're a leaf */
int             lclient_dirty;              /* we need a garbage collect */
uint32_t        lclient_id;
uint32_t        lclient_serial;
uint32_t        lclient_max;
struct list     lclient_list;               /* list with all of them */
struct list     lclient_lists[4];           /* unreg, clients, servers, opers */
uint64_t        lclient_seed;
char            lclient_recvbuf[IRCD_BUFSIZE];
unsigned long   lclient_recvb[2];
unsigned long   lclient_sendb[2];
unsigned long   lclient_recvm[2];
unsigned long   lclient_sendm[2];
char           *lclient_types[] = {
  "unregistered",
  "client",
  "server",
  "oper"
};

/* ------------------------------------------------------------------------ */
int lclient_get_log() { return lclient_log; }

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void lclient_shift(int64_t delta)
{
  struct lclient *lcptr = NULL;
  struct node    *nptr;

  dlink_foreach_data(&lclient_list, nptr, lcptr)
  {
    if(lcptr->ping)
      lcptr->ping += delta;
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static uint32_t lclient_count(net_addr_t ip)
{
  struct lclient *lcptr = NULL;
  struct node    *nptr;
  uint32_t        ret = 0;

  dlink_foreach_data(&lclient_lists[LCLIENT_UNKNOWN], nptr, lcptr)
  {
    if(ip == lcptr->addr_remote)
      ret++;
  }

  dlink_foreach_data(&lclient_lists[LCLIENT_USER], nptr, lcptr)
  {
    if(ip == lcptr->addr_remote)
      ret++;
  }

  return ret;
}

/* -------------------------------------------------------------------------- *
 * Initialize lclient module                                                  *
 * -------------------------------------------------------------------------- */
void lclient_init(void)
{
  lclient_log = log_source_register("lclient");

  /* Zero all lclient lists */
  dlink_list_zero(&lclient_list);
  dlink_list_zero(&lclient_lists[LCLIENT_UNKNOWN]);
  dlink_list_zero(&lclient_lists[LCLIENT_USER]);
  dlink_list_zero(&lclient_lists[LCLIENT_SERVER]);
  dlink_list_zero(&lclient_lists[LCLIENT_OPER]);

  /* Setup lclient heap & timer */
  mem_static_create(&lclient_heap, sizeof(struct lclient), LCLIENT_BLOCK_SIZE);
  mem_static_note(&lclient_heap, "lclient heap");

  /* Register irc protocol handlers */
  net_register(NET_SERVER, "irc", lclient_accept);
  net_register(NET_CLIENT, "irc", lclient_connect);

  lclient_uplink = NULL;
  lclient_dirty = 0;
  lclient_serial = 0;

  /* Add myself as local client */
  lclient_id = 0;
  lclient_max = 0;
  lclient_me = lclient_new(-1, NET_ADDR_ANY, 0);

  lclient_recvb[0] = 0;
  lclient_recvb[1] = 0;
  lclient_sendb[0] = 0;
  lclient_sendb[1] = 0;
  lclient_recvm[0] = 0;
  lclient_recvm[1] = 0;
  lclient_sendm[0] = 0;
  lclient_sendm[1] = 0;

  timer_shift_register(lclient_shift);

  log(lclient_log, L_status, "Initialised [lclient] module.");
}

/* -------------------------------------------------------------------------- *
 * Shutdown lclient module                                                    *
 * -------------------------------------------------------------------------- */
void lclient_shutdown(void)
{
  struct lclient *lcptr;
  struct lclient *next;

  log(lclient_log, L_status, "Shutting down [lclient] module.");

  timer_shift_unregister(lclient_shift);

  /* Push all lclients */
  dlink_foreach_safe(&lclient_list, lcptr, next)
  {
    if(lcptr->refcount)
      lcptr->refcount--;

    lclient_delete(lcptr);
  }

  /* Destroy static heap */
  mem_static_destroy(&lclient_heap);

  /* Unregister log source */
  log_source_unregister(lclient_log);
}

/* -------------------------------------------------------------------------- *
 * Garbage collect lclient blocks                                             *
 * -------------------------------------------------------------------------- */
void lclient_collect(void)
{
  mem_static_collect(&lclient_heap);
}

/* -------------------------------------------------------------------------- *
 * Create a new lclient block                                                 *
 * -------------------------------------------------------------------------- */
struct lclient *lclient_new(int fd, net_addr_t addr, net_port_t port)
{
  struct lclient *lcptr = NULL;

  /* Allocate and zero lclient block */
  if(!(lcptr = mem_static_alloc(&lclient_heap)))
    return NULL;

  memset(lcptr, 0, sizeof(struct lclient));

  /* Link it to the main list and to the appropriate typed list */
  dlink_add_tail(&lclient_list, &lcptr->node, lcptr);
  dlink_add_tail(&lclient_lists[lcptr->type], &lcptr->tnode, lcptr);

  /* Initialise the block */
  lcptr->refcount = 1;
  lcptr->type = LCLIENT_UNKNOWN;
  lcptr->fds[0] = fd;
  lcptr->fds[1] = fd;
  lcptr->ctrlfds[0] = -1;
  lcptr->ctrlfds[1] = -1;
  lcptr->id = lclient_id++;
  lcptr->serial = lclient_serial;
  lcptr->user = NULL;
  lcptr->client = NULL;
  lcptr->server = NULL;
  lcptr->shut = 0;
  lcptr->silent = 0;
  lcptr->name[0] = '\0';
  lcptr->ping = 0LLU;
  lcptr->plugdata[LCLIENT_PLUGDATA_COOKIE] = NULL;
  lcptr->addr_remote = addr;
  lcptr->port_remote = port;

  if(fd >= 0 && fd < IO_MAX_FDS)
  {
    /* Convert remote address to string for
       temporary client name (until he sends NICK cmd) */
    net_ntoa_r(lcptr->addr_remote, lcptr->hostip);
    strcpy(lcptr->host, lcptr->hostip);

    /* Get the local address. */
    net_getsockname(fd, &lcptr->addr_local, &lcptr->port_local);
  }

  if(lclient_list.size > lclient_max)
    lclient_max = lclient_list.size;

  return lcptr;
}

/* -------------------------------------------------------------------------- *
 * Delete a lclient block                                                     *
 * -------------------------------------------------------------------------- */
void lclient_delete(struct lclient *lcptr)
{
  lclient_release(lcptr);

  /* Unlink from main list and typed list */
  dlink_delete(&lclient_list, &lcptr->node);
  dlink_delete(&lclient_lists[lcptr->type], &lcptr->tnode);

  debug(lclient_log, "Deleted lclient block: %s:%u",
        net_ntoa(lcptr->addr_remote), lcptr->port_remote);

  /* Free the block */
  mem_static_free(&lclient_heap, lcptr);
}

/* -------------------------------------------------------------------------- *
 * Loose all references of an lclient block                                   *
 * -------------------------------------------------------------------------- */
void lclient_release(struct lclient *lcptr)
{
  hooks_call(lclient_release, HOOK_DEFAULT, lcptr);

  if(io_valid(lcptr->fds[1]))
  {
    io_unregister(lcptr->fds[1], IO_CB_READ);
    io_unregister(lcptr->fds[1], IO_CB_WRITE);
    io_close(lcptr->fds[1]);
  }

  if(io_valid(lcptr->fds[0]) && lcptr->fds[1] != lcptr->fds[0])
  {
    io_unregister(lcptr->fds[0], IO_CB_READ);
    io_unregister(lcptr->fds[0], IO_CB_WRITE);
    io_close(lcptr->fds[0]);
  }

  lcptr->fds[0] = -1;
  lcptr->fds[1] = -1;

  class_push(&lcptr->class);

  if(lcptr->user)
  {
    user_delete(lcptr->user);
    lcptr->user = NULL;
  }

  if(lcptr->client)
  {
    lcptr->client->lclient = NULL;
    lcptr->client = NULL;
  }

  if(lcptr->server)
  {
    lcptr->server->lclient = NULL;
    lcptr->server = NULL;
  }

  lcptr->listen = NULL;
  lcptr->connect = NULL;
/*  listen_push(&lcptr->listen);
  connect_push(&lcptr->connect);*/
  timer_cancel(&lcptr->ptimer);

  lcptr->refcount = 0;
  lcptr->shut = 1;
  lclient_dirty++;
}

/* -------------------------------------------------------------------------- *
 * Get a reference to an lclient block                                        *
 * -------------------------------------------------------------------------- */
struct lclient *lclient_pop(struct lclient *lcptr)
{
  if(lcptr)
  {
    if(!lcptr->refcount)
      debug(lclient_log, "Poping deprecated lclient %s:%u",
            net_ntoa(lcptr->addr_remote), lcptr->port_remote);

    lcptr->refcount++;
  }

  return lcptr;
}

/* -------------------------------------------------------------------------- *
 * Push back a reference to an lclient block                                  *
 * -------------------------------------------------------------------------- */
struct lclient *lclient_push(struct lclient **lcptrptr)
{
  if(*lcptrptr)
  {
    if((*lcptrptr)->refcount)
    {
      if(--(*lcptrptr)->refcount == 0)
        lclient_release(*lcptrptr);
    }

    (*lcptrptr) = NULL;
  }

  return *lcptrptr;
}

/* -------------------------------------------------------------------------- *
 * Set the type of an lclient and move it to the appropriate list             *
 * -------------------------------------------------------------------------- */
void lclient_set_type(struct lclient *lcptr, uint32_t type)
{
  uint32_t newtype = (type & 0x03);

  /* If the type changes, then move to another list */
  if(newtype != lcptr->type)
  {
    /* Delete from old list */
    dlink_delete(&lclient_lists[lcptr->type & 0x03], &lcptr->tnode);

    /* Move to new list */
    dlink_add_tail(&lclient_lists[newtype], &lcptr->tnode, lcptr);

    /* Set type */
    lcptr->type = newtype;
  }
}

/* -------------------------------------------------------------------------- *
 * Set the name of an lclient block                                           *
 * -------------------------------------------------------------------------- */
void lclient_set_name(struct lclient *lcptr, const char *name)
{
  /* Update lclient->name/hash */
  strlcpy(lcptr->name, name, sizeof(lcptr->name));

  lcptr->hash = str_ihash(lcptr->name);

  debug(lclient_log, "Set name for %s:%u to %s",
        net_ntoa(lcptr->addr_remote), lcptr->port_remote, lcptr->name);

  io_note(lcptr->fds[0], "local client: %s", lcptr->name);
}

/* -------------------------------------------------------------------------- *
 * Accept a local client                                                      *
 *                                                                            *
 * <fd>                     - filedescriptor of the new connection            *
 *                            (may be invalid?)                               *
 * <listen>                 - the corresponding listen{} block                *
 * -------------------------------------------------------------------------- */
void lclient_accept(int fd, struct listen *listen)
{
  struct lclient     *lcptr;
  struct conf_listen *lconf;
  struct class       *clptr;

  /* We're accepting a connection */
  if(listen->status == LISTEN_CONNECTION)
  {
    /* We must have a valid fd */
    if(!io_valid(fd))
    {
      log(lclient_log, L_warning,
          "Invalid filedescriptor while accepting local client");

      return;
    }

    /* Get class reference */
    lconf = listen->args;

    clptr = class_find_name(lconf->class);

    /* Whine if its an invalid class */
    if(clptr == NULL)
    {
      log(lclient_log, L_warning,
          "Invalid class in listener %s: %s", listen->name, lconf->class);
      io_close(fd);
      return;
    }

    if(((int)clptr->refcount) - ((int)listen_list.size) -
       ((int)connect_list.size) > (int)clptr->max_clients)
    {
      log(lclient_log, L_warning, "Too many connects in class '%s'.",
          clptr->name);
      io_close(fd);
      return;
    }

    if(lclient_count(listen->addr_remote) >= clptr->clients_per_ip)
    {
      log(lclient_log, L_warning, "Too many connects from %s.",
          net_ntoa(listen->addr_remote));
      io_close(fd);
      return;
    }

    /* Add a local client to LCLIENT_UNKNOWN list */
    if((lcptr = lclient_new(fd, listen->addr_remote, listen->port_remote)) == NULL)
    {
      log(lclient_log, L_warning,
          "Could not allocate new lclient struct for listen %s",
          listen->name);

      io_close(fd);
      return;
    }

    /* Inform about the new lclient */
    log(lclient_log, L_verbose, "New local client %s:%u on listener %s (class %s)",
        lcptr->host, lcptr->port_remote, listen->name, clptr->name);

    /* Get references */
    lcptr->listen = listen_pop(listen);
    lcptr->class = class_pop(clptr);

    /* Setup ping timeout callback */
    lcptr->ptimer = timer_start(lclient_exit, clptr->ping_freq, lcptr,
                                "timeout: %llumsecs", clptr->ping_freq);
    
    timer_note(lcptr->ptimer, "ping timer for %s:%u",
               net_ntoa(lcptr->addr_remote), lcptr->port_remote);

    /* Set queue behaviour */
    io_queue_control(fd, ON, ON, OFF);

    /* Register it for the readable callback */
    io_register(fd, IO_CB_READ, lclient_recv, lcptr);

    io_note(lcptr->fds[0], "unknown client from %s:%u",
            net_ntoa(lcptr->addr_remote),
            (uint32_t)lcptr->port_remote);
  }
  /* We failed accepting the connection */
  else
  {
    /* Warn about the pity */
    log(lclient_log, L_warning, "Error accepting local client: %s",
        syscall_strerror(io_list[fd].error));

    /* We had a valid fd, shut it! */
    io_push(&fd);
  }
}

/* -------------------------------------------------------------------------- *
 * Connected to a server                                                      *
 *                                                                            *
 * <fd>                     - filedescriptor of the new connection            *
 *                            (may be invalid?)                               *
 * <connect>                - the corresponding connect{} block               *
 * -------------------------------------------------------------------------- */
void lclient_connect(int fd, struct connect *connect)
{
  struct conf_connect *cconf;
  struct lclient      *lcptr;
  struct class        *clptr;

  if(connect->status == CONNECT_DONE)
  {
    /* We must have a valid fd */
    if(!io_valid(fd))
    {
      log(lclient_log, L_warning,
          "Invalid filedescriptor while connecting to local client");

      return;
    }

    cconf = connect->args;

    clptr = class_find_name(cconf->class);

    /* Whine if its an invalid class */
    if(clptr == NULL)
    {
      log(lclient_log, L_warning,
          "Invalid class in connect %s: %s", connect->name, cconf->class);

      io_shutup(fd);
      return;
    }

    /* Allocate new lclient struct */
    if((lcptr = lclient_new(fd, connect->addr_remote, connect->port_remote)) == NULL)
    {
      log(lclient_log, L_warning,
          "Could not allocate new lclient struct for connect %s",
          connect->name);

      io_push(&fd);
      return;
    }

    lcptr->connect = connect_pop(connect);
    lcptr->class = class_pop(clptr);
    lcptr->addr_local = connect->addr_local;
    lcptr->port_local = connect->port_local;

    /* Set queue behaviour */
    io_queue_control(fd, ON, ON, OFF);

    /* Register it for the readable callback */
    io_register(fd, IO_CB_WRITE, NULL, lcptr);
    io_register(fd, IO_CB_READ, lclient_recv, lcptr);

    io_note(lcptr->fds[0], "unregistered server %s from %s:%u",
            connect->name,
            net_ntoa(lcptr->addr_remote),
            (uint32_t)lcptr->port_remote);

    /* Send the handshake stuff */
    server_send_pass(lcptr);
    server_send_capabs(lcptr);
    server_send_server(lcptr);

    /* Setup ping timeout callback */
    lcptr->ptimer = timer_start(lclient_exit, clptr->ping_freq, lcptr);

    timer_note(lcptr->ptimer, "ping timer for %s:%u",
               net_ntoa(lcptr->addr_remote), lcptr->port_remote);
  }
  /* We failed accepting the connection */
  else
  {
    /* Warn about the pity */
/*    log(lclient_log, L_warning, "Error connecting to %s: %s",
        connect->name,
        syscall_strerror(io_list[fd].error));*/

    /* We had a valid fd, shut it! */
    io_push(&fd);
  }
}

/* -------------------------------------------------------------------------- *
 * Read data from a local connection and process it                           *
 * -------------------------------------------------------------------------- */
void lclient_recv(int fd, struct lclient *lcptr)
{
  if(io_list[fd].status.dead)
    return;

  /* Check if the socket was closed */
  if(io_list[fd].status.closed || io_list[fd].status.err)
  {
    if(io_list[fd].error <= 0)
    {
      lclient_exit(lcptr, "connection closed");
    }
    else if(io_list[fd].error > 0 && io_list[fd].error < 125)
    {
      lclient_exit(lcptr, "%s", syscall_strerror(io_list[fd].error));
    }
    else if(io_list[fd].error == 666)
    {
      lclient_exit(lcptr, "%s", ssl_strerror(fd));
    }
    else
    {
      lclient_exit(lcptr, "unknown error: %i", io_list[fd].error);
    }

    return;
  }

  /* Check if we exceeded receive queue size */
  if(io_list[fd].recvq.size > lcptr->class->recvq)
  {
    lclient_exit(lcptr, "recvq exceeded (%u bytes)",
                 io_list[fd].recvq.size);
    return;
  }

  /* Flood hook */
  hooks_call(lclient_recv, HOOK_DEFAULT, fd, lcptr);

  /* Get lines from the queue as long as there are */
  while(io_list[fd].recvq.lines && !lcptr->shut)
    lclient_process(fd, lcptr);
}

/* -------------------------------------------------------------------------- *
 * Read a line from queue and process it                                      *
 * -------------------------------------------------------------------------- */
void lclient_process(int fd, struct lclient *lcptr)
{
  int n = 0;

#ifdef DEBUG
  char *p;
#endif
  n = io_gets(fd, lclient_recvbuf, IRCD_BUFSIZE);

  lclient_update_recvb(lcptr, n);
#ifdef DEBUG
  p = strchr(lclient_recvbuf, '\r');
  if(p)
  {
    *p = '\0';
  }
  else
  {
    p = strchr(lclient_recvbuf, '\n');
    if(p) *p = '\0';
  }
  debug(ircd_log_in, "From %s: %s", lcptr->name, lclient_recvbuf);
#endif /* DEBUG */
  lclient_parse(lcptr, lclient_recvbuf, n);
}

/* -------------------------------------------------------------------------- *
 * Parse the prefix and the command                                           *
 * -------------------------------------------------------------------------- */
void lclient_parse(struct lclient *lcptr, char *s, size_t n)
{
  char *argv[256];

  if(hooks_call(lclient_parse, HOOK_DEFAULT, lcptr, s))
    return;

  /* is the message prefixed? */
  if(s[0] == ':')
  {
    /* yes, get prefix */
    argv[0] = &s[1];

    /* now loop until we read whitespace */
    while(*s && !chars_isspace(*s)) s++;

    /* null-terminate prefix */
    *s++ = '\0';
  }
  else
  {
    /* no prefix received */
    argv[0] = NULL;
  }

  /* skip whitespace */
  while(*s && chars_isspace(*s)) s++;

  /* got the command */
  argv[1] = s;

  /* ugh, we're at string end :( */
  if(*s == '\0')
    return;

  /* skip the command */
  while(*s && !chars_isspace(*s)) s++;

  /* null-terminate the command */
  *s++ = '\0';

  /* skip whitespace */
  while(*s && chars_isspace(*s)) s++;

  /* process the command */
  lclient_message(lcptr, argv, s, n);
}

/* -------------------------------------------------------------------------- *
 * Decide whether a message is numeric or not and call the appropriate        *
 * message handler.                                                           *
 * -------------------------------------------------------------------------- */
void lclient_message(struct lclient *client, char **argv, char *arg, size_t n)
{
  size_t i;
  int isnum = 1;

  /* loop through argv[1] char by char */
  for(i = 0; argv[1][i]; i++)
  {
    /* if we find a non-digit char, abort and set isnum = 0 */
    if(!chars_isdigit(argv[1][i]))
    {
      isnum = 0;
      break;
    }
  }

  /* call numeric/command handler */
  if(isnum)
    lclient_numeric(client, argv, arg);
  else
    lclient_command(client, argv, arg, n);
}

/* -------------------------------------------------------------------------- *
 * Process a numeric message                                                  *
 * -------------------------------------------------------------------------- */
void lclient_numeric(struct lclient *lcptr, char **argv, char *arg)
{
  struct client *acptr;
  char           dest[IRCD_NICKLEN + 1];
  size_t         i;

  if(!lclient_is_server(lcptr))
    return;

  for(i = 0; *arg; i++)
  {
    if(!chars_isnickchar(*arg) && !chars_isuidchar(*arg))
      break;

    dest[i] = *arg++;
  }

  dest[i] = '\0';

  while(*arg && (*arg == ' ' || *arg == '\t'))
    *arg++ = '\0';

  if(lcptr->caps & CAP_UID)
    acptr = client_find_uid(dest);
  else
    acptr = client_find_nick(dest);

  if(acptr == NULL)
  {
    log(lclient_log, L_warning,
        "Dropping numeric %s with unknown destination %s",
        argv[1], dest);
    return;
  }

  for(i = 0; arg[i]; i++)
  {
    if(arg[i] == '\r' || arg[i] == '\n')
    {
      arg[i] = '\0';
      break;
    }
  }

  lclient_send(acptr->source, ":%s %s %s %s", argv[0], argv[1],
               (acptr->source->caps & CAP_UID ? acptr->user->uid : acptr->name),
               arg);
}

/* -------------------------------------------------------------------------- *
 * Process a command                                                          *
 * -------------------------------------------------------------------------- */
void lclient_command(struct lclient *lcptr, char **argv, char *arg, size_t n)
{
  struct msg    *msg;
  size_t         ac;
  struct client *cptr;

  /* find message structure for the command */
  msg = msg_find(argv[1]);

  /* did not find the command */
  if(msg == NULL)
  {
    /* send error message & return */
    if(lclient_is_unknown(lcptr))
      lclient_exit(lcptr, "protocol mismatch: %s", argv[1]);
    else if(lclient_is_server(lcptr))
      lclient_exit(lcptr, "unknown command: %s", argv[1]);
    else
      numeric_lsend(lcptr, ERR_UNKNOWNCOMMAND, argv[1]);

    return;
  }

  if(!(lclient_is_unknown(lcptr) && (msg->flags & MFLG_UNREG)))
  {
    if((msg->flags & MFLG_SERVER) && !lclient_is_server(lcptr))
    {
      if(!lclient_is_unknown(lcptr))
        numeric_lsend(lcptr, ERR_SERVERCOMMAND, msg->cmd);
      return;
    }

    if((msg->flags & MFLG_OPER) &&
       !lclient_is_oper(lcptr) && !lclient_is_server(lcptr))
    {
      if(!lclient_is_unknown(lcptr))
      numeric_lsend(lcptr, ERR_NOPRIVILEGES, msg->cmd);
      return;
    }
  }

  if(msg->handlers[lcptr->type] == NULL)
  {
    /* send error message & return */
    if(lclient_is_unknown(lcptr))
      lclient_exit(lcptr, "protocol mismatch");
    else if(lclient_is_server(lcptr))
      lclient_exit(lcptr, "unknown command: %s", argv[1]);
    else
      numeric_lsend(lcptr, ERR_UNKNOWNCOMMAND, argv[1]);

    return;
  }

  argv[1] = msg->cmd;

  /* tokenize remaining line appropriate to message struct */
  ac = str_tokenize(arg, &argv[2], msg->maxargs ? msg->maxargs : 253);

  /* check required args */
  if(msg->args && ac < msg->args)
  {
    if(lclient_is_unknown(lcptr))
      lclient_exit(lcptr, "protocol mismatch: need %u args, got %u",
                   msg->args, ac);
    else if(!lclient_is_server(lcptr))
      numeric_lsend(lcptr, ERR_NEEDMOREPARAMS, msg->cmd);
    else
      lclient_exit(lcptr, "need %u args for %s, got %u.",
                   msg->args, msg->cmd, ac);
    return;
  }

  /* catch commands without MFLG_UNREG on unknown clients */
  if(lcptr->type == LCLIENT_UNKNOWN)
  {
    if(!(msg->flags & MFLG_UNREG))
    {
      lclient_exit(lcptr, "invalid command for unregistered user: %s",
                   argv[1]);
      return;
    }
  }

  /* now parse the prefix */
  if(lcptr->type == LCLIENT_SERVER)
  {
    if(argv[0])
      cptr = lclient_prefix(lcptr, argv[0]);
    else
      cptr = lcptr->client;

    if(cptr == NULL)
    {
      log(lclient_log, L_warning, "Invalid prefix from %s: %s (dropped)",
          lcptr->name, argv[0]);

      return;
    }

    if(cptr->source != lcptr)
    {
      log(lclient_log, L_warning,
          "Message (%s %s) for %s from wrong direction: %s, should come from %s.",
          argv[0], argv[1], cptr->name, lcptr->name, cptr->source->name);
      return;
    }
  }
  else
  {
    cptr = lcptr->client;

    if(argv[0])
    {
      log(lclient_log, L_warning, "Message with prefix from non-server");
      argv[0] = NULL;
    }
  }

  /* call the message handler */
  msg->counts[lcptr->type]++;
  msg->bytes += n;
  msg->handlers[lcptr->type](lcptr, cptr, ac + 2, argv);
}

/* -------------------------------------------------------------------------- *
 * Parse the prefix and find the appropriate client                           *
 * -------------------------------------------------------------------------- */
struct client *lclient_prefix(struct lclient *lcptr, const char *pfx)
{
  struct server *sptr;

  if(lcptr->caps & CAP_UID)
  {
    struct user *uptr;

    if((uptr = user_find_uid(pfx)))
      return uptr->client;
  }

  if((sptr = server_find_name(pfx)))
    return sptr->client;

  return client_find_nick(pfx);
}

/* -------------------------------------------------------------------------- *
 * Exit a local client and leave him an error message if he has registered.   *
 * -------------------------------------------------------------------------- */
void lclient_vexit(struct lclient *lcptr, char *format, va_list args)
{
  /* Format the exit message */
  char buf[IRCD_TOPICLEN + 1];

  str_vsnprintf(buf, sizeof(buf), format, args);

  hooks_call(lclient_exit, HOOK_DEFAULT, lcptr, format, args);

  /* Leave a log notice */
  log(lclient_log, L_verbose, "Exiting %s:%u: %s",
      net_ntoa(lcptr->addr_remote), lcptr->port_remote, buf);

  /* If he has registered send him an error */
  if(!lclient_is_unknown(lcptr) && io_valid(lcptr->fds[1]))
    lclient_send(lcptr, "ERROR :%s", buf);

  if(lcptr->server)
  {
    server_exit(lcptr, NULL, lcptr->server, buf);

    lcptr->server = NULL;

    if(lcptr->client)
      lcptr->client->server = NULL;
  }

  if(lcptr->client)
  {
    lcptr->client->lclient = NULL;
    client_vexit(lcptr, lcptr->client, buf, args);
    lcptr->client = NULL;
    lcptr->user = NULL;
  }

  if(lcptr->user)
  {
    user_delete(lcptr->user);

    lcptr->user = NULL;

    if(lcptr->client)
      lcptr->client->user = NULL;
  }

  /* Maybe connect retry on servers */
  if(lcptr->connect)
  {
    connect_cancel(lcptr->connect);

    if(lcptr->connect->autoconn)
      connect_retry(lcptr->connect);
  }

  if(io_valid(lcptr->fds[1]))
    io_close(lcptr->fds[1]);

  if(io_valid(lcptr->fds[0]) && lcptr->fds[1] != lcptr->fds[0])
    io_close(lcptr->fds[0]);

  lcptr->fds[0] = -1;
  lcptr->fds[1] = -1;
  lcptr->shut = 1;
  lcptr->refcount = 0;

  lclient_delete(lcptr);

/*  lclient_collect();*/
}

int lclient_exit(struct lclient *lcptr, char *format, ...)
{
  va_list args;

  va_start(args, format);
  lclient_vexit(lcptr, format, args);
  va_end(args);

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Update client message/byte counters                                        *
 * -------------------------------------------------------------------------- */
void lclient_update_recvb(struct lclient *lcptr, size_t n)
{
  if(lclient_is_server(lcptr))
  {
    lclient_recvb[CLIENT_SERVER] += n;
    lclient_recvm[CLIENT_SERVER] += 1;
  }
  else
  {
    lclient_recvb[CLIENT_USER] += n;
    lclient_recvm[CLIENT_USER] += 1;
  }

  /* message count */
  lcptr->recvm++;

  /* increment bytes */
  lcptr->recvb += n;

  /* bytes are beyond 1023, so update kbytes */
  if(lcptr->recvb >= 0x0400)
  {
    lcptr->recvk += (lcptr->recvb >> 10);
    lcptr->recvb &= 0x03ff; /* 2^10 = 1024, 3ff = 1023 */
  }
}

void lclient_update_sendb(struct lclient *lcptr, size_t n)
{
  if(lclient_is_server(lcptr))
  {
    lclient_sendb[CLIENT_SERVER] += n;
    lclient_sendm[CLIENT_SERVER] += 1;
  }
  else
  {
    lclient_sendb[CLIENT_USER] += n;
    lclient_sendm[CLIENT_USER] += 1;
  }

  /* message count */
  lcptr->sendm++;

  /* increment bytes */
  lcptr->sendb += n;

  /* bytes are beyond 1023, so update kbytes */
  if(lcptr->sendb >= 0x0400)
  {
    lcptr->sendk += (lcptr->sendb >> 10);
    lcptr->sendb &= 0x03ff; /* 2^10 = 1024, 3ff = 1023 */
  }
}

/* -------------------------------------------------------------------------- *
 * Send a line to a local client                                              *
 * -------------------------------------------------------------------------- */
void lclient_vsend(struct lclient *lcptr, const char *format, va_list args)
{
  char   buf[IRCD_LINELEN + 1];
  size_t n;

  if(lcptr == NULL)
    return;

  client_source = lcptr;

  /* Formatted print */
  n = str_vsnprintf(buf, sizeof(buf) - 2, format, args);

  debug(ircd_log_out, "To %s: %s", lcptr->name, buf);

  /* Add line separator */
  buf[n++] = '\r';
  buf[n++] = '\n';

  /* Queue the data */
  io_write(lcptr->fds[1], buf, n);

  /* Update sendbytes */
  lclient_update_sendb(lcptr, n);
}

void lclient_send(struct lclient *lcptr, const char *format, ...)
{
  va_list args;

  va_start(args, format);

  lclient_vsend(lcptr, format, args);

  va_end(args);
}

/* -------------------------------------------------------------------------- *
 * Send a line to a client list but one                                       *
 * -------------------------------------------------------------------------- */
void lclient_vsend_list(struct lclient *one,    struct list *list,
                        const char     *format, va_list      args)
{
  struct lclient *lcptr;
  struct node    *node;
  struct fqueue   multi;
  size_t          n;
  char            buf[IRCD_LINELEN + 1];

  /* Formatted print */
  n = str_vsnprintf(buf, sizeof(buf) - 2, format, args);

  /* Add line separator */
  buf[n++] = '\r';
  buf[n++] = '\n';

  io_multi_start(&multi);

  io_multi_write(&multi, buf, n);

  dlink_foreach(list, node)
  {
    lcptr = node->data;

    /* Update sendbytes */
    lclient_update_sendb(lcptr, n);

    io_multi_link(&multi, lcptr->fds[1]);
  }

  io_multi_end(&multi);
}

void lclient_send_list(struct lclient *one,    struct list  *list,
                       const char     *format, ...)
{
  va_list args;

  va_start(args, format);

  lclient_vsend_list(one, list, format, args);

  va_end(args);
}

/* -------------------------------------------------------------------------- *
 * Start client handshake after valid USER/NICK has been sent.                *
 * -------------------------------------------------------------------------- */
void lclient_handshake(struct lclient *lcptr)
{
  /* When nickname is invalid then let the user try again */
  if(!chars_valid_nick(lcptr->name))
  {
    numeric_lsend(lcptr, ERR_ERRONEUSNICKNAME, lcptr->name);
    return;
  }

  /* When the username is invalid then exit the user */
  if(!chars_valid_user(lcptr->user->name))
  {
    lclient_set_type(lcptr, LCLIENT_USER);
    lclient_exit(lcptr, "invalid username [%s]", lcptr->user->name);
    return;
  }

  /* If no hooks were called then register the client */
  if(hooks_call(lclient_handshake, HOOK_DEFAULT, lcptr) == 0)
    lclient_register(lcptr);
}

/* -------------------------------------------------------------------------- *
 * Check for valid USER/NICK and start handshake                              *
 * -------------------------------------------------------------------------- */
int lclient_register(struct lclient *lcptr)
{
  struct msg *motd;

  if(lcptr->refcount == 0 || lcptr->user == NULL)
    return 1;

  if(lcptr->type == LCLIENT_UNKNOWN)
  {
    char *usermodearg[2] = { lcptr->user->mode, NULL };

    /* Hooks for sauth stuff and k-lines */
    if(hooks_call(lclient_register, HOOK_DEFAULT, lcptr))
      return 0;

    /* Create a block in global client pool */
    lcptr->client = client_new_local(CLIENT_USER, lcptr);

    lcptr->client->user = user_pop(lcptr->user);
    lcptr->user->client = client_pop(lcptr->client);

    /* Set usermode */
    usermode_make(lcptr->user, usermodearg, NULL,
                  USERMODE_OPTION_LINKALL |
                  USERMODE_OPTION_SINGLEARG |
                  USERMODE_OPTION_PERMISSION);

    if(io_list[lcptr->fds[0]].ssl)
    {
      lcptr->user->modes |= ((int)'p' - 0x40);
      usermode_assemble(lcptr->user->modes, lcptr->user->mode);
    }

    /* Welcome the client */
    lclient_welcome(lcptr);
    client_lusers(lcptr->client);

    /* Send the MOTD if available */
    if((motd = msg_find("MOTD")) == NULL || motd->handlers[lcptr->type] == NULL)
      numeric_send(lcptr->client, ERR_NOMOTD);
    else
      motd->handlers[lcptr->type](lcptr, lcptr->client, 2, NULL);

    /* let the user know his modes */
    usermode_send_local(lcptr, lcptr->client, lcptr->user->mode, NULL);

    /* Cancel timeout timer */
    timer_cancel(&lcptr->ptimer);

    /* Start the ping timeout timer */
    lcptr->ptimer = timer_start(lclient_ping, lcptr->class->ping_freq, lcptr);

    timer_note(lcptr->ptimer, "ping timer for user %s from %s:%u",
               lcptr->name,
               net_ntoa(lcptr->addr_remote),
               (uint32_t)lcptr->port_remote);

    /* Hooks for flood stuff */
    hooks_call(lclient_register, HOOK_2ND, lcptr);

    io_note(lcptr->fds[0], "registered client %s from %s:%u",
            lcptr->name,
            net_ntoa(lcptr->addr_remote),
            (uint32_t)lcptr->port_remote);
  }

  lcptr->shut = 0;

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Check if we got a PONG, if not exit the client otherwise send another PING *
 * -------------------------------------------------------------------------- */
int lclient_ping(struct lclient *lcptr)
{
  int saved;

  if(lcptr->lag > -1LL)
  {
    /* We got a PONG, send another PING */
    lclient_send(lcptr, "PING :%s", lclient_me->name);

    /* Set the time we sent the ping and reset lag */
    lcptr->ping = timer_mtime;
    lcptr->lag = -1LL;
  }
  else
  {
    /* We didn't get a PONG, exit the client */
    saved = lcptr->fds[0];
    client_exit(NULL, lcptr->client, "ping timeout: %u seconds",
                (uint32_t)((lcptr->class->ping_freq + 500LL) / 1000LL));
    io_close(saved);
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * USER/NICK has been sent but not yet validated                              *
 * -------------------------------------------------------------------------- */
void lclient_login(struct lclient *lcptr)
{
  struct client *cptr;

  cptr = client_find_nick(lcptr->name);

  if(cptr)
  {
    numeric_lsend(lcptr, ERR_NICKNAMEINUSE, lcptr->name);
    lcptr->name[0] = '\0';
    return;
  }

  if(hooks_call(lclient_login, HOOK_DEFAULT, lcptr) == 0)
    lclient_handshake(lcptr);
}

/* -------------------------------------------------------------------------- *
 * Send welcome messages to the client                                        *
 * -------------------------------------------------------------------------- */
void lclient_welcome(struct lclient *lcptr)
{
  char usermodes[sizeof(uint64_t) * 8 + 1];
  char chanmodes[sizeof(uint64_t) * 8 + 1];

  usermodes[0] = '\0';
  chanmodes[0] = '\0';

  chanmode_flags_build(chanmodes, CHANMODE_TYPE_ALL, CHFLG(ALL));
  usermode_assemble(-1LL, usermodes);

  if(usermodes[0] == '\0')
    strcpy(usermodes, "-");

  if(chanmodes[0] == '\0')
    strcpy(chanmodes, "-");

  /* 001 */
  numeric_lsend(lcptr, RPL_WELCOME, lcptr->name);

  /* 002 */
  numeric_lsend(lcptr, RPL_YOURHOST, lclient_me->name,
                ircd_package, ircd_release);

  /* 003 */
  numeric_lsend(lcptr, RPL_CREATED, ircd_uptime());

  /* 004 */
  numeric_lsend(lcptr, RPL_MYINFO, lclient_me->name, ircd_version,
                usermodes, chanmodes);

  /* 005 */
  ircd_support_show(lcptr->client);
}

/* -------------------------------------------------------------------------- *
 * Find a lclient by its id                                                   *
 * -------------------------------------------------------------------------- */
struct lclient *lclient_find_id(int id)
{
  struct lclient *lcptr;

  dlink_foreach(&lclient_list, lcptr)
    if(lcptr->id == (uint32_t)id)
      return lcptr;

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * Find a lclient by its name                                                 *
 * -------------------------------------------------------------------------- */
struct lclient *lclient_find_name(const char *name)
{
  struct lclient *lcptr;
  hash_t          hash;
  
  hash = str_ihash(name);

  dlink_foreach(&lclient_list, lcptr)
  {
    if(lcptr->hash == hash)
    {
      if(!str_icmp(lcptr->name, name))
        return lcptr;
    }
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void lclient_dump(struct lclient *lcptr)
{
  if(lcptr == NULL)
  {
    struct node *node;

    dump(lclient_log, "[============== lclient summary ===============]");

    if(lclient_lists[LCLIENT_UNKNOWN].size)
    {
      dump(lclient_log, " ---------------- unregistered ----------------");

      dlink_foreach_data(&lclient_lists[LCLIENT_UNKNOWN], node, lcptr)
        dump(lclient_log, " #%03u: [%u] %-20s (%s@%s)",
              lcptr->id, lcptr->refcount,
              lcptr->name[0] ? lcptr->name : "<unregistered>",
              lcptr->user ? lcptr->user->name : "unknown",
              lcptr->host);
    }
    if(lclient_lists[LCLIENT_USER].size)
    {
      dump(lclient_log, " ------------- registered clients -------------");

      dlink_foreach_data(&lclient_lists[LCLIENT_USER], node, lcptr)
        dump(lclient_log, " #%03u: [%u] %-20s (%s@%s)",
              lcptr->id, lcptr->refcount, lcptr->name,
              lcptr->user->name, lcptr->host);
    }
    if(lclient_lists[LCLIENT_SERVER].size)
    {
      dump(lclient_log, " ------------------- servers ------------------");

      dlink_foreach_data(&lclient_lists[LCLIENT_SERVER], node, lcptr)
        dump(lclient_log, " #%03u: [%u] %-20s (%s)",
              lcptr->id, lcptr->refcount, lcptr->name,
              lcptr->host);
    }
    if(lclient_lists[LCLIENT_OPER].size)
    {
      dump(lclient_log, " ------------------ operators -----------------");

      dlink_foreach_data(&lclient_lists[LCLIENT_OPER], node, lcptr)
        dump(lclient_log, " #%03u: [%u] %-20s (%s@%s)",
              lcptr->id, lcptr->refcount, lcptr->name,
              lcptr->user->name, lcptr->host);
    }

    dump(lclient_log, "[=========== end of lclient summary ===========]");
  }
  else
  {
    dump(lclient_log, "[============== lclient dump ===============]");
    dump(lclient_log, "         id: #%u", lcptr->id);
    dump(lclient_log, "   refcount: %u", lcptr->refcount);
    dump(lclient_log, "       type: %s", lclient_types[lcptr->type]);
    dump(lclient_log, "       hash: %p", lcptr->hash);
    dump(lclient_log, "       user: %-27s [%u]",
          lcptr->user ? lcptr->user->name : "(nil)",
          lcptr->user ? lcptr->user->refcount : 0);
    dump(lclient_log, "     client: %-27s [%u]",
          lcptr->client ? lcptr->client->name : "(nil)",
          lcptr->client ? lcptr->client->refcount : 0);
    dump(lclient_log, "     server: %-27s [%u]",
          lcptr->server ? lcptr->server->name : "(nil)",
          lcptr->server ? lcptr->server->refcount : 0);
    dump(lclient_log, "     listen: %-27s [%u]",
          lcptr->listen ? lcptr->listen->name : "(nil)",
          lcptr->listen ? lcptr->listen->refcount : 0);
    dump(lclient_log, "    connect: %-27s [%u]",
          lcptr->connect ? lcptr->connect->name : "(nil)",
          lcptr->connect ? lcptr->connect->refcount : 0);
    dump(lclient_log, "      fds[]: { %i, %i }",
          lcptr->fds[0], lcptr->fds[1]);
    dump(lclient_log, "  ctrlfds[]: { %i, %i }",
          lcptr->ctrlfds[0], lcptr->ctrlfds[1]);
    dump(lclient_log, "     remote: %s:%u",
          net_ntoa(lcptr->addr_remote), (uint32_t)lcptr->port_remote);
    dump(lclient_log, "      local: %s:%u",
          net_ntoa(lcptr->addr_local), (uint32_t)lcptr->port_local);
    dump(lclient_log, "     ptimer: #%i [%u]",
          lcptr->ptimer ? lcptr->ptimer->id : -1,
          lcptr->ptimer ? lcptr->ptimer->refcount : 0);
    dump(lclient_log, "       recv: %ub %uk %um",
          lcptr->recvb, lcptr->recvk, lcptr->recvm);
    dump(lclient_log, "       send: %ub %uk %um",
          lcptr->sendb, lcptr->sendk, lcptr->sendm);
    dump(lclient_log, "       caps: %llu", lcptr->caps);
    dump(lclient_log, "         ts: %lu", lcptr->ts);
    dump(lclient_log, "     serial: %u", lcptr->serial);
    dump(lclient_log, "        lag: %llu", lcptr->lag);
    dump(lclient_log, "       ping: %llu", lcptr->ping);
    dump(lclient_log, "       name: %s", lcptr->name);
    dump(lclient_log, "       host: %s", lcptr->host);
    dump(lclient_log, "     hostip: %s", lcptr->hostip);
    dump(lclient_log, "       pass: %s", lcptr->pass);
    dump(lclient_log, "       info: %s", lcptr->info);

    dump(lclient_log, "[=========== end of lclient dump ===========]");
  }
}
