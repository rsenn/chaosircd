/* Use select() for i/o multiplexing */
#cmakedefine USE_SELECT 1

/* Use poll() for i/o multiplexing */
#cmakedefine USE_POLL 1

/* Use dlopen() dynamic shared object loading */
#cmakedefine USE_DSO 1

/* Support for SSL encrypted connections */
#cmakedefine HAVE_SSL 1

/* Define this if your OS has a socket-based filtering mechanism */
#cmakedefine HAVE_SOCKET_FILTER 1

/* Define this if you have BSD-type socket filters */
#cmakedefine BSD_SOCKET_FILTER 1

/* Define this if you have Linux-type socket filters */
#cmakedefine LINUX_SOCKET_FILTER 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if you have the <signal.h> header file. */
#cmakedefine HAVE_SIGNAL_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#cmakedefine HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#cmakedefine HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#cmakedefine HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/poll.h> header file. */
#cmakedefine HAVE_SYS_POLL_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H 1

/* Define to 1 if you have the <io.h> header file. */
#cmakedefine HAVE_IO_H 1

/* Define to 1 if you have the <limits.h> header file. */
#cmakedefine HAVE_LIMITS_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <winsock2.h> header file. */
#cmakedefine HAVE_WINSOCK2_H 1

/* Suffix for shared loadable modules */
#define DLLEXT ".dll"

/* The size of `uintptr_t', as computed by sizeof. */
#cmakedefine SIZEOF_UINTPTR_T 32

/* Dynamic linkable library filename extension */
#cmakedefine DLLEXT "@DLLEXT@"
