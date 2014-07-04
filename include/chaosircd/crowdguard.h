/* 
 * Copyright (C) 2013-2014  CrowdGuard organisation
 * All rights reserved.
 *
 * Author: Roman Senn <rls@crowdguard.org>
 */

#ifndef SRC_CROWDGUARD_H
#define SRC_CROWDGUARD_H

#include "chaosircd/user.h"
#include "chaosircd/client.h"
#include "chaosircd/channel.h"
#include "chaosircd/chanuser.h"

#define channel_owner_flags ((cuptr->flags & CHFLG(o)) | \
                             (cuptr->flags & CHFLG(h)) | \
                             (cuptr->flags & CHFLG(v)))

#define chanuser_is_owner(cuptr) \
  ((cuptr->flags & channel_owner_flags) == channel_owner_flags)

#define channel_is_persistent(chptr) \
  ((chptr)->modes & CHFLG(P))

CHAOS_INLINE_FN(int
client_is_participant(struct client *cptr))
{
  struct chanuser *cuptr;

  if(!client_is_user(cptr))
    return 0;

  dlink_foreach(&cptr->user->channels, cuptr)
  {
    
  }
}

#endif /* SRC_CROWDGUARD_H */
