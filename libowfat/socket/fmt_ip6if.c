#include "fmt.h"
#include "ip6.h"
#include "socket.h"
#include "str.h"

unsigned int fmt_ip6if(char *dest, const char *ip, uint32 scope_id) {
  int i = fmt_ip6(dest, ip);
  if (scope_id) {
    if (dest) {
      dest[i] = '%';
      ++i;
      dest += i;
    }
    i += fmt_str(dest, socket_getifname(scope_id));
  }
  return i;
}
