/* chaosircd - pi-networks irc server
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
 * $Id: ircd.c,v 1.6 2006/09/28 08:38:31 roman Exp $
 */

#define _GNU_SOURCE

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/connect.h>
#include <libchaos/syscall.h>
#include <libchaos/filter.h>
#include <libchaos/listen.h>
#include <libchaos/module.h>
#include <libchaos/child.h>
#include <libchaos/dlink.h>
#include <libchaos/graph.h>
#include <libchaos/htmlp.h>
#include <libchaos/httpc.h>
#include <libchaos/image.h>
#include <libchaos/mfile.h>
#include <libchaos/queue.h>
#include <libchaos/sauth.h>
#include <libchaos/timer.h>
#include <libchaos/hook.h>
#include <libchaos/gif.h>
#include <libchaos/ini.h>
#include <libchaos/log.h>
#include <libchaos/mem.h>
#include <libchaos/net.h>
#include <libchaos/str.h>
#include <libchaos/ssl.h>
#include <libchaos/io.h>

/* -------------------------------------------------------------------------- *
 * Program headers                                                            *
 * -------------------------------------------------------------------------- */
#include <chaosircd/config.h>
#include <chaosircd/ircd.h>
#include <chaosircd/chanmode.h>
#include <chaosircd/usermode.h>
#include <chaosircd/chanuser.h>
#include <chaosircd/channel.h>
#include <chaosircd/lclient.h>
#include <chaosircd/numeric.h>
#include <chaosircd/service.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/conf.h>
#include <chaosircd/oper.h>
#include <chaosircd/user.h>
#include <chaosircd/msg.h>

/* -------------------------------------------------------------------------- *
 * System headers                                                             *
 * -------------------------------------------------------------------------- */
#include "../config.h"

#include <sys/types.h>

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif /* HAVE_SYS_MMAN_H */

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif /* HAVE_SIGNAL_H */

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

/* -------------------------------------------------------------------------- *
 * Global variables for the daemon code                                       *
 * -------------------------------------------------------------------------- */
const char  *ircd_package = PROJECT_NAME;
const char  *ircd_version = PROJECT_VERSION;
const char  *ircd_release = PROJECT_RELEASE;
uint64_t     ircd_start;
struct dlog *ircd_drain;
struct sheap ircd_heap;
int          ircd_log;
int          ircd_log_in;
int          ircd_log_out;
struct list  ircd_support;
int          ircd_argc = 0;
char       **ircd_argv = NULL;
char       **ircd_envp = NULL;
char         ircd_path[PATHLEN];

/* -------------------------------------------------------------------------- *
 * Install mmap()ed and mprotect()ed stack into ircd core                     *
 * -------------------------------------------------------------------------- */
#if 0 /* (defined __linux__) && (defined __i386__)*/
static void ircd_stack_install(void)
{
  void   *old_esp;
  void   *old_ebp;
  size_t  old_size;
  void   *new_esp;
  void   *new_ebp;

  ircd_stack = syscall_mmap(NULL, IRCD_STACKSIZE, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

  syscall_mprotect(ircd_stack, IRCD_STACKSIZE, PROT_READ|PROT_WRITE);

  __asm__ __volatile__("movl\t%%esp,%0\n\t"
                       "movl\t%%ebp,%1\n\t"
                       : "=a" (old_esp), "=b" (old_ebp));

  old_size = IRCD_LINUX_STACKTOP - (size_t)old_esp;
  new_esp = ircd_stack + (size_t)(IRCD_STACKSIZE - old_size);

  memcpy(new_esp, old_esp, old_size);

  old_size = IRCD_LINUX_STACKTOP - (size_t)old_ebp;
  new_ebp = ircd_stack + (size_t)(IRCD_STACKSIZE - old_size);

  old_size = IRCD_LINUX_STACKTOP - (*(size_t *)old_ebp);
  *(void **)new_ebp = ircd_stack + (size_t)(IRCD_STACKSIZE - old_size);
  *(void **)old_ebp = ircd_stack + (size_t)(IRCD_STACKSIZE - old_size);

  __asm__ __volatile__("movl\t%0,%%esp\n\t"
                       "movl\t%1,%%ebp\n\t"
                       : : "a" (new_esp), "b" (new_ebp));
}
#endif /* (defined __linux__) && (defined __i386__) */

/* -------------------------------------------------------------------------- *
 * Die if we cannot bind to a port during coldstart.                          *
 * -------------------------------------------------------------------------- */
static void ircd_listen(const char *address, uint16_t port, const char *error)
{
  log(ircd_log, L_fatal, "Could not bind to %s:%u: %s",
      address, (uint32_t)port, error);
  syscall_exit(1);
}

/* -------------------------------------------------------------------------- *
 * Print the uptime of the ircd instance in a human readable format.          *
 * -------------------------------------------------------------------------- */
const char *ircd_uptime(void)
{
  static char upstr[IRCD_LINELEN - 1];
  uint32_t    msecs;
  uint32_t    secs;
  uint32_t    mins;
  uint32_t    hrs;
  uint32_t    days;
  uint64_t    uptime;

  uptime = timer_mtime - ircd_start;

  msecs = (uint32_t)(uptime  % 1000L);
  secs = ((uint32_t)(uptime /= 1000L) % 60);
  mins = ((uint32_t)(uptime /= 60L)   % 60);
  hrs  = ((uint32_t)(uptime /= 60L)   % 24);
  days =  (uint32_t)(uptime / 24L);

  if(days == 0)
  {
    if(hrs == 0)
    {
      if(mins == 0)
        str_snprintf(upstr, sizeof(upstr), "%u seconds, %u msecs", secs, msecs);
      else
        str_snprintf(upstr, sizeof(upstr), "%u minutes, %u seconds", mins, secs);
    }
    else
    {
      str_snprintf(upstr, sizeof(upstr), "%u hours, %u minutes", hrs, mins);
    }
  }
  else
  {
    str_snprintf(upstr, sizeof(upstr), "%u days, %u hours", days, hrs);
  }

  return upstr;
}

/* -------------------------------------------------------------------------- *
 * Write the process ID to a file.                                            *
 * -------------------------------------------------------------------------- */
static int ircd_writepid(struct config *config, long pid)
{
  int fd;

  fd = io_open(config->global.pidfile,
               IO_OPEN_WRITE|IO_OPEN_TRUNCATE|IO_OPEN_CREATE, 0644);

  if(fd == -1)
    return -1;

  io_queue_control(fd, OFF, OFF, OFF);
  io_puts(fd, "%u", pid);
  io_close(fd);

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Create a detached child and exit the parent (going to background)          *
 * -------------------------------------------------------------------------- */
static void ircd_detach(struct config *config)
{
#ifndef WIN32
  long pid;
  
  pid = syscall_fork();

  if(pid == -1)
    return;

  if(pid == 0)
  {
    log_drain_level(ircd_drain, L_fatal);
    log_drain_delete(ircd_drain);
    syscall_setsid();
  }
  else
  {
    if(ircd_writepid(config, pid) == -1)
    {
      log(ircd_log, L_status, "*** Could not write PID file!!! ***");
      syscall_kill(pid, SIGTERM);
      syscall_exit(1);
    }

    log(ircd_log, L_status, "*** Detached [%u] ***", pid);

    syscall_exit(0);
  }
#endif /* WIN32 */
}

/* -------------------------------------------------------------------------- *
 * Read the file with the process ID and check if there is already a running  *
 * chaosircd...                                                               *
 * -------------------------------------------------------------------------- */
static long ircd_check(struct config *config)
{
  struct stat st;
  long       pid;
  char        proc[32];
  char        buf[16];
  int         fd;

  fd = io_open(config->global.pidfile, IO_OPEN_READ);

  if(fd == -1)
    return 0;

  io_queue_control(fd, OFF, OFF, OFF);

  if(io_read(fd, buf, sizeof(buf)) > 0)
  {
    pid = str_toul(buf, NULL, 10);
    
    str_snprintf(proc, sizeof(proc), "/proc/%u", pid);
    
    if(syscall_stat(proc, &st) == 0)
      return pid;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * After coldstart we check config file sanity and then we initialize the     *
 * client instance referring to ourselves...                                  *
 * -------------------------------------------------------------------------- */
static int ircd_coldstart(struct config *config)
{
  long pid;
  
  log(ircd_log, L_status, "*** Config file coldstart done ***");

  if(config->global.name[0] == '\0')
  {
    log(ircd_log, L_fatal, "chaosircd has no name!!!");
    syscall_exit(1);
  }

  if(config->global.pidfile[0] == '\0')
  {
    log(ircd_log, L_fatal, "chaosircd has no PID file!!!");
    syscall_exit(1);
  }

  if((pid = ircd_check(config)))
  {
    log(ircd_log, L_fatal, "chaosircd already running [%u]", pid);
    syscall_exit(1);
  }

  strlcpy(server_me->name, config->global.name, sizeof(server_me->name));

  client_set_name(client_me, config->global.name);
  lclient_set_name(lclient_me, config->global.name);
  server_set_name(server_me, config->global.name);

  strlcpy(client_me->info, config->global.info, sizeof(client_me->info));
  strlcpy(lclient_me->info, config->global.info, sizeof(lclient_me->info));

  if(!config->global.nodetach)
  {
    ircd_detach(config);
  }
  else
  {
    if(ircd_writepid(config, syscall_getpid()))
    {
      log(ircd_log, L_fatal, "*** Could not write PID file!!! ***");
    }
  }

  ircd_start = timer_mtime;

  hook_unregister(conf_done, HOOK_DEFAULT, ircd_coldstart);

  return 0;
}

/* -------------------------------------------------------------------------- *
 * Initialize things.                                                         *
 * -------------------------------------------------------------------------- */
void ircd_init(int argc, char **argv, char **envp)
{
  log(ircd_log, L_startup, "*** Firing up %s v%s - %s ***",
      PROJECT_NAME, PROJECT_VERSION, PROJECT_RELEASE);

  log_init(STDOUT_FILENO, LOG_ALL, L_status);
  io_init_except(STDOUT_FILENO, STDOUT_FILENO, STDOUT_FILENO);
  mem_init();
  str_init();
  timer_init();
  listen_init();
  connect_init();
  queue_init();
  dlink_init();
  module_init();
  net_init();
  ini_init();
  hook_init();
  child_init();
  sauth_init();
  mfile_init();
  ssl_init();
  httpc_init();
  htmlp_init();
#ifdef HAVE_SOCKET_FILTER
  filter_init();
#endif /* HAVE_SOCKET_FILTER */
  gif_init();
  image_init();
  graph_init();

  module_setpath(PLUGINDIR);

  ircd_log = log_source_register("ircd");
  ircd_log_in = log_source_register("in");
  ircd_log_out = log_source_register("out");

  log_source_filter = (~log_sources[ircd_log_in].flag) & (~log_sources[ircd_log_out].flag);

  mem_static_create(&ircd_heap, sizeof(struct support), SUPPORT_BLOCK_SIZE);
  mem_static_note(&ircd_heap, "support heap");
  dlink_list_zero(&ircd_support);
  
  log(ircd_log, L_status, "*** Done initialising %s library ***", PROJECT_NAME);

  lclient_init();
  server_init();
  client_init();
  user_init();
  channel_init();
  usermode_init();
  chanmode_init();
  chanuser_init();
  msg_init();
  class_init();
  oper_init();
  service_init();
  
  log(ircd_log, L_status, "*** Done initialising %s core ***", PROJECT_NAME);

#ifdef DEBUG
  ircd_drain = log_drain_setfd(1, LOG_ALL & log_source_filter, L_debug, 0);
#else
  ircd_drain = log_drain_setfd(1, LOG_ALL & log_source_filter, L_status, 0);
#endif /* DEBUG */

  hook_register(conf_done, HOOK_DEFAULT, ircd_coldstart);
  hook_register(listen_add, HOOK_DEFAULT, ircd_listen);

  conf_init(argc, argv, envp);

}

/* -------------------------------------------------------------------------- *
 * Loop around some timer stuff and the i/o multiplexer.                      *
 * -------------------------------------------------------------------------- */
void ircd_loop(void)
{
  int ret = 0;
  int64_t *timeout;
  int64_t remain = 0LL;

  while(ret >= 0)
  {
    /* Calculate timeout value */
    timeout = timer_timeout();

    /* Do I/O multiplexing and event handling */
#if (defined USE_SELECT)
    ret = io_select(&remain, timeout);
#elif (defined USE_POLL)
    ret = io_poll(&remain, timeout);
#endif /* USE_SELECT | USE_POLL */

    /* Remaining time is 0msecs, we need to run a timer */
    if(remain == 0LL)
      timer_run();

    if(timeout)
      timer_drift(*timeout - remain);

    io_handle();

    timer_collect();
/*    ircd_collect();*/
  }
}

/* -------------------------------------------------------------------------- *
 * Dump status information.                                                   *
 * -------------------------------------------------------------------------- */
#ifdef DEBUG
void ircd_dump(void)
{
  debug(ircd_log, "--- chaosircd complete dump ---");

/*  conf_dump(&conf_current);*/

  connect_dump(NULL);
  listen_dump(NULL);
  log_source_dump(-1);
  log_drain_dump(NULL);
  timer_dump(NULL);
/*  queue_dump(NULL);*/
  dlink_dump();
  module_dump(NULL);
  net_dump(NULL);
/*  ini_dump(NULL);*/

  debug(ircd_log, "--- end of chaosircd complete dump ---");
}
#endif /* DEBUG */

/* -------------------------------------------------------------------------- *
 * Garbage collect.                                                           *
 * -------------------------------------------------------------------------- */
void ircd_collect(void)
{
/*  child_collect();*/
  connect_collect();
  listen_collect();
  log_collect();
  dlink_collect();
  timer_collect();
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int ircd_restart(void)
{
#ifndef WIN32
  long pid;
  int status;

#ifdef HAVE_SOCKET_FILTER
  filter_shutdown();
#endif /* HAVE_SOCKET_FILTER */
  listen_shutdown();
  child_shutdown();

  syscall_unlink(conf_current.global.pidfile);

  pid = fork();

  if(pid)
  {
    log(ircd_log, L_status, "new child status: %i", waitpid(pid, &status, WNOHANG));
  }
  else
  {
    syscall_execve(ircd_path, ircd_argv, ircd_envp);

    log(ircd_log, L_status, "Failed executing myself (%s)!", ircd_path);
    ircd_shutdown();
  }

  log(ircd_log, L_status, "Restart succeeded.");

  ircd_shutdown();
#endif /* WIN32 */
  return 0;
}

/* -------------------------------------------------------------------------- *
 * Clean things up.                                                           *
 * -------------------------------------------------------------------------- */
void ircd_shutdown(void)
{
  log(ircd_log, L_status, "*** Shutting down %s ***", PROJECT_NAME);

  syscall_unlink(conf_current.global.pidfile);

  if(!conf_new.global.nodetach)
    log_drain_delete(ircd_drain);

  module_shutdown();

  service_shutdown();
  channel_shutdown();
  chanuser_shutdown();
  chanmode_shutdown();
  client_shutdown();
  lclient_shutdown();
  usermode_shutdown();
  server_shutdown();
  user_shutdown();
  oper_shutdown();
  class_shutdown();
  msg_shutdown();

  mem_static_destroy(&ircd_heap);

  graph_shutdown();
  image_shutdown();
  gif_shutdown();
#ifdef HAVE_SOCKET_FILTER
  filter_shutdown();
#endif /* HAVE_SOCKET_FILTER */
  httpc_shutdown();
  htmlp_shutdown();
  ssl_shutdown();
  mfile_shutdown();
  listen_shutdown();
  connect_shutdown();
  sauth_shutdown();
  child_shutdown();
  hook_shutdown();
  ini_shutdown();
  net_shutdown();
  io_shutdown();
  dlink_shutdown();
  queue_shutdown();
  log_shutdown();
  timer_shutdown();
  str_shutdown();
  mem_shutdown();

  syscall_exit(0);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct support *ircd_support_new(void)
{
  struct support *suptr;

  suptr = mem_static_alloc(&ircd_heap);

  dlink_add_tail(&ircd_support, &suptr->node, suptr);

  return suptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct support *ircd_support_find(const char *name)
{
  struct support *suptr;

  dlink_foreach(&ircd_support, suptr)
  {
    if(!str_icmp(suptr->name, name))
      return suptr;
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void ircd_support_unset(const char *name)
{
  struct support *suptr;

  if((suptr = ircd_support_find(name)))
  {
    dlink_delete(&ircd_support, &suptr->node);
    mem_static_free(&ircd_heap, suptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct support *ircd_support_set(const char *name, const char *value, ...)
{
  struct support *suptr;
  va_list         args;

  va_start(args, value);

  if((suptr = ircd_support_find(name)) == NULL)
  {
    suptr = ircd_support_new();
    strlcpy(suptr->name, name, sizeof(suptr->name));
  }

  if(value)
    str_vsnprintf(suptr->value, sizeof(suptr->value), value, args);
  else
    suptr->value[0] = '\0';

  va_end(args);

  return suptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct node *ircd_support_assemble(char *buf, struct node *nptr, size_t n)
{
  struct support *suptr;
  size_t          i = 0;
  size_t          len;

  if(nptr == NULL)
    return NULL;

  do
  {
    suptr = nptr->data;

    len = str_len(suptr->name) + 1 +
      (suptr->value[0] ? str_len(suptr->value) + 1 : 0);

    if(len + 2 > n - i)
      break;

    if(i)
      buf[i++] = ' ';

    i += strlcpy(&buf[i], suptr->name, n - i + 1);

    if(suptr->value[0])
    {
      buf[i++] = '=';
      i += strlcpy(&buf[i], suptr->value, n - i + 1);
    }
  }
  while((nptr = nptr->next));

  buf[i] = '\0';

  return nptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void ircd_support_show(struct client *cptr)
{
  struct node *nptr;
  char         support[96];

  if(ircd_support.head == NULL)
    return;

  for(nptr = ircd_support.head->data; nptr;)
  {
    nptr = ircd_support_assemble(support, nptr, sizeof(support));

    numeric_send(cptr, RPL_ISUPPORT, support);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
