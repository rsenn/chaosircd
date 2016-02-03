/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: cache.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include "libchaos/defs.h"
#include "libchaos/net.h"
#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * System headers                                                             *
 * -------------------------------------------------------------------------- */
#ifdef HAVE_CONFIG_H
<<<<<<< HEAD:libchaos/servauth/cache.c
#include "../config.h"
#endif /* HAVE_CONFIG_H */
=======
#include "config.h"
#endif
>>>>>>> ba8ffb52eff4a0e3c2d6c42458ab53e8ab3d38b7:lib/servauth/cache.c

#include <stdarg.h>
#include <stdlib.h>

#ifdef __CYGWIN__
#include <cygwin/in.h>
#elif  defined(WIN32)
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

/* -------------------------------------------------------------------------- *
 * Program headers                                                            *
 * -------------------------------------------------------------------------- */
#include "servauth/cache.h"
#include "servauth/control.h"
#include "servauth/servauth.h"

/* -------------------------------------------------------------------------- *
 * Get oldest entry with specified status                                     *
 * -------------------------------------------------------------------------- */
static struct cache_entry_auth *
cache_auth_entry_oldest(struct cache_auth *cache, int status)
{
  uint32_t i;
  struct cache_entry_auth *oldest;

  oldest = NULL;

  for(i = 0; i < cache->size; i++)
  {
    if(cache->entries[i].status != status)
      continue;

    if(oldest)
    {
      if(cache->entries[i].created < oldest->created)
        oldest = &cache->entries[i];
    }
    else
      oldest = &cache->entries[i];
  }

  return oldest;
}

/* -------------------------------------------------------------------------- *
 * get oldest entry with specified status                                     *
 * -------------------------------------------------------------------------- */
static struct cache_entry_dns *
cache_dns_entry_oldest(struct cache_dns *cache, int status)
{
  uint32_t i;
  struct cache_entry_dns *oldest;

  oldest = NULL;

  for(i = 0; i < cache->size; i++)
  {
    if(cache->entries[i].status != status)
      continue;

    if(oldest)
    {
      if(cache->entries[i].created < oldest->created)
        oldest = &cache->entries[i];
    }
    else
      oldest = &cache->entries[i];
  }

  return oldest;
}

/* -------------------------------------------------------------------------- *
 * get oldest entry with specified status                                     *
 * -------------------------------------------------------------------------- */
static struct cache_entry_proxy *
cache_proxy_entry_oldest(struct cache_proxy *cache, int status)
{
  uint32_t i;
  struct cache_entry_proxy *oldest;

  oldest = NULL;

  for(i = 0; i < cache->size; i++)
  {
    if(cache->entries[i].status != status)
      continue;

    if(oldest)
    {
      if(cache->entries[i].created < oldest->created)
        oldest = &cache->entries[i];
    }
    else
      oldest = &cache->entries[i];
  }

  return oldest;
}

/* -------------------------------------------------------------------------- *
 * try to get free entry, else free a CACHE_AUTH_TIMEOUT.                     *
 * -------------------------------------------------------------------------- */
static struct cache_entry_auth *
cache_auth_entry_new(struct cache_auth *cache)
{
  struct cache_entry_auth *entry;
  uint32_t i;

  for(i = 0; i < cache->size; i++)
  {
    if(cache->entries[i].status == CACHE_AUTH_NONE)
      break;
  }

  if(i < cache->size)
    return &cache->entries[i];

  entry = cache_auth_entry_oldest(cache, CACHE_AUTH_RESET);

  if(!entry)
    entry = cache_auth_entry_oldest(cache, CACHE_AUTH_TIMEOUT);

  entry->status = CACHE_AUTH_NONE;
  return entry;
}

/* -------------------------------------------------------------------------- *
 * try to get free entry, else free a CACHE_DNS_FORWARD entry if it fails it  *
 * it frees a CACHE_DNS_REVERSE.                                              *
 * -------------------------------------------------------------------------- */
static struct cache_entry_dns *cache_dns_entry_new(struct cache_dns *cache)
{
  struct cache_entry_dns *entry;
  uint32_t i;

  for(i = 0; i < cache->size; i++)
  {
    if(cache->entries[i].status == CACHE_DNS_NONE)
      break;
  }

  if(i < cache->size)
    return &cache->entries[i];

  entry = cache_dns_entry_oldest(cache, CACHE_DNS_FORWARD);

  if(!entry)
    entry = cache_dns_entry_oldest(cache, CACHE_DNS_REVERSE);

  entry->status = CACHE_DNS_NONE;
  return entry;
}

/* -------------------------------------------------------------------------- *
 * try to get free entry, else free a CACHE_PROXY_OPEN.                       *
 * -------------------------------------------------------------------------- */
static struct cache_entry_proxy *
cache_proxy_entry_new(struct cache_proxy *cache)
{
  struct cache_entry_proxy *entry;
  uint32_t i;

  for(i = 0; i < cache->size; i++)
  {
    if(cache->entries[i].status == CACHE_PROXY_NONE)
      break;
  }

  if(i < cache->size)
    return &cache->entries[i];

  entry = cache_proxy_entry_oldest(cache, CACHE_PROXY_CLOSED);

  if(!entry)
    entry = cache_proxy_entry_oldest(cache, CACHE_PROXY_DENIED);

  if(!entry)
    entry = cache_proxy_entry_oldest(cache, CACHE_PROXY_OPEN);

  if(!entry)
    entry = cache_proxy_entry_oldest(cache, CACHE_PROXY_FILTERED);

  if(!entry)
    entry = cache_proxy_entry_oldest(cache, CACHE_PROXY_TIMEOUT);

  entry->status = CACHE_PROXY_NONE;

  return entry;
}

/* -------------------------------------------------------------------------- *
 * generate new cache for timed out and failed auth requests                  *
 * -------------------------------------------------------------------------- */
int cache_auth_new(struct cache_auth *cache, uint32_t size)
{
  cache->size = 0;
  cache->entries = calloc(size, sizeof(struct cache_entry_auth));

  if(cache->entries)
  {
    cache->free = size;
    cache->size = size;
    return 0;
  }

  return -1;
}

/* -------------------------------------------------------------------------- *
 * generate new cache for dns requests                                        *
 * -------------------------------------------------------------------------- */
int cache_dns_new(struct cache_dns *cache, uint32_t size)
{
  cache->size = 0;
  cache->entries = calloc(size, sizeof(struct cache_entry_dns));

  if(cache->entries)
  {
    cache->free = size;
    cache->size = size;
    return 0;
  }

  return -1;
}

/* -------------------------------------------------------------------------- *
 * generate new cache for proxy                                               *
 * -------------------------------------------------------------------------- */
int cache_proxy_new(struct cache_proxy *cache, uint32_t size)
{
  cache->size = 0;
  cache->entries = calloc(size, sizeof(struct cache_entry_proxy));

  if(cache->entries)
  {
    cache->free = size;
    cache->size = size;
    return 0;
  }

  return -1;
}

/* -------------------------------------------------------------------------- *
 * put a failed auth request into cache.                                      *
 * -------------------------------------------------------------------------- */
void cache_auth_put(struct cache_auth *cache, int status, net_addr_t addr,
                    uint64_t t)
{
  struct cache_entry_auth *entry;

  entry = cache_auth_entry_new(cache);

  entry->status = status;
  entry->addr = addr;
  entry->created = t;
}

/* -------------------------------------------------------------------------- *
 * put a dns request into cache.                                              *
 * -------------------------------------------------------------------------- */
void cache_dns_put(struct cache_dns *cache, int status, net_addr_t addr,
                   const char *name, uint64_t t)
{
  struct cache_entry_dns *entry;

  entry = cache_dns_entry_new(cache);

  entry->status = status;
  entry->addr = addr;

  if(name)
    strlcpy(entry->name, name, sizeof(entry->name));
  else
    entry->name[0] = '\0';

  entry->created = t;
}

/* -------------------------------------------------------------------------- *
 * put a proxy request into cache.                                            *
 * -------------------------------------------------------------------------- */
void cache_proxy_put(struct cache_proxy *cache, int status, net_addr_t addr,
                     uint16_t port, int type, uint64_t t)
{
  struct cache_entry_proxy *entry;

  entry = cache_proxy_entry_new(cache);

  entry->status = status;
  entry->addr = addr;
  entry->port = port;
  entry->created = t;
  entry->type = type;
}

/* -------------------------------------------------------------------------- *
 * pick a auth query from cache.                                              *
 * -------------------------------------------------------------------------- */
int cache_auth_pick(struct cache_auth *cache, net_addr_t addr, uint64_t t)
{
  uint32_t i;

  for(i = 0; i < cache->size; i++)
  {

    if(!cache->entries[i].status)
      continue;

    if(cache->entries[i].addr == addr)
    {
      cache->entries[i].created = t;
      return cache->entries[i].status;
    }
  }

  return CACHE_AUTH_NONE;
}

/* -------------------------------------------------------------------------- *
 * pick a reverse dns query from cache.                                       *
 * -------------------------------------------------------------------------- */
int cache_dns_pick_reverse(struct cache_dns *cache, net_addr_t addr,
                           const char **namep, uint64_t t)
{
  uint32_t i;

  for(i = 0; i < cache->size; i++)
  {
    if(cache->entries[i].status != CACHE_DNS_REVERSE)
      continue;

    if(cache->entries[i].addr == addr)
    {
      cache->entries[i].created = t;
      *namep = cache->entries[i].name;
      return 1;
    }
  }

  return 0;
}

/* -------------------------------------------------------------------------- *
 * pick a reverse dns query from cache.                                       *
 * -------------------------------------------------------------------------- */
net_addr_t cache_dns_pick_forward(struct cache_dns *cache,
                                      const char *name, uint64_t t)
{
  uint32_t i;
  net_addr_t null;

  for(i = 0; i < cache->size; i++)
  {
    if(cache->entries[i].status != CACHE_DNS_FORWARD)
      continue;

    if(!str_cmp(cache->entries[i].name, name))
    {
      cache->entries[i].created = t;
      return cache->entries[i].addr;
    }
  }

  null = INADDR_ANY;

  return null;
}

/* -------------------------------------------------------------------------- *
 * pick a proxy query from cache.                                             *
 * -------------------------------------------------------------------------- */
int cache_proxy_pick(struct cache_proxy *cache, net_addr_t addr,
                     uint16_t port, int type, uint64_t t)
{
  uint32_t i;

  for(i = 0; i < cache->size; i++)
  {
    if(!cache->entries[i].status)
      continue;

    if(cache->entries[i].addr == addr &&
       cache->entries[i].port == port &&
       cache->entries[i].type == type)
    {
      cache->entries[i].created = t;
      return cache->entries[i].status;
    }
  }

  return CACHE_PROXY_NONE;
}

