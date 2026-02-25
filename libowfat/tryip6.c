#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

main() {
  struct sockaddr_in6 sa;
  sa.sin6_family = PF_INET6;
}
