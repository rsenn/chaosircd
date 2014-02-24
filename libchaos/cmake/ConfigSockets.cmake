include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)

check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(sys/select.h HAVE_SYS_SELECT_H)
check_include_file(sys/poll.h HAVE_SYS_POLL_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(io.h HAVE_IO_H)
check_include_file(netinet/in.h HAVE_NETINET_IN_H)
check_include_file(net/bpf.h HAVE_NET_BPF_H)
check_include_file(net/ethernet.h HAVE_NET_ETHERNET_H)
check_include_file(cygwin/in.h HAVE_CYGWIN_IN_H)
check_include_file(linux/filter.h HAVE_LINUX_FILTER_H)
check_include_file(linux/types.h HAVE_LINUX_TYPES_H)


if(CMAKE_HOST_WIN32)
#  check_include_file(winsock.h HAVE_WINSOCK2_H)
  check_include_file(winsock2.h HAVE_WINSOCK2_H)
  check_include_file(ws2tcpip.h HAVE_WS2TCPIP_H)
endif(CMAKE_HOST_WIN32)


if(HAVE_SYS_SOCKET_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} sys/socket.h)
endif(HAVE_SYS_SOCKET_H)

if(HAVE_WINSOCK2_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} winsock2.h)
endif(HAVE_WINSOCK2_H)

if(HAVE_WS2TCPIP_H)
  set(CMAKE_EXTRA_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES} ws2tcpip.h)
endif(HAVE_WS2TCPIP_H)

check_type_size(socklen_t SOCKLEN_T)

check_function_exists(select HAVE_SELECT)