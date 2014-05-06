
/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/io.h"
#include "libchaos/dlink.h"
#include "libchaos/timer.h"
#include "libchaos/hook.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include "chaosircd/ircd.h"
#include "chaosircd/lclient.h"
#include "chaosircd/client.h"
#include "chaosircd/msg.h"

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static int              lc_crowdguard_hook    (struct lclient   *lcptr);

/* -------------------------------------------------------------------------- *
 * Local variables                                                            *
 * -------------------------------------------------------------------------- */
static int lc_crowdguard_login_log;

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int lc_crowdguard_load(void)
{
  lc_crowdguard_login_log = log_source_find("login");                                                                                                                            
  if(lc_crowdguard_login_log == -1)                                                                                                                                              
    lc_crowdguard_login_log = log_source_register("login");              

  if(hook_register(lclient_login, HOOK_DEFAULT, lc_crowdguard_hook) == NULL)
    return -1;

  return 0;
}

void lc_crowdguard_unload(void)
{
  hook_unregister(lclient_login, HOOK_DEFAULT, lc_crowdguard_hook);

  log_source_unregister(lc_crowdguard_login_log);                                                                                                                                
  lc_crowdguard_login_log = -1;                                                                                                                                                   
}

/* -------------------------------------------------------------------------- *
 * Hook before user/nick is validated                                         *
 * -------------------------------------------------------------------------- */
static int lc_crowdguard_hook(struct lclient *lcptr)
{
  log(lc_crowdguard_login_log, L_status, "User login from %s with handle %s, info: %s",
      lcptr->host, lcptr->name, lcptr->info);

  return 0;
}

