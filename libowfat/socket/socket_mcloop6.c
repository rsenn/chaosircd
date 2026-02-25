#include <sys/param.h>
#include <sys/types.h>
#ifndef __MINGW32__
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#include "haveip6.h"
#include "socket.h"
#include "windoze.h"

int socket_mcloop6(int s, char loop) {
#ifdef LIBC_HAS_IP6
  if (noipv6)
    return winsock2errno(
        setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof loop));
  else
    return winsock2errno(
        setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof loop));
#else
  return winsock2errno(
      setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof loop));
#endif
}
