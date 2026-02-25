#include <sys/types.h>
#ifndef __MINGW32__
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include "ndelay.h"
#include "socket.h"
#include "windoze.h"

int socket_tcp4b(void) {
  int s;
  __winsock_init();
  s = winsock2errno(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP));
  if (s == -1)
    return -1;
  return s;
}
