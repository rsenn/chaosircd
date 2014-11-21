/* chaosircd - CrowdGuard IRC daemon
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

/* -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/syscall.h"
#include "libchaos/str.h"
/* -------------------------------------------------------------------------- */

#include "../config.h"

#include "ircd/ircd.h"

#include <unistd.h>

#ifdef HAVE_SETRLIMIT
#warning setrlimit
#include <sys/resource.h>
#endif
#include <signal.h>

/* -------------------------------------------------------------------------- */
/*int          ircd_argc = 0;
char       **ircd_argv = NULL;
char       **ircd_envp = NULL;
char         ircd_path[PATHLEN];*/
extern void ircd_stack_install(void);
/* -------------------------------------------------------------------------- *
 * Program entry.                                                             *
 * -------------------------------------------------------------------------- */
int main(int argc, char **argv, char **envp)
{
  char          link[64];
  int           n;

#ifdef HAVE_SETRLIMIT
  static struct rlimit 
# ifdef __CYGWIN__
   { unsigned long a; unsigned long b; } 
# endif
   stack = { 1024, 1024 }
  ;
  syscall_setrlimit(RLIMIT_STACK, &stack);
#endif
  ircd_stack_install();

  ircd_argc = argc;
  ircd_argv = argv;
  ircd_envp = envp;

  /* Change to working directory */
  syscall_chdir(PREFIX);

  /* Get argv0 */
#ifndef WIN32
  str_snprintf(link, sizeof(link), "/proc/%u/exe", syscall_getpid());

  if((n = syscall_readlink(link, ircd_path, sizeof(ircd_path) - 1)) > -1)
  {
    ircd_path[n] = '\0';
  }
  else
  {
    ircd_path[0] = '\0';
  }
#endif /* WIN32 */

  /* Catch some signals */
#ifndef WIN32
  syscall_signal(SIGINT, (void *)ircd_shutdown);
  syscall_signal(SIGHUP, (void *)ircd_shutdown);
  syscall_signal(SIGTERM, (void *)ircd_shutdown);
  syscall_signal(SIGPIPE, (void *)1);
#endif /* WIN32 */

  /* Always dump core! */
#ifdef HAVE_SETRLIMIT
  struct rlimit 
# ifdef __CYGWIN__
   { unsigned long a; unsigned long b; } 
# endif
  core = { RLIM_INFINITY, RLIM_INFINITY };

  syscall_setrlimit(RLIMIT_CORE, &core);
#endif /* WIN32 */

  /* Initialise all modules */
  ircd_init(argc, argv, envp);

  /* Handle events */
  ircd_loop();

  /* Shutdown all modules */
  ircd_shutdown();

  return 0;
}

/* -------------------------------------------------------------------------- */
