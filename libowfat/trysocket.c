#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

main() { int fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); }
