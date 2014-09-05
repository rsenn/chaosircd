/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003-2006  Manuel Kohler
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
 * $Id: usermode.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#define _GNU_SOURCE

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "defs.h"
#include "io.h"
#include "dlink.h"
#include "log.h"
#include "mem.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/usermode.h>
#include <ircd/numeric.h>
#include <ircd/lclient.h>
#include <ircd/client.h>
#include <ircd/server.h>
#include <ircd/chars.h>
#include <ircd/user.h>

/* -------------------------------------------------------------------------- *
 * Defines                                                                    *
 * -------------------------------------------------------------------------- */
#define USERMODE_LIST(c) (usermode_table[USERMODE_CHARTOINDEX(c)]->list)
#define USERMODE_NOLIST -1
#define USERMODE_VALID(c) (((c) >= 'A' && (c) <= 'Z') || \
                            ((c) >= 'a' && (c) <= 'z'))

void usermode_linkall_mode   (struct usermode *umptr);
void usermode_unlinkall_mode (struct usermode *umptr);

/* -------------------------------------------------------------------------- *
 * Globals                                                                    *
 * -------------------------------------------------------------------------- */
int usermode_log;

/* -------------------------------------------------------------------------- *
 * Globals                                                                    *
 * -------------------------------------------------------------------------- */
struct usermode *usermode_table[USERMODE_TABLE_SIZE];
struct list      usermode_list;
struct sheap     usermodechange_heap;
struct list      usermodechange_list;

/* ------------------------------------------------------------------------ */
int usermode_get_log() { return usermode_log; }

/* -------------------------------------------------------------------------- *
 * Initialize the usermode module                                             *
 * -------------------------------------------------------------------------- */
void usermode_init(void)
{
  usermode_log = log_source_register("usermode");


  mem_static_create(&usermodechange_heap, sizeof(struct usermodechange),
                    USERMODE_BLOCK_SIZE);
  mem_static_note(&usermodechange_heap, "usermode modechange heap");

  dlink_list_zero(&usermode_list);
  dlink_list_zero(&usermodechange_list);

  log(usermode_log, L_status, "Initialised [usermode] module");
}

/* -------------------------------------------------------------------------- *
 * Shut down the usermode module                                              *
 * -------------------------------------------------------------------------- */
void usermode_shutdown(void)
{
  log(usermode_log, L_status, "Shutting down [usermode] module...");

  mem_static_destroy(&usermodechange_heap);

  log_source_unregister(usermode_log);
}

/* -------------------------------------------------------------------------- *
 * register a usermode from a module                                          *
 * -------------------------------------------------------------------------- */
int
usermode_register(struct usermode *umptr)
{
  int table_index;

  if(!USERMODE_VALID(umptr->character))
  {
    log(usermode_log, L_warning, "usermode register failed: "
        "invalid usermode %c", umptr->character);

    return -1;
  }

  /* get the table index */
  table_index = (uint32_t)umptr->character - 0x40;

  /* is this usermode already registered? */
  if(usermode_table[table_index] != NULL)
  {
    log(usermode_log, L_warning, "usermode register failed: "
        "%c already registered", umptr->character);

    return -1;
  }

  /* register new usermode! */

  /* add user modeto usermode_table */
  usermode_table[table_index] = umptr;

  /* link new usermode to the usermode list */
  dlink_node_zero(&umptr->node);
  dlink_add_tail(&usermode_list, &umptr->node, umptr);

  debug(usermode_log, "usermode %c registered", umptr->character);

  /* set default vaules */

  /* set the flag for struct user->modes */
  umptr->flag = 1ULL << table_index;

  /* clear list of users kept for this usermode */
  dlink_list_zero(&umptr->list);

  /* link users to the list (list_mode = USERMODE_LIST_OFF) */
  usermode_linkall_mode(umptr);

  usermode_support();

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void usermode_unregister(struct usermode *umptr)
{
  int table_index;

  /* get the index of this usermode in the table */
  table_index = (uint32_t)umptr->character - 0x40;

  if(usermode_table[table_index] == NULL)
  {
    log(usermode_log, L_warning, "usermode unregister failed: not registered");
    return;
  }

  /* delete usermode to usermode_table */
  usermode_table[table_index] = NULL;

  /* unlink usermode from usermode list */
  dlink_delete(&usermode_list, &umptr->node);

  /* unlink all users from this mode's list */
  usermode_unlinkall_mode(umptr);

  usermode_support();

  log(usermode_log, L_debug, "usermode %c unregistered", umptr->character);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void usermode_assemble(uint64_t modes, char *umbuf)
{
  struct node     *n;
  struct usermode *umptr;
  char            *p = umbuf;

  dlink_foreach_data(&usermode_list, n, umptr)
    if(umptr->flag & modes)
      *p++ = umptr->character;

  *p = '\0';
}

/* -------------------------------------------------------------------------- *
 * Answer to a user mode request                                              *
 * -------------------------------------------------------------------------- */
void usermode_show(struct client *cptr)
{
  numeric_send(cptr, RPL_UMODEIS, cptr->user->mode);
}


/* -------------------------------------------------------------------------- *
 * Removes all usermode changes from the usermodechange_list and free the memory    *
 * -------------------------------------------------------------------------- */
void usermode_change_delete(struct usermodechange *umcptr)
{
  dlink_delete(&usermodechange_list, &umcptr->node);

  mem_static_free(&usermodechange_heap, umcptr);
}

/* -------------------------------------------------------------------------- *
 * Add a usermode change to the usermode_heap                                 *
 * -------------------------------------------------------------------------- */
void usermode_change_add(struct usermode *umptr, int change, char *arg)
{
  struct node *node;
  struct node *next;
  struct usermodechange *umcptr;

  /* go through the list of changes and check for similar ones */
  dlink_foreach_safe_data(&usermodechange_list, node, next, umcptr)
  {
    if(umcptr->mode != umptr)
      continue;

    if(umcptr->arg != NULL && arg != NULL)
    {
      if(str_icmp(umcptr->arg, arg))
        continue;
    }
    else if(umcptr->arg != arg)
      continue;

    /* remove this if there's already the opposite */
    dlink_delete(&usermodechange_list, node);

    mem_static_free(&usermodechange_heap, umcptr);

    break;
  }

  umcptr = mem_static_alloc(&usermodechange_heap);

  umcptr->mode   = umptr;
  umcptr->change = change;
  umcptr->arg    = arg;

  dlink_add_tail(&usermodechange_list, &umcptr->node, umcptr);
}

/* -------------------------------------------------------------------------- *
 * Removes all usermode changes from the usermodechange_list and free the memory    *
 * -------------------------------------------------------------------------- */
void usermode_change_destroy(void)
{
  struct node *node;
  struct node *next;
  struct chanmodechange *umcptr;

  dlink_foreach_safe_data(&usermodechange_list, node, next, umcptr)
  {
    dlink_delete(&usermodechange_list, node);
    mem_static_free(&usermodechange_heap, umcptr);
  }
}

/* -------------------------------------------------------------------------- *
 * Parse a user mode buffer                                                   *
 * -------------------------------------------------------------------------- */
int
usermode_parse(uint64_t       modes, char    **args,
               struct client *cptr,  uint32_t  flags)
{
  /* local variables to parse modes */
  struct usermode *usermode;
  char            *p;
  int              change           = USERMODE_ON;
  char           **arg;
  char            *singlearg[2];
  char            *changearg;

  /* destroy old changes */
  usermode_change_destroy();

  if(flags & USERMODE_OPTION_SINGLEARG)
  {
    singlearg[0] = *args;
    singlearg[1] = NULL;
    arg = singlearg;
  }
  else
    arg = args;

  for(; *arg != NULL; arg++)
  {
    for(p = *arg; *p != '\0'; p++)
    {
      if(*p == '+')
      {
        change = USERMODE_ON;
        continue;
      }

      if(*p == '-')
      {
        change = USERMODE_OFF;
        continue;
      }

      if(USERMODE_VALID(*p))
      {
        if((usermode = usermode_table[(uint32_t)*p - 0x40]) != NULL)
        {
          changearg = NULL;

          if(usermode->arg & USERMODE_ARG_ENABLE)
          {
            changearg = *(arg + 1);

            if(changearg == NULL)
            {
              if(!(usermode->arg & USERMODE_ARG_EMPTY))
              {
                numeric_send(cptr, ERR_NEEDMOREPARAMS);
                continue;
              }
            }
            else
              arg++;
          }

          if(changearg == NULL || (!(usermode->arg)))
          {
            if(change == USERMODE_ON)
            {
              if(modes & usermode->flag)
                continue;
            }
            else
            {
              if(!(modes & usermode->flag))
                continue;
            }
          }

          usermode_change_add(usermode, change, changearg);

          continue;
        }
      }

      /* unknown mode character */
      if(cptr != NULL)
        numeric_send(cptr, ERR_UMODEUNKNOWNFLAG);
    }
  }

  return usermodechange_list.size;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int usermode_link_test(struct user *uptr, struct usermode *umptr, int change)
{
  /* don't do anything if no list ist kept for this usermode */
  if(umptr->list_mode == USERMODE_LIST_NOLIST)
    return 0;

  /* */
  if(umptr->list_type != USERMODE_LIST_GLOBAL)
  {
    /* is the user a local client? */
    if(umptr->list_type != uptr->client->location)
      return 0;
  }

  if(umptr->list_mode != change)
    return -1;

  return 1;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void usermode_link(struct user *uptr, struct usermode *umptr)
{
  struct node *node;

  /*  debug(usermode_log, "user %s linked to list of user mode %c",
        uptr->name, umptr->character); */

  node = dlink_node_new();

  dlink_add_tail(&umptr->list, node, uptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void usermode_unlink(struct user *uptr, struct usermode *umptr)
{
  dlink_find_delete(&umptr->list, uptr);

/*  debug(usermode_log, "user %s unlinked from list of user mode %c",
        uptr->name, umptr->character); */
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void usermode_linkall_user(struct user *uptr)
{
  struct node     *n;
  struct usermode *umptr;

  dlink_foreach_data(&usermode_list, n, umptr)
  {
    if(usermode_link_test(uptr, umptr, USERMODE_OFF) > 0)
      usermode_link(uptr, umptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void usermode_unlinkall_user(struct user *uptr)
{
  struct node     *n;
  struct usermode *umptr;

  dlink_foreach_data(&usermode_list, n, umptr)
    dlink_find_delete(&umptr->list, uptr);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void usermode_linkall_mode(struct usermode *umptr)
{
  struct node *n;
  struct user *uptr;

  dlink_foreach_data(&user_list, n, uptr)
  {
    if(usermode_link_test(uptr, umptr, USERMODE_ON) > 0)
      usermode_link(uptr, umptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void usermode_unlinkall_mode(struct usermode *umptr)
{
  dlink_destroy(&umptr->list);
}

/* -------------------------------------------------------------------------- *
 * Do the change                                                              *
 * -------------------------------------------------------------------------- */
int usermode_apply(struct user *uptr, uint64_t *modes, uint32_t flags)
{
  struct node *node;
  struct node *next;
  struct usermodechange *umcptr;
  int link;

  dlink_foreach_safe_data(&usermodechange_list, node, next, umcptr)
  {
    /* call bounce handler */
    if(umcptr->mode->handler != NULL)
    {
      if((umcptr->mode->handler)(uptr, umcptr, flags))
      {
        usermode_change_delete(umcptr);
        continue;
      }
    }

    /* change modes */
    if(umcptr->arg == NULL)
    {
      if(umcptr->change == USERMODE_ON)
        *modes |= umcptr->mode->flag;
      else
        *modes &= ~umcptr->mode->flag;
    }

    /* update list of users for this mode */
    if(!(flags & USERMODE_OPTION_LINKALL))
    {
      if((link = usermode_link_test(uptr, umcptr->mode, umcptr->change)))
      {
        if(link > 0)
          usermode_link(uptr, umcptr->mode);
        else
          usermode_unlink(uptr, umcptr->mode);
      }
    }
  }

  return usermodechange_list.size;
}

/* -------------------------------------------------------------------------- *
 * Send the usermode change to the local client
 * -------------------------------------------------------------------------- */
void
usermode_send_local(struct lclient *lcptr,
                    struct client  *cptr,
                    char           *umcbuf,
                    char           *umcargs)
{
  char noargs = '\0';

  /* oO this is lame, but useful */
  if(umcargs == NULL)
    umcargs = &noargs;

  if(umcbuf[0] != '+' && umcbuf[0] != '-')
    lclient_send(cptr->lclient, ":%s!%s@%s MODE %s +%s%s", cptr->name,
                 cptr->user->name, cptr->host, cptr->name, umcbuf, umcargs);
  else
    lclient_send(cptr->lclient, ":%s!%s@%s MODE %s %s%s", cptr->name,
                 cptr->user->name, cptr->host, cptr->name, umcbuf, umcargs);
}

/* -------------------------------------------------------------------------- *
 * Send the usermode change to all servers                                    *
 * -------------------------------------------------------------------------- */
void
usermode_send_remote(struct lclient *lcptr,
                     struct client  *cptr,
                     char           *umcbuf,
                     char           *umcargs)
{
  char noargs = '\0';

  if(umcargs == NULL)
    umcargs = &noargs;

  server_send(lcptr, NULL, CAP_UID, CAP_NONE, ":%s UMODE %s%s",
              client_is_user(cptr) ? cptr->user->uid : cptr->name,
              umcbuf, umcargs);

  server_send(lcptr, NULL, CAP_NONE, CAP_UID, ":%s UMODE %s%s",
              cptr->name, umcbuf, umcargs);
}

void
usermode_change_send(struct lclient *lcptr,
                     struct client  *cptr,
                     int             remote)
{
  struct node *node;
  struct usermodechange *umcptr;
  char umcbuf[IRCD_LINELEN];
  char umcargs[IRCD_LINELEN];
  char *pb = umcbuf;
  char *pa = umcargs;
  int count = 0;
  int change = USERMODE_NOCHANGE;
  int arglen;
  int maxchange = remote ? USERMODE_PERLINE_REMOTE : USERMODE_PERLINE_LOCAL;

  dlink_foreach_data(&usermodechange_list, node, umcptr)
  {
    /* if necessery append '+' or '-' to the mode string */
    if(umcptr->change != change)
    {
      change = umcptr->change;
      *pb++ = change == USERMODE_ON ? '+' : '-';
    }

    /* add character to change buffer */
    *pb++ = umcptr->mode->character;

    /* if necessery append '+' or '-' to the mode string */
    if(umcptr->arg != NULL)
    {
      *pa++ = ' ';

      arglen = str_len(umcptr->arg);
      memcpy(pa, umcptr->arg, arglen);
      pa += arglen;
    }

    if(++count >= maxchange || node->next == NULL)
    {
      *pb = '\0';
      *pa = '\0';

      if(remote)
        usermode_send_remote(lcptr, cptr, umcbuf, umcargs);
      else
        usermode_send_local(lcptr, cptr, umcbuf, umcargs);

      pb = umcbuf;
      pa = umcargs;
      count = 0;
      change = USERMODE_NOCHANGE;
    }

  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int usermode_make(struct user   *uptr, char    **umbuf,
                  struct client *cptr, uint32_t  flags)
{
  int ret = 0;

  /* parse the string and make struct usermodechange */
  if(usermode_parse(uptr->modes, umbuf, cptr, flags))
    /* make the changes out of usermodechanges */
    if(usermode_apply(uptr, &uptr->modes, flags))
    {
      usermode_assemble(uptr->modes, uptr->mode);

      ret = 1;
    }

  if(flags & USERMODE_OPTION_LINKALL)
    usermode_linkall_user(uptr);

  return ret;
}

/* -------------------------------------------------------------------------- *
 * Find a usermode by it's character                                          *
 * -------------------------------------------------------------------------- */
struct usermode *usermode_find(char character)
{
  struct usermode *umptr;

  if(!USERMODE_VALID(character))
    return NULL;

  umptr = usermode_table[(uint32_t)character - 0x40];

  return umptr;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void usermode_support(void)
{
  char modes[129];

  uint32_t i;
  uint32_t di = 0;

  for(i = 0; i < USERMODE_TABLE_SIZE; i++)
  {
    if(usermode_table[i])
    {
      modes[di++] = usermode_table[i]->character;

      if(usermode_table[i]->arg & USERMODE_ARG_ENABLE)
        modes[di++] = ',';
    }
  }

  modes[di] = '\0';

  ircd_support_set("USERMODES", "%s", modes);
}

/* -------------------------------------------------------------------------- *
 * Dump usermodes                                                             *
 * -------------------------------------------------------------------------- */
void
usermode_dump(struct usermode *umptr)
{
  if(umptr == NULL)
  {
    struct node *n;
    struct usermode *um;

    dump(usermode_log, "[============== usermode summary ===============]");

    dlink_foreach_data(&usermode_list, n, um)
    {
      dump(usermode_log, "%c %x", um->character, um->flag);
    }

    dump(usermode_log, "[=========== end of usermode summary ===========]");
  }
  else
  {
    dump(usermode_log, "[============== usermode dump ===============]");
    dump(usermode_log, "  character: %c", umptr->character);

    if(umptr->list_mode == USERMODE_LIST_NOLIST)
    {
      dump(usermode_log, "  list mode: <no list>");
    }
    else
    {
      char *modestr = NULL;
      char *typestr = NULL;

      switch(umptr->list_mode)
      {
        case USERMODE_LIST_ON:  modestr = "on";  break;
        case USERMODE_LIST_OFF: modestr = "off"; break;
      }

      switch(umptr->list_type)
      {
        case USERMODE_LIST_GLOBAL: typestr = "global"; break;
        case USERMODE_LIST_LOCAL:  typestr = "local";  break;
        case USERMODE_LIST_REMOTE: typestr = "remote"; break;
      }

      if(modestr == NULL)
        dump(usermode_log, "  list mode: %i (?)", umptr->list_mode);
      else
        dump(usermode_log, "  list mode: %s", modestr);

      if(typestr == NULL)
        dump(usermode_log, "  list type: %i (?)", umptr->list_type);
      else
        dump(usermode_log, "  list type: %s", typestr);

      dump(usermode_log, " list count: %i", umptr->list.size);
    }

    dump(usermode_log, "   argument: %s",
         umptr->arg & USERMODE_ARG_ENABLE ? "yes" : "no");

    if(umptr->arg & USERMODE_ARG_ENABLE)
      dump(usermode_log, "empty arg: %s",
           umptr->arg & USERMODE_ARG_EMPTY ? "yes" : "no");

    if(umptr->handler == NULL)
    {
      dump(usermode_log, "    handler: <no handler>", umptr->handler);
    }
    else
      dump(usermode_log, "    handler: %x", umptr->handler);

    dump(usermode_log, "       flag: %x", umptr->flag);
    dump(usermode_log, "[=========== end of usermode dump ===========]");
  }
}
