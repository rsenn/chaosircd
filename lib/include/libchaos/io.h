/* chaosircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 *
 * $Id: io.h,v 1.4 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_IO_H
#define LIB_IO_H

#include <stdarg.h>

/* ------------------------------------------------------------------------ *
 * Forward declarations                                                     *
 * ------------------------------------------------------------------------ */
struct io;

/* ------------------------------------------------------------------------ *
 * Library headers                                                            *
 * ------------------------------------------------------------------------ */
#include "libchaos/config.h"
#include "libchaos/defs.h"
#include "libchaos/syscall.h"
#include "libchaos/queue.h"

#ifdef WIN32
//#include <winsock2.h>
#else
#include <sys/select.h>
#endif

#ifdef HAVE_SSL
#include "libchaos/ssl.h"
#endif /* HAVE_SSL */

#ifdef USE_POLL
#define io_multiplex io_poll
#else
#define io_multiplex io_select
#endif


#define io_valid(x) ((x) >= 0 && (x) < IO_MAX_FDS)

/* ------------------------------------------------------------------------ *
 * Constants                                                                  *
 * ------------------------------------------------------------------------ */
#define IO_MAX_FDS 1024

#define IO_OPEN_READ     1
#define IO_OPEN_WRITE    2
#define IO_OPEN_RDWR     (IO_OPEN_READ|IO_OPEN_WRITE)
#define IO_OPEN_CREATE   4
#define IO_OPEN_TRUNCATE 8
#define IO_OPEN_APPEND   16

#define IO_READ_SIZE  4096    /* How many bytes we read at once */
#define IO_WRITE_SIZE 4096    /* How many bytes we write at once */
#define IO_LINE_SIZE  1024

#define IO_READ_FLAGS  (O_RDONLY | O_NONBLOCK)
#define IO_WRITE_FLAGS (O_WRONLY | O_NONBLOCK | O_CREAT)

#define IO_ERROR   0x01
#define IO_READ    0x02
#define IO_WRITE   0x04
#define IO_ACCEPT  0x08
#define IO_CONNECT 0x10

#define IO_CB_ERROR   0x00
#define IO_CB_READ    0x01
#define IO_CB_WRITE   0x02
#define IO_CB_ACCEPT  0x03
#define IO_CB_CONNECT 0x04
#define IO_CB_MAX     IO_CB_CONNECT + 1

/* ------------------------------------------------------------------------ *
 * Types                                                                      *
 * ------------------------------------------------------------------------ */
enum
{
  OFF = 0,
  ON = 1
};


enum
{
  FD_NONE   = 0,        /* unassigned */
  FD_FILE   = 1,        /* something that can be fsync'd and memory mapped,
                           on win32 it means that the fd is a HANDLE returned
                           from CreateFile() */
  FD_SOCKET = 2,        /* a network socket */
  FD_PIPE   = 3
};

typedef void (io_callback_t)(int fd, void *, void *, void *, void *);

struct io
{
  int                type;
  int                error;
  /*
   * configuration flags
   *
   *    sendq        - when set to 1, an io_write() will
   *                   not go directly to the file descriptor.
   *                   it'll be put into the queue and the
   *                   queue will be emptied on a write event.
   *
   *    recvq        - when set to 1, an io_read() will not
   *                   directly read from a file descriptor.
   *                   it'll read from a queue which is
   *                   filled on read events.
   *
   *    linebuf      - this works only when the recvq
   *                   is enabled.
   *                   when is is set to 1 then the
   *                   read callback for the socket
   *                   will only be called if there
   *                   is a line in the linebuffer.
   *                   otherwise its called simply
   *                   when there is data in the queue.
   *
   *    waitdns      - call IO_CB_ACCEPT after completed
   *                   reverse DNS.
   */
  struct {
    int sendq:1;     /* queue incoming data */
    int recvq:1;     /* queue outgoing data */
    int linebuf:1;   /* linebuffer incoming data */
    int events:5;
  } control;

  /*
   * event flags
   *
   *    err          - there was an error
   *
   *    line         - there is a line in the queue
   *
   *    timeout      - operation timed out
   */
  struct {
    int events;
    int err:1;       /* got error */
    int eof:1;
    int closed:1;
    int onread:1;
    int onwrite:1;
    int dead:1;
  } status;

  int                ret;
  void              *args[4];
  long               flags;    /* posix filedescriptor flags (do we really need to track them?) */
  struct fqueue      sendq;
  struct fqueue      recvq;
  struct stat        stat;
  int                index;    /* index on the pollfd list */
  char               note[64]; /* description string */
//  struct sockaddr_in a_remote; /* remote address */
//  struct sockaddr_in a_local;  /* local address we bound to */
  io_callback_t     *callbacks[IO_CB_MAX];

  struct ssl_st       *ssl;
  int                  sslstate;
  int                  sslerror;
  int                  sslwhere; /* where ssl error happened */
};


/* ------------------------------------------------------------------------ *
 * Global variables                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_DATA(int)         io_log;
CHAOS_DATA(struct io *) io_list;
CHAOS_DATA(int)         io_top;

/* ------------------------------------------------------------------------ */
CHAOS_API(int) io_get_log(void);

/* ------------------------------------------------------------------------ *
 * Initialize I/O code.                                                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_init           (void);
CHAOS_API(void)       io_init_except    (int            fd0,
                                         int            fd1,
                                         int            fd2);

/* ------------------------------------------------------------------------ *
 * Shutdown I/O code.                                                         *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_shutdown       (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_flush          (int            fd);

/* ------------------------------------------------------------------------ *
 * Put a file descriptor into non-blocking mode.                              *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_nonblocking       (int            fd);

/* ------------------------------------------------------------------------ *
 * Control the queue behaviour.                                               *
 *                                                                            *
 * Use ON/OFF flags for all arguments except fd.                              *
 *                                                                            *
 * fd            - File descriptor                                            *
 * recvq         - queue incoming data.                                       *
 * sendq         - queue outgoing data.                                       *
 * linebuf       - Only call read callback when there's a full line of data.  *
 *                                                                            *
 * Note that none of the queues can be disabled if they still contain data.   *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_queue_control  (int            fd,
                                         int            recvq,
                                         int            sendq,
                                         int            linebuf);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_queued_read    (int            fd);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_queued_write   (int            fd);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_handle_fd      (int            fd);

/* ------------------------------------------------------------------------ *
 * Register a file descriptor to the io_list.                                 *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_new            (int            fd,
                                         int            type);

/* ------------------------------------------------------------------------ *
 * Open a file.                                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_open           (const char    *path,
                                         int            flags,
                                         ...);

/* ------------------------------------------------------------------------ *
 * Shut a filedescriptor                                                      *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_shutup         (int            fd);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_push           (int           *fdptr);

/* ------------------------------------------------------------------------ *
 * Close an fd.                                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_destroy          (int            fd);

/* ------------------------------------------------------------------------ *
 * Write a description string.                                                *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_note           (int            fd,
                                         const char    *format,
                                         ...);

/* ------------------------------------------------------------------------ *
 * Register a I/O event callback.                                             *
 *                                                                            *
 * type            - on which type of event to call the callback              *
 *                   IO_CB_ERROR   - I/O error                                *
 *                   IO_CB_READ    - incoming data                            *
 *                   IO_CB_WRITE   - outgoing data                            *
 *                   IO_CB_ACCEPT  - a client connecting                      *
 *                   IO_CB_CONNECT - an established or failed connection      *
 *                                                                            *
 * callback        - a callback in the form:                                  *
 *                                                                            *
 *                   void callback(int fd, void *arg)                         *
 *                                                                            *
 * userarg         - a user-defined pointer to pass to the callback           *
 *                                                                            *
 * timeout         - if the event doesn't occur after this miliseconds        *
 *                   then call the callback anyway.                           *
 *                                                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_vregister      (int            fd,
                                         int            type,
                                         void          *callback,
                                         va_list        args);

CHAOS_API(int)        io_register       (int            fd,
                                         int            type,
                                         void          *callback,
                                         ...);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_unregister     (int           fd,
                                         int           type);

/* ------------------------------------------------------------------------ *
 * Read either from the fd directly or from its queue.                        *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_read           (int            fd,
                                         void          *buf,
                                         size_t         n);

/* ------------------------------------------------------------------------ *
 * Write either to the fd directly or to its queue.                           *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_write          (int            fd,
                                         const void    *buf,
                                         size_t         n);

/* ------------------------------------------------------------------------ *
 * Read a line from queue.                                                    *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_gets           (int            fd,
                                         void          *buf,
                                         size_t         n);

/* ------------------------------------------------------------------------ *
 * Write a line to fd or queue.                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_puts           (int            fd,
                                         const char    *s,
                                         ...);

/* ------------------------------------------------------------------------ *
 * Write a line to fd or queue.                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)           io_vputs       (int            fd,
                                         const char    *s,
                                         va_list        args);

/* ------------------------------------------------------------------------ *
 *                                                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_multi_start    (struct fqueue *fifoptr);
CHAOS_API(uint32_t)   io_multi_write    (struct fqueue *fifoptr,
                                         const void    *buf,
                                         uint32_t       n);
CHAOS_API(void)       io_multi_link     (struct fqueue *fifoptr,
                                         int            fd);
CHAOS_API(void)       io_multi_end      (struct fqueue *fifoptr);

/* ------------------------------------------------------------------------ *
 * Do a select() system call.                                                 *
 *                                                                            *
 * If timeout == NULL then return only when an event occurred.                *
 * else return after <timeout> miliseconds or when there was an event.        *
 * In the latter case the remaining time will be in *timeout.                 *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_select         (int64_t       *remain,
                                         int64_t       *timeout);

/* ------------------------------------------------------------------------ *
 * Do a poll() system call.                                                   *
 *                                                                            *
 * If timeout == NULL then return only when an event occurred.                *
 * else return after <timeout> miliseconds or when there was an event.        *
 * In the latter case the remaining time will be in *timeout.                 *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)        io_poll           (int64_t       *remain,
                                         int64_t       *timeout);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_wait_blocking           (void);

/* ------------------------------------------------------------------------ *
 * Handle pending I/O events                                                  *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_handle         (void);

/* ------------------------------------------------------------------------ *
 * Move an fd.                                                                *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_move           (int            from,
                                         int            to);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_vset_args      (int            fd,
                                         va_list        args);

CHAOS_API(void)       io_set_args       (int            fd,
                                         ...);

/* ------------------------------------------------------------------------ *
 * This function will set the necessary flags in the fd_sets/pollfds for the  *
 * requested events.                                                          *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_set_events     (int            fd,
                                         int            events);

/* ------------------------------------------------------------------------ *
 * This function will unset the necessary flags in the fd_sets/pollfds for    *
 * the requested events.                                                      *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_unset_events   (int            fd,
                                         int            events);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)       io_dump           (int            fd);

#endif /* LIB_IO_H */
