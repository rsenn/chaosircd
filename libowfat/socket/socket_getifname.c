#include <sys/types.h>
#ifndef __MINGW32__
#include <net/if.h>
#include <sys/socket.h>
#endif
#include "haven2i.h"
#include "socket.h"

#ifdef HAVE_N2I
/* legacy BSD name */
#ifndef IF_NAMESIZE
#define IF_NAMESIZE IFNAMSIZ
#endif

static char ifname[IF_NAMESIZE];

const char *socket_getifname(uint32 _interface) {
  char *tmp = if_indextoname(_interface, ifname);
  if (tmp)
    return tmp;
  else
    return "[unknown]";
}
#else
const char *socket_getifname(uint32 _interface) { return "[unknown]"; }
#endif
