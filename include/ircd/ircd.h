/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: ircd.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_IRCD_H
#define SRC_IRCD_H

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif /* HAVE_STDINT_H */

#include <stddef.h>

#include <libchaos/dlink.h>

#define CLIENT_BLOCK_SIZE    64
#define LCLIENT_BLOCK_SIZE   32
#define USER_BLOCK_SIZE      32
#define MSG_BLOCK_SIZE       32
#define USERMODE_BLOCK_SIZE  16
#define CHANNEL_BLOCK_SIZE   32
#define CHANMODE_BLOCK_SIZE 128
#define BAN_BLOCK_SIZE      128
#define CLASS_BLOCK_SIZE      8
#define OPER_BLOCK_SIZE       8
#define SERVER_BLOCK_SIZE    16
#define CHANUSER_BLOCK_SIZE 128
#define SUPPORT_BLOCK_SIZE   32
#define STATS_BLOCK_SIZE     16
#define SERVICE_BLOCK_SIZE   16

#define IRCD_IDLEN      8
#define IRCD_CIPHERLEN  8
#define IRCD_COOKIELEN  8
#define IRCD_USERLEN    16
#define IRCD_PROTOLEN   16
#define IRCD_KEYLEN     24
#define IRCD_NICKLEN    32
#define IRCD_PASSWDLEN  32
#define IRCD_CLASSLEN   32
#define IRCD_INFOLEN    512
#define IRCD_CHANNELLEN 128
#define IRCD_TOPICLEN   384
#define IRCD_KICKLEN    256
#define IRCD_AWAYLEN    256
#define IRCD_PREFIXLEN  IRCD_NICKLEN + 1 + \
                        IRCD_USERLEN + 1 + \
                        IRCD_HOSTLEN

#define IRCD_LINELEN    2048
#define IRCD_BUFSIZE    2048

#define IRCD_PATHLEN    64

#define IRCD_HOSTLEN    64
#define IRCD_HOSTIPLEN  16

#define IRCD_MODEBUFLEN 64
#define IRCD_PARABUFLEN 256

#define IRCD_MODESPERLINE 4
#define IRCD_MAXCHANNELS  16
#define IRCD_MAXTARGETS   4
#define IRCD_MAXBANS      64

#define IRCD_STACKSIZE       262144
#define IRCD_LINUX_STACKTOP  0xc0000000

#if defined(WIN32) && !defined(STATIC_IRCD)
# ifdef BUILD_MODULES
#  define IRCD_MODULE(type) extern __attribute__((dllexport)) type
# endif
# ifdef BUILD_IRCD
#  define IRCD_DATA(type) extern __attribute__((dllexport)) type
#  define IRCD_API(type)         __attribute__((dllexport)) type
# else
#  define IRCD_DATA(type) extern __attribute__((dllimport)) type
#  define IRCD_API(type)                                    type
# endif
#endif

#ifndef IRCD_MODULE
#define IRCD_MODULE(type) extern type
#endif

#ifndef IRCD_DATA
#define IRCD_DATA(type) extern type
#endif

#ifndef IRCD_API
#define IRCD_API(type) type
#endif

extern int         ircd_log;
extern int         ircd_log_in;
extern int         ircd_log_out;
extern const char *ircd_package;
extern const char *ircd_version;
extern const char *ircd_release;
extern struct list ircd_support;

typedef enum {
   false = 0,
   true = 1
} bool;

struct support {
  struct node node;
  char        name[64];
  char        value[128];
};

struct client;

/* -------------------------------------------------------------------------- */
IRCD_DATA(int)     ircd_argc;
IRCD_DATA(char **) ircd_argv;
IRCD_DATA(char **) ircd_envp;
IRCD_DATA(char)    ircd_path[PATHLEN];

/* -------------------------------------------------------------------------- *
 * Initialize things.                                                         *
 * -------------------------------------------------------------------------- */
IRCD_API(void) ircd_init(int argc, char **argv, char **envp);

/* -------------------------------------------------------------------------- *
 * Clean things up.                                                           *
 * -------------------------------------------------------------------------- */
IRCD_API(void) ircd_shutdown(void);

/* -------------------------------------------------------------------------- *
 * Loop around some timer stuff and the i/o multiplexer.                      *
 * -------------------------------------------------------------------------- */
IRCD_API(void) ircd_loop(void);

/* -------------------------------------------------------------------------- *
 * Assemble uptime string                                                     *
 * -------------------------------------------------------------------------- */
extern const char     *ircd_uptime       (void);

/* -------------------------------------------------------------------------- *
 * Garbage collect.                                                           *
 * -------------------------------------------------------------------------- */
extern void            ircd_collect      (void);

/* -------------------------------------------------------------------------- *
 * Restart the daemon.                                                        *
 * -------------------------------------------------------------------------- */
extern int             ircd_restart      (void);

/* -------------------------------------------------------------------------- *
 * Clean things up.                                                           *
 * -------------------------------------------------------------------------- */
extern void            ircd_shutdown     (void);

/* -------------------------------------------------------------------------- *
 * Add a new support value                                                    *
 * -------------------------------------------------------------------------- */
extern struct support *ircd_support_new  (void);

/* -------------------------------------------------------------------------- *
 * Find a support entry by name                                               *
 * -------------------------------------------------------------------------- */
extern struct support *ircd_support_find (const char *name);

/* -------------------------------------------------------------------------- *
 * Unset a support value                                                      *
 * -------------------------------------------------------------------------- */
extern void            ircd_support_unset(const char *name);

/* -------------------------------------------------------------------------- *
 * Set a support value                                                        *
 * -------------------------------------------------------------------------- */
extern struct support *ircd_support_set  (const char *name,
                                          const char *value, ...);

/* -------------------------------------------------------------------------- *
 * Show support numeric to a client                                           *
 * -------------------------------------------------------------------------- */
extern void            ircd_support_show (struct client *cptr);

#endif /* SRC_IRCD_H */
