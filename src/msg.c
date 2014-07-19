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
 * $Id: msg.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include <libchaos/defs.h>
#include <libchaos/io.h>
#include <libchaos/log.h>
#include <libchaos/mem.h>
#include <libchaos/str.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#include <ircd/numeric.h>
#include <ircd/lclient.h>
#include <ircd/client.h>
#include <ircd/ircd.h>
#include <ircd/msg.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int         msg_log;
uint32_t    msg_id;
struct list msg_table[MSG_HASH_SIZE];

/* ------------------------------------------------------------------------ */
int msg_get_log() { return msg_log; }

/* -------------------------------------------------------------------------- *
 * Initialize message heap.                                                   *
 * -------------------------------------------------------------------------- */
void msg_init(void)
{
  msg_log = log_source_register("msg");
  msg_id = 0;

  memset(msg_table, 0, MSG_HASH_SIZE * sizeof(struct list));

  log(msg_log, L_status, "Initialised [msg] module.");
}

/* -------------------------------------------------------------------------- *
 * Destroy message heap.                                                      *
 * -------------------------------------------------------------------------- */
void msg_shutdown(void)
{
  struct node *next;
  struct msg  *mptr;
  size_t       i;

  log(msg_log, L_status, "Shutting down [msg] module...");

  for(i = 0; i < MSG_HASH_SIZE; i++)
  {
    dlink_foreach_safe(&msg_table[i], mptr, next)
      dlink_delete(&msg_table[i], &mptr->node);
  }

  log_source_unregister(msg_log);
}

/* -------------------------------------------------------------------------- *
 * Find a message.                                                            *
 * -------------------------------------------------------------------------- */
struct msg *msg_find(const char *name)
{
  struct node *node;
  struct msg  *m;
  hash_t       hash;
  
  hash = str_ihash(name);

  dlink_foreach(&msg_table[hash % MSG_HASH_SIZE], node)
  {
    m = node->data;

    if(m->hash == hash && !str_icmp(m->cmd, name))
      return m;
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct msg *msg_find_id(uint32_t id)
{
  struct node *nptr;
  struct msg  *m = NULL;
  uint32_t     i;

  for(i = 0; i < MSG_HASH_SIZE; i++)
  {
    dlink_foreach_data(&msg_table[i], nptr, m)
      if(m->id == id)
        return m;
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * Register a message.                                                        *
 * -------------------------------------------------------------------------- */
struct msg *msg_register(struct msg *msg)
{
  if(msg_find(msg->cmd))
  {
    log(msg_log, L_warning, "Message %s was already registered.", msg->cmd);
    return NULL;
  }

  msg->hash = str_ihash(msg->cmd);
  msg->id = msg_id++;

  dlink_add_tail(&msg_table[msg->hash % MSG_HASH_SIZE], &msg->node, msg);

  return msg;
}

/* -------------------------------------------------------------------------- *
 * Unregister a message.                                                      *
 * -------------------------------------------------------------------------- */
void msg_unregister(struct msg *msg)
{
  dlink_delete(&msg_table[msg->hash % MSG_HASH_SIZE], &msg->node);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void m_unregistered(struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv)
{
  lclient_send(lcptr, numeric_format(ERR_NOTREGISTERED),
               client_me->name, lcptr->name);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void m_registered(struct lclient *lcptr, struct client *cptr,
                  int             argc,  char         **argv)
{
  lclient_send(lcptr, numeric_format(ERR_ALREADYREGISTRED),
               client_me->name, cptr->name);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void m_ignore(struct lclient *lcptr, struct client *cptr,
              int             argc,  char         **argv)
{
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void m_not_oper(struct lclient *lcptr, struct client *cptr,
                int             argc,  char         **argv)
{
  lclient_send(lcptr, numeric_format(ERR_NOPRIVILEGES),
               client_me->name, argv[0] ? argv[0] : "*");
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void msg_dump(struct msg *mptr)
{
  struct node *nptr;
  uint32_t     i;

  if(mptr == NULL)
  {
    dump(msg_log, "[================ msg summary ================]");

    for(i = 0; i < MSG_HASH_SIZE; i++)
    {
      dlink_foreach_data(&msg_table[i], nptr, mptr)
        dump(msg_log, " #%03u: %-12s (%3u/%3u/%3u/%3u)",
             mptr->id, mptr->cmd,
             mptr->counts[MSG_UNREGISTERED],
             mptr->counts[MSG_CLIENT],
             mptr->counts[MSG_SERVER],
             mptr->counts[MSG_OPER]);
    }

    dump(msg_log, "[============= end of msg summary ============]");
  }
  else
  {
    dump(msg_log, "[================= msg dump =================]");

    dump(msg_log, "        cmd: #%u", mptr->cmd);
    dump(msg_log, "       args: #%u", mptr->args);
    dump(msg_log, "    maxargs: #%u", mptr->maxargs);
    dump(msg_log, "      flags:%s%s%s%s",
         (mptr->flags & MFLG_UNREG ? " MFLG_UNREG" : ""),
         (mptr->flags & MFLG_CLIENT ? " MFLG_CLIENT" : ""),
         (mptr->flags & MFLG_OPER ? " MFLG_OPER" : ""),
         (mptr->flags & MFLG_SERVER ? " MFLG_SERVER" : ""));
    dump(msg_log, "   handlers: %p, %p, %p, %p",
         mptr->handlers[MSG_UNREGISTERED],
         mptr->handlers[MSG_CLIENT],
         mptr->handlers[MSG_SERVER],
         mptr->handlers[MSG_OPER]);
    dump(msg_log, "       help: %p", mptr->help);
    dump(msg_log, "       hash: %p", mptr->hash);
    dump(msg_log, "     counts: %u, %u, %u, %u",
         mptr->counts[MSG_UNREGISTERED],
         mptr->counts[MSG_CLIENT],
         mptr->counts[MSG_SERVER],
         mptr->counts[MSG_OPER]);
    dump(msg_log, "      bytes: %u", mptr->bytes);
    dump(msg_log, "         id: %u", mptr->id);

    dump(msg_log, "[============== end of msg dump =============]");
  }
}
