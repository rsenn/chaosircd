/* 
 * Copyright (C) 2013-2014  CrowdGuard organisation
 * All rights reserved.
 *
 * Author: Roman Senn <rls@crowdguard.org>
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/hook.h"
#include "libchaos/log.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "ircd/ircd.h"
#include "ircd/numeric.h"
#include "ircd/channel.h"
#include "ircd/chanmode.h"
#include "ircd/chanuser.h"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define CM_ALARM_CHAR 'A'

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_alarm_msg_hook(struct client   *cptr, struct channel *acptr,
                             intptr_t         type, const char     *text);

static int cm_alarm_join_hook(struct lclient *lcptr, struct client *cptr,
                              struct channel *chptr);
static int cm_alarm_bounce   (struct lclient        *lcptr,                                                                                                                
                              struct client         *cptr,                                                                                                                 
                              struct channel        *chptr,                                                                                                                
                              struct chanuser       *cuptr,                                                                                                                
                              struct list           *lptr,                                                                                                                 
                              struct chanmodechange *cmcptr);                                                                                                              

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
  cm_alarm_bounce,         /* Bounce handler */
  cm_alarm_help            /* Help text */
};

static int cm_alarm_event_log;

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int cm_alarm_load(void)
{
  /* register the channel mode */
  if(chanmode_register(&cm_alarm_mode) == NULL)
    return -1;

  cm_alarm_event_log = log_source_find("event");                                                                                                              
  if(cm_alarm_event_log == -1)                                                                                                                                
    cm_alarm_event_log = log_source_register("event");            

  hook_register(channel_message, HOOK_DEFAULT, cm_alarm_msg_hook);
  hook_register(channel_join, HOOK_4TH, cm_alarm_join_hook);

  return 0;
}

void cm_alarm_unload(void)
{
  /* unregister the channel mode */
  chanmode_unregister(&cm_alarm_mode);

  log_source_unregister(cm_alarm_event_log);
  cm_alarm_event_log = -1;

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

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int cm_alarm_bounce   (struct lclient        *lcptr,                                                                                                                
                              struct client         *cptr,                                                                                                                 
                              struct channel        *chptr,                                                                                                                
                              struct chanuser       *cuptr,                                                                                                                
                              struct list           *lptr,                                                                                                                
                              struct chanmodechange *cmcptr)
{
  if(cmcptr->what == CHANMODE_ADD)                                                                                                                                        
  {
    if(!(chptr->modes & CHFLG(A))) {
      log(cm_alarm_event_log, L_warning, "Alarm triggered in %s", chptr->name);
    }
  }

  return chanmode_bounce_simple(lcptr, cptr, chptr, cuptr, lptr, cmcptr);
}
