#include <sys/types.h>
#ifndef __MINGW32__
#include <net/if.h>
#include <sys/socket.h>
#endif
#include "haven2i.h"
#include "socket.h"

uint32 socket_getifidx(const char *ifname) {
#ifdef HAVE_N2I
  return if_nametoindex(ifname);
#else
  return 0;
#endif
}
