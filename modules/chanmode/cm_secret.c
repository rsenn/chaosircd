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
 * $Id: cm_secret.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/hook.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/numeric.h>
#include <ircd/channel.h>
#include <ircd/chanmode.h>
#include <ircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define CM_SECRET_CHAR 's'

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_secret_hook(struct client   *cptr, struct client *acptr,
                          struct chanuser *acuptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static const char *cm_secret_help[] =
{
  "+s              Secret channel. The channel will not appear in /LIST and",
  "                /WHOIS (actually it does when the user doing the /WHOIS",
  "                is a member of the channel.",
  NULL
};

static struct chanmode cm_secret_mode =
{
  CM_SECRET_CHAR,          /* Mode character */
  '\0',                    /* No prefix, because its not a privilege */
  CHANMODE_TYPE_SINGLE,    /* Channel mode is a single flag */
  CHFLG(o) | CHFLG(h),     /* Only OPs and Halfops can change the flag */
  0,                       /* No order and no reply */
  chanmode_bounce_simple,  /* Bounce handler */
  cm_secret_help           /* Help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_secret_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_secret_mode) == NULL)
    return -1;

  hook_register(chanuser_whois, HOOK_DEFAULT, cm_secret_hook);

  return 0;
}

void cm_secret_unload(void)
{
  /* unregister the channel mode */
  chanmode_unregister(&cm_secret_mode);

  hook_unregister(chanuser_whois, HOOK_DEFAULT, cm_secret_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_secret_hook(struct client   *cptr, struct client *acptr,
                          struct chanuser *acuptr)
{
  if(acuptr->channel->modes & CHFLG(s))
    return 1;

  return 0;
}

