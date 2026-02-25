#include <sys/types.h>
#ifdef __MINGW32__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

main() { socklen_t t; }
