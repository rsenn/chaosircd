/* cgircd - CrowdGuard IRC daemon
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
 * $Id: str.h,v 1.6 2006/09/28 09:44:11 roman Exp $
 */

#ifndef LIB_STR_H
#define LIB_STR_H 1

#include <string.h>

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/mem.h"

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#define str_isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')
#define str_isdigit(c) (c <= '9' && c >= '0')
#define str_isalpha(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
#define str_isalnum(c) (str_isdigit((c)) || str_isalpha((c)))
#define str_tolower(c) (c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c)
#define str_islower(c) (c >= 'a' && c <= 'z')
#define str_toupper(c) (c >= 'a' && c <= 'a' ? c - ('a' - 'A') : c)
#define str_isupper(c) (c >= 'A' && c <= 'Z')

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
typedef void (str_format_cb)(char   **pptr, size_t  *bptr,
                             size_t   n,    int      padding,
                             int      left, void    *arg);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(const char)     str_hexchars[16];
CHAOS_API(str_format_cb *)str_table[64];

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void) str_init       (void);
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void) str_shutdown   (void);
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void) str_register   (char           c,
                            str_format_cb *cb);
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void) str_unregister (char           c);

/* ------------------------------------------------------------------------ *
 * Get string length.                                                         *
 * ------------------------------------------------------------------------ */
#if 1 //def USE_IA32_LINUX_INLINE
CHAOS_API(size_t) str_len(const char *s);

/*CHAOS_INLINE  size_t str_len(const char *s)
{
  size_t len = 0;

  for(EVER)
  {
    if(!s[len]) return len; len++;
    if(!s[len]) return len; len++;
    if(!s[len]) return len; len++;
    if(!s[len]) return len; len++;
  }
}*/
#endif /* USE_IA32_LINUX_INLINE */

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
/*
CHAOS_API(char *)str_cat(char *d, const char *s);

CHAOS_INLINE( char *str_cat(char *d, const char *s)
{
  size_t i;

  i = 0;

  for(EVER)
  {
    if(d[i] == '\0') break; i++;
    if(d[i] == '\0') break; i++;
    if(d[i] == '\0') break; i++;
    if(d[i] == '\0') break; i++;
  }

  for(EVER)
  {
    if(!(d[i] = *s++)) break; i++;
    if(!(d[i] = *s++)) break; i++;
    if(!(d[i] = *s++)) break; i++;
    if(!(d[i] = *s++)) break; i++;
  }

  return d;
})
*/
/* ------------------------------------------------------------------------ *
 * Copy string from <s> to <d>.                                               *
 * ------------------------------------------------------------------------ */
/*
CHAOS_API(size_t) str_copy(char *d, const char *s);

CHAOS_INLINE( size_t str_copy(char *d, const char *s)
{
  size_t i = 0;

  for(EVER)
  {
    if(!(d[i] = s[i])) break; i++;
    if(!(d[i] = s[i])) break; i++;
    if(!(d[i] = s[i])) break; i++;
    if(!(d[i] = s[i])) break; i++;
  }

  d[i] = '\0';

  return i;
}*/

/* ------------------------------------------------------------------------ *
 * Copy string from <s> to <d>. Write max <n> bytes to <d> and always         *
 * null-terminate it. Returns new string length of <d>.                       *
 * ------------------------------------------------------------------------ */
#ifndef HAVE_STRLCPY
CHAOS_API(size_t) strlcpy(char *d, const char *s, size_t n);

CHAOS_INLINE( size_t strlcpy(char *d, const char *s, size_t n)
{
  size_t i = 0;

  if(n == 0)
    return 0;

  if(--n > 0)
  {
    for(EVER)
    {
      if(!(d[i] = s[i])) break; if(++i == n) break;
      if(!(d[i] = s[i])) break; if(++i == n) break;
      if(!(d[i] = s[i])) break; if(++i == n) break;
      if(!(d[i] = s[i])) break; if(++i == n) break;
    }
  }

  d[i] = '\0';

  return i;
} )
#endif /* HAVE_STRLCPY */

/* ------------------------------------------------------------------------ *
 * Append string <src> to <dst>. Don't let <dst> be bigger than <siz> bytes   *
 * and always null-terminate. Returns new string length of <dst>              *
 * ------------------------------------------------------------------------ */
#ifndef HAVE_STRLCAT
CHAOS_API(size_t) strlcat(char *d, const char *s, size_t n);

CHAOS_INLINE( size_t strlcat(char *d, const char *s, size_t n)
{
  size_t i = 0;

  if(n == 0)
    return 0;

  if(--n > 0)
  {
    for(EVER)
    {
      if(!d[i]) break; if(++i == n) break;
      if(!d[i]) break; if(++i == n) break;
      if(!d[i]) break; if(++i == n) break;
      if(!d[i]) break; if(++i == n) break;
    }
  }

  d[i] = '\0';

  if(n > i)
  {
    for(EVER)
    {
      if(!(d[i] = *s++)) break; if(++i == n) break;
      if(!(d[i] = *s++)) break; if(++i == n) break;
      if(!(d[i] = *s++)) break; if(++i == n) break;
      if(!(d[i] = *s++)) break; if(++i == n) break;
    }

    d[i] = '\0';
  }

  return i;
})
#endif /* HAVE_STRLCAT */

/* ------------------------------------------------------------------------ *
 * Compare string.                                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(int) str_cmp(const char *s1, const char *s2);

CHAOS_INLINE( int str_cmp(const char *s1, const char *s2)
{
  size_t i = 0;

  for(EVER)
  {
    if(s1[i] != s2[i]) break; if(!s1[i]) break; i++;
    if(s1[i] != s2[i]) break; if(!s1[i]) break; i++;
    if(s1[i] != s2[i]) break; if(!s1[i]) break; i++;
    if(s1[i] != s2[i]) break; if(!s1[i]) break; i++;
  }

  return ((int)(unsigned int)(unsigned char)s1[i]) -
         ((int)(unsigned int)(unsigned char)s2[i]);
})

/* ------------------------------------------------------------------------ *
 * Compare string.                                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(int) str_icmp(const char *s1, const char *s2);

CHAOS_INLINE( int str_icmp(const char *s1, const char *s2)
{
  size_t i = 0;

  for(EVER)
  {
    if(str_tolower(s1[i]) != str_tolower(s2[i])) break; if(!s1[i]) break; i++;
    if(str_tolower(s1[i]) != str_tolower(s2[i])) break; if(!s1[i]) break; i++;
    if(str_tolower(s1[i]) != str_tolower(s2[i])) break; if(!s1[i]) break; i++;
    if(str_tolower(s1[i]) != str_tolower(s2[i])) break; if(!s1[i]) break; i++;
  }

  return ((int)(unsigned int)(unsigned char)str_tolower(s1[i])) -
         ((int)(unsigned int)(unsigned char)str_tolower(s2[i]));
})

/* ------------------------------------------------------------------------ *
 * Compare string, abort after <n> chars.                                     *
 * ------------------------------------------------------------------------ */
//#undef str_ncmp
CHAOS_API(int) str_ncmp(const char *s1, const char *s2, size_t n);

CHAOS_INLINE( int str_ncmp(const char *s1, const char *s2, size_t n)
{
  size_t i = 0;

  if(n == 0)
    return 0;

  for(EVER)
  {
    if(s1[i] != s2[i]) break; if(--n == 0 || !s1[i]) return 0; i++;
    if(s1[i] != s2[i]) break; if(--n == 0 || !s1[i]) return 0; i++;
    if(s1[i] != s2[i]) break; if(--n == 0 || !s1[i]) return 0; i++;
    if(s1[i] != s2[i]) break; if(--n == 0 || !s1[i]) return 0; i++;
  }

  return ((int)(unsigned int)(unsigned char)s1[i]) -
         ((int)(unsigned int)(unsigned char)s2[i]);
})

/* ------------------------------------------------------------------------ *
 * Compare string, abort after <n> chars.                                     *
 * ------------------------------------------------------------------------ */
CHAOS_API(int) str_nicmp(const char *s1, const char *s2, size_t n);

CHAOS_INLINE( int str_incmp(const char *s1, const char *s2, size_t n)
{
  size_t i = 0;

  if(n == 0)
    return 0;

  for(EVER)
  {
    if(i == n) return 0; if(str_tolower(s1[i]) != str_tolower(s2[i])) break; if(!s1[i]) break; i++;
    if(i == n) return 0; if(str_tolower(s1[i]) != str_tolower(s2[i])) break; if(!s1[i]) break; i++;
    if(i == n) return 0; if(str_tolower(s1[i]) != str_tolower(s2[i])) break; if(!s1[i]) break; i++;
    if(i == n) return 0; if(str_tolower(s1[i]) != str_tolower(s2[i])) break; if(!s1[i]) break; i++;
  }

  return ((int)(unsigned int)(unsigned char)str_tolower(s1[i])) -
         ((int)(unsigned int)(unsigned char)str_tolower(s2[i]));
})

/* ------------------------------------------------------------------------ *
 * Formatted vararg print to string                                           *
 * ------------------------------------------------------------------------ */
//#undef str_vsnprintf
//#define str_vsnprintf str_vsnprintf
CHAOS_API(int) str_vsnprintf(char *str, size_t n, const char *format, va_list args);

/* ------------------------------------------------------------------------ *
 * Converts a string to a signed int.                                         *
 * ------------------------------------------------------------------------ */
CHAOS_API(int) str_toi(const char *s);

/* ------------------------------------------------------------------------ *
 * Formatted print to string                                                  *
 * ------------------------------------------------------------------------ */
//#undef str_snprintf
//#define str_snprintf str_snprintf
CHAOS_API(int) str_snprintf(char *str, size_t n, const char *format, ...);

CHAOS_INLINE( int str_snprintf(char *str, size_t n, const char *format, ...)
{
  int ret;

  va_list args;

  va_start(args, format);

  ret = str_vsnprintf(str, n, format, args);

  va_end(args);

  return ret;
})

/* ------------------------------------------------------------------------ *
 * Converts a string to a signed int.                                         *
 * ------------------------------------------------------------------------ */
CHAOS_API(int) str_toi(const char *s);

#define ISNUM(c) ((c) >= '0' && (c) <= '9')
CHAOS_INLINE( int str_toi(const char *s)
{
  register uint32_t i = 0;
  register uint32_t sign = 0;
  register const uint8_t *p = (const uint8_t *)s;

  /* Conversion starts at the first numeric character or sign. */
  while(*p && !ISNUM(*p) && *p != '-') p++;

  /*
   * If we got a sign, set a flag.
   * This will negate the value before return.
   */
  if(*p == '-')
  {
    sign++;
    p++;
  }

  /* Don't care when 'u' overflows (Bug?) */
  while(ISNUM(*p))
  {
    i *= 10;
    i += *p++ - '0';
  }

  /* Return according to sign */
  if(sign)
    return - i;
  else
    return i;

})
#undef ISNUM

/* ------------------------------------------------------------------------ *
 * Splits a string into tokens.                                               *
 *                                                                            *
 * delimiters are hardcoded to [ \t]                                          *
 * irc style: a ':' at token start will stop tokenizing                       *
 *                                                                            *
 *                                                                            *
 *                 - string to be tokenized.                                  *
 *                   '\0' will be written to whitespace                       *
 *                                                                            *
 * v[]             - will contain pointers to tokens, must be at least        *
 *                   (maxtok + 1) * sizeof(char *) in size.                   *
 *                                                                            *
 * v[return value] - will be NULL                                             *
 *                                                                            *
 * return value will not be bigger than maxtok                                *
 * ------------------------------------------------------------------------ */
CHAOS_API(size_t) str_tokenize(char *s, char **v, size_t maxtok);

#if 1
CHAOS_INLINE( size_t str_tokenize(char *s, char **v, size_t maxtok)
{
  size_t c = 0;

  if(!maxtok)
    return 0;

  for(EVER)
  {
    /* Skip and zero whitespace */
    while(*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
      *s++ = '\0';

    /* We finished */
    if(*s == '\0')
      break;

    /* Stop tokenizing when we spot a ':' at token start */
    if(*s == ':')
    {
      /* The remains are a single argument
         so it can include blanks also */
      v[c++] = &s[1];
      break;
    }
    /* Add to token list */
    v[c++] = s;

    /* We finished */
    if(c == maxtok)
      break;

    /* Scan for end or whitespace */
    while(*s != ' ' && *s != '\t' && *s != '\0' && *s != '\r' && *s != '\n')
      s++;
  }

  v[c] = NULL;

  return c;
})
#endif

/* ------------------------------------------------------------------------ *
 * Splits a string into tokens.                                               *
 *                                                                            *
 * Like the one above but allows to specify delimiters.                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(size_t)str_tokenize_d(char       *s,
                                char      **v,
                                size_t      maxtok,
                                const char *delim);

/* ------------------------------------------------------------------------ *
 * Splits a string into tokens.                                               *
 *                                                                            *
 * Like the one above but allows to specify one delimiter.                    *
 * ------------------------------------------------------------------------ */
CHAOS_API(size_t)str_tokenize_s(char       *s,
                                char      **v,
                                size_t      maxtok,
                                char        delim);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
//#undef str_dup
//#define str_dup str_strdup
CHAOS_API(char *)str_dup(const char *s);

CHAOS_INLINE( char *str_dup(const char *s)
{
  char *r;

  r = malloc(str_len(s) + 1);

  if(r != NULL)
    strcpy(r, s);

  return r;
})

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(hash_t )str_hash(const char *s);

#define ROR(v, n) ((v >> (n & (HASH_BIT_SIZE-1))) | (v << (HASH_BIT_SIZE - (n & (HASH_BIT_SIZE-1)))))
#define ROL(v, n) ((v >> (n & (HASH_BIT_SIZE-1))) | (v << (HASH_BIT_SIZE - (n & (HASH_BIT_SIZE-1)))))
CHAOS_INLINE(hash_t str_hash(const char *s)
{
  hash_t ret = 0xdefaced;
  hash_t temp;
  hash_t i;

#if HASH_BIT_SIZE > 32
  ret <<= 32;
  ret |= 0xcafebabe;
#endif 
  
  if(s == NULL)
    return ret;

  for(i = 0; s[i]; i++)
  {
    temp = ret;
    ret = ROR(ret, s[i]);
    temp ^= s[i];
    temp = ROL(temp, ret);
    ret -= temp;
  }

  return ret;
})
#undef ROL
#undef ROR

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(hash_t)str_ihash(const char *s);

#define ROR(v, n) ((v >> (n & (HASH_BIT_SIZE-1))) | (v << (HASH_BIT_SIZE - (n & (HASH_BIT_SIZE-1)))))
#define ROL(v, n) ((v >> (n & (HASH_BIT_SIZE-1))) | (v << (HASH_BIT_SIZE - (n & (HASH_BIT_SIZE-1)))))
CHAOS_INLINE(hash_t str_ihash(const char *s)
{
  hash_t ret = 0xdefaced;
  hash_t temp;
  hash_t i;

#if HASH_BIT_SIZE > 32
  ret <<= 32;
  ret |= 0xcafebabe;
#endif 

  if(s == NULL)
    return ret;

  for(i = 0; s[i]; i++)
  {
    temp = ret;
    ret = ROR(ret, str_tolower(s[i]));
    temp ^= str_tolower(s[i]);
    temp = ROL(temp, ret);
    ret -= temp;
  }

  return ret;
})
#undef ROL
#undef ROR

/* ------------------------------------------------------------------------ *
 * Convert a string to an unsigned long.                                      *
 * ------------------------------------------------------------------------ */
CHAOS_API(unsigned long int) str_toul(const char *nptr, char **endptr, int base);

/* ------------------------------------------------------------------------ *
 * Convert a string to a long.                                                *
 * ------------------------------------------------------------------------ */
CHAOS_API(long int) str_tol(const char *nptr, char **endptr, int base);

/* ------------------------------------------------------------------------ *
 * Convert a string to a unsigned long long.                                  *
 * ------------------------------------------------------------------------ */
CHAOS_API(unsigned long long int) str_toull(const char *nptr, char **endptr, int base);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int) str_match(const char *str, const char *mask);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int) str_imatch(const char *str, const char *mask);

/*
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memmove(void *dest, const void *src, size_t n);
*/

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void) str_trim(char *s);


#endif /* LIB_STR_H */

