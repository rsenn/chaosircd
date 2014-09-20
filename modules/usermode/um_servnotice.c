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
 * $Id: um_servnotice.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "libchaos/log.h"

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/user.h>
#include <ircd/oper.h>
#include <ircd/usermode.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
int um_servernotice_bounce(struct user *uptr, struct usermodechange *umptr,
                           uint32_t flag);

/* -------------------------------------------------------------------------- *
 * Locals                                                                     *
 * -------------------------------------------------------------------------- */
static struct usermode um_servnotice =
{
  's',
  USERMODE_LIST_OFF,
  USERMODE_LIST_LOCAL,
  USERMODE_ARG_DISABLE,
  NULL
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int um_servnotice_load(void)
{
  if(usermode_register(&um_servnotice))
    return -1;

  return 0;
}

void um_servnotice_unload(void)
{
  usermode_unregister(&um_servnotice);
}

int um_servernotice_bounce(struct user *uptr, struct usermodechange *umcptr,
                           uint32_t flags)
{
  int      index;
  uint64_t flag;

  if(uptr->oper == NULL)
    return -1;

  if(umcptr->arg == NULL)
    flag = 1ULL;

  /* if the change is + or -s <argument> */
  else
  {
    if((index = log_source_find(umcptr->arg)) == -1)
      return -1;

    flag = log_sources[index].flag;
  }

  if(umcptr->change == USERMODE_ON)
    uptr->oper->sources |= flag;
  else
    uptr->oper->sources &= ~flag;

  return 0;
}
