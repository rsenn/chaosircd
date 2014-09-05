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
 * $Id: chanmode.c,v 1.3 2006/09/28 09:44:11 roman Exp $
 */

#define _GNU_SOURCE

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "defs.h"
#include "io.h"
#include "dlink.h"
#include "hook.h"
#include "log.h"
#include "mem.h"
#include "str.h"
#include "timer.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/class.h>
#include <ircd/chanmode.h>
#include <ircd/chanuser.h>
#include <ircd/channel.h>
#include <ircd/numeric.h>
#include <ircd/lclient.h>
#include <ircd/client.h>
#include <ircd/server.h>
#include <ircd/chars.h>
#include <ircd/user.h>

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
int             chanmode_log;
struct sheap    chanmode_heap;
struct sheap    chanmode_item_heap;
struct chanmode chanmode_table      [0x40];
char            chanmode_flags      [CHANMODE_PER_LINE * 2 + 1];
char            chanmode_args_id    [IRCD_LINELEN];
char            chanmode_args_nick  [IRCD_LINELEN];
char            chanmode_cmd_id     [IRCD_LINELEN];
char            chanmode_cmd_nick   [IRCD_LINELEN];
char            chanmode_cmd_prefix [IRCD_LINELEN];

/* -------------------------------------------------------------------------- *
 * Initialize the chanmode module                                             *
 * -------------------------------------------------------------------------- */
void chanmode_init(void)
{
  chanmode_log = log_source_register("chanmode");

  memset(chanmode_table, 0, sizeof(chanmode_table));

  mem_static_create(&chanmode_heap, sizeof(struct chanmodechange),
                    CHANMODE_BLOCK_SIZE);
  mem_static_note(&chanmode_heap, "chanmode change heap");

  mem_static_create(&chanmode_item_heap, sizeof(struct chanmodeitem),
                    CHANMODE_BLOCK_SIZE);
  mem_static_note(&chanmode_item_heap, "chanmode modelist heap");

  ircd_support_set("MAXBANS", "%u", IRCD_MAXBANS);
  ircd_support_set("MODES", "%u", IRCD_MODESPERLINE);

  log(chanmode_log, L_status, "Initialised [chanmode] module.");
}

/* -------------------------------------------------------------------------- *
 * Shut down the chanmode module                                              *
 * -------------------------------------------------------------------------- */
void chanmode_shutdown(void)
{
  log(chanmode_log, L_status, "Shutting down [chanmode] module...");

  ircd_support_unset("MODES");
  ircd_support_unset("MAXBANS");

  mem_static_destroy(&chanmode_item_heap);
  mem_static_destroy(&chanmode_heap);

  log_source_unregister(chanmode_log);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct chanmode *chanmode_register(struct chanmode *cmptr)
{
  struct chanmode *mode;

  if(!chars_isalpha(cmptr->letter))
    return NULL;

  mode = &chanmode_table[(uint32_t)cmptr->letter - 0x40];

  if(mode->type != 0)
  {
    log(chanmode_log, L_warning,
        "Channel mode char '%c' already registered.", cmptr->letter);
    return NULL;
  }

  mode->flag = 1LLU << (cmptr->letter - 0x40);
  mode->type = cmptr->type;
  mode->prefix = cmptr->prefix;
  mode->cb = cmptr->cb;
  mode->need = cmptr->need;
  mode->letter = cmptr->letter;
  mode->order = cmptr->order;
  mode->help = cmptr->help;

  if(cmptr->type & CHANMODE_TYPE_PRIVILEGE)
    chanuser_support();
  else
    chanmode_support();

  return mode;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int chanmode_unregister(struct chanmode *cmptr)
{
  struct chanmode *mode;

  if(!chars_isalpha(cmptr->letter))
    return -1;

  mode = &chanmode_table[(uint32_t)cmptr->letter - 0x40];

  if(mode->type == 0)
  {
    log(chanmode_log, L_warning,
        "Channel mode char '%c' not registered.", cmptr->letter);
    return -1;
  }

  if(mode->type & CHANMODE_TYPE_PRIVILEGE)
    chanuser_support();
  else
    chanmode_support();

  mode->flag = 0LLU;
  mode->type = 0;
  mode->prefix = 0;

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct chanmodechange *chanmode_change_add(struct list     *list,
                                           int              what,
                                           char             mode,
                                           char            *arg,
                                           struct chanuser *acuptr)
{
  struct chanmodechange *cmcptr = NULL;

  if(chanmode_table[(uint32_t)mode - 0x40].type)
  {
    cmcptr = mem_static_alloc(&chanmode_heap);

    cmcptr->bounced = 0;
    cmcptr->mode = &chanmode_table[(uint32_t)mode - 0x40];
    cmcptr->what = what;
    cmcptr->nmask = NULL;
    cmcptr->umask = NULL;
    cmcptr->hmask = NULL;

    if(acuptr)
    {
      cmcptr->acptr = acuptr->client;
      cmcptr->target = acuptr;
    }
    else
    {
      cmcptr->acptr = NULL;
      cmcptr->target = NULL;
    }

    cmcptr->ihash = 0;
    cmcptr->info[0] = '\0';
    cmcptr->ts = 0L;

    if(arg)
      strlcpy(cmcptr->arg, arg, sizeof(cmcptr->arg));
    else
      cmcptr->arg[0] = '\0';

    dlink_add_tail(list, &cmcptr->node, cmcptr);
  }

  return cmcptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct chanmodechange *chanmode_change_insert(struct list           *list,
                                              struct chanmodechange *before,
                                              int                    what,
                                              char                   mode,
                                              char                  *arg)
{
  struct chanmodechange *cmcptr = NULL;

  if(chanmode_table[(uint32_t)mode - 0x40].type)
  {
    cmcptr = mem_static_alloc(&chanmode_heap);

    cmcptr->bounced = 0;
    cmcptr->mode = &chanmode_table[(uint32_t)mode - 0x40];
    cmcptr->what = what;
    cmcptr->info[0] = '\0';
    cmcptr->ts = 0L;
    cmcptr->nmask = NULL;
    cmcptr->umask = NULL;
    cmcptr->hmask = NULL;

    cmcptr->acptr = NULL;
    cmcptr->target = NULL;

    if(arg)
      strlcpy(cmcptr->arg, arg, sizeof(cmcptr->arg));
    else
      cmcptr->arg[0] = '\0';

    dlink_add_before(list, &cmcptr->node, &before->node, cmcptr);
  }

  return cmcptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanmode_change_destroy(struct list *list)
{
  struct chanmodechange *cmcptr;
  struct node           *next;

  dlink_foreach_safe(list, cmcptr, next)
  {
    dlink_delete(list, &cmcptr->node);

    mem_static_free(&chanmode_heap, cmcptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct chanmodechange *chanmode_prepare(struct lclient  *lcptr,
                                        struct client   *cptr,
                                        struct channel  *chptr,
                                        int              what,
                                        struct chanmode *mode,
                                        char            *arg,
                                        uint32_t        *aiptr)
{
  struct chanmodechange  change;
  struct chanmodechange *ret;
  uint32_t               ai = *aiptr;

  /* Initialise mode change */
  change.bounced = 0;
  change.mode = mode;
  change.what = what;
  change.arg[0] = '\0';
  change.acptr = NULL;
  change.target = NULL;
  change.nmask = NULL;
  change.umask = NULL;
  change.hmask = NULL;

  /* Now check argument for this mode change */
  switch(mode->type)
  {
    /* Normal mode flags, no argument at all */
    case CHANMODE_TYPE_SINGLE:
    {
      break;
    }
    /* Key and limit mode flags, argument for adding but none for removing */
    case CHANMODE_TYPE_LIMIT:
    case CHANMODE_TYPE_KEY:
    {
      if(what == CHANMODE_ADD)
      {
        /* No argument :/ cancel this mode */
        if(arg == NULL)
        {
          return NULL;
        }
        else
        {
          strlcpy(change.arg, arg, sizeof(change.arg));
          ai++;
        }
      }

      break;
    }
    /* Banlist modes need argument for adding and removing */
    case CHANMODE_TYPE_LIST:
    {
      if(what != CHANMODE_QUERY)
      {
        /* No argument :/ set what to QUERY */
        if(arg == NULL)
        {
          change.what = CHANMODE_QUERY;
        }
        else
        {
          strlcpy(change.arg, arg, sizeof(change.arg));
          ai++;
        }
      }

      break;
    }
    /* Privilege modes need a client as argument */
    case CHANMODE_TYPE_PRIVILEGE:
    {
      if(what != CHANMODE_QUERY)
      {
        /* No argument :/ cancel this mode */
        if(arg == NULL)
        {
          return NULL;
        }
        else
        {
          strlcpy(change.arg, arg, sizeof(change.arg));
          ai++;
        }

        if(change.acptr == NULL)
        {
          if(lclient_is_server(lcptr))
          {
            if(lcptr->caps & CAP_UID)
              change.acptr = client_find_uid(change.arg);
            else
              change.acptr = client_find_nick(change.arg);
          }
          else
          {
            change.acptr = client_find_nickhw(cptr, change.arg);
          }
        }

/*        if(change.acptr == NULL)
          log(chanmode_log, L_warning, "mode argument %s not found!",
              change.arg);*/

        if(change.acptr == NULL)
        {
          numeric_send(cptr, ERR_NOSUCHNICK, change.arg);
          return NULL;
        }

        /* Find the chanlink and report errors */
        change.target =
          chanuser_find(chptr, change.acptr);

        if(change.target == NULL)
          return NULL;
      }

      break;
    }
  }

  *aiptr = ai;

  ret = mem_static_alloc(&chanmode_heap);

  memcpy(ret, &change, sizeof(struct chanmodechange));

  return ret;
}

/* -------------------------------------------------------------------------- *
 * Parse channel mode changes                                                 *
 *                                                                            *
 * <cptr>            remote mode change source                                *
 * <chptr>           channel the mode will be changed on                      *
 * <cuptr>           chanlink of the mode change source                       *
 * <modes>           what+flags                                               *
 * <args>            args                                                     *
 * <changes>         mode change array                                        *
 * <pc>              parse only this many changes                             *
 *                                                                            *
 * if a chanuser is present then <cptr> and <chptr> will be ignored.          *
 * -------------------------------------------------------------------------- */
void chanmode_parse(struct lclient  *lcptr, struct client   *cptr,
                    struct channel  *chptr, struct chanuser *cuptr,
                    struct list     *lptr,  char            *modes,
                    char            *args,  uint32_t         pc)
{
  struct chanmodechange *change;
  struct chanmode       *mode;
  uint32_t               mi;
  uint32_t               ci = 0;
  uint32_t               ai = 0;
  int                    what;
  char                  *argp[CHANMODE_PER_LINE + 1];

  dlink_list_zero(lptr);

  /* Default what is query */
  what = CHANMODE_QUERY;

  /* We got a chanuser */
  if(cuptr)
  {
    cptr = cuptr->client;
    chptr = cuptr->channel;
  }

  /* If we have args then tokenize them */
  if(args && args[0])
  {
    str_tokenize(args, argp, CHANMODE_PER_LINE);
    argp[CHANMODE_PER_LINE] = NULL;
  }
  else
  {
    argp[ai] = NULL;
  }

  /* Walk through mode flag string */
  for(mi = 0; modes[mi]; mi++)
  {
    switch(modes[mi])
    {
      /* What changes? */
      case '=':
      {
        what = CHANMODE_QUERY;
        break;
      }
      case '+':
      {
        what = CHANMODE_ADD;
        break;
      }
      case '-':
      {
        what = CHANMODE_DEL;
        break;
      }
      /* A flag */
      default:
      {
        if(!chars_isalpha(modes[mi]))
        {
          numeric_send(cptr, ERR_UNKNOWNMODE, modes[mi]);
          break;
        }

        /* Get the chanmode entry */
        mode = &chanmode_table[(uint32_t)modes[mi] - 0x40];

        /* Check for valid mode char */
        if(mode->type == 0)
        {
          numeric_send(cptr, ERR_UNKNOWNMODE, modes[mi]);
          break;
        }

        /* Check if the client has the privileges to change this mode */
/*        if(cuptr && what != CHANMODE_QUERY)
        {
          if(!(cuptr->flags & mode->need))
          {
            if(!denied)
            {
              numeric_send(cptr, ERR_CHANOPRIVSNEEDED, chptr->name);
              denied++;
            }
            break;
          }
        }*/

        change = chanmode_prepare(lcptr, cptr, chptr, what,
                                  mode, argp[ai], &ai);
        if(change)
          dlink_add_tail(lptr, &change->node, change);

        if(lptr->size == IRCD_MODESPERLINE)
          return;

        break;
      }
    }

    if(ci == CHANMODE_PER_LINE || ci == pc)
      break;
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
uint32_t chanmode_flags_build(char *dst, int types, uint64_t flags)
{
  uint32_t i;
  uint32_t di = 0;

  for(i = 0; i < 0x40; i++)
  {
    if(types & chanmode_table[i].type)
    {
      if(flags & chanmode_table[i].flag)
        dst[di++] = chanmode_table[i].letter;
    }
  }

  dst[di] = '\0';

  return di;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
uint32_t chanmode_args_build(char *dst, struct channel *chptr)
{
  uint32_t i;
  uint32_t di = 0;

  for(i = 0; i < 0x40; i++)
  {
    if(CHANMODE_TYPE_KEY & chanmode_table[i].type)
    {
      if(chptr->modes & chanmode_table[i].flag)
        hooks_call(chanmode_args_build, HOOK_DEFAULT, dst, chptr,
                   &di, &chanmode_table[i].flag);
    }
  }

  dst[di] = '\0';

  return di;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#ifdef DEBUG
void chanmode_changes_dump(struct list *lptr)
{
  struct chanmodechange *cmcptr;

  debug(chanmode_log, "--------- dumping modechanges ---------");

  dlink_foreach(lptr, cmcptr)
  {
    if(cmcptr->mode)
    {
      debug(chanmode_log, "%c%c %-20s (bounced: %u)",
            (cmcptr->what == CHANMODE_QUERY ? '=' :
             cmcptr->what == CHANMODE_ADD ? '+' : '-'),
            cmcptr->mode->letter, cmcptr->arg, cmcptr->bounced);
    }
  }
  debug(chanmode_log, "---------- end of modechanges ---------");
}
#endif /* DEBUG */

/* -------------------------------------------------------------------------- *
 * Apply channel mode changes                                                 *
 *                                                                            *
 * <lcptr>           local mode change source                                 *
 * <cptr>            remote mode change source                                *
 * <chptr>           channel the mode will be changed on                      *
 * <cuptr>           chanlink of the mode change source                       *
 * <changes>         mode change array                                        *
 * <n>               apply this many changes                                  *
 *                                                                            *
 * if a chanuser is present then <cptr> and <chptr> will be ignored.          *
 * -------------------------------------------------------------------------- */
uint32_t chanmode_apply(struct lclient        *lcptr,  struct client   *cptr,
                        struct channel        *chptr,  struct chanuser *cuptr,
                        struct list           *lptr)
{
  struct chanmodechange *cmcptr;
  uint32_t               ret = 0;
  int                    denied = 0;
  int                    full = 0;

  if(cuptr)
  {
    cptr = cuptr->client;
    chptr = cuptr->channel;
  }

  /* Loop through the mode changes and fire the callbacks */
  dlink_foreach(lptr, cmcptr)
  {
    /* Check if the client has the privileges to change this mode */
    if(cuptr && cmcptr->what != CHANMODE_QUERY)
    {
      if(!(cuptr->flags & cmcptr->mode->need) &&
         client_is_user(cptr) && client_is_local(cptr))
      {
        if(!denied)
        {
          numeric_send(cptr, ERR_CHANOPRIVSNEEDED, cuptr->channel->name);
          denied++;
        }

        cmcptr->bounced++;

        continue;
      }
    }

    if(cuptr == NULL && cmcptr->what != CHANMODE_QUERY && client_is_user(cptr))
    {
      log(chanmode_log, L_warning, "%N (%U@%H) enforces mode %c%c on %s.",
          cptr, cptr, cptr, (cmcptr->what == CHANMODE_ADD ? '+' : '-'),
          cmcptr->mode->letter, chptr->name);
    }

    switch(cmcptr->mode->cb(lcptr, cptr, chptr, cuptr, lptr, cmcptr))
    {
      case -2:
      {
        if(!full)
        {
          if(client_is_user(cptr) && client_is_local(cptr))
            numeric_send(cptr, ERR_BANLISTFULL, chptr->name, cmcptr->arg);

          full++;
        }

        cmcptr->bounced++;
        continue;
      }

      /* Permission denied from inside of the callback */
      case -1:
      {
        if(!denied)
        {
          if(client_is_user(cptr) && client_is_local(cptr))
            numeric_send(cptr, ERR_CHANOPRIVSNEEDED, cuptr->channel->name);

          denied++;
        }

        cmcptr->bounced++;
        continue;
      }
      /* Mode has been bounced inside callback */
      case 1:
      {
        cmcptr->bounced++;
        continue;
      }
      /* Everything is fine, but filter out mode queries */
      default:
      {
        if(cmcptr->what == CHANMODE_QUERY && !cmcptr->bounced)
        {
          if(cmcptr->mode->type == CHANMODE_TYPE_LIST)
            chanmode_list(cptr, chptr, cmcptr->mode->letter);

          cmcptr->bounced++;
          continue;
        }
        ret++;
      }
    }
  }

  debug(chanmode_log, "%u got through bounces", ret);

  return ret;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct node *chanmode_assemble(char        *modebuf, char  *parabuf,
                               struct node *nptr,    size_t n,
                               size_t       count,   int    uid)
{
  struct chanmodechange *cmcptr;
  char                  *arg;
  size_t                 mi;
  size_t                 pi;
  size_t                 i;
  size_t                 len;
  int                    what = -1;

  if(nptr == NULL)
    return NULL;

  mi = 0;
  pi = 0;
  i = 0;

  do
  {
    cmcptr = nptr->data;

    if(cmcptr->bounced)
      continue;

    if(cmcptr->acptr)
    {
      if(uid)
        arg = cmcptr->acptr->user->uid;
      else
        arg = cmcptr->acptr->name;
    }
    else
    {
      arg = cmcptr->arg;
    }

    if(cmcptr->mode->type != CHANMODE_TYPE_SINGLE && arg)
    {
      len = str_len(arg);

      if(pi + len + 2 > n)
        break;

      if(pi)
        parabuf[pi++] = ' ';

      strcpy(&parabuf[pi], arg);
      pi += len;
    }

    if(what != cmcptr->what)
    {
      modebuf[mi++] = (cmcptr->what == CHANMODE_ADD ? '+' : '-');
      what = cmcptr->what;
    }

    modebuf[mi++] = cmcptr->mode->letter;
    i++;
  }
  while((nptr = nptr->next) && i < count && i < CHANMODE_PER_LINE);

  modebuf[mi] = '\0';
  parabuf[pi] = '\0';

  return nptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanmode_send_local(struct client *cptr, struct channel *chptr,
                         struct node   *nptr, size_t          n)
{
  struct node *node;
  size_t       len;
  size_t       arglen;
  char         cmd[IRCD_LINELEN - 1];
  char         args[IRCD_LINELEN - 1];
  char         flags[(CHANMODE_PER_LINE * 2) + 1];

  if(client_is_user(cptr))
  {
    len = str_snprintf(cmd, sizeof(cmd), ":%s!%s@%s MODE %s",
                   cptr->name, cptr->user->name, cptr->host,
                   chptr->name);
  }
  else
  {
    len = str_snprintf(cmd, sizeof(cmd), ":%s MODE %s",
                   cptr->name, chptr->name);
  }

  arglen = (IRCD_LINELEN - 2) - len - ((CHANMODE_PER_LINE * 2) + 1) - 3;

  for(node = nptr; node;)
  {
    node = chanmode_assemble(flags, args, node, arglen, n, 0);

    if(flags[0])
    {
      if(args[0])
      {
        channel_send(NULL, chptr, CHFLG(NONE), CHFLG(NONE),
                     "%s %s %s", cmd, flags, args);
      }
      else
      {
        channel_send(NULL, chptr, CHFLG(NONE), CHFLG(NONE),
                     "%s %s", cmd, flags);
      }
    }
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanmode_send_remote(struct lclient *lcptr, struct client *cptr,
                          struct channel *chptr, struct node   *nptr)
{
  char         cmd[IRCD_LINELEN - 1];
  char         args[IRCD_LINELEN - 1];
  char         flags[(CHANMODE_PER_LINE * 2) + 1];
  size_t       len;
  size_t       arglen;
  struct node *node;

  if(client_is_user(cptr))
  {
    len = str_snprintf(cmd, sizeof(cmd), ":%s MODE %s",
                   cptr->user->uid, chptr->name);
  }
  else
  {
    len = str_snprintf(cmd, sizeof(cmd), ":%s MODE %s",
                   cptr->name, chptr->name);
  }

  arglen = (IRCD_LINELEN - 2) - len - ((CHANMODE_PER_LINE * 2) + 1) - 3;

  for(node = nptr; node;)
  {
    node = chanmode_assemble(flags, args, node, arglen,
                             CHANMODE_PER_LINE, CAP_UID);
    if(flags[0])
    {
      if(args[0])
        server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                     "%s %s %s", cmd, flags, args);
      else
        server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                     "%s %s", cmd, flags);
    }
  }

  if(client_is_user(cptr))
  {
    len = str_snprintf(cmd, sizeof(cmd), ":%s MODE %s",
                   cptr->name, chptr->name);
    arglen = (IRCD_LINELEN - 2) - len - ((CHANMODE_PER_LINE * 2) + 1) - 3;
  }

  for(node = nptr; node;)
  {
    node = chanmode_assemble(flags, args, node, arglen,
                             CHANMODE_PER_LINE, CAP_NONE);
    if(flags[0])
    {
      if(args[0])
      {
        server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                    "%s %s %s", cmd, flags, args);
      }
      else
      {
        server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                    "%s %s", cmd, flags);
      }
    }
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int chanmode_bounce_simple(struct lclient *lcptr, struct client         *cptr,
                           struct channel *chptr, struct chanuser       *cuptr,
                           struct list    *lptr,  struct chanmodechange *cmcptr)
{
  struct chanmode *mode   = cmcptr->mode;

  if(cmcptr->what == CHANMODE_DEL)
  {
    if(!(chptr->modes & mode->flag))
      return 1;

    chptr->modes &= ~mode->flag;
  }

  if(cmcptr->what == CHANMODE_ADD)
  {
    if((chptr->modes & mode->flag))
      return 1;

    chptr->modes |= mode->flag;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int chanmode_bounce_ban(struct lclient *lcptr, struct client         *cptr,
                        struct channel *chptr, struct chanuser       *cuptr,
                        struct list    *lptr,  struct chanmodechange *cmcptr)
{
  uint32_t i;
  char     nick[IRCD_NICKLEN + 1];
  char     user[IRCD_USERLEN + 1];
  char     host[IRCD_HOSTLEN + 1];
  char    *sep1;
  char    *sep2;

  if(cmcptr->what == CHANMODE_ADD)
  {
    sep1 = str_chr(cmcptr->arg, '!');
    sep2 = str_chr(cmcptr->arg, '@');

    nick[0] = '*';
    user[0] = '*';
    host[0] = '*';
    nick[1] = '\0';
    user[1] = '\0';
    host[1] = '\0';

    if(sep2 < sep1)
    {
      *sep1 = '\0';
      sep1 = NULL;
    }

    if(sep1 == NULL && sep2 == NULL)
    {
      if(cmcptr->arg[0])
        strlcpy(nick, cmcptr->arg, sizeof(nick));
    }
    else if(sep1 == NULL)
    {
      *sep2++ = '\0';

      if(cmcptr->arg[0])
        strlcpy(user, cmcptr->arg, sizeof(user));

      if((sep1 = str_chr(cmcptr->arg, '@')))
        *sep1 = '\0';

      if(sep2[0])
        strlcpy(host, sep2, sizeof(host));
    }
    else if(sep2 == NULL)
    {
      *sep1++ = '\0';

      if(cmcptr->arg[0])
        strlcpy(nick, cmcptr->arg, sizeof(nick));

      if((sep2 = str_chr(cmcptr->arg, '!')))
        *sep2 = '\0';

      if(sep1[0])
        strlcpy(user, sep1, sizeof(user));
    }
    else
    {
      char *tmp;

      *sep1++ = '\0';

      if(cmcptr->arg[0])
        strlcpy(nick, cmcptr->arg, sizeof(nick));

      *sep2++ = '\0';

      if((tmp = str_chr(sep1, '!')))
        *tmp = '\0';

      if(sep1[0])
        strlcpy(user, sep1, sizeof(user));

      if((tmp = str_chr(sep2, '@')))
        *tmp = '\0';

      if((tmp = str_chr(sep2, '!')))
        *tmp = '\0';

      if(sep2[0])
        strlcpy(host, sep2, sizeof(host));
    }

    for(i = 0; nick[i]; i++)
    {
      if(!chars_isnickchar(nick[i]) && !chars_iskwildchar(nick[i]))
      {
        if(i == 0)
          nick[i++] = '*';

        nick[i] = '\0';
        break;
      }
    }

    for(i = 0; user[i]; i++)
    {
      if(!chars_isuserchar(user[i]) && !chars_iskwildchar(user[i]))
      {
        if(i == 0)
          user[i++] = '*';

        user[i] = '\0';
        break;
      }
    }

    for(i = 0; host[i]; i++)
    {
      if(!chars_ishostchar(host[i]) && !chars_iskwildchar(host[i]))
      {
        if(i == 0)
          host[i++] = '*';

        host[i] = '\0';
        break;
      }
    }

    str_snprintf(cmcptr->arg, sizeof(cmcptr->arg), "%s!%s@%s", nick, user, host);

    cmcptr->nmask = nick;
    cmcptr->umask = user;
    cmcptr->hmask = host;
  }

  return chanmode_bounce_mask(lcptr, cptr, chptr, cuptr, lptr, cmcptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int chanmode_bounce_mask(struct lclient *lcptr, struct client         *cptr,
                         struct channel *chptr, struct chanuser       *cuptr,
                         struct list    *lptr,  struct chanmodechange *cmcptr)
{
  struct chanmodeitem *cmiptr;
  struct chanmodeitem *next;
  uint32_t             index;

  index = (uint32_t)(cmcptr->mode->letter - 0x40);

  if(cmcptr->what == CHANMODE_ADD)
  {
    dlink_foreach_safe(&chptr->modelists[index], cmiptr, next)
    {
      /* new mask is matched by a mask in list, bounce it! */
      if(str_match(cmcptr->arg, cmiptr->mask))
        return 1;

      /* a mask in list is matched by the new mask, drop old mask */
      if(str_match(cmiptr->mask, cmcptr->arg))
      {
        chanmode_change_insert(lptr, cmcptr, CHANMODE_DEL,
                               cmcptr->mode->letter, cmiptr->mask);
        chanmode_mask_delete(&chptr->modelists[index], cmiptr);
      }
    }

    return chanmode_mask_add(cptr, &chptr->modelists[index], cmcptr);
  }
  else if(cmcptr->what == CHANMODE_DEL)
  {
    dlink_foreach_safe(&chptr->modelists[index], cmiptr, next)
    {
      if(str_match(cmcptr->arg, cmiptr->mask) ||
         str_match(cmiptr->mask, cmcptr->arg))
      {
        chanmode_change_insert(lptr, cmcptr, CHANMODE_DEL,
                               cmcptr->mode->letter, cmiptr->mask);
        chanmode_mask_delete(&chptr->modelists[index], cmiptr);
      }
    }

    return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int chanmode_match_ban(struct client *cptr, struct channel *chptr,
                       struct list   *mlptr)
{
  struct chanmodeitem *cmiptr;

  dlink_foreach(mlptr, cmiptr)
  {
    if(!str_match(cptr->name, cmiptr->nmask))
      continue;

    if(cptr->user)
    {
      if(!str_match(cptr->user->name, cmiptr->umask))
        continue;
    }

    if(str_match(cptr->host, cmiptr->hmask))
      return 1;

    if(str_match(cptr->hostreal, cmiptr->hmask))
      return 1;

    if(str_match(cptr->hostip, cmiptr->hmask))
      return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int chanmode_match_amode(struct client *cptr, struct channel *chptr,
                         struct list   *mlptr)
{
  struct chanmodeitem *cmiptr;

  dlink_foreach(mlptr, cmiptr)
  {
    if(!str_match(cptr->name, cmiptr->nmask))
      continue;

    if(cptr->user)
    {
      if(!str_match(cptr->user->name, cmiptr->umask))
        continue;
    }

    if(str_match(cptr->hostreal, cmiptr->hmask))
      return 1;

    if(str_match(cptr->hostip, cmiptr->hmask))
      return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int chanmode_match_deny(struct client *cptr, struct channel *chptr,
                        struct list   *mlptr)
{
  struct chanmodeitem *cmiptr;

  dlink_foreach(mlptr, cmiptr)
  {
    if(str_match(cptr->info, cmiptr->mask))
      return 1;
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int chanmode_mask_add(struct client         *cptr,   struct list *mlptr,
                      struct chanmodechange *cmcptr)
{
  struct chanmodeitem *acmiptr;
  struct chanmodeitem *cmiptr;

  if(mlptr->size == IRCD_MAXBANS)
    return -2;

  cmiptr = mem_static_alloc(&chanmode_item_heap);

  if(cptr->user)
  {
    str_snprintf(cmiptr->info, sizeof(cmiptr->info), "%s!%s@%s",
             cptr->name, cptr->user->name, cptr->host);
    cmiptr->ts = timer_systime;
  }
  else
  {
    if(cmcptr->info[0])
    {
      strlcpy(cmiptr->info, cmcptr->info, sizeof(cmiptr->info));
      cmiptr->ts = cmcptr->ts;
    }
    else
    {

      strlcpy(cmiptr->info, cptr->name, sizeof(cmiptr->info));
      cmiptr->ts = timer_systime;
    }
  }

  strlcpy(cmiptr->mask, cmcptr->arg, sizeof(cmiptr->mask));

  if(cmcptr->nmask)
    strlcpy(cmiptr->nmask, cmcptr->nmask, sizeof(cmiptr->nmask));
  else
    cmiptr->nmask[0] = '\0';

  if(cmcptr->umask)
    strlcpy(cmiptr->umask, cmcptr->umask, sizeof(cmiptr->umask));
  else
    cmiptr->umask[0] = '\0';

  if(cmcptr->hmask)
    strlcpy(cmiptr->hmask, cmcptr->hmask, sizeof(cmiptr->hmask));
  else
    cmiptr->hmask[0] = '\0';

  cmiptr->ihash = str_hash(cmiptr->info);

  dlink_foreach(mlptr, acmiptr)
  {
    if(acmiptr->ihash == cmiptr->ihash)
    {
      if(acmiptr->node.next == NULL)
        break;

      if(((struct chanmodeitem *)acmiptr->node.next->data)->ihash !=
         cmiptr->ihash)
        break;
    }
  }

  if(acmiptr)
    dlink_add_after(mlptr, &cmiptr->node, &acmiptr->node, cmiptr);
  else
    dlink_add_tail(mlptr, &cmiptr->node, cmiptr);

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanmode_mask_delete(struct list *mlptr, struct chanmodeitem *cmiptr)
{
  dlink_delete(mlptr, &cmiptr->node);
  mem_static_free(&chanmode_item_heap, cmiptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanmode_prefix_make(char *pfx, uint64_t flags)
{
  uint32_t         i;
  uint32_t         di = 0;
  uint32_t         current;
  uint32_t         top = (uint32_t)-1;
  struct chanmode *mode;

  for(;;)
  {
    current = 0;
    mode = NULL;

    for(i = 0; i < 64; i++)
    {
      if(chanmode_table[i].type != CHANMODE_TYPE_PRIVILEGE)
        continue;

      if(chanmode_table[i].order < top &&
         chanmode_table[i].order > current &&
         chanmode_table[i].flag & flags)
      {
        mode = &chanmode_table[i];
        current = mode->order;
      }
    }

    if(mode == NULL)
      break;

    top = current;

/*    if(type == CHANMODE_FLAG_LETTER)
      pfx[di++] = mode->letter;
    else*/
      pfx[di++] = mode->prefix;
  }

  pfx[di] = '\0';
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanmode_changes_make(struct list *list, int what, struct chanuser *cuptr)
{
  uint32_t         i;
  uint32_t         current;
  uint32_t         top = (uint32_t)-1;
  struct chanmode *mode;

  for(;;)
  {
    current = 0;
    mode = NULL;

    for(i = 0; i < 64; i++)
    {
      if(chanmode_table[i].type != CHANMODE_TYPE_PRIVILEGE)
        continue;

      if(chanmode_table[i].order < top &&
         chanmode_table[i].order > current &&
         chanmode_table[i].flag & cuptr->flags)
      {
        mode = &chanmode_table[i];
        current = mode->order;
      }
    }

    if(mode == NULL)
      break;

    top = current;

    chanmode_change_add(list, what, mode->letter, NULL, cuptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
uint64_t chanmode_prefix_parse(const char *pfx)
{
  size_t   i;
  size_t   j;
  uint64_t ret = 0LLU;

  for(i = 0; pfx[i]; i++)
  {
    if(pfx[i] == ':')
      break;

    for(j = 0; j < sizeof(chanmode_table) / sizeof(chanmode_table[0]); j++)
    {
      if(chanmode_table[j].type == CHANMODE_TYPE_PRIVILEGE)
      {
        if(chanmode_table[j].prefix == pfx[i])
        {
          ret |= chanmode_table[j].flag;
          break;
        }
      }
    }

    if(j == sizeof(chanmode_table) / sizeof(chanmode_table[0]))
      break;
  }

  return ret;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanmode_show(struct client  *cptr, struct channel *chptr)
{
  char     mbuf[64 + 2];
  char     abuf[IRCD_KEYLEN * 8 + 1];
  uint32_t di = 0;

  mbuf[di++] = '+';
  abuf[0] = '\0';

  if(chptr->modes)
  {
    di += chanmode_flags_build(&mbuf[di], CHANMODE_TYPE_SINGLE, chptr->modes);
    chanmode_flags_build(&mbuf[di], CHANMODE_TYPE_KEY, chptr->modes);
    chanmode_args_build(abuf, chptr);
  }
  else
    mbuf[1] = '\0';

  if(abuf[0])
  {
    strlcat(mbuf, " ", sizeof(mbuf));
    strlcat(mbuf, abuf, sizeof(abuf));
  }

  numeric_send(cptr, RPL_CHANNELMODEIS, chptr->name, mbuf);
  numeric_send(cptr, RPL_CREATIONTIME, chptr->name, chptr->ts);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanmode_list(struct client *cptr, struct channel *chptr, char c)
{
  struct chanmode     *mode;
  uint32_t             numeric;
  uint32_t             index = c - 0x40;
  struct chanmodeitem *cmiptr;

  mode = &chanmode_table[index];

  numeric = mode->order;

  dlink_foreach(&chptr->modelists[index], cmiptr)
    numeric_send(cptr, numeric, chptr->name,
                 cmiptr->mask, cmiptr->info, cmiptr->ts);

  numeric_send(cptr, numeric + 1, chptr->name);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct node *chanmode_assemble_list(char *buf, struct node *nptr, size_t len)
{
  struct chanmodechange *cmcptr;
  char                   flagbuf[0x40 + 1];
  char                   infobuf[IRCD_LINELEN - 1];
  char                   timebuf[IRCD_LINELEN - 1];
  char                   maskbuf[IRCD_LINELEN - 1];
  size_t                 flen = 0;
  size_t                 ilen = 0;
  size_t                 tlen = 0;
  size_t                 mlen = 0;
  hash_t                 lasthash = 0;
  time_t                 lastts = 0;

  flagbuf[flen] = '\0';
  infobuf[ilen] = '\0';
  timebuf[tlen] = '\0';
  maskbuf[mlen] = '\0';

  do
  {
    cmcptr = nptr->data;

    if(/*cmcptr->bounced ||*/ cmcptr->what != CHANMODE_ADD)
      continue;

    if(lasthash && cmcptr->info && lasthash == cmcptr->ihash)
    {
      if(flen + ilen + tlen + mlen + 2 +
         1 + 2 + 12 + str_len(cmcptr->arg) + 2 + 1 > len ||
         flen == CHANMODE_PER_LINE)
        break;

      infobuf[ilen++] = ';';
      infobuf[ilen++] = '*';
    }
    else
    {
      if(flen + ilen + tlen + mlen + 2 +
         (cmcptr->info ? str_len(cmcptr->info) : 1)
         + 2 + 12 + str_len(cmcptr->arg) + 2 + 1 > len ||
         flen == CHANMODE_PER_LINE)
        break;

      if(ilen)
        infobuf[ilen++] = ';';

      if(cmcptr->info[0])
      {
        ilen += strlcpy(&infobuf[ilen], cmcptr->info,
                        sizeof(infobuf) - ilen - 1);
        lasthash = cmcptr->ihash;
      }
      else
      {
        infobuf[ilen++] = '-';
      }
    }

    flagbuf[flen++] = cmcptr->mode->letter;

    if(tlen)
      timebuf[tlen++] = ';';

    if(cmcptr->ts)
    {
      if(lastts)
      {
        tlen += str_snprintf(&timebuf[tlen], sizeof(timebuf) - tlen - 1,
                         "%li", (long)(cmcptr->ts - lastts));
      }
      else
      {
        tlen += str_snprintf(&timebuf[tlen], sizeof(timebuf) - tlen - 1,
                         "%lu", (unsigned long)(cmcptr->ts));
        lastts = cmcptr->ts;
      }
    }
    else
    {
      timebuf[tlen++] = '-';
    }

    if(mlen)
      maskbuf[mlen++] = ';';

    if(cmcptr->arg[0])
      mlen += strlcpy(&maskbuf[mlen], cmcptr->arg, sizeof(maskbuf) - mlen - 1);
    else
      maskbuf[mlen++] = '-';
  }
  while((nptr = nptr->next));

  if(flen)
  {
    flagbuf[flen] = '\0';
    infobuf[ilen] = '\0';
    timebuf[tlen] = '\0';
    maskbuf[mlen] = '\0';

    str_snprintf(buf, len, "%s %s %s %s", flagbuf, infobuf, timebuf, maskbuf);
  }

  return nptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanmode_introduce(struct lclient *lcptr, struct client *cptr,
                        struct channel *chptr, struct node   *nptr)
{
  char   buf[IRCD_LINELEN - 1];
  size_t len;

  len = str_snprintf(buf, sizeof(buf), ":%s NMODE %s %lu ",
                 cptr->name, chptr->name, (unsigned long)(chptr->ts));

  for(; nptr;)
  {
    nptr = chanmode_assemble_list(&buf[len], nptr, sizeof(buf) - len - 1);

    if(buf[len])
      server_send(lcptr, NULL, CAP_NONE, CAP_NONE, "%s", buf);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
size_t chanmode_burst(struct lclient *lcptr, struct channel *chptr)
{
  struct chanmodechange *cmcptr;
  struct chanmodeitem   *cmiptr;
  struct list            modelist;
  struct node           *nptr;
  size_t                 i;
  size_t                 len;
  char                   buf[IRCD_LINELEN - 1];

  debug(chanmode_log, "Bursting chanmodes for %s to %s",
        chptr->name, lcptr->name);

  dlink_list_zero(&modelist);

  for(i = 0; i < 0x40; i++)
  {
    struct chanmode *mode = &chanmode_table[i];

    if(mode->type == CHANMODE_TYPE_SINGLE)
    {
      if(chptr->modes & mode->flag)
        chanmode_change_add(&modelist, CHANMODE_ADD, i + 0x40, NULL, NULL);
    }
  }

  for(i = 0; i < 0x40; i++)
  {
    if(chanmode_table[i].type != CHANMODE_TYPE_LIST)
      continue;

    dlink_foreach(&chptr->modelists[i], cmiptr)
    {
      cmcptr = chanmode_change_add(&modelist, CHANMODE_ADD,
                                   i + 0x40, cmiptr->mask, NULL);
      strlcpy(cmcptr->info, cmiptr->info, sizeof(cmcptr->info));
      cmcptr->ts = cmiptr->ts;
      cmcptr->ihash = cmiptr->ihash;
    }
  }

  len = str_snprintf(buf, sizeof(buf), "NMODE %s %lu ",
                  chptr->name, (unsigned long)(chptr->ts));

  for(nptr = modelist.head; nptr;)
  {
    nptr = chanmode_assemble_list(&buf[len], nptr, sizeof(buf) - len - 1);

    if(buf[len])
      lclient_send(lcptr, "%s", buf);
  }

  len = modelist.size;

  chanmode_change_destroy(&modelist);

  return len;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanmode_drop(struct client *cptr, struct channel *chptr)
{
  struct chanmodechange *cmcptr;
  struct chanmodeitem   *cmiptr;
  struct list            modelist;
  size_t                 i;

  dlink_list_zero(&modelist);

  for(i = 0; i < 0x40; i++)
  {
    struct chanmode *mode = &chanmode_table[i];

    if(mode->type == CHANMODE_TYPE_SINGLE && (chptr->modes & mode->flag))
      chanmode_change_add(&modelist, CHANMODE_DEL, i + 0x40, NULL, NULL);
  }

  chptr->modes = 0LLU;

  for(i = 0; i < 0x40; i++)
  {
    if(chanmode_table[i].type != CHANMODE_TYPE_LIST)
      continue;

    dlink_foreach(&chptr->modelists[i], cmiptr)
    {
      cmcptr = chanmode_change_add(&modelist, CHANMODE_DEL,
                                   i + 0x40, cmiptr->mask, NULL);
      mem_static_free(&chanmode_item_heap, cmiptr);
    }

    dlink_list_zero(&chptr->modelists[i]);
  }

  chanmode_send_local(cptr, chptr, modelist.head, IRCD_MODESPERLINE);

  chanmode_change_destroy(&modelist);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanmode_support(void)
{
  char modes[129];

  uint32_t i;
  uint32_t di = 0;

  for(i = 0; i < 0x40; i++)
  {
    if(chanmode_table[i].type & CHANMODE_TYPE_NONPRIV)
    {
      modes[di++] = chanmode_table[i].letter;

      if(!(chanmode_table[i].type & CHANMODE_TYPE_SINGLE))
        modes[di++] = ',';
    }
  }

  modes[di] = '\0';

  ircd_support_set("CHANMODES", "%s", modes);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
