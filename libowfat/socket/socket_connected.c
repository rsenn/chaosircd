#include <sys/types.h>
#ifndef __MINGW32__
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#include "havesl.h"
#include "socket.h"

int socket_connected(int s) {
  struct sockaddr si;
  socklen_t sl = sizeof si;
  if (getpeername(s, &si, &sl))
    return 0;
  return 1;
}
