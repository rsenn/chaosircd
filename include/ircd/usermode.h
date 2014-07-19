/* chaosircd - pi-networks irc server
 *
 * Copyright (C) 2003-2006  Manuel Kohler
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: usermode.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_USERMODE_H
#define SRC_USERMODE_H


/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/dlink.h>
#include <libchaos/mem.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/client.h>


/* -------------------------------------------------------------------------- *
 * Defines                                                                    *
 * -------------------------------------------------------------------------- */
#define USERMODE_COUNT             52
#define USERMODE_TABLE_SIZE        0x40
#define USERMODE_PERLINE_REMOTE    8
#define USERMODE_PERLINE_LOCAL     3
#define USERMODE_NOFLAG            0ULL

/* these are defines for the change type (on, off..) */
#define USERMODE_NOCHANGE         -1
#define USERMODE_OFF               0
#define USERMODE_ON                1

/* values for struct usermode->list_mode and ->list_type*/
#define USERMODE_LIST_NOLIST       -1

#define USERMODE_LIST_OFF          USERMODE_OFF
#define USERMODE_LIST_ON           USERMODE_ON

#define USERMODE_LIST_LOCAL        CLIENT_LOCAL
#define USERMODE_LIST_REMOTE       CLIENT_REMOTE
#define USERMODE_LIST_GLOBAL       CLIENT_GLOBAL

/* flags for struct usermode->arg */
#define USERMODE_ARG_DISABLE       0x00
#define USERMODE_ARG_ENABLE        0x01
#define USERMODE_ARG_EMPTY         0x02

/* */
#define USERMODE_OPTION_PERMISSION 0x01
#define USERMODE_OPTION_LINKALL    0x02
#define USERMODE_OPTION_SINGLEARG  0x04

#define USERMODE_SEND_LOCAL        0
#define USERMODE_SEND_REMOTE       1

/* -------------------------------------------------------------------------- *
 * Types                                                                      *
 * -------------------------------------------------------------------------- */

struct usermodechange;
struct usermode;

typedef int (um_handler_t)(struct user *,
                           struct usermodechange *,
                           uint32_t flags);

struct usermode {
  /* these values are set by the usermode modules... */
  char          character; /* the letter */
  int           list_mode; /* says what is kept in the list.
                            * USERMODE_LIST_ON, USERMODE_LIST_OF or
                            * USERMODE_LIST_NOLIST. */
  int           list_type; /* what kind of users kept in the list.
                            * USERMODE_LIST_LOCAL, USERMODE_LIST_REMOTE
                            * OR USERMODE_LIST_GLOBAL. */
  int           arg;       /* information about arguments for this usermode.
                            * bitwise-ORed with USERMODE_ARG_ENABLE and
                            * USERMODE_ARG_EMPTY */
  um_handler_t *handler;   /* called on a change on this flag */

  /* ..and these by usermode.c */
  struct list   list;      /* the list users are kept in when flag is on */
  uint64_t      flag;      /* the bit flag */
  struct node   node;
};

struct usermodechange {
  struct node      node;
  struct usermode *mode;   /* the mode */
  int              change; /* the change */
  char            *arg;
};


/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
int usermode_log;


/* ------------------------------------------------------------------------ */
IRCD_API(int) usermode_get_log(void);

/* -------------------------------------------------------------------------- *
 * Initialize the usermode module                                             *
 * -------------------------------------------------------------------------- */
extern void
usermode_init          (void);

/* -------------------------------------------------------------------------- *
 * Shut down the USERMODE module                                              *
 * -------------------------------------------------------------------------- */
extern void
usermode_shutdown      (void);

/* -------------------------------------------------------------------------- *
 * Called to register a usermode                                              *
 * -------------------------------------------------------------------------- */
extern int
usermode_register      (struct usermode *umptr);

/* -------------------------------------------------------------------------- *
 * Called to unregister a usermode                                            *
 * -------------------------------------------------------------------------- */
extern void
usermode_unregister    (struct usermode *umptr);

/* -------------------------------------------------------------------------- *
 * Answer a usermode request                                                  *
 * -------------------------------------------------------------------------- */
extern void
usermode_show          (struct client  *cptr);

/* -------------------------------------------------------------------------- *
 * Add a usermode change to the usermode_heap                                 *
 * -------------------------------------------------------------------------- */
extern void
usermode_change_add    (struct usermode *mode, int change, char *arg);

/* -------------------------------------------------------------------------- *
 * Removes all usermode changes from the usermode_list and free the memory    *
 * -------------------------------------------------------------------------- */
extern void
usermode_change_destroy(void);

/* -------------------------------------------------------------------------- *
 * Parses a user mode buffer                                                  *
 * -------------------------------------------------------------------------- */
extern int
usermode_parse         (uint64_t        modes,     char         **args,
                       struct client   *cptr,      uint32_t       flags);

/* -------------------------------------------------------------------------- *
 * Form a string representating the modes given                               *
 * -------------------------------------------------------------------------- */
extern void
usermode_assemble      (uint64_t        modes,     char          *umbuf);

/* -------------------------------------------------------------------------- *
 * Change the flags in modes after usermode_parse() is called                 *
 * -------------------------------------------------------------------------- */
extern int
usermode_apply         (struct user    *uptr,      uint64_t      *modes,
                        uint32_t        flags);

extern void
usermode_unlinkall_user(struct user    *uptr);

extern void
usermode_linkall_user  (struct user    *uptr);

/* -------------------------------------------------------------------------- *
 * To call this function is normaly all stuff to do :)                        *
 * -------------------------------------------------------------------------- */
extern int
usermode_make          (struct user    *uptr,      char         **args,
                        struct client  *cptr,      uint32_t       flags);

/* -------------------------------------------------------------------------- *
 * Send a usermode change back to the user                                    *
 * -------------------------------------------------------------------------- */
extern void
usermode_send_local    (struct lclient *lcptr,     struct client *cptr,
                        char           *umcbuf,    char          *umcargs);

/* -------------------------------------------------------------------------- *
 * Send a usermode change to the whole network                                *
 * -------------------------------------------------------------------------- */
extern void
usermode_send_remote   (struct lclient *lcptr,     struct client *cptr,
                        char           *umcbuf,    char          *umcargs);

/* -------------------------------------------------------------------------- *
 * Make a usermode change string after usermode_parse() is called             *
 * -------------------------------------------------------------------------- */
extern void
usermode_change_send   (struct lclient *lcptr,     struct client *cptr,
                        int             remote);


/* -------------------------------------------------------------------------- *
 * Find a usermode by it's character                                          *
 * -------------------------------------------------------------------------- */
extern struct usermode *
usermode_find          (char character);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void
usermode_support       (void);

/* -------------------------------------------------------------------------- *
 * Dump usermodes                                                             *
 * -------------------------------------------------------------------------- */
extern void
usermode_dump          (struct usermode *umptr);


#endif /* SRC_CHANMODE_H */
