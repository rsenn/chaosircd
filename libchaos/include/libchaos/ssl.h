/* chaosircd - pi-networks irc server
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
 * $Id: ssl.h,v 1.5 2006/09/28 08:38:31 roman Exp $   
 */

#ifndef LIB_SSL_H
#define LIB_SSL_H

/* ------------------------------------------------------------------------ * 
 * Library headers                                                          *
 * ------------------------------------------------------------------------ */

#include "libchaos/log.h"
#include "libchaos/dlink.h"
#include "libchaos/io.h"

/* ------------------------------------------------------------------------ * 
 * Constants                                                                *
 * ------------------------------------------------------------------------ */
#define SSL_CONTEXT_SERVER 0
#define SSL_CONTEXT_CLIENT 1

#define SSL_RANDOM_SIZE 2048
#define SSL_RANDOM_FILE "/dev/urandom"

#define SSL_READ    1
#define SSL_WRITE   2
#define SSL_ACCEPT  3
#define SSL_CONNECT 4

#define SSL_WRITE_WANTS_READ    1
#define SSL_READ_WANTS_WRITE    2
#define SSL_ACCEPT_WANTS_READ   3
#define SSL_ACCEPT_WANTS_WRITE  4
#define SSL_CONNECT_WANTS_READ  5
#define SSL_CONNECT_WANTS_WRITE 6

/* ------------------------------------------------------------------------ *
 * SSL context structure, assigned to each filedescriptor with an SSL       *
 * connection                                                               *
 * ------------------------------------------------------------------------ */
struct ssl_context 
{
  struct node                 node;
  uint32_t                    id;
  uint32_t                    refcount;
  const struct ssl_ctx_st    *ctxt;
  const struct ssl_method_st *meth;
  hash_t                      hash;
  int                         context;
  char                        name[64];
  char                        cert[PATHLEN];
  char                        key[PATHLEN];
  char                        ciphers[64];
};

/* ------------------------------------------------------------------------ *
 * Globals                                                                  *
 * ------------------------------------------------------------------------ */
CHAOS_DATA(int                 )ssl_log;
CHAOS_DATA(struct sheap        )ssl_heap;
CHAOS_DATA(struct list         )ssl_list;
CHAOS_DATA(uint32_t            )ssl_id;

/* ------------------------------------------------------------------------ */
CHAOS_API(int) ssl_get_log(void);

/* ------------------------------------------------------------------------ * 
 * Prototypes                                                               *
 * ------------------------------------------------------------------------ */
CHAOS_API(void                )ssl_init       (void);
CHAOS_API(void                )ssl_shutdown   (void);
CHAOS_API(void                )ssl_seed       (void);
CHAOS_API(void                )ssl_default    (struct ssl_context  *scptr);
CHAOS_API(struct ssl_context *)ssl_add        (const char          *name, 
                                               int                  context,
                                               const char          *cert, 
                                               const char          *key,  
                                               const char          *ciphers);  
CHAOS_API(int                 )ssl_update     (struct ssl_context  *scptr,
                                               const char          *name, 
                                               int                  context,
                                               const char          *cert, 
                                               const char          *key,  
                                               const char          *ciphers);  
CHAOS_API(struct ssl_context *)ssl_find_name  (const char          *name);
CHAOS_API(struct ssl_context *)ssl_find_id    (uint32_t             id);
CHAOS_API(int                 )ssl_new        (int                  fd, 
                                               struct ssl_context  *scptr);
CHAOS_API(int                 )ssl_accept     (int                  fd);    
CHAOS_API(int                 )ssl_connect    (int                  fd);    
CHAOS_API(int                 )ssl_read       (int                  fd,
                                               void                *buf,
                                               size_t               n);  
CHAOS_API(int                 )ssl_write      (int                  fd,
                                               const void          *buf,
                                               size_t               n);  
CHAOS_API(const char         *)ssl_strerror   (int                  err);
CHAOS_API(void                )ssl_close      (int                  fd);  
CHAOS_API(int                 )ssl_handshake  (int                  fd,
                                               struct io           *iofd);
CHAOS_API(void                )ssl_cipher     (int                  fd,
                                               char                *ciphbuf, 
                                               size_t               n);
CHAOS_API(struct ssl_context *)ssl_pop        (struct ssl_context  *scptr);
CHAOS_API(struct ssl_context *)ssl_push       (struct ssl_context **scptr);
CHAOS_API(void                )ssl_delete     (struct ssl_context  *scptr);
CHAOS_API(void)                ssl_dump       (struct ssl_context  *scptr);

#endif /* LIB_SSL_H */
