#ifndef SELECT_H
#define SELECT_H

/* sysdep: +sysselect */

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

/* braindead BSD uses bzero in FD_ZERO but doesn't #include string.h */
#include <string.h>

extern int select();

#endif
