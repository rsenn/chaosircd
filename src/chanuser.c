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
 * $Id: chanuser.c,v 1.3 2006/09/28 09:44:11 roman Exp $
 */

#define _GNU_SOURCE

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "defs.h"
#include "io.h"
#include "dlink.h"
#include "hook.h"
#include "log.h"
#include "mem.h"
#include "str.h"

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <ircd/ircd.h>
#include <ircd/user.h>
#include <ircd/chanuser.h>
#include <ircd/chanmode.h>
#include <ircd/channel.h>
#include <ircd/numeric.h>
#include <ircd/lclient.h>
#include <ircd/client.h>
#include <ircd/server.h>
#include <ircd/chars.h>

/* -------------------------------------------------------------------------- *
 * Global variables                                                           *
 * -------------------------------------------------------------------------- */
int          chanuser_log;
struct sheap chanuser_heap;
uint32_t     chanuser_serial;

/* -------------------------------------------------------------------------- *
 * Initialize the chanuser module                                             *
 * -------------------------------------------------------------------------- */
void chanuser_init(void)
{
  chanuser_log = log_source_register("chanuser");

  chanuser_serial = 0;

  mem_static_create(&chanuser_heap, sizeof(struct chanuser),
                    CHANUSER_BLOCK_SIZE);
  mem_static_note(&chanuser_heap, "chanuser heap");

  log(chanuser_log, L_status, "Initialised [chanuser] module.");
}

/* -------------------------------------------------------------------------- *
 * Shut down the chanuser module                                              *
 * -------------------------------------------------------------------------- */
void chanuser_shutdown(void)
{
  log(chanuser_log, L_status, "Shutting down [chanuser] module...");

  mem_static_destroy(&chanuser_heap);

  log_source_unregister(chanuser_log);
}

/* -------------------------------------------------------------------------- *
 * Create a chanuser block                                                    *
 * -------------------------------------------------------------------------- */
struct chanuser *chanuser_new(struct channel *chptr, struct client *cptr)
{
  struct chanuser *cuptr;

  cuptr = mem_static_alloc(&chanuser_heap);

  cuptr->channel = channel_pop(chptr);
  cuptr->client = client_pop(cptr);
  cuptr->flags = 0LLU;
  cuptr->serial = chanuser_serial;
  cuptr->local = 0;

  cuptr->prefix[0] = '\0';

  return cuptr;
}

/* -------------------------------------------------------------------------- *
 * Create a chanuser block, link it to the channel and the user and           *
 * set the flags                                                              *
 * -------------------------------------------------------------------------- */
struct chanuser *chanuser_add(struct channel *chptr, struct client *cptr)
{
  struct chanuser *cuptr = NULL;

  if(client_is_user(cptr) || client_is_service(cptr))
  {
    /* Get a chanuser block */
    cuptr = chanuser_new(chptr, cptr);

    /* Add to global member list */
    dlink_add_tail(&chptr->chanusers, &cuptr->gnode, cuptr);

    /* If the client is local then add to local member list */
    cuptr->local = client_is_local(cptr);

    if(cuptr->local)
      dlink_add_tail(&chptr->lchanusers, &cuptr->lnode, cuptr);
    else
      dlink_add_tail(&chptr->rchanusers, &cuptr->rnode, cuptr);

    /* Add to the users channel list */
    dlink_add_tail(&cptr->user->channels, &cuptr->unode, cuptr);
  }

  return cuptr;
}

/* -------------------------------------------------------------------------- *
 * Find a chanuser block                                                      *
 * -------------------------------------------------------------------------- */
struct chanuser *chanuser_find(struct channel *chptr, struct client *cptr)
{
  struct chanuser *cuptr;

  /* We search the client->user->channels list because this
     list is usually smaller than the channel member list */
  if(client_is_user(cptr) || client_is_service(cptr))
  {
    /* Walk through the channel links of the user */
    dlink_foreach(&cptr->user->channels, cuptr)
      if(cuptr->channel == chptr)
        return cuptr;
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * Find a chanuser block and warn                                             *
 * -------------------------------------------------------------------------- */
struct chanuser *chanuser_find_warn(struct client  *cptr,
                                    struct channel *chptr,
                                    struct client  *acptr)
{
  struct chanuser *cuptr;

  cuptr = chanuser_find(chptr, acptr);

  if(cuptr)
    return cuptr;

  if(client_is_user(cptr))
  {
    if(acptr == cptr)
      numeric_send(cptr, ERR_NOTONCHANNEL, chptr->name);
    else
      numeric_send(cptr, ERR_USERNOTINCHANNEL, chptr->name, acptr->name);
  }

  return NULL;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- *
 * Delete chanuser block                                                      *
 * -------------------------------------------------------------------------- */
void chanuser_delete(struct chanuser *cuptr)
{
  struct channel *chptr;

  chptr = cuptr->channel;

  /* Unlink from the users channel links */
  dlink_delete(&cuptr->client->user->channels, &cuptr->unode);

  /* Unlink from global memberlist */
  dlink_delete(&cuptr->channel->chanusers, &cuptr->gnode);

  /* If the client is local then unlink from local memberlist */
  if(cuptr->local)
    dlink_delete(&cuptr->channel->lchanusers, &cuptr->lnode);
  else
    dlink_delete(&cuptr->channel->rchanusers, &cuptr->rnode);

  /* Now free the block */
  mem_static_free(&chanuser_heap, cuptr);
}

/* -------------------------------------------------------------------------- *
 * Show all chanusers to a client                                             *
 * -------------------------------------------------------------------------- */
void chanuser_show(struct client   *cptr,  struct channel *chptr,
                   struct chanuser *cuptr, int             eon)
{
  struct chanuser *cu2ptr;
  struct node     *node;
  char             buf[IRCD_LINELEN - 1];
  size_t           len;
  int              did_send = 0;
  struct node     *nptr;
  struct list      list;

  /* User requesting the names is actually a member */
  if(cuptr)
  {
    cptr = cuptr->client;
    chptr = cuptr->channel;
  }

  dlink_list_zero(&list);

  /* Don't assembe list when channel is secret and the user is not member */
  if(!((chptr->modes & CHFLG(s)) && cuptr == NULL))
  {
    dlink_foreach_data(&chptr->chanusers, node, cu2ptr)
    {
/*      if(cu2ptr->client->user->modes & UFLG(i) && cuptr == NULL)
        continue;*/

      nptr = dlink_node_new();
      dlink_add_tail(&list, nptr, cu2ptr);
    }
  }

  /* Reply names list */
  nptr = list.head;

  if(nptr)
  {
    len = str_snprintf(buf, sizeof(buf), numeric_format(RPL_NAMREPLY),
                   client_me->name, cptr->name,
                   (chptr->modes & CHFLG(s) ? "@" : "="), chptr->name);
    do
    {
      nptr = chanuser_assemble(&buf[len], nptr,  sizeof(buf) - len, 1, 0);

      client_send(cptr, "%s ", buf);
    }
    while(nptr);

    dlink_destroy(&list);

    did_send = 1;
  }

  /* End of names reply? */
  if(eon == 1 || (eon == 2 && did_send))
    numeric_send(cptr, RPL_ENDOFNAMES, chptr->name);
}

/* -------------------------------------------------------------------------- *
 * Parse chanusers                                                            *
 * -------------------------------------------------------------------------- */
uint32_t chanuser_parse(struct lclient *lcptr,  struct list *lptr,
                        struct channel *chptr,  char        *args,
                        int             prefix)
{
  struct chanuser *cuptr;
  struct client   *acptr;
  struct node     *nptr;
  uint64_t         flags;
  size_t           n;
  size_t           i;
  size_t           modes = 0;
  char            *uv[CHANUSER_PER_LINE + 1];
  char            *pfx;
  char            *usr;

  dlink_list_zero(lptr);

  n = str_tokenize(args, uv, CHANUSER_PER_LINE);

  for(i = 0; i < n; i++)
  {
    pfx = uv[i];
    usr = uv[i];

    while(*usr && !chars_isnickchar(*usr) && !chars_isuidchar(*usr))
      usr++;

    if(usr == pfx)
      pfx = NULL;

    if(prefix && pfx)
      flags = chanmode_prefix_parse(pfx);
    else
      flags = 0LLU;

    if(pfx)
      modes += usr - pfx;

    if(lcptr->caps & CAP_UID)
      acptr = client_find_uid(usr);
    else
      acptr = client_find_nick(usr);

    if(acptr == NULL)
    {
      log(chanuser_log, L_warning, "Invalid user '%s' in NJOIN.",
          usr);
      continue;
    }

    if(client_is_local(acptr))
    {
      log(chanuser_log, L_warning, "Local user '%s' in NJOIN.",
          acptr->name);
      continue;
    }

    if(!channel_is_member(chptr, acptr))
    {
      cuptr = chanuser_new(chptr, acptr);
      nptr = dlink_node_new();

      dlink_add_tail(lptr, nptr, cuptr);

      cuptr->local = client_is_local(acptr);
      cuptr->flags = flags;

      if(flags)
        chanmode_prefix_make(cuptr->prefix, flags);
      else
        cuptr->prefix[0] = '\0';
    }
  }

  return modes;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct node *chanuser_assemble(char  *buf, struct node *nptr,
                               size_t n,   int          of,
                               int    uid)
{
  struct chanuser *cuptr;
  const char      *name;
  uint32_t         count = 0;
  size_t           i = 0;
  size_t           nlen = 0;
  size_t           plen = 0;
  size_t           len;

  buf[i] = '\0';

  if(nptr == NULL)
    return NULL;

  do
  {
    cuptr = nptr->data;
    name = (uid ? cuptr->client->user->uid : cuptr->client->name);
    nlen = str_len(name);
    plen = (of ? 1 : str_len(cuptr->prefix));
    len = nlen + (cuptr->flags ? plen : 0);

    if(i + 2 + len > n)
      break;

    if(i)
      buf[i++] = ' ';

    if(cuptr->flags)
    {
      if(plen == 1)
      {
        buf[i++] = cuptr->prefix[0];
        strcpy(&buf[i], name);
      }
      else
      {
        strcpy(&buf[i], cuptr->prefix);
        i += plen;
        strcpy(&buf[i], name);
      }
    }
    else
    {
      strcpy(&buf[i], name);
    }

    i += nlen;

    count++;
  }
  while((nptr = nptr->next) && count < CHANUSER_PER_LINE);

  return nptr;
}

/* -------------------------------------------------------------------------- *
 * Sends NJOINs to all servers except <lcptr>. if <cptr> is present then the  *
 * message will be prefixed with it.                                          *
 * -------------------------------------------------------------------------- */
void chanuser_introduce(struct lclient *lcptr, struct client *cptr,
                        struct node    *nptr)
{
  struct chanuser *cuptr;
  struct channel  *chptr;
  size_t           len;
  char             buf[IRCD_LINELEN - 1];
  struct node     *node;

  if(nptr == NULL)
    return;

  cuptr = nptr->data;
  chptr = cuptr->channel;

  debug(chanuser_log, "introducing new members to %s", chptr->name);

  if(cptr == NULL || cptr == client_me)
    len = str_snprintf(buf, sizeof(buf), "NJOIN %s %lu :",
                   chptr->name, (unsigned long)(chptr->ts));
  else
    len = str_snprintf(buf, sizeof(buf), ":%s NJOIN %s %lu :",
                   cptr->name, chptr->name, (unsigned long)(chptr->ts));

  for(node = nptr; node;)
  {
    cuptr = node->data;

    node = chanuser_assemble(&buf[len], node, sizeof(buf) - len - 1, 0, 1);

    server_send(lcptr, NULL, CAP_UID, CAP_NONE, "%s", buf);
  }

  for(node = nptr; node;)
  {
    cuptr = node->data;

    node = chanuser_assemble(&buf[len], node, sizeof(buf) - len - 1, 0, 0);

    server_send(lcptr, NULL, CAP_NONE, CAP_UID, "%s", buf);
  }
}

/* -------------------------------------------------------------------------- *
 * Sends NJOINs to one server                                                 *
 * -------------------------------------------------------------------------- */
size_t chanuser_burst(struct lclient *lcptr, struct channel *chptr)
{
  struct chanuser *cuptr;
  struct node     *nptr;
  size_t           len;
  size_t           ret = 0;
  char             buf[IRCD_LINELEN - 1];

  len = str_snprintf(buf, sizeof(buf), "NJOIN %s %lu :",
                 chptr->name, (unsigned long)(chptr->ts));

  for(nptr = chptr->chanusers.head; nptr;)
  {
    cuptr = nptr->data;

    nptr = chanuser_assemble(&buf[len], nptr, sizeof(buf) - len - 1,
                             0, lcptr->caps & CAP_UID);

    lclient_send(lcptr, "%s", buf);

    ret += str_len(cuptr->prefix);
  }

  return ret;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanuser_discharge(struct lclient *lcptr, struct chanuser *cuptr,
                        const char     *reason)
{
  if(reason)
  {
    server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                ":%s PART %s :%s",
                cuptr->client->user->uid,
                cuptr->channel->name,
                reason);
    server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                ":%s PART %s :%s",
                cuptr->client->name,
                cuptr->channel->name,
                reason);
  }
  else
  {
    server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                ":%s PART %s",
                cuptr->client->user->uid,
                cuptr->channel->name);
    server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                ":%s PART %s",
                cuptr->client->name,
                cuptr->channel->name);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanuser_send_joins(struct lclient *lcptr, struct node *nptr)
{
  struct chanuser *cuptr;
  struct channel  *chptr;

  if(nptr == NULL)
    return;

  cuptr = nptr->data;
  chptr = cuptr->channel;

  do
  {
    cuptr = nptr->data;

    channel_send(lcptr, chptr, CHFLG(NONE), CHFLG(NONE),
                 ":%N!%U@%H JOIN %s",
                 cuptr->client,
                 cuptr->client,
                 cuptr->client,
                 chptr->name);
  }
  while((nptr = nptr->next));
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanuser_send_modes(struct lclient *lcptr, struct client *cptr,
                         struct node    *nptr)
{
  struct chanuser *cuptr;
  struct channel  *chptr;
  struct list      modes;

  if(nptr == NULL)
    return;

  cuptr = nptr->data;
  chptr = cuptr->channel;

  dlink_list_zero(&modes);

  do
  {
    cuptr = nptr->data;

    chanmode_changes_make(&modes, CHANMODE_ADD, cuptr);
  }
  while((nptr = nptr->next));

  chanmode_send_local(cptr, chptr, modes.head, IRCD_MODESPERLINE);

  chanmode_change_destroy(&modes);
}

/* -------------------------------------------------------------------------- *
 * Walk through all channels a client is in and message all members           *
 * -------------------------------------------------------------------------- */
void chanuser_vsend(struct lclient *lcptr,  struct client *cptr,
                    const char     *format, va_list        args)
{
  struct chanuser *cuptr;
  struct chanuser *cu2ptr;
  struct client   *acptr;
  struct fqueue    multi;
  struct node     *node;
  size_t           n;
  char             buf[IRCD_LINELEN + 1];

  if(!client_is_user(cptr))
    return;

  client_serial++;

  /* Formatted print */
  n = str_vsnprintf(buf, sizeof(buf) - 2, format, args);

  /* Add line separator */
  buf[n++] = '\r';
  buf[n++] = '\n';

  /* Start a multicast queue */
  io_multi_start(&multi);
  io_multi_write(&multi, buf, n);

  /* Walk through the channels the user is in */
  dlink_foreach(&cptr->user->channels, cuptr)
  {
    /* Walk through the local userlist of this channel */
    dlink_foreach(&cuptr->channel->lchanusers, node)
    {
      cu2ptr = node->data;
      acptr = cu2ptr->client;

      if(acptr->lclient == NULL)
        continue;

      /* Huh? What the hell does a remote user on local chanuser list? */
      if(!client_is_local(acptr))
        continue;

      /* The lcptr we shouldn't send to */
      if(acptr->lclient == lcptr)
        continue;

      /* Did we already send to this client? */
      if(acptr->serial == client_serial)
        continue;

      /* Mark as sent */
      acptr->serial = client_serial;

      /* Link it to the local queue */
      io_multi_link(&multi, acptr->lclient->fds[1]);
      lclient_update_sendb(acptr->lclient, n);
#ifdef DEBUG
      buf[n - 2] = '\0';
      debug(ircd_log_out, "To %s: %s", acptr->lclient->name, buf);
#endif /* DEBUG */
    }
  }

  if(client_is_local(cptr))
  {
    if(cptr->source != lcptr && cptr->serial != client_serial)
    {
      lclient_update_sendb(cptr->source, n);
      io_multi_link(&multi, cptr->source->fds[1]);
#ifdef DEBUG
      buf[n - 2] = '\0';
      debug(ircd_log_out, "To %s: %s", cptr->lclient->name, buf);
#endif /* DEBUG */
    }
  }

  /* End multicasting */
  io_multi_end(&multi);
}

void chanuser_send(struct lclient *lcptr,  struct client *cptr,
                   const char     *format, ...)
{
  va_list args;

  va_start(args, format);
  chanuser_vsend(lcptr, cptr, format, args);
  va_end(args);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanuser_mode_set(struct chanuser *cuptr, uint64_t flags)
{
  cuptr->flags = flags;
  chanmode_prefix_make(cuptr->prefix, cuptr->flags);

  debug(chanuser_log, "new prefix for %s on %s: %s",
        cuptr->client->name, cuptr->channel->name, cuptr->prefix);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int chanuser_mode_bounce(struct lclient *lcptr, struct client         *cptr,
                         struct channel *chptr, struct chanuser       *cuptr,
                         struct list    *lptr,  struct chanmodechange *cmcptr)
{
  struct chanuser *target = cmcptr->target;
  struct chanmode *mode   = cmcptr->mode;

  /* Now apply the changes */
  if(cmcptr->what == CHANMODE_DEL)
  {
    /* Target has not op, bounce the change */
    if(!(target->flags & mode->flag))
      return 1;

    /* Clear the flag */
    chanuser_mode_set(target, target->flags & ~mode->flag);
  }

  if(cmcptr->what == CHANMODE_ADD)
  {
    /* Target already has op, bounce the change */
    if((target->flags & mode->flag))
      return 1;

    /* Set the flag */
    chanuser_mode_set(target, target->flags | mode->flag);
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanuser_drop(struct client *cptr, struct channel *chptr)
{
  struct chanuser *cuptr;
  struct node     *node;
  struct list      modelist;

  dlink_list_zero(&modelist);

  dlink_foreach_data(&chptr->chanusers, node, cuptr)
  {
    chanmode_changes_make(&modelist, CHANMODE_DEL, cuptr);

    cuptr->prefix[0] = '\0';
    cuptr->flags = 0LLU;
  }

#ifdef DEBUG
  chanmode_changes_dump(&modelist);
#endif /* DEBUG */

  chanmode_send_local(cptr, chptr, modelist.head, IRCD_MODESPERLINE);

  chanmode_change_destroy(&modelist);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanuser_whois(struct client *cptr, struct user *auptr)
{
  struct chanuser *cuptr;
  struct chanuser *acuptr;
  size_t           rpllen;
  uint32_t         rplidx;
  char             rplbuf[IRCD_LINELEN];
  size_t           len;

  channel_serial++;

  rpllen = str_snprintf(rplbuf, IRCD_LINELEN, ":%s %03u %s %s :",
                    client_me->name, RPL_WHOISCHANNELS,
                    client_is_local(cptr) ? cptr->name : cptr->user->uid,
                    auptr->client->name);
  rplidx = 0;

  /* common channels first */
  dlink_foreach(&cptr->user->channels, cuptr)
  {
    acuptr = chanuser_find(cuptr->channel, auptr->client);

    if(acuptr)
    {
      len = str_len(acuptr->channel->name) + str_len(acuptr->prefix);

      if(len + rpllen + rplidx + 1 > IRCD_LINELEN - 2)
      {
        client_send(cptr, "%s", rplbuf);
        rplidx = 0;
      }

      if(rplidx != 0)
        rplbuf[rpllen + rplidx++] = ' ';

      str_copy(&rplbuf[rpllen + rplidx], acuptr->prefix);
      str_cat(&rplbuf[rpllen + rplidx], acuptr->channel->name);

      rplidx += len;

      acuptr->channel->serial = channel_serial;
    }
  }

  /* now all non-secret channels */
  dlink_foreach(&auptr->channels, acuptr)
  {
    if(acuptr->channel->serial == channel_serial)
      continue;

    if(hooks_call(chanuser_whois, HOOK_DEFAULT, cptr, auptr->client, acuptr))
      continue;

    len = str_len(acuptr->channel->name) + str_len(acuptr->prefix);

    if(len + rpllen + rplidx + 1 > IRCD_LINELEN - 2)
    {
      client_send(cptr, "%s", rplbuf);
      rplidx = 0;
    }

    if(rplidx != 0)
      rplbuf[rpllen + rplidx++] = ' ';

    strcpy(&rplbuf[rpllen + rplidx], acuptr->prefix);
    strcat(&rplbuf[rpllen + rplidx], acuptr->channel->name);

    rplidx += len;
  }

  if(rplidx)
  {
    client_send(cptr, "%s", rplbuf);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanuser_kick(struct lclient *lcptr,   struct client   *cptr,
                   struct channel *chptr,   struct chanuser *cuptr,
                   char          **targetv, const char      *reason)
{
  uint32_t i;

  for(i = 0; targetv[i]; i++)
  {
    struct chanuser *acuptr;
    struct client   *acptr;

    if(lcptr->caps & CAP_UID)
    {
      if((acptr = client_find_uid(targetv[i])) == NULL)
        continue;
    }
    else
    {
      if((acptr = client_find_nickhw(cptr, targetv[i])) == NULL)
        continue;
    }

    if((acuptr = chanuser_find_warn(cptr, chptr, acptr)) == NULL)
      continue;

    if(hooks_call(chanuser_kick, HOOK_DEFAULT,
                  lcptr, cptr, chptr, cuptr, acuptr, reason) == 0)
    {
      numeric_send(cptr, ERR_CHANOPRIVSNEEDED, chptr->name);
      continue;
    }

    if(client_is_user(cptr))
    {
      channel_send(NULL, chptr, CHFLG(NONE), CHFLG(NONE),
                   ":%N!%U@%H KICK %s %N :%s",
                   cptr, cptr, cptr, chptr->name, acptr, reason);

      server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                  ":%s KICK %s %s :%s",
                  cptr->user->uid, chptr->name, acptr->user->uid, reason);
      server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                  ":%s KICK %s %s :%s",
                  cptr->name, chptr->name, acptr->name, reason);
    }
    else
    {
      channel_send(NULL, chptr, CHFLG(NONE), CHFLG(NONE),
                   ":%N KICK %s %N :%s",
                   cptr, chptr->name, acptr, reason);

      server_send(lcptr, NULL, CAP_UID, CAP_NONE,
                  ":%s KICK %s %s :%s",
                  cptr->name, chptr->name, acptr->user->uid, reason);
      server_send(lcptr, NULL, CAP_NONE, CAP_UID,
                  ":%s KICK %s %s :%s",
                  cptr->name, chptr->name, acptr->name, reason);
    }

    chanuser_delete(acuptr);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void chanuser_support(void)
{
  char prefixes[65];
  char modes[65];

  chanmode_prefix_make(prefixes, CHFLG(ALL));
  chanmode_flags_build(modes, CHANMODE_TYPE_ALL, CHFLG(ALL));

  ircd_support_set("PREFIX", "(%s)%s", modes, prefixes);
}
