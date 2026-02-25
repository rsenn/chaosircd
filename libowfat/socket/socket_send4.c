#include <sys/param.h>
#include <sys/types.h>
#ifndef __MINGW32__
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#include "byte.h"
#include "socket.h"
#include "windoze.h"

ssize_t socket_send4(int s, const char *buf, size_t len, const char ip[4],
                     uint16 port) {
  struct sockaddr_in si;

  byte_zero(&si, sizeof si);
  si.sin_family = AF_INET;
  uint16_pack_big((char *)&si.sin_port, port);
  *((uint32 *)&si.sin_addr) = *((uint32 *)ip);
  return winsock2errno(sendto(s, buf, len, 0, (void *)&si, sizeof si));
}
