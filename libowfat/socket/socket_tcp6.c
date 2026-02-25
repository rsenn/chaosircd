#include "ndelay.h"
#include "socket.h"
#include <unistd.h>

int socket_tcp6(void) {
  int s = socket_tcp6b();
  if (s == -1)
    return -1;
  if (ndelay_on(s) == -1) {
    close(s);
    return -1;
  }
  return s;
}
