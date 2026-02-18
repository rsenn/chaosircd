/* chaosircd - Chaoz's IRC daemon daemon
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
 * $Id: defs.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef CHAOS_DEFS_H
#define CHAOS_DEFS_H

#ifdef HAVE_CONFIG_H
/*#include "libchaos/config.h"*/
#include "config.h"
#endif

#ifndef NULL
#define NULL (void *)0
#endif

#include <stdlib.h>
#include <stdint.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <limits.h>
#include <time.h>

#define HASH_BIT_SIZE (SIZEOF_UINTPTR_T*8)

typedef uintptr_t hash_t;

#ifdef _MSC_VER
#include <windows.h>
# define inline  __forceinline
typedef HANDLE pid_t;
# ifdef WIN64
typedef __int64 ssize_t;
# else
typedef int ssize_t;
# endif
#endif

#if defined(WIN32) || defined(_WIN32) || defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__)
# ifndef STATIC_LIBCHAOS
#  ifdef BUILD_LIBCHAOS
#   define CHAOS_API(type...) __declspec(dllexport) type
#   define CHAOS_DATA(type...) extern __declspec(dllexport) type
#  else
#   define CHAOS_API(type...) type
#   define CHAOS_DATA(type...) extern __declspec(dllimport) type
#  endif
# endif
#endif

#ifndef CHAOS_API
# define CHAOS_API(type...) type;
#endif
#ifndef CHAOS_DATA
# define CHAOS_DATA(type...) extern type
#endif

#ifndef CHAOS_DATA_DECL
# define CHAOS_DATA_DECL(type...) type
#endif

#ifndef CHAOS_INLINE
# ifdef __clang__
#define CHAOS_INLINE(x...) x
#define CHAOS_INLINE_FN(x...) static __inline__ x
# elif defined __GNUC__
//#define CHAOS_INLINE(x...)  static __inline__ x
#  if __GNUC__ > 4
#warning GNUC > 4
#define CHAOS_INLINE(proto) 
#define CHAOS_INLINE_API(proto)  proto;
#define CHAOS_INLINE_FN(x...)  static __inline__ x
#  else
#define CHAOS_INLINE(proto) 
#define CHAOS_INLINE_API(proto) proto;
#define CHAOS_INLINE_FN(x...)  extern __inline__ x
#  endif
# else
#define CHAOS_INLINE(x...) /*extern inline x*/
#define CHAOS_INLINE_API(proto) proto;
#define CHAOS_INLINE_FN(x...) static inline x
# endif
#endif
#ifdef CHAOS_INLINE_API
#warning CHAOS_INLNIE_API defined
#endif

#ifdef CHAOS_INLINE_API_FN
#warning CHAOS_INLNIE_API_FN defined
#endif
#undef CHAOS_INLINE
#ifndef CHAOS_INLINE
# define CHAOS_INLINE(proto) 
#endif
#ifndef CHAOS_INLINE_API
# define CHAOS_INLINE_API(proto) proto;
#endif



#ifdef __clang__
#undef NO_C99
#else
#define NO_C99 1
#endif

#ifdef  __GNUC__
#define ALREADY_INLINED 1
#endif

/*#ifdef HAVE_SYS_TYPES_H
#ifndef _BSD_SIZE_T_
#define _BSD_SIZE_T_ unsigned int
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

/* hehe, nice hack */
#ifndef EVER
#define EVER ;;
#endif

/* max. buf size */
#define BUFSIZE      1024

#define HOSTLEN      64
#define HOSTIPLEN    15
#define PROTOLEN     32
#define USERLEN      32
#define PATHLEN     256

/* dynamic heap blocks are 128kb */
#define DYNAMIC_BLOCK_SIZE (1024 * 128)

/* garbage collect heaps every 5 minutes */
#define GARBAGE_COLLECT_INTERVAL (300 * 1000LL)
#define GC_INTERVAL GARBAGE_COLLECT_INTERVAL

/* timers per heap block */
#define TIMER_BLOCK_SIZE 16

/* queue chunks per heap block */
#define QUEUE_BLOCK_SIZE 256

/* dlink nodes per heap block */
#define DLINK_BLOCK_SIZE 256

/* log entries per heap block */
#define LOG_BLOCK_SIZE   16

/* mfile entries per heap block */
#define MFILE_BLOCK_SIZE 8

/* module entries per heap block */
#define MODULE_BLOCK_SIZE 96

/* listen entries per heap block */
#define LISTEN_BLOCK_SIZE 8

/* connect entries per heap block */
#define CONNECT_BLOCK_SIZE 8

/* net entries per heap block */
#define NET_BLOCK_SIZE 8

/* ini entries per heap block */
#define INI_BLOCK_SIZE 8

/* ini keys per heap block */
#define KEY_BLOCK_SIZE 256

/* ini keys per heap block */
#define SEC_BLOCK_SIZE 64

/* hook per heap block */
#define HOOK_BLOCK_SIZE 64

/* child per heap block */
#define CHILD_BLOCK_SIZE 2

/* sauth per heap block */
#define SAUTH_BLOCK_SIZE 64

/* ssl contexts per heap block */
#define SSL_BLOCK_SIZE 8

/* http clients per heap block */
#define HTTPC_BLOCK_SIZE 8

/* html parsers per heap block */
#define HTMLP_BLOCK_SIZE 8

/* filters per heap block */
#define FILTER_BLOCK_SIZE 8

/* gifs per heap block */
#define GIF_BLOCK_SIZE 8

/* images per heap block */
#define IMAGE_BLOCK_SIZE 8

/* graphs per heap block */
#define GRAPH_BLOCK_SIZE 8

/* cfgs per heap block */
#define CFG_BLOCK_SIZE 1

/* db connects per heap block */
#define DB_BLOCK_SIZE 4

/* ttf fonts per heap block */
#define TTF_BLOCK_SIZE 4

/* wav fonts per heap block */
#define WAV_BLOCK_SIZE 4

/*
 * Warn if system time differs more than +/-TIMER_WARN_DELTA
 * from expected system time.
 */
#define TIMER_MAX_DRIFT  10000LL

/*
 * Warn if a timer gets executed TIMER_WARN_DELTA
 * miliseconds too early or too late.
 */
#define TIMER_WARN_DELTA  10LL

/*
 * Warn if expected return from poll() drifts
 * from real time by more than these msecs.
 */
#define POLL_WARN_DELTA 10LL

#endif
