#include "io_internal.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

void io_closeonexec(int64 d) {
#ifndef __MINGW32__
  fcntl(d, F_SETFL, fcntl(d, F_GETFL, 0) | FD_CLOEXEC);
#endif
}
