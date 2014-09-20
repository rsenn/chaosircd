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
 * $Id: server.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_SERVER_H
#define SRC_SERVER_H

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include <stdarg.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define SERVER_LOCAL  0
#define SERVER_REMOTE 1

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct class;
struct channel;

struct stats
{
  struct node node;
  char        letter;
  void      (*cb)(struct client *);
};

struct capab
{
  char    *name;
  uint32_t cap;
};

struct cryptcap
{
  char    *name;     /* name of capability (cipher name) */
  uint32_t cap;      /* mask value */
  int      keylen;   /* keylength (bytes) */
  int      cipherid; /* ID number of cipher type (3DES, AES, IDEA) */
};

struct burst
{
  uint32_t sendq;
  uint32_t servers;
  uint32_t clients;
  uint32_t channels;
  uint32_t chanmodes;
};

struct server
{
  struct node     node;
  struct node     rnode;
  struct node     lnode;
  struct node     snode;
  uint32_t        id;
  int             refcount;
  hash_t          hash;
  int             location;
  struct client  *client;
  struct lclient *lclient;
  struct connect *connect;          /* connect{} pointer for this server */
  struct list     deps[3];          /* servers and users on this server */
  uint64_t        bstart;           /* burst start */
  uint32_t        cserial;          /* channel burst serial */
  struct burst    in;
  struct burst    out;
  uint8_t         progress;
  char            name[IRCD_HOSTLEN];
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */

#define CAP_CAP         0x00000001   /* received a CAP to begin with */
#define CAP_HUB         0x00000002   /* This server is a HUB */
#define CAP_EOB         0x00000004   /* Can do EOB message */
#define CAP_UID         0x00000008   /* Can do UIDs */
#define CAP_KLN         0x00000010   /* Can do KLINE message */
#define CAP_GLN         0x00000020   /* Can do GLINE message */
#define CAP_HOP         0x00000040   /* can do half ops (+h) */
#define CAP_PAR         0x00000080
#define CAP_SPF         0x00000100
#define CAP_RW          0x00000200   /* Does remote whois */
#define CAP_DE          0x00000400   /* supports deny */
#define CAP_AM          0x00000800   /* supports automode */
#define CAP_NSV         0x00001000   /* supports NSERVER */
#define CAP_CLK         0x00002000   /* supports clock sync */
#define CAP_TS          0x00004000
#define CAP_SVC         0x00008000
#define CAP_SSL         0x00010000

#define CAP_MASK       (CAP_HUB | CAP_EOB | CAP_UID | CAP_KLN | CAP_GLN | \
                        CAP_HOP | CAP_PAR | CAP_SPF | CAP_RW  | CAP_DE  |  \
                        CAP_AM  | CAP_SSL | CAP_NSV | CAP_CLK)
#define CAP_DEFAULT    (CAP_HUB | CAP_UID | CAP_SSL | CAP_SVC)

/* -------------------------------------------------------------------------- *
 * Cipher ID numbers                                                          *
 * - DO NOT CHANGE THESE!  Otherwise you break backwards compatibility        *
 *      If you wish to add a new cipher, append it to the list.  Do not       *
 *      have it's value replace another.                                      *
 * -------------------------------------------------------------------------- */
#define CIPHER_AES_128  1
#define CIPHER_AES_192  2
#define CIPHER_AES_256  3
#define CIPHER_3DES     4
#define CIPHER_IDEA     5

#define CAP_ENC_AES_128         0x00000001
#define CAP_ENC_AES_192         0x00000002
#define CAP_ENC_AES_256         0x00000004
#define CAP_ENC_3DES_168        0x00000008
#define CAP_ENC_IDEA_128        0x00000010

#define CAP_ENC_ALL             0xffffffff
#define CAP_ENC_MASK            CAP_ENC_ALL

#define NOCAP  0
#define NOCAPS 0
#define CAP_NONE 0


#define IsCapableEnc(x, cap)    ((x)->local->enc_caps &   (cap))
#define SetCapableEnc(x, cap)   ((x)->local->enc_caps |=  (cap))
#define ClearCapEnc(x, cap)     ((x)->local->enc_caps &= ~(cap))

#define DoesCAP(x)      ((x)->caps)

#define server_is_capable(x, cap)       ((x)->caps &   (cap))

#define SetCapable(x, cap)      ((x)->local->caps |=  (cap))
#define ClearCap(x, cap)        ((x)->local->caps &= ~(cap))

#define SERVER_NOCRYPT 0
#define SERVER_CRYPT   1

#define SERVER_NOTS 0
#define SERVER_TS   1

#define server_is_local(s) (s->location == SERVER_LOCAL)
#define server_is_remote(s) (s->location == SERVER_REMOTE)

/* -------------------------------------------------------------------------- *
  * -------------------------------------------------------------------------- */
IRCD_DATA(int               )server_log;
IRCD_DATA(struct sheap      )server_heap;
IRCD_DATA(struct sheap      )server_stats_heap;
IRCD_DATA(struct timer     *)server_timer;
IRCD_DATA(struct list       )server_list;
IRCD_DATA(struct list       )server_lists[2];
IRCD_DATA(struct server    *)server_me;
IRCD_DATA(uint32_t          )server_id;
IRCD_DATA(uint32_t          )server_serial;
IRCD_DATA(struct stats      )server_stats[];
IRCD_DATA(int               )server_default_caps;
IRCD_DATA(int               )server_default_cipher;
IRCD_DATA(struct capab      )server_caps[];
IRCD_DATA(struct cryptcap   )server_ciphers[];

/* ------------------------------------------------------------------------ */
IRCD_API(int) server_get_log(void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_init        (void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_shutdown    (void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct server *)server_new         (const char     *name,
                                             int             location);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct server *)server_new_local   (struct lclient *lcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct server *)server_new_remote  (struct lclient *lcptr,
                                             struct client  *cptr,
                                             const char     *name,
                                             const char     *info);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_delete      (struct server  *sptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_exit        (struct lclient *lcptr,
                                             struct client  *cptr,
                                             struct server  *sptr,
                                             const char     *reason);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_release     (struct server  *sptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_set_name    (struct server  *sptr,
                                             const char     *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct server *)server_find_id     (uint32_t        id);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct server *)server_find_name   (const char     *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(struct server *)server_find_namew  (struct client  *cptr,
                                             const char     *name);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int            )server_connect     (struct client  *sptr,
                                             struct connect *connect);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int            )server_relay       (struct lclient *lcptr,
                                             struct client  *cptr,
                                             struct server  *sptr,
                                             const char     *format,
                                             char          **argv);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int            )server_relay_always(struct lclient *lcptr,
                                             struct client  *cptr,
                                             uint32_t        sindex,
                                             const char     *format,
                                             int            *argc,
                                             char          **argv);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int            )server_relay_maybe (struct lclient *lcptr,
                                             struct client  *cptr,
                                             uint32_t        sindex,
                                             const char     *format,
                                             int            *argc,
                                             char          **argv);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int            )server_find_capab  (const char     *capstr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_send_pass   (struct lclient *lcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_send_capabs (struct lclient *lcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_send_server (struct lclient *lcptr);

/* -------------------------------------------------------------------------- *
 * After valid PASS/CAPAB/SERVER has been received                            *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_login       (struct lclient *lcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_register    (struct lclient *lcptr,
                                             struct class   *clptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_introduce   (struct lclient *lcptr,
                                             struct server  *sptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_links       (struct client  *cptr,
                                             struct server  *sptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_burst       (struct lclient *cptr);


/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(int            )server_ping        (struct lclient *lcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_vsend       (struct lclient *one,
                                             struct channel *chptr,
                                             uint64_t        caps,
                                             uint64_t        nocaps,
                                             const char     *format,
                                             va_list         args);

IRCD_API(void           )server_send        (struct lclient *one,
                                             struct channel *chptr,
                                             uint64_t        caps,
                                             uint64_t        nocaps,
                                             const char     *format,
                                             ...);

/* -------------------------------------------------------------------------- *
 * Get a reference to a server block                                          *
 * -------------------------------------------------------------------------- */
IRCD_API(struct server *)server_pop         (struct server  *sptr);

/* -------------------------------------------------------------------------- *
 * Push back a reference to a server block                                    *
 * -------------------------------------------------------------------------- */
IRCD_API(struct server *)server_push        (struct server **sptrptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_stats_register   (char   letter,
                                                  void (*cb)(struct client *));

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_stats_unregister (char   letter);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_stats_show       (struct client *cptr,
                                                  char           letter);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
IRCD_API(void           )server_dump             (struct server  *sptr);

#endif /* SRC_SERVER_H */


