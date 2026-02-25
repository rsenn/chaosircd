#include "io_internal.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __MINGW32__
#include "windoze.h"
#include <winsock2.h>
#endif

#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif

void io_block(int64 d) {
  io_entry *e = array_get(&io_fds, sizeof(io_entry), d);
#ifdef __MINGW32__
  unsigned long i = 0;
  if (ioctlsocket(d, FIONBIO, &i) == 0)
    if (e)
      e->nonblock = 0;
#else
  if (fcntl(d, F_SETFL, fcntl(d, F_GETFL, 0) & ~O_NDELAY) == 0)
    if (e)
      e->nonblock = 0;
#endif
}
