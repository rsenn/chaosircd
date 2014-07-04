/*
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
 * $Id: mem.c,v 1.4 2006/09/28 08:38:31 roman Exp $
 */

#define _GNU_SOURCE

/* ------------------------------------------------------------------------ *
 * DEBUGGING: do an mprotect()ed safety margin around heap blocks....       *
 * ------------------------------------------------------------------------ */
/*#define SAFETY_MARGIN 1*/
#undef SAFETY_MARGIN

/* ------------------------------------------------------------------------ *
 * Library headers                                                          *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/str.h"
#include "libchaos/dlink.h"
#include "libchaos/syscall.h"

/* ------------------------------------------------------------------------ *
 * System headers                                                           *
 * ------------------------------------------------------------------------ */
#include "../config.h"

#include <sys/types.h>

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif /* HAVE_SYS_MMAN_H */

/* ------------------------------------------------------------------------ *
 * Blah! Workaround :p                                                      *
 * ------------------------------------------------------------------------ */
#ifndef MAP_ANON
# ifdef MAP_ANONYMOUS
#  define MAP_ANON MAP_ANONYMOUS
# endif /* MAP_ANONYMOUS */
#endif /* MAP_ANON */

/* ------------------------------------------------------------------------ *
 * For mmap() we either map anonymously using MAP_ANON or we map /dev/zero  *
 * ------------------------------------------------------------------------ */
#ifdef HAVE_MMAP

# ifdef MAP_ANON
#  define MMAP_FLAGS (MAP_PRIVATE|MAP_ANON)
#  define MMAP_FD    (-1)
# else
#  define MMAP_FLAGS (MAP_PRIVATE)
#  define MMAP_FD    (mem_zero)

static int   mem_zero = -1;
# endif /* MAP_ANON */

#endif /* HAVE_MMAP */

/* ------------------------------------------------------------------------ *
 * Global variables                                                         *
 * ------------------------------------------------------------------------ */
int          mem_log;      /* Log source */
struct dheap mem_dheap;    /* Dynamic heap for malloc() and consorts */
struct list  mem_slist;    /* List of static heaps (fixed size blocks) */
struct list  mem_dlist;    /* List of dynamic heaps (arbitrary size blocks) */
uint32_t     mem_id;

/* ------------------------------------------------------------------------ */
int mem_get_log() { return mem_log; }

/* ------------------------------------------------------------------------ *
 * Initialize allocator                                                     *
 * ------------------------------------------------------------------------ */
void mem_init(void)
{
  /* Get a log source */
  mem_log = log_source_register("mem");

#ifdef HAVE_MMAP
# ifndef MAP_ANON

  /* Open /dev/zero if we don't have MAP_ANON */
  mem_zero = io_open(MEM_DEV_ZERO, IO_OPEN_READ|IO_OPEN_WRITE);

  if(mem_zero < 0)
    mem_fatal();

# endif /* MAP_ANON */
#endif /* HAVE_MMAP */

  /* Create the default dynamic heap */
  mem_dynamic_create(&mem_dheap, MEM_DYNAMIC_SIZE);
  mem_dynamic_note(&mem_dheap, "generic dynamic heap");

  mem_id = 0;
}

/* ------------------------------------------------------------------------ *
 * Shutdown allocator                                                       *
 * ------------------------------------------------------------------------ */
void mem_shutdown(void)
{
  /* Destroy the default heap */
  mem_dynamic_destroy(&mem_dheap);

  /* Close /dev/zero if we don't have MAP_ANON */
#ifdef HAVE_MMAP
# ifndef MAP_ANON

  io_close(mem_zero);

# endif /* MAP_ANON */
#endif /* HAVE_MMAP */

  /* Unregister log source */
  log_source_unregister(mem_log);
}

/* ------------------------------------------------------------------------ *
 * Whine and exit if we got no memory                                       *
 * ------------------------------------------------------------------------ */
void mem_fatal(void)
{
  log(mem_log, L_fatal,
      "I'm out of memory and there's nothing I can do about.");

  syscall_exit(1);
}

/* ------------------------------------------------------------------------ *
 * Allocates a block                                                        *
 * ------------------------------------------------------------------------ */
static inline void *mem_block_alloc(size_t size)
{
  void *ptr;

#ifdef HAVE_MMAP
# ifdef SAFETY_MARGIN

  size_t sz = size;
  size += MEM_PAD_BLOCKS * 2;

# endif /* SAFETY_MARGIN */

  ptr = (void *)
    syscall_mmap(NULL, size, PROT_READ|PROT_WRITE, MMAP_FLAGS, MMAP_FD, 0);

  if(ptr == MAP_FAILED)
    return NULL;

# ifdef SAFETY_MARGIN

  syscall_mprotect(ptr, MEM_PAD_BLOCKS, PROT_NONE);
  ptr = (ptrdiff_t)ptr + MEM_PAD_BLOCKS;
  syscall_mprotect((ptrdiff_t)ptr + sz, MEM_PAD_BLOCKS, PROT_NONE);

# endif /* SAFETY_MARGIN */
#else

  /* Fall back to ordinary malloc() if the
     system doesn't support the other methods */
  ptr = malloc(size);

#endif /* HAVE_MMAP */

  return ptr;
}

/* ------------------------------------------------------------------------ *
 * Frees a previously allocated block                                       *
 * ------------------------------------------------------------------------ */
static inline void mem_block_free(void *ptr, size_t size)
{
#ifdef HAVE_MMAP
# ifdef SAFETY_MARGIN

  ptrdiff_t map;

  map = ptr;

  syscall_mprotect(map + size, MEM_PAD_BLOCKS,
                   PROT_READ|PROT_WRITE|PROT_EXEC);

  ptr = map - MEM_PAD_BLOCKS;
  size += MEM_PAD_BLOCKS * 2;

  syscall_mprotect(ptr, MEM_PAD_BLOCKS,
                   PROT_READ|PROT_WRITE|PROT_EXEC);

# endif /* SAFETY_MARGIN */

  syscall_munmap(ptr, size);

#else

  free(ptr);

#endif /* HAVE_MMAP */
}

/* ------------------------------------------------------------------------ *
 * Allocates a new block on a specific heap                                 *
 *                                                                          *
 * heap   -> the heap to allocate the block on                              *
 *                                                                          *
 * Returns zero on success.                                                 *
 * ------------------------------------------------------------------------ */
static int mem_static_block_new(struct sheap *shptr)
{
  struct schunk *scptr;
  struct sblock *sbptr;
  size_t         i;

  /*
   * Allocate block.
   * We need space for the block descriptor and for
   * several chunks (+1 for null termination)
   */
  sbptr = mem_block_alloc(shptr->block_size);

  if(sbptr == NULL)
    return -1;

  sbptr->chunks = (struct schunk *)&sbptr[1];
  sbptr->size = shptr->block_size;

  /* How many chunks this block will have */
  sbptr->free_chunks = shptr->chunks_per_block;

  /* Initialize lists with free/used blocks */
  dlink_list_zero(&sbptr->free_ones);
  dlink_list_zero(&sbptr->used_ones);

  /* Put it in front of the chain in the heap */
  sbptr->next = shptr->base;
  sbptr->heap = shptr;

  scptr = (struct schunk *)&sbptr[1];

  /* Go through the block and initialize newly created chunks */
  for(i = 0; i < shptr->chunks_per_block; i++)
  {
    /* Add to 'free' list */
    dlink_add_head(&sbptr->free_ones, &scptr->node, &scptr[1]);
    scptr = (struct schunk *)((size_t)&scptr[1] + shptr->chunk_size);
  }

  /* Update heap structure */
  shptr->block_count++;
  shptr->free_chunks += shptr->chunks_per_block;
  shptr->base = sbptr;

  return 0;
}

/* ------------------------------------------------------------------------ *
 * Frees a block previously allocated through mem_static_block_delete       *
 * ------------------------------------------------------------------------ */
static void mem_static_block_delete(struct sblock *sbptr)
{
  mem_block_free(sbptr, sbptr->size);
}

/* ------------------------------------------------------------------------ *
 * Get a free chunk from a block.                                         *
 * ------------------------------------------------------------------------ */
static void *mem_static_block_alloc(struct sblock *sbptr)
 {
  struct schunk *scptr;

  sbptr->heap->free_chunks--;
  sbptr->free_chunks--;

  scptr = (struct schunk *)sbptr->free_ones.head;

  scptr->block = sbptr;

  dlink_delete(&sbptr->free_ones, &scptr->node);
  dlink_add_tail(&sbptr->used_ones, &scptr->node, &scptr[1]);

  return scptr->node.data;
}

/* ------------------------------------------------------------------------ *
 * DEBUG FUNCTIONS: see if an chunk is valid.                             *
 * ------------------------------------------------------------------------ */
#ifdef DEBUG
static int mem_static_block_valid(struct sblock *sbptr, struct schunk *scptr)
{
  ssize_t offs;
  size_t ees;

  offs = (size_t)scptr - (size_t)&sbptr[1];
  ees = (sbptr->heap->chunk_size + sizeof(struct schunk));

  if(offs < 0)
    return 0;

  if(offs % ees)
    return 0;

  offs /= ees;

  if(offs >= sbptr->heap->chunks_per_block)
    return 0;

  return 1;
}
#endif /* DEBUG */

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct sheap *mem_static_find(uint32_t id)
{
  struct sheap *shptr;

  dlink_foreach(&mem_slist, shptr)
  {
    if(shptr->id == id)
      return shptr;
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void mem_static_dump(struct sheap *shptr)
{
  if(shptr == NULL)
  {
    uint32_t total_usage = 0;
    uint32_t total_mapped = 0;

    dump(mem_log, "[============== sheap summary ===============]");

    dlink_foreach(&mem_slist, shptr)
    {
      dump(mem_log, " #%u: [%2u x %2uk = %3uk] (%3u/%3u) %-20s",
           shptr->id,
           shptr->block_count, shptr->block_size / 1024,
           (shptr->block_count * shptr->block_size) / 1024,
           (shptr->chunks_per_block * shptr->block_count) - shptr->free_chunks,
           (shptr->chunks_per_block * shptr->block_count),
           shptr->note);

      total_mapped += shptr->block_count * shptr->block_size;
      total_usage += ((shptr->chunks_per_block * shptr->block_count) - shptr->free_chunks) * shptr->chunk_size;
    }

    dump(mem_log, "[=========== end of sheap summary ===========]");

    dump(mem_log, "Total allocated memory: %u bytes", total_usage);
    dump(mem_log, "Total mapped memory: %u bytes", total_mapped);
  }
  else
  {
    struct sblock *sbptr;
    uint32_t       i = 0;

    dump(mem_log, "[============== sheap dump ===============]");

    dump(mem_log, "          id: %u", shptr->id);
    dump(mem_log, "        base: %p", shptr->base);
    dump(mem_log, "  block size: %u", shptr->block_size);
    dump(mem_log, " block count: %u", shptr->block_count);
    dump(mem_log, " free chunks: %u", shptr->free_chunks);
    dump(mem_log, "  chunk size: %u", shptr->chunk_size);
    dump(mem_log, "c. per block: %u", shptr->chunks_per_block);

    dump(mem_log, "-------------- sheap blocks --------------");

    for(sbptr = shptr->base; sbptr; sbptr = sbptr->next)
    {
      dump(mem_log, "#%03u: %3u/%3u (%u/%u bytes)",
           i, sbptr->used_ones.size, shptr->chunks_per_block,
           sbptr->used_ones.size * shptr->chunk_size,
           shptr->chunks_per_block * shptr->chunk_size);
    }

    dump(mem_log, "[=========== end of sheap dump ===========]");
  }
}

/* ------------------------------------------------------------------------ *
 * Frees a block previously allocated through mem_dynamic_alloc()              *
 * ------------------------------------------------------------------------ */
static void mem_dynamic_block_delete(struct dblock *dbptr)
{
  mem_block_free(dbptr, dbptr->size);
}

/* ------------------------------------------------------------------------ *
 * Allocates a new block on a specific dynamic heap                         *
 *                                                                          *
 * heap   -> the heap to allocate the block on                              *
 *                                                                          *
 * Returns zero on success.                                                 *
 * ------------------------------------------------------------------------ */
static struct dblock *mem_dynamic_block_new(struct dheap *dhptr)
{
  struct dblock *dbptr;

  dbptr = (struct dblock *)mem_block_alloc(dhptr->block_size);

  if(dbptr)
  {
    dbptr->size = dhptr->block_size;
    dbptr->next = dhptr->base;
    dbptr->free_bytes = dbptr->size - sizeof(struct dblock);
    dbptr->used_bytes = 0;
    dbptr->max_bytes = dbptr->free_bytes - sizeof(struct dblock);
    dbptr->heap = dhptr;

    dlink_list_zero(&dbptr->chunks);

    dhptr->block_count++;
    dhptr->free_bytes += dbptr->size - sizeof(struct dblock);
    dhptr->max_bytes = dbptr->size - sizeof(struct dblock);

    dhptr->base = dbptr;
  }

  return dbptr;
}

/* ------------------------------------------------------------------------ *
 * Allocate a new dynamic chunk                                             *
 * ------------------------------------------------------------------------ */
static void *mem_dynamic_block_alloc(struct dblock *dbptr, size_t size)
{
  struct dchunk *chnk;
  struct dchunk *newchnk = NULL;
  struct node   *node;
  struct dchunk *end;
  struct dheap  *dhptr;
  size_t         avail;
  size_t         biggest = 0; /* biggest available chunk */

  if(dbptr->chunks.head == NULL)
  {
    /* Chunk is at block start */
    newchnk = (struct dchunk *)&dbptr[1];

    /* Link the chunk */
    dlink_add_head(&dbptr->chunks, &newchnk->node, newchnk);

    /* New biggest continuous chunk */
    biggest = dbptr->size - size - sizeof(struct dchunk);
  }
  else
  {
    /* end points always to the end of the previous chunk */
    end = (struct dchunk *)&dbptr[1];

    /* Check the chunk list for free space */
    dlink_foreach(&dbptr->chunks, node)
    {
      chnk = (struct dchunk *)node;

      /* Available bytes before this chunk */
      avail = (size_t)node - (size_t)end;

      /* Check whether we got space between previous and this chunk */
      if(newchnk == NULL && avail >= size + sizeof(struct dchunk))
      {
        newchnk = end;

        /* Yeah, add it in front of the chunk */
        dlink_add_before(&dbptr->chunks, &end->node, node, end);

        end = (struct dchunk *)(((size_t)&newchnk[1]) + size);
        avail -= size + sizeof(struct dchunk);
      }
      else
      {
        end = (struct dchunk *)(((size_t)&chnk[1]) + chnk->size);
      }

/*      if(avail > biggest)
        biggest = avail;*/
    }

    /* Now check space after last chunk */
    avail = (size_t)&dbptr[1] + dbptr->size - (size_t)end;

    if(newchnk == NULL && (size_t)end + sizeof(struct dchunk) + size <= (size_t)dbptr + dbptr->size)
    {
      newchnk = end;

      dlink_add_tail(&dbptr->chunks, &newchnk->node, newchnk);
    }
    else if(newchnk == NULL)
      return NULL;
#if 0
    {
      /* damn, this shouldn't happen */
      if(avail < size + sizeof(struct dchunk))
      {
        log(mem_log, L_fatal,
            "i should have a chunk of %u bytes, but didn't find it.", size);
      }

      dlink_add_tail(&dbptr->chunks, &end->node, end);

      newchnk = end;
      end = (void *)(((size_t)&newchnk[1]) + size);
      avail -= size + sizeof(struct dchunk);
    }
#endif
    if(avail > biggest)
      biggest = avail;
  }

  /* Initiliaze the chunk */
  newchnk->size = size;
  newchnk->node.data = dbptr;

  dhptr = dbptr->heap;

  if(biggest > dbptr->max_bytes)
  {
    if(biggest > dhptr->max_bytes)
      dhptr->max_bytes = biggest;

    dbptr->max_bytes = biggest;
  }

  dbptr->free_bytes -= sizeof(struct dchunk) + size;
  dbptr->used_bytes += sizeof(struct dchunk) + size;
  dhptr->free_bytes -= sizeof(struct dchunk) + size;
  dhptr->used_bytes += sizeof(struct dchunk) + size;
/*  dbptr->chunk_count++;*/

  return &newchnk[1];
}

/* ------------------------------------------------------------------------ *
 * Remove a chunk from a block.                                             *
 * ------------------------------------------------------------------------ */
static void mem_dynamic_block_free(struct dblock *dbptr, struct dchunk *chnk)
{
  struct dchunk    *tmp;
  struct dheap  *heap;
  void            *bottom;
  void            *top;
  size_t           size;
  size_t           avail;

  heap = dbptr->heap;      /* Points to the heap this block is from */
  size = chnk->size;      /* Size of the chunk to free */

  /* First lets calculate the bottom of the new free space */
  if(chnk->node.prev)
  {
    /* Bottom is at end of previous chunk */
    tmp = (struct dchunk *)chnk->node.prev;
    bottom = (void *)((size_t)&tmp[1] + tmp->size);
  }
  else
  {
    /* No previous chunk, bottom is at block start */
    bottom = &dbptr[1];
  }

  /* Top of new free space */
  if(chnk->node.next)
  {
    /* Top is the start of the next chunk */
    top = chnk->node.next;
  }
  else
  {
    /* Top is the block end */
    top = (void *)((size_t)&dbptr[1] + dbptr->size);
  }

  /* Delete reference to this chunk */
  dlink_delete(&dbptr->chunks, &chnk->node);

  /* Size of the new free space */
  avail = (size_t)top - (size_t)bottom;

  /* Maybe we have a new biggest continuous block */
  if(avail > dbptr->max_bytes)
  {
    dbptr->max_bytes = avail;

    if(avail > heap->max_bytes)
      heap->max_bytes = avail;
  }

  /* Update memory statistics */
  dbptr->free_bytes += size + sizeof(struct dchunk);
  dbptr->used_bytes -= size + sizeof(struct dchunk);
  heap->free_bytes += size + sizeof(struct dchunk);
  heap->used_bytes -= size + sizeof(struct dchunk);
/*  dbptr->chunk_count--;*/

  if(dbptr->chunks.size == 0)
    mem_dynamic_collect(dbptr->heap);
}

/* ------------------------------------------------------------------------ *
 * DEBUG FUNCTIONS: see if a chunk is valid.                                *
 * this is time-consuming and will not be built without -DDEBUG             *
 * ------------------------------------------------------------------------ */
#ifdef DEBUG
static int mem_dynamic_block_valid(struct dblock *dbptr, struct dchunk *chnk)
{
  struct dchunk *ptr;
  struct node   *node;

  /* walk through all chunks */
  dlink_foreach(&dbptr->chunks, node)
  {
    ptr = (struct dchunk *)node;

    /* we found our chunk :D */
    if(ptr == chnk)
      return 1;
  }

  return 0;
}
#endif /* DEBUG */

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct dheap *mem_dynamic_find(uint32_t id)
{
  struct dheap *dhptr;

  dlink_foreach(&mem_dlist, dhptr)
  {
    if(dhptr->id == id)
      return dhptr;
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void mem_dynamic_dump(struct dheap *dhptr)
{
  if(dhptr == NULL)
  {
    dump(mem_log, "[============== dheap summary ===============]");

    dlink_foreach(&mem_dlist, dhptr)
    {
      dump(mem_log, " #%u: [%2u x %2uk = %3uk] (%3u/%3u:%3u) %-20s",
           dhptr->id,
           dhptr->block_count, dhptr->block_size / 1024,
           (dhptr->block_count * dhptr->block_size) / 1024,
           dhptr->used_bytes,
           dhptr->free_bytes + dhptr->used_bytes,
           dhptr->max_bytes,
           dhptr->note);
    }

    dump(mem_log, "[=========== end of dheap summary ===========]");
  }
  else
  {
    struct dblock *dbptr;
    uint32_t       i = 0;

    dump(mem_log, "[============== dheap dump ===============]");

    dump(mem_log, "          id: %u", dhptr->id);
    dump(mem_log, "        base: %p", dhptr->base);
    dump(mem_log, " block count: %u", dhptr->block_count);
    dump(mem_log, "  block size: %u", dhptr->block_size);
    dump(mem_log, "  free bytes: %u", dhptr->free_bytes);
    dump(mem_log, "  used bytes: %u", dhptr->used_bytes);
    dump(mem_log, "   max bytes: %u", dhptr->max_bytes);
    dump(mem_log, "        note: %s", dhptr->note);

    dump(mem_log, "-------------- dheap blocks --------------");

    for(dbptr = dhptr->base; dbptr; dbptr = dbptr->next)
    {
      dump(mem_log, "#%03u: %3u/%3u:%3u (%u chunks)",
           i,
           dbptr->used_bytes,
           dbptr->free_bytes + dbptr->used_bytes,
           dbptr->max_bytes,
           dbptr->chunks.size);
    }

    dump(mem_log, "[=========== end of dheap dump ===========]");
  }
}

/* ------------------------------------------------------------------------ *
 * Create a static heap                                                     *
 *                                                                          *
 * <msptr>          Pointer to a static heap structure                      *
 * <size>           How big any chunk is (usually the size of the structure *
 *                  you want to store)                                      *
 * <count>          How many chunks a block can contain                     *
 * ------------------------------------------------------------------------ */
void mem_static_create(struct sheap *shptr, size_t size, size_t count)
{
  size_t blocksize;

  /* The blocksize we would get if we don't pad */
  blocksize = ((size + sizeof(struct schunk)) * count) + sizeof(struct sblock);

  /* Now align size to next boundary */
  if(blocksize & (MEM_PAD_BLOCKS - 1))
  {
    blocksize += MEM_PAD_BLOCKS - 1;
    blocksize &= ~(MEM_PAD_BLOCKS - 1);
  }

  /* This may have created space for additional chunks */
  count = (blocksize - sizeof(struct sblock)) /
          (size + sizeof(struct schunk));

  /* Init heap struct */
  shptr->chunk_size = size;
  shptr->chunks_per_block = count;
  shptr->block_size = blocksize;
  shptr->block_count = 0;
  shptr->free_chunks = 0;
  shptr->base = NULL;

  /* Now get an initial block */
  if(mem_static_block_new(shptr))
    mem_fatal();

  dlink_add_tail(&mem_slist, &shptr->node, shptr);
  shptr->id = mem_id++;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void mem_static_note(struct sheap *shptr, const char *format, ...)
{
  va_list args;

  va_start(args, format);

  str_vsnprintf(shptr->note, sizeof(shptr->note), format, args);

  va_end(args);
}

/* ------------------------------------------------------------------------ *
 * Return pointer to a free chunk on the heap.                              *
 * Allocate a block if necessary.                                           *
 * ------------------------------------------------------------------------ */
void *mem_static_alloc(struct sheap *shptr)
{
  struct sblock *sbptr;

  if(shptr->free_chunks == 0)
  {
    /* Allocate new block */
    if(mem_static_block_new(shptr))
    {
      mem_static_collect(shptr);

      if(!shptr->free_chunks)
        mem_fatal();
    }
  }

  /* Occupy a free chunk in the first block which has space */
  for(sbptr = shptr->base; sbptr; sbptr = sbptr->next)
  {
    /* Found block with free chunks */
    if(sbptr->free_chunks)
      return mem_static_block_alloc(sbptr);
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * Free an chunk allocated by block_alloc()                                 *
 * ------------------------------------------------------------------------ */
void mem_static_free(struct sheap *shptr, void *ptr)
{
  struct sblock *dbptr;
  struct schunk *scptr;

  /* Ignore stewpid calls */
  if(shptr == NULL || ptr == NULL)
    return;

  /* Get pointer to chunk header */
  scptr = (void *)((size_t)ptr - sizeof(struct schunk));

#ifdef DEBUG
  if(!mem_static_valid(shptr, scptr))
  {
    log(mem_log, L_fatal, "invalid chunk in mem_static_free(%p, %p)",
        shptr, ptr);
    syscall_exit(-1);
  }
#endif /* DEBUG */

  /* How can this happen? */
  if(scptr->block == NULL)
    mem_fatal();

  /* Modify usage count */
  dbptr = scptr->block;
  shptr->free_chunks++;
  dbptr->free_chunks++;

  /* Remove from used linklist */
  dlink_delete(&dbptr->used_ones, &scptr->node);
  dlink_add_head(&dbptr->free_ones, &scptr->node, ptr);

  if(dbptr->used_ones.size == 0)
    mem_static_collect(dbptr->heap);
}

/* ------------------------------------------------------------------------ *
 * Free all blocks which have no used chunks.                               *
 * ------------------------------------------------------------------------ */
int mem_static_collect(struct sheap *shptr)
{
  struct sblock *dbptr;
  struct sblock *tofr;
  struct sblock *prev;

  /* Huh? */
  if(shptr == NULL)
    return 0;

  /* Heap must have at least 1 block */
  if(shptr->free_chunks < shptr->chunks_per_block || shptr->block_count == 1)
    return 0;

  prev = NULL;
  dbptr = shptr->base;

  /* Now loop through all blocks */
  while(dbptr)
  {
    /* Nice, we found an unused one */
    if(dbptr->free_chunks == shptr->chunks_per_block)
    {
      tofr = dbptr;

      /*
       * There was a previous block, remove its
       * reference to the current block
       */
      if(prev)
      {
        prev->next = dbptr->next;
        dbptr = prev->next;
      }
      /*
       * There was no previous block,
       * update the base pointer.
       */
      else
      {
        shptr->base = dbptr->next;
        dbptr = shptr->base;
      }

      /* Update allocation statistics */
      shptr->block_count--;
      shptr->free_chunks -= shptr->chunks_per_block;

      mem_static_block_delete(tofr);
    }
    /* Still beeing used, skip it */
    else
    {
      prev = dbptr;
      dbptr = dbptr->next;
    }
  }

  return 0;
}

/* ------------------------------------------------------------------------ *
 * Destroy the whole heap.                                                  *
 * ------------------------------------------------------------------------ */
void mem_static_destroy(struct sheap *shptr)
{
  struct sblock *dbptr;
  struct sblock *next;

  /* Walk through all blocks and free them */
  for(dbptr = shptr->base; dbptr; dbptr = next)
  {
    next = dbptr->next;

    mem_static_block_delete(dbptr);
  }

  /* Zero the heap struct */
  shptr->base = NULL;
  shptr->block_count = 0;
  shptr->free_chunks = 0;
  shptr->chunk_size = 0;
  shptr->chunks_per_block = 0;

  dlink_delete(&mem_slist, &shptr->node);
}

/* ------------------------------------------------------------------------ *
 * DEBUG FUNCTION: see if <chunk> is valid.                                 *
 * ------------------------------------------------------------------------ */
#ifdef DEBUG
int mem_static_valid(struct sheap *shptr, void *scptr)
{
  struct sblock *sbptr;

  /* walk through all blocks */
  for(sbptr = shptr->base; sbptr; sbptr = sbptr->next)
  {
    /* we found our chunk :D */
    if(mem_static_block_valid(sbptr, scptr))
      return 1;
  }

  return 0;
}
#endif /* DEBUG */

/* ------------------------------------------------------------------------ *
 * Create a new dynamic heap                                                *
 * ------------------------------------------------------------------------ */
void mem_dynamic_create(struct dheap *dhptr, size_t blocksize)
{
  if(!dhptr || !blocksize)
    mem_fatal();

  if(blocksize & (MEM_PAD_BLOCKS - 1))
  {
    blocksize += MEM_PAD_BLOCKS - 1;
    blocksize &= ~(MEM_PAD_BLOCKS - 1);
  }

  /* Init heap struct */
  dhptr->block_count = 0;
  dhptr->block_size = blocksize;
  dhptr->free_bytes = 0;
  dhptr->used_bytes = 0;

  if(mem_dynamic_block_new(dhptr) == NULL)
    mem_fatal();

  dlink_add_tail(&mem_dlist, &dhptr->node, dhptr);
  dhptr->id = mem_id++;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void mem_dynamic_note(struct dheap *dhptr, const char *format, ...)
{
  va_list args;

  va_start(args, format);

  str_vsnprintf(dhptr->note, sizeof(dhptr->note), format, args);

  va_end(args);
}

/* ------------------------------------------------------------------------ *
 * Return pointer to a newly allocated chunk on the heap.                   *
 * ------------------------------------------------------------------------ */
void *mem_dynamic_alloc(struct dheap *dhptr, size_t size)
{
  struct dblock *dbptr;
  void            *ret;

  /* Pad to pointer boundary */
  if((size & (sizeof(void *) - 1)) != 0)
  {
    size += sizeof(void *);
    size &= ~(sizeof(void *) - 1);
  }

  /* Does it fit? Allocate dynblock otherwise */
/*  if(size + sizeof(struct dchunk) > dhptr->max_bytes)
  {
  }*/

  /* Find a block with a gap thats big enough for our new chunk */
  for(dbptr = dhptr->base; dbptr; dbptr = dbptr->next)
  {
    /* Get a chunk */
    if((ret = mem_dynamic_block_alloc(dbptr, size)))
    {
      memset(ret, 0, size);

      return ret;
    }
  }

  if((dbptr = mem_dynamic_block_new(dhptr)) == NULL)
  {
    mem_fatal();
  }

  if((ret = mem_dynamic_block_alloc(dbptr, size)))
  {
    memset(ret, 0, size);

    return ret;
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * Try to resize block, otherwise free and allocate new one.                *
 * ------------------------------------------------------------------------ */
void *mem_dynamic_realloc(struct dheap *dhptr, void *ptr, size_t size)
{
  struct dchunk *dcptr;
  struct dblock *dbptr;
  void          *top;
  int            avail;
  size_t         oldsize;

  if(ptr == NULL)
    return mem_dynamic_alloc(dhptr, size);

  dcptr = &((struct dchunk *)ptr)[-1];

#ifdef DEBUG
  if(!mem_dynamic_valid(dhptr, dcptr))
  {
    log(mem_log, L_fatal, "invalid chunk in mem_dynamic_realloc(%p, %p, %u)",
        dhptr, ptr, size);
    syscall_exit(-1);
  }
#endif /* DEBUG */

  dbptr = dcptr->node.data;

  /* Calculate free space after chunk */
  if(dcptr->node.next)
    top = dcptr->node.next;
  else
    top = (void *)((size_t)&dbptr[1] + dbptr->size);

  avail = (size_t)top - (size_t)ptr;
  oldsize = dcptr->size;

  /* Shrink block */
  if(size <= dcptr->size)
  {
    dcptr->size = size;
  }
  /* Rocks, chunk still fits with new size */
  else if(avail >= size)
  {
    if(size - oldsize)
      memset((void *)((size_t)ptr + oldsize), 0, size - oldsize);

    dcptr->size = size;
  }
  /* *shrug* we have to alloc new chunk and free this one :/ */
  else
  {
    top = mem_dynamic_alloc(dhptr, size);

    if(top == NULL)
      return top;

    memcpy(top, ptr, dcptr->size);

    mem_dynamic_free(dhptr, ptr);

    return top;
  }

  if(avail == dbptr->max_bytes)
  {
    avail += oldsize - size;

    if(avail < 0)
      avail = 0;

    if(avail >= 0)
    {
      if(dhptr->max_bytes == dbptr->max_bytes)
        dhptr->max_bytes = avail;

      dbptr->max_bytes = avail;
    }
  }

  /* Update memory statistics */
  dbptr->free_bytes += oldsize - size;
  dbptr->used_bytes += size - oldsize;
  dhptr->free_bytes += oldsize - size;
  dhptr->used_bytes += size - oldsize;

  return ptr;
}

/* ------------------------------------------------------------------------ *
 * Free a chunk. haha                                                       *
 * ------------------------------------------------------------------------ */
void mem_dynamic_free(struct dheap *dhptr, void *ptr)
{
  struct dchunk *dcptr;

  dcptr = &((struct dchunk *)ptr)[-1];

#ifdef DEBUG
  if(!mem_dynamic_valid(dhptr, dcptr))
  {
    log(mem_log, L_fatal, "invalid chunk in mem_dynamic_free(%p, %p)", dhptr, ptr);
    dcptr = NULL;
    dcptr->node.data = NULL;
    syscall_exit(-1);
  }
#endif /* DEBUG */

  mem_dynamic_block_free(dcptr->node.data, dcptr);
}

/* ------------------------------------------------------------------------ *
 * Free all blocks which have no chunks.                                    *
 * ------------------------------------------------------------------------ */
int mem_dynamic_collect(struct dheap *dhptr)
{
  struct dblock *dbptr;
  struct dblock *tofr;
  struct dblock *prev;

  /* Huh? */
  if(dhptr == NULL)
    return 0;

  /* Heap must have at least 1 block */
  if(dhptr->block_count == 1)
    return 0;

  prev = NULL;
  dbptr = dhptr->base;

  /* Now loop through all blocks */
  while(dbptr)
  {
    /* Nice, we found an unused one */
    if(dbptr->chunks.size == 0)
    {
      tofr = dbptr;

      /*
       * There was a previous block, remove its
       * reference to the current block
       */
      if(prev)
      {
        prev->next = dbptr->next;
        dbptr = prev->next;
      }
      /*
       * There was no previous block,
       * update the base pointer.
       */
      else
      {
        dhptr->base = dbptr->next;
        dbptr = dhptr->base;
      }

      /* Update allocation statistics */
      dhptr->block_count--;

      mem_dynamic_block_delete(tofr);
    }
    /* Still beeing used, skip it */
    else
    {
      prev = dbptr;
      dbptr = dbptr->next;
    }
  }

  return 0;
}

/* ------------------------------------------------------------------------ *
 * Destroy a dynamic heap, munmap() the blocks.                             *
 * ------------------------------------------------------------------------ */
void mem_dynamic_destroy(struct dheap *dhptr)
{
  struct dblock *dbptr;
  struct dblock *next;

  /* Walk through all blocks and free them */
  for(dbptr = dhptr->base; dbptr; dbptr = next)
  {
    next = dbptr->next;

    mem_dynamic_block_delete(dbptr);
  }

  /* Zero the heap struct */
  dhptr->base = NULL;
  dhptr->block_count = 0;
  dhptr->block_size = 0;
  dhptr->free_bytes = 0;
  dhptr->used_bytes = 0;
  dhptr->max_bytes = 0;

  dlink_delete(&mem_dlist, &dhptr->node);
}

/* ------------------------------------------------------------------------ *
 * DEBUG FUNCTION: see if <dcptr> is valid.                                 *
 * this is time-consuming and will not be built without -DDEBUG             *
 * ------------------------------------------------------------------------ */
#ifdef DEBUG
int mem_dynamic_valid(struct dheap *dhptr, void *dcptr)
{
  struct dblock *dbptr;

  /* Walk through all blocks */
  for(dbptr = dhptr->base; dbptr; dbptr = dbptr->next)
  {
    /* We found our chunk :D */
    if(mem_dynamic_block_valid(dbptr, dcptr))
      return 1;
  }

  return 0;
}
#endif /* DEBUG */

/* ------------------------------------------------------------------------ *
 * Fuck libc! :P                                                            *
 * ------------------------------------------------------------------------ */
#if 0 //def USE_IA32_LINUX_INLINE
void *memset(void *s, int c, size_t n)
{
  size_t i;

  /* n is a multiple of 8, so do 64bit copying */
  if(!(n & 0x07) && (n & -8))
  {
    int64_t q = ((int64_t)(c & 0xff) <<  0) |
                ((int64_t)(c & 0xff) <<  8) |
                ((int64_t)(c & 0xff) << 16) |
                ((int64_t)(c & 0xff) << 24) |
                ((int64_t)(c & 0xff) << 32) |
                ((int64_t)(c & 0xff) << 40) |
                ((int64_t)(c & 0xff) << 48) |
                ((int64_t)(c & 0xff) << 56);
    n >>= 3;

    for(i = 0; i < n; i++)
      ((int64_t *)s)[i] = q;
  }
  /* n is a multiple of 4, so do 32bit copying */
  else if(!(n & 0x03) && (n & -4))
  {
    int32_t q = ((int32_t)(c & 0xff) <<  0) |
                ((int32_t)(c & 0xff) <<  8) |
                ((int32_t)(c & 0xff) << 16) |
                ((int32_t)(c & 0xff) << 24);
    n >>= 2;

    for(i = 0; i < n; i++)
      ((int32_t *)s)[i] = q;
  }
  /* n is a multiple of 2, so do 16bit copying */
  else if(!(n & 0x01) && (n & -2))
  {
    int16_t q = ((int16_t)(c & 0xff) << 0) |
                ((int16_t)(c & 0xff) << 8);
    n >>= 1;

    for(i = 0; i < n; i++)
      ((int16_t *)s)[i] = q;
  }
  /* otherwise do 8bit copying */
  else
  {
    int8_t q = (int8_t)(c & 0xff);

    for(i = 0; i < n; i++)
      ((int8_t *)s)[i] = q;
  }

  return s;
}
#endif /* USE_IA32_LINUX_INLINE */

/* ------------------------------------------------------------------------ *
 * Well known stdlib functions.                                             *
 * ------------------------------------------------------------------------ */
#ifdef USE_IA32_LINUX_INLINE
void *memcpy(void *d, const void *s, size_t n)
{
  size_t i;

  /* n is a multiple of 8, so do 64bit copying */
  if(!(n & 0x07) && (n >= 8))
  {
    n >>= 3;

    for(i = 0; i < n; i++)
      ((int64_t *)d)[i] = ((int64_t *)s)[i];
  }
  /* n is a multiple of 4, so do 32bit copying */
  else if(!(n & 0x03) && (n >= 4))
  {
    n >>= 2;

    for(i = 0; i < n; i++)
      ((int32_t *)d)[i] = ((int32_t *)s)[i];
  }
  /* n is a multiple of 2, so do 16bit copying */
  else if(!(n & 0x01) && (n >= 2))
  {
    n >>= 1;

    for(i = 0; i < n; i++)
      ((int16_t *)d)[i] = ((int16_t *)s)[i];
  }
  /* otherwise do 8bit copying */
  else
  {
    for(i = 0; i < n; i++)
      ((int8_t *)d)[i] = ((int8_t *)s)[i];
  }

  return d;
}

void *memmove(void *d, const void *s, size_t n)
{
  size_t i;
  ssize_t dist;

  dist = (size_t)d - (size_t)s;

  if(dist <= 0)
  {
    /* n is a multiple of 8, so do 64bit copying */
    if(!(n & 0x07) && (n >= 8) && (dist >= 8 || dist == 0))
    {
      n >>= 3;

      for(i = 0; i < n; i++)
        ((int64_t *)d)[i] = ((int64_t *)s)[i];
    }
    /* n is a multiple of 4, so do 32bit copying */
    else if(!(n & 0x03) && (n >= 4) && (dist >= 4))
    {
      n >>= 2;

      for(i = 0; i < n; i++)
        ((int32_t *)d)[i] = ((int32_t *)s)[i];
    }
    /* n is a multiple of 2, so do 16bit copying */
    else if(!(n & 0x01) && (n >= 2) && (dist >= 2))
    {
      n >>= 1;

      for(i = 0; i < n; i++)
        ((int16_t *)d)[i] = ((int16_t *)s)[i];
    }
    /* otherwise do 8bit copying */
    else
    {
      for(i = 0; i < n; i++)
        ((int8_t *)d)[i] = ((int8_t *)s)[i];
    }
  }
  else
  {
    /* n is a multiple of 8, so do 64bit copying */
    if(!(n & 0x07) && (n >= 8) && (dist <= -8))
    {
      n >>= 3;

      for(i = n - 1;; i--)
      {
        ((int64_t *)d)[i] = ((int64_t *)s)[i];
        if(i == 0)
          break;
      }
    }
    /* n is a multiple of 4, so do 32bit copying */
    else if(!(n & 0x03) && (n >= 4) && (dist >= 4))
    {
      n >>= 2;

      for(i = n - 1;; i--)
      {
        ((int32_t *)d)[i] = ((int32_t *)s)[i];
        if(i == 0)
          break;
      }
    }
    /* n is a multiple of 2, so do 16bit copying */
    else if(!(n & 0x01) && (n >= 2) && (dist >= 2))
    {
      n >>= 1;

      for(i = n - 1;; i--)
      {
        ((int16_t *)d)[i] = ((int16_t *)s)[i];
        if(i == 0)
          break;
      }
    }
    /* otherwise do 8bit copying */
    else
    {
      for(i = n - 1;; i--)
      {
        ((int8_t *)d)[i] = ((int8_t *)s)[i];
        if(i == 0)
          break;
      }
    }
  }

  return d;
}

int memcmp(const void *d, const void *s, size_t n)
{
  size_t i;

  /* n is a multiple of 8, so do 64bit comparing */
  if(!(n & 0x07) && (n >= 8))
  {
    n >>= 3;

    for(i = 0; i < n; i++)
      if(((int64_t *)d)[i] != ((int64_t *)s)[i])
        return 1;
  }
  /* n is a multiple of 4, so do 32bit comparing */
  else if(!(n & 0x03) && (n >= 4))
  {
    n >>= 2;

    for(i = 0; i < n; i++)
      if(((int32_t *)d)[i] != ((int32_t *)s)[i])
        return 1;
  }
  /* n is a multiple of 2, so do 16bit comparing */
  else if(!(n & 0x01) && (n >= 2))
  {
    n >>= 1;

    for(i = 0; i < n; i++)
      if(((int16_t *)d)[i] != ((int16_t *)s)[i])
        return 1;
  }
  /* otherwise do 8bit comparing */
  else
  {
    for(i = 0; i < n; i++)
      if(((int8_t *)d)[i] != ((int8_t *)s)[i])
        return 1;
  }

  return 0;
}
#endif /* USE_IA32_LINUX_INLINE */

