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
 * $Id: um_client.c,v 1.2 2006/09/28 08:38:31 roman Exp $
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
#include "ircd/server.h"
#include "ircd/lclient.h"
#include "ircd/numeric.h"
#include "ircd/usermode.h"

/* -------------------------------------------------------------------------- *
 * Constants                                                                  *
 * -------------------------------------------------------------------------- */
#define UM_CLIENT_CHAR 'c'
/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static int  um_client_bounce     (struct user           *uptr,
                                  struct usermodechange *umcptr,
                                  uint32_t               flags);
static void um_client_reg        (struct lclient        *lcptr);
static void um_client_quit       (struct lclient        *lcptr,
                                  struct client         *cptr);

/* -------------------------------------------------------------------------- *
 * Locals                                                                     *
 * -------------------------------------------------------------------------- */
static struct usermode um_client = {
  UM_CLIENT_CHAR,
  USERMODE_LIST_ON,
  USERMODE_LIST_LOCAL,
  USERMODE_ARG_DISABLE,
  um_client_bounce
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int um_client_load(void)
{
  if(usermode_register(&um_client))
    return -1;

  hook_register(lclient_register, HOOK_2ND, um_client_reg);
  hook_register(client_exit, HOOK_DEFAULT, um_client_quit);

  return 0;
}

void um_client_unload(void)
{
  hook_unregister(client_exit, HOOK_DEFAULT, um_client_quit);
  hook_unregister(lclient_register, HOOK_2ND, um_client_reg);

  usermode_unregister(&um_client);
}

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
static int um_client_bounce(struct user *uptr, struct usermodechange *umcptr,
                            uint32_t flags)
{
  if(client_is_local(uptr->client))
  {
    /* +o */
    if(umcptr->change == USERMODE_ON)
    {
      /* deny this request if it's from a user */
      if(uptr->oper == NULL)
      {
        numeric_send(uptr->client, ERR_NOPRIVILEGES, "UMODE");
        return -1;
      }
    }
  }

  return 0;
}

static void um_client_reg(struct lclient *lcptr)
{
  struct usermode *umptr;
  struct node     *nptr;
  struct user     *uptr = NULL;

  if(lcptr->client == NULL)
    return;

  if((umptr = usermode_find(UM_CLIENT_CHAR)) == NULL)
    return;

  dlink_foreach_data(&umptr->list, nptr, uptr)
  {
    if(uptr->client == NULL)
      continue;

    client_send(uptr->client, ":%S NOTICE %N :*** client connecting: %N (%U@%s)",
                server_me, uptr->client, lcptr->client, lcptr->client, lcptr->client->hostreal);
  }
}

static void um_client_quit(struct lclient *lcptr, struct client *cptr)
{
  struct usermode *umptr;
  struct node     *nptr;
  struct user     *uptr = NULL;

  if(!client_is_local(cptr) || cptr->user == NULL)
    return;

  if((umptr = usermode_find(UM_CLIENT_CHAR)) == NULL)
    return;

  dlink_foreach_data(&umptr->list, nptr, uptr)
  {
    if(uptr->client == NULL)
      continue;

    client_send(uptr->client, ":%S NOTICE %N :*** client exiting: %N (%U@%s)",
                server_me, uptr->client, cptr, cptr, cptr->hostreal);
  }
}

