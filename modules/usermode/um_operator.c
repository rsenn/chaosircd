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
 * $Id: um_operator.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/hook.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/oper.h"
#include "ircd/user.h"
#include "ircd/numeric.h"
#include "ircd/usermode.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static int  um_oper_bounce     (struct user           *uptr,
                                struct usermodechange *umcptr,
                                uint32_t               flags);

static void um_oper_up_hook    (struct user           *uptr);

static void um_oper_whois_hook (struct client         *cptr,
                                struct user           *uptr);

/* -------------------------------------------------------------------------- *
 * Locals                                                                     *
 * -------------------------------------------------------------------------- */
static struct usermode um_operator =
{
  'o',
  USERMODE_LIST_ON,
  USERMODE_LIST_GLOBAL,
  USERMODE_ARG_ENABLE | USERMODE_ARG_EMPTY,
  um_oper_bounce
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int um_operator_load(void)
{
  if(usermode_register(&um_operator))
    return -1;

  hook_register(oper_up, HOOK_DEFAULT, um_oper_up_hook);
  hook_register(user_whois, HOOK_DEFAULT, um_oper_whois_hook);

  return 0;
}

void um_operator_unload(void)
{
  hook_unregister(user_whois, HOOK_DEFAULT, um_oper_whois_hook);
  hook_unregister(oper_up, HOOK_DEFAULT, um_oper_up_hook);

  usermode_unregister(&um_operator);
}

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
static int um_oper_bounce(struct user *uptr, struct usermodechange *umcptr,
                          uint32_t flags)
{
  if(client_is_local(uptr->client))
  {
    /* +o */
    if(umcptr->change == USERMODE_ON)
    {
      /* denie this request if it's from a user */
      if(flags & USERMODE_OPTION_PERMISSION)
        return -1;
    }

    /* -o */
    else
      if(uptr->oper)
        oper_down(uptr->oper, uptr->client);
  }

  return 0;
}

static void um_oper_up_hook(struct user *uptr)
{
  usermode_change_destroy();
  usermode_change_add(&um_operator, USERMODE_ON, NULL);

  if(usermode_apply(uptr, &uptr->modes, 0UL))
  {
    usermode_assemble(uptr->modes, uptr->mode);

    usermode_change_send(uptr->client->lclient, uptr->client,
                         USERMODE_SEND_LOCAL);
    usermode_change_send(uptr->client->lclient, uptr->client,
                         USERMODE_SEND_REMOTE);
  }
}

static void um_oper_whois_hook(struct client *cptr,
                               struct user   *uptr)
{
  if(uptr->modes & um_operator.flag)
    numeric_send(cptr, RPL_WHOISOPERATOR, uptr->client->name);
}

