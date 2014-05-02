/* 
 * Copyright (C) 2013-2014  CrowdGuard organisation
 * All rights reserved.
 *
 * Author: Roman Senn <rls@crowdguard.org>
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/hook.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/numeric.h>
#include <chaosircd/channel.h>
#include <chaosircd/chanmode.h>
#include <chaosircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define CM_ALARM_CHAR 'A'

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_alarm_msg_hook(struct client   *cptr, struct channel *acptr,
                             intptr_t         type, const char     *text);

static int cm_alarm_join_hook(struct lclient *lcptr, struct client *cptr,
                               struct channel *chptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static const char *cm_alarm_help[] =
{
  "+A              CrowdGuard alarm.",
  NULL
};

static struct chanmode cm_alarm_mode =
{
  CM_ALARM_CHAR,           /* Mode character */
  '\0',                    /* No prefix, because its not a privilege */
  CHANMODE_TYPE_SINGLE,    /* Channel mode is a single flag */
  CHFLG(o),                /* Only OPs can change the flag */
  0,                       /* No order and no reply */
  chanmode_bounce_simple,  /* Bounce handler */
  cm_alarm_help            /* Help text */
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_alarm_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_alarm_mode) == NULL)
    return -1;

  hook_register(channel_message, HOOK_DEFAULT, cm_alarm_msg_hook);
  hook_register(channel_join, HOOK_4TH, cm_alarm_join_hook);

  return 0;
}

void cm_alarm_unload(void)
{
  /* unregister the channel mode */
  chanmode_unregister(&cm_alarm_mode);

  hook_unregister(channel_join, HOOK_4TH, cm_alarm_join_hook);
  hook_unregister(channel_message, HOOK_DEFAULT, cm_alarm_msg_hook);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_alarm_msg_hook(struct client   *cptr, struct channel *chptr,
                             intptr_t         type, const char     *text)
{
  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_alarm_join_hook(struct lclient *lcptr, struct client *cptr,
                              struct channel *chptr)
{
	return 0;
}
