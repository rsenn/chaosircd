#include "io_internal.h"
#include <signal.h>
#include <sys/types.h>

/* this is an internal function, called by io_trywrite and io_waitwrite */

void io_sigpipe(void) {
#ifndef __MINGW32__
  static int isitdone;
  if (!isitdone) {
    signal(SIGPIPE, SIG_IGN);
    isitdone = 1;
  }
#endif
}
