#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>

int main() {
  static char ifname[IFNAMSIZ];
  char *tmp = if_indextoname(0, ifname);
}
