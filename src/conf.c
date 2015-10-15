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
 * $Id: conf.c,v 1.5 2006/09/28 09:44:11 roman Exp $
 */

#define _GNU_SOURCE

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/connect.h"
#include "libchaos/syscall.h"
#include "libchaos/listen.h"
#include "libchaos/module.h"
#include "libchaos/mfile.h"
#include "libchaos/hook.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/str.h"
#include "libchaos/ssl.h"
#include "libchaos/io.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/config.h"
#include "ircd/ircd.h"
#include "ircd/client.h"
#include "ircd/class.h"
#include "ircd/conf.h"
#include "ircd/oper.h"

#include "../config.h"

#include "../config.h"

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
int           conf_fd;
struct config conf_current;
struct config conf_new;
int           conf_log;

extern char   conffile[PATHLEN];
/* -------------------------------------------------------------------------- *
 * Get command line options - stolen from dietlibc                            *
 * -------------------------------------------------------------------------- */
static int    conf_longindex = 0;
static int    conf_optind = 1;
static int    conf_optopt;
static char  *conf_optarg = NULL;

static int conf_getopt(int argc, char **argv, const char *optstring,
                       struct option *longopts)
{
  static int lastidx = 0, lastofs = 0;
  char *tmp;

  /* Return immediately if we already read all
     arguments or when theres no switch left. */
  if(conf_optind > argc || !argv[conf_optind] ||
     argv[conf_optind][0] != '-' ||
     argv[conf_optind][1] == '\0')
    return -1;

again:

  if(argv[conf_optind] == NULL)
    return -1;

  /* Skip this argument if its '--' */
  if(argv[conf_optind][1] == '-' && argv[conf_optind][2] == 0)
  {
    conf_optind++;
    return -1;
  }

  /* Its a long option */
  if(argv[conf_optind][1] == '-')
  {
    char *arg = &argv[conf_optind][2];      /* Option name */
    char *max = strchr(arg, '=');      /* Points to end of name */
    const struct option *o;            /* Points to switch in option struct */

    /* We haven't found a '=', set end of name */
    if(!max)
      max = &arg[str_len(arg)];

    /* Walk through the option list */
    for(o = longopts; o->name; o++)
    {
      /* Compare option name */
      if(!str_ncmp(o->name, arg, (size_t)(max - arg)))
      {
        conf_longindex = (size_t)(o - longopts);

        if(o->has_arg > 0)
        {
          /* The conf_optarg is in this argv[] */
          if(*max == '=')
          {
            conf_optarg = max + 1;
          }
          /* The conf_optarg is in next argv[] */
          else
          {
            conf_optarg = argv[conf_optind + 1];

            /* Check if we have an argument */
            if(!conf_optarg && o->has_arg == 1)
            {
              if(*optstring == ':')
                return ':';

              log(conf_log, L_fatal, "argument required: `%s'.", arg);
              conf_optind++;

              return '?';
            }

            conf_optind++;
          }
        }

        /* Skip to next arg */
        conf_optind++;

        /* Set flag if pointer present */
        if(o->flag)
          *(o->flag) = o->val;
        else
          return o->val;

        return 0;
      }
    }

    if(*optstring == ':')
      return ':';

    log(conf_log, L_fatal, "invalid option`%s'.", arg);

    conf_optind++;

    return '?';
  }

  /* Update last index */
  if(lastidx != conf_optind)
  {
    lastidx = conf_optind;
    lastofs = 0;
  }

  conf_optopt = argv[conf_optind][lastofs + 1];

  if((tmp = strchr(optstring, conf_optopt)))
  {
    /* Apparently, we looked for \0, i.e. end of argument */
    if(*tmp == 0)
    {
      conf_optind++;
      goto again;
    }

    /* Argument expected */
    if(tmp[1] == ':')
    {
      /* "-foo", return "oo" as conf_optarg */
      if(tmp[2] == ':' || argv[conf_optind][lastofs + 2])
      {
        if(!*(conf_optarg = argv[conf_optind] + lastofs + 2))
          conf_optarg = 0;

        goto found;
      }

      conf_optarg = argv[conf_optind + 1];

      /* Missing argument */
      if(!conf_optarg)
      {
        conf_optind++;

        if(*optstring == ':')
          return ':';

        log(conf_log, L_fatal, "missing argument for -%c.", conf_optopt);

        return ':';
      }

      conf_optind++;
    }
    else
    {
      lastofs++;
      return conf_optopt;
    }

found:
    conf_optind++;
    return conf_optopt;
  }
  /* not found */
  else
  {
    log(conf_log, L_fatal, "unknown option -%c.", conf_optopt);
    conf_optind++;
    return '?';
  }
}

/* -------------------------------------------------------------------------- *
 * Show usage.                                                                *
 * -------------------------------------------------------------------------- */
static void usage(char **argv)
{
  puts(PACKAGE_NAME"-"PACKAGE_VERSION
       " - pi-networks irc server\n");
  puts("Usage: %s [options]\n", *argv);
  puts("  -h, --help           show this help");
  puts("  -d, --nodetach       do not detach from tty");
  puts("  -f, --config=FILE    specify config file");
  puts("");
  syscall_exit(0);
}

/* -------------------------------------------------------------------------- *
 * Getopt and configfile coldstart.                                           *
 * -------------------------------------------------------------------------- */
static struct option longoptions[] = {
  { "help",   0, NULL, 'h', },
  { "nofork", 1, NULL, 'd', },
  { "config", 1, NULL, 'f', },
  { NULL,     0, NULL, '\0' }
};

void conf_init(int argc, char **argv, char **envp)
{
  int c;

  conf_log = log_source_register("conf");

  memset(&conf_current, 0, sizeof(struct config));
  memset(&conf_new, 0, sizeof(struct config));

  strcpy(conf_current.global.configfile, SYSCONFDIR"/ircd.conf");
  conf_new.global.nodetach = 0;

  while((c = conf_getopt(argc, argv, "hdf:", longoptions)) > 0)
  {
    switch(c)
    {
      case 'h':
        usage(argv);
        break;
      case 'f':
        strlcpy(conf_current.global.configfile, conf_optarg, PATHLEN);
        break;
      case 'd':
        conf_new.global.nodetach = 1;
        break;
      default:
        usage(argv);
        break;
    }
  }

  conf_read();
}

/* -------------------------------------------------------------------------- *
 * Shutdown config code.                                                      *
 * -------------------------------------------------------------------------- */
void conf_shutdown(void)
{
/*  conf_free(&conf_current);
  conf_free(&conf_new);*/
  io_destroy(conf_fd);

  log_source_unregister(conf_log);
}
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void conf_free(struct config *config)
{
/*  struct node *node;

  dlink_foreach(&config->logs, node)
    log_drain_push(node->data);

  dlink_foreach(&config->classes, node)
    class_push(node->data);

  dlink_foreach(&config->listeners, node)
    listen_push(node->data);*/
}

/* -------------------------------------------------------------------------- *
 * This is called when a config file is read successfully.                    *
 * -------------------------------------------------------------------------- */
void conf_done(void)
{
  memcpy(&conf_current.logs, &conf_new.logs, sizeof(struct config) - sizeof(struct conf_global));

  conf_current.global.nodetach = conf_new.global.nodetach;
  strcpy(conf_current.global.name, conf_new.global.name);
  strcpy(conf_current.global.info, conf_new.global.info);
  strcpy(conf_current.global.pidfile, conf_new.global.pidfile);
/*  if(conf_current.global.name[0])
    strlcpy(me->name, conf_current.global.name, sizeof(me->name));

  if(conf_current.global.info[0])
    strlcpy(me->info, conf_current.global.info, sizeof(me->info));

  for(i = 0; i < sizeof(me->name); i++)
    if(chars_isspace(me->name[i]))
      me->name[i] = '\0';*/

  hooks_call(conf_done, HOOK_DEFAULT, &conf_current);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void conf_read_callback(int fd, void *ptr)
{
  /* Check if we're done reading the file */
  if(io_list[fd].status.eof)
  {
    log(conf_log, L_status, "Read %u lines from %s.",
        io_list[fd].recvq.lines,
        conf_current.global.configfile);

/*    conf_free(&conf_new);*/
    yyparse();

    conf_free(&conf_current);
    conf_done();

    io_destroy(fd);
  }
}

/* -------------------------------------------------------------------------- *
 * Read the config file(s).                                                   *
 * -------------------------------------------------------------------------- */
void conf_read(void)
{
  /* Open the file in non-blocking mode */
  log(conf_log, L_status, "Reading config from %s.",
      conf_current.global.configfile);

  conf_fd = io_open(conf_current.global.configfile, IO_OPEN_READ);

  strlcpy(conffile, conf_current.global.configfile, sizeof(conffile));

  if(conf_fd == -1)
  {
    log(conf_log, L_fatal, "Could not open config file: %s.",
        conf_current.global.configfile);
    return;
  }

  /* Enable the read queue */
  io_queue_control(conf_fd, ON, OFF, OFF);

  /* Set up a handler that will be called when file is read */
  io_register(conf_fd, IO_CB_READ, conf_read_callback, NULL);
}

/* -------------------------------------------------------------------------- *
 * Bison parser error functions.                                              *
 * -------------------------------------------------------------------------- */
void conf_yy_error(char *msg)
{
  log(conf_log, L_warning, "%s:%u: %s: %s",
      io_list[conf_fd].note, lineno + 1, msg, linebuf);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void conf_rehash_hook(struct config *config)
{
  log(conf_log, L_status, "Rehash done.");

  hook_unregister(conf_done, HOOK_DEFAULT, conf_rehash_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void conf_rehash(void)
{
  struct oper        *optr;
  struct class       *clptr;
  struct mfile       *mfptr;
  struct module      *mptr;
  struct listen      *lptr;
  struct connect     *cnptr;
  struct ssl_context *scptr;

  conf_read();

  dlink_foreach(&listen_list, lptr)
  {
    if(--lptr->refcount == 0)
    {
      log(conf_log, L_status, "Lost listen block: %s", lptr->name);
      listen_delete(lptr);
    }
  }

  dlink_foreach(&connect_list, cnptr)
  {
    if(--cnptr->refcount == 0)
    {
      log(conf_log, L_status, "Lost connect block: %s", cnptr->name);
      connect_delete(cnptr);
    }
  }

  dlink_foreach(&module_list, mptr)
  {
    if(--mptr->refcount == 0)
    {
      log(conf_log, L_status, "Lost module: %s", mptr->name);
      module_delete(mptr);
    }
  }

  dlink_foreach(&mfile_list, mfptr)
  {
    if(--mfptr->refcount == 0)
    {
      log(conf_log, L_status, "Lost mfile: %s", mfptr->name);
      mfile_delete(mfptr);
    }
  }

  dlink_foreach(&class_list, clptr)
  {
    if(--clptr->refcount == 0)
    {
      log(conf_log, L_status, "Lost class block: %s", clptr->name);
      class_delete(clptr);
    }
  }

  dlink_foreach(&ssl_list, scptr)
  {
    if(--scptr->refcount == 0)
    {
      log(conf_log, L_status, "Lost ssl context: %s", scptr->name);
      ssl_delete(scptr);
    }
  }

  dlink_foreach(&oper_list, optr)
  {
    if(--optr->refcount == 0)
    {
      log(conf_log, L_status, "Lost oper block: %s", optr->name);
      oper_delete(optr);
    }
  }

  hook_register(conf_done, HOOK_DEFAULT, conf_rehash_hook);
}

