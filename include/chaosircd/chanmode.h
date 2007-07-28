/* chaosircd - pi-networks irc server
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
 * $Id: chanmode.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#ifndef SRC_CHANMODE_H
#define SRC_CHANMODE_H

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define CHANMODE_PER_LINE 16       /* max. 16 modes per line (for server-server) */

#define CHANMODE_ADD   0
#define CHANMODE_DEL   1
#define CHANMODE_QUERY 2

#define CHANMODE_TYPE_SINGLE    0x01
#define CHANMODE_TYPE_LIMIT     0x02
#define CHANMODE_TYPE_KEY       0x04
#define CHANMODE_TYPE_LIST      0x08
#define CHANMODE_TYPE_PRIVILEGE 0x10
#define CHANMODE_TYPE_ALL       0x1f
#define CHANMODE_TYPE_NONPRIV   0x0f

#define CHANMODE_FLAG_NONE      0x0000000000000000LLU
#define CHANMODE_FLAG_0x40      0x0000000000000001LLU
#define CHANMODE_FLAG_A         0x0000000000000002LLU
#define CHANMODE_FLAG_B         0x0000000000000004LLU
#define CHANMODE_FLAG_C         0x0000000000000008LLU
#define CHANMODE_FLAG_D         0x0000000000000010LLU
#define CHANMODE_FLAG_E         0x0000000000000020LLU
#define CHANMODE_FLAG_F         0x0000000000000040LLU
#define CHANMODE_FLAG_G         0x0000000000000080LLU
#define CHANMODE_FLAG_H         0x0000000000000100LLU
#define CHANMODE_FLAG_I         0x0000000000000200LLU
#define CHANMODE_FLAG_J         0x0000000000000400LLU
#define CHANMODE_FLAG_K         0x0000000000000800LLU
#define CHANMODE_FLAG_L         0x0000000000001000LLU
#define CHANMODE_FLAG_M         0x0000000000002000LLU
#define CHANMODE_FLAG_N         0x0000000000004000LLU
#define CHANMODE_FLAG_O         0x0000000000008000LLU
#define CHANMODE_FLAG_P         0x0000000000010000LLU
#define CHANMODE_FLAG_Q         0x0000000000020000LLU
#define CHANMODE_FLAG_R         0x0000000000040000LLU
#define CHANMODE_FLAG_S         0x0000000000080000LLU
#define CHANMODE_FLAG_T         0x0000000000100000LLU
#define CHANMODE_FLAG_U         0x0000000000200000LLU
#define CHANMODE_FLAG_V         0x0000000000400000LLU
#define CHANMODE_FLAG_W         0x0000000000800000LLU
#define CHANMODE_FLAG_X         0x0000000001000000LLU
#define CHANMODE_FLAG_Y         0x0000000002000000LLU
#define CHANMODE_FLAG_Z         0x0000000004000000LLU
#define CHANMODE_FLAG_0x5B      0x0000000008000000LLU
#define CHANMODE_FLAG_0x5C      0x0000000010000000LLU
#define CHANMODE_FLAG_0x5D      0x0000000020000000LLU
#define CHANMODE_FLAG_0x5E      0x0000000040000000LLU
#define CHANMODE_FLAG_0x5F      0x0000000080000000LLU
#define CHANMODE_FLAG_0x60      0x0000000100000000LLU
#define CHANMODE_FLAG_a         0x0000000200000000LLU
#define CHANMODE_FLAG_b         0x0000000400000000LLU
#define CHANMODE_FLAG_c         0x0000000800000000LLU
#define CHANMODE_FLAG_d         0x0000001000000000LLU
#define CHANMODE_FLAG_e         0x0000002000000000LLU
#define CHANMODE_FLAG_f         0x0000004000000000LLU
#define CHANMODE_FLAG_g         0x0000008000000000LLU
#define CHANMODE_FLAG_h         0x0000010000000000LLU
#define CHANMODE_FLAG_i         0x0000020000000000LLU
#define CHANMODE_FLAG_j         0x0000040000000000LLU
#define CHANMODE_FLAG_k         0x0000080000000000LLU
#define CHANMODE_FLAG_l         0x0000100000000000LLU
#define CHANMODE_FLAG_m         0x0000200000000000LLU
#define CHANMODE_FLAG_n         0x0000400000000000LLU
#define CHANMODE_FLAG_o         0x0000800000000000LLU
#define CHANMODE_FLAG_p         0x0001000000000000LLU
#define CHANMODE_FLAG_q         0x0002000000000000LLU
#define CHANMODE_FLAG_r         0x0004000000000000LLU
#define CHANMODE_FLAG_s         0x0008000000000000LLU
#define CHANMODE_FLAG_t         0x0010000000000000LLU
#define CHANMODE_FLAG_u         0x0020000000000000LLU
#define CHANMODE_FLAG_v         0x0040000000000000LLU
#define CHANMODE_FLAG_w         0x0080000000000000LLU
#define CHANMODE_FLAG_x         0x0100000000000000LLU
#define CHANMODE_FLAG_y         0x0200000000000000LLU
#define CHANMODE_FLAG_z         0x0400000000000000LLU
#define CHANMODE_FLAG_0x7B      0x0800000000000000LLU
#define CHANMODE_FLAG_0x7C      0x1000000000000000LLU
#define CHANMODE_FLAG_0x7D      0x2000000000000000LLU
#define CHANMODE_FLAG_0x7E      0x4000000000000000LLU
#define CHANMODE_FLAG_0x7F      0x8000000000000000LLU
#define CHANMODE_FLAG_ALL       0x07fffffe07fffffeLLU

#define CHFLG(c) (CHANMODE_FLAG_ ## c)

#define chanmode_index(c) ((uint32_t)c - 0x40)

#define CHANMODE_FLAG_LETTER 0 
#define CHANMODE_FLAG_PREFIX 1
  
/* -------------------------------------------------------------------------- *
 * Types                                                                      *
 * -------------------------------------------------------------------------- */
struct channel;
struct client;
struct lclient;
struct chanuser;
struct chanmodechange;

typedef int (chanmode_callback_t)(struct lclient *, struct client         *,
                                  struct channel *, struct chanuser       *,
                                  struct list    *, struct chanmodechange *);

struct chanmode {
  char                 letter;
  char                 prefix;   /* prefix for privileges */
  uint32_t             type;     /* type of the mode */
  uint64_t             need;     /* the flags you need to change this mode */
  int                  order;    /* -1 on non-privileges */
  chanmode_callback_t *cb;       /* the callback which changes this mode */
  const char         **help;
  uint64_t             flag;     /* mode flag */
};

struct chanmodechange {
  struct node      node;
  struct chanmode *mode;                    /* mode that gets changed */
  int              what;                    /* add/del/query */
  struct client   *acptr;                   /* argument for privilege modes */
  struct chanuser *target;                  /* target for privilege modes */
  int              bounced;                 /* mode has been bounced */
  time_t           ts;
  char            *nmask;
  char            *umask;
  char            *hmask;
  uint32_t         ihash;
  char             info[IRCD_PREFIXLEN + 1];
  char             arg[IRCD_PREFIXLEN + 1]; /* arg as a string */
};

struct chanmodeitem {
  struct node node;
  char        mask[IRCD_PREFIXLEN + 1];
  char        nmask[IRCD_NICKLEN + 1];
  char        umask[IRCD_USERLEN + 1];
  char        hmask[IRCD_HOSTLEN + 1];
  char        info[IRCD_PREFIXLEN + 1];
  time_t      ts;
  uint32_t    ihash;
};

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
extern int             chanmode_log;
/*struct sheap    chanmode_heap;
struct sheap    chanmode_item_heap;*/
extern struct chanmode chanmode_table      [0x40];
extern char            chanmode_flags      [CHANMODE_PER_LINE * 2 + 1];
extern char            chanmode_args_id    [IRCD_LINELEN];
extern char            chanmode_args_nick  [IRCD_LINELEN];
extern char            chanmode_cmd_id     [IRCD_LINELEN];
extern char            chanmode_cmd_nick   [IRCD_LINELEN];
extern char            chanmode_cmd_prefix [IRCD_LINELEN];

/* -------------------------------------------------------------------------- *
 * Initialize the chanmode module                                             *
 * -------------------------------------------------------------------------- */
extern void             chanmode_init           (void);

/* -------------------------------------------------------------------------- *
 * Shut down the chanmode module                                              *
 * -------------------------------------------------------------------------- */
extern void             chanmode_shutdown       (void);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct chanmode *chanmode_register       (struct chanmode       *cmptr);
  
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int              chanmode_unregister     (struct chanmode       *cmptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct chanmodechange *
                        chanmode_change_add     (struct list           *list,
                                                 int                    what,
                                                 char                   mode, 
                                                 char                  *arg,
                                                 struct chanuser       *acuptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct chanmodechange *
                        chanmode_change_insert  (struct list           *list,
                                                 struct chanmodechange *before,
                                                 int                    what,
                                                 char                   mode, 
                                                 char                  *arg);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void             chanmode_change_destroy (struct list *list);
    

/* -------------------------------------------------------------------------- *
 * Parse channel mode changes                                                 *
 *                                                                            *
 * <cptr>            remote mode change source                                *
 * <chptr>           channel the mode will be changed on                      *
 * <cuptr>           chanlink of the mode change source                       *
 * <modes>           what+flags                                               *
 * <args>            args                                                     *
 * <changes>         mode change array                                        *
 * <pc>              parse only this many changes                             *
 *                                                                            *
 * if a chanuser is present then <cptr> and <chptr> will be ignored.          *
 * -------------------------------------------------------------------------- */
extern void             chanmode_parse          (struct lclient        *lcptr,
                                                 struct client         *cptr,
                                                 struct channel        *chptr,
                                                 struct chanuser       *cuptr,
                                                 struct list           *list,
                                                 char                  *modes,
                                                 char                  *args,
                                                 uint32_t               pc);

/* -------------------------------------------------------------------------- *
 * Apply channel mode changes                                                 *
 *                                                                            *
 * <lcptr>           local mode change source                                 *
 * <cptr>            remote mode change source                                *
 * <chptr>           channel the mode will be changed on                      *
 * <cuptr>           chanlink of the mode change source                       *
 * <changes>         mode change array                                        *
 * <n>               apply this many changes                                  *
 *                                                                            *
 * if a chanuser is present then <cptr> and <chptr> will be ignored.          *
 * -------------------------------------------------------------------------- */
extern uint32_t         chanmode_apply          (struct lclient        *lcptr,
                                                 struct client         *cptr,
                                                 struct channel        *chptr,
                                                 struct chanuser       *cuptr,
                                                 struct list           *lptr);

/* -------------------------------------------------------------------------- *
 *  * -------------------------------------------------------------------------- */
extern void             chanmode_send_local     (struct client         *cptr, 
                                                 struct channel        *chptr,
                                                 struct node           *nptr, 
                                                 size_t                 n);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void             chanmode_send_remote    (struct lclient        *lcptr,
                                                 struct client         *cptr,
                                                 struct channel        *chptr,
                                                 struct node           *nptr);
  
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void             chanmode_send           (struct lclient        *lcptr,
                                                 struct client         *cptr,
                                                 struct channel        *chptr,
                                                 struct list           *lptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern uint32_t         chanmode_flags_build    (char                  *dst,
                                                 int                    types, 
                                                 uint64_t               flags);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern uint32_t         chanmode_args_build     (char                  *dst,
                                                 struct channel        *chptr);
  
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void             chanmode_show           (struct client         *cptr,
                                                 struct channel        *chptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int              chanmode_bounce_simple  (struct lclient        *lcptr, 
                                                 struct client         *cptr,
                                                 struct channel        *chptr, 
                                                 struct chanuser       *cuptr,
                                                 struct list           *lptr,  
                                                 struct chanmodechange *cmcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int              chanmode_bounce_ban     (struct lclient        *lcptr, 
                                                 struct client         *cptr,
                                                 struct channel        *chptr,
                                                 struct chanuser       *cuptr,
                                                 struct list           *lptr,  
                                                 struct chanmodechange *cmcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int              chanmode_bounce_mask    (struct lclient        *lcptr, 
                                                 struct client         *cptr,
                                                 struct channel        *chptr,
                                                 struct chanuser       *cuptr,
                                                 struct list           *lptr,  
                                                 struct chanmodechange *cmcptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int              chanmode_match_ban      (struct client         *cptr, 
                                                 struct channel        *chptr,
                                                 struct list           *mlptr);  

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int              chanmode_match_amode    (struct client         *cptr, 
                                                 struct channel        *chptr,
                                                 struct list           *mlptr);  

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int              chanmode_match_deny     (struct client         *cptr, 
                                                 struct channel        *chptr,
                                                 struct list           *mlptr);  

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern int              chanmode_mask_add       (struct client         *cptr,
                                                 struct list           *mlptr,
                                                 struct chanmodechange *cmcptr);
  
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void             chanmode_mask_delete    (struct list           *mlptr, 
                                                 struct chanmodeitem   *cmiptr);  

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void             chanmode_prefix_make    (char                  *buf,
                                                 uint64_t               flags);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void             chanmode_changes_make   (struct list           *list,
                                                 int                    what, 
                                                 struct chanuser       *cuptr);  

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#ifdef DEBUG
extern void             chanmode_changes_dump   (struct list           *lptr);
#endif /* DEBUG */

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern uint64_t         chanmode_prefix_parse   (const char            *pfx);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void             chanmode_list           (struct client         *cptr,
                                                 struct channel        *chptr, 
                                                 char                   c);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern struct node     *chanmode_assemble_list  (char                  *buf,
                                                 struct node           *nptr, 
                                                 size_t                 len);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void             chanmode_introduce      (struct lclient        *lcptr, 
                                                 struct client         *cptr,
                                                 struct channel        *chptr,
                                                 struct node           *nptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern size_t           chanmode_burst          (struct lclient        *lcptr,
                                                 struct channel        *chptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void             chanmode_drop           (struct client         *cptr, 
                                                 struct channel        *chptr);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
extern void             chanmode_support        (void);

#endif /* SRC_CHANMODE_H */
