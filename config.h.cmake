#ifndef CONFIG_H__
#define CONFIG_H__ 1

/* Define to 1 if you have the <net/bpf.h> header file. */
#cmakedefine HAVE_NET_BPF_H 1

/* Define to 1 if you have the <net/ethernet.h> header file. */
#cmakedefine HAVE_NET_ETHERNET_H 1

/* Define to 1 if you have the <elf.h> header file. */
#cmakedefine HAVE_ELF_H 1

/* Define to 1 if you have the <limits.h> header file. */
#cmakedefine HAVE_LIMITS_H 1

/* Define to 1 if you have the <winsock2.h> header file. */
#cmakedefine HAVE_WINSOCK2_H 1

/* Define to 1 if you have the <ws2tcpip.h> header file. */
#cmakedefine HAVE_WS2TCPIP_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the socklen_t type. */
#cmakedefine HAVE_SOCKLEN_T 1

/* Dynamic linkable library filename extension */
#cmakedefine DLLEXT "@DLLEXT@"

#define SIZEOF_UINTPTR_T @SIZEOF_UINTPTR_T@


/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT "@PACKAGE_BUGREPORT@"

/* Define to the full name of this package. */
#cmakedefine PACKAGE_NAME "@PACKAGE_NAME@"

/* Define to the release name of this package. */
#cmakedefine PACKAGE_RELEASE "@PACKAGE_RELEASE@"

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING "@PACKAGE_STRING@"

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME "@PACKAGE_TARNAME@"

/* Define to the home page for this package. */
#cmakedefine PACKAGE_URL "@PACKAGE_URL@"

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION "@PACKAGE_VERSION@"

#endif
/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Creation time of this server */
#ifndef CREATION
#cmakedefine CREATION "@CREATION@"
#endif

/* Platform this server runs on */
#ifndef PLATFORM
#cmakedefine PLATFORM "@PLATFORM@"
#endif
