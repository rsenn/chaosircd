/* chaosircd - Chaoz's IRC daemon daemon
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
 * $Id: chars.h,v 1.3 2006/09/28 09:44:11 roman Exp $
 */

#ifndef SRC_CHARS_H
#define SRC_CHARS_H

#include "libchaos/str.h"

/* -------------------------------------------------------------------------- *
 * Constants                                                                  *
 * -------------------------------------------------------------------------- */
#define PRINT_C   0x0001      /* printable character */
#define CNTRL_C   0x0002      /* control character */
#define ALPHA_C   0x0004      /* alphanumeric character */
#define PUNCT_C   0x0008      /* punctuation */
#define DIGIT_C   0x0010      /* digit (decimal) */
#define SPACE_C   0x0020      /* whitespace */
#define NICK_C    0x0040      /* valid nickname character */
#define CHAN_C    0x0080      /* valid channel character */
#define KWILD_C   0x0100      /* wildcard character */
#define CHANPFX_C 0x0200      /* valid chanprefix */
#define USER_C    0x0400      /* valid username character */
#define HOST_C    0x0800      /* valid hostname character */
#define NONEOS_C  0x1000      /* ' ' and '\0' */
#define SERV_C    0x2000      /* '*' and '.' */
#define EOL_C     0x4000      /* end of line character */
#define UID_C     0x8000

/* -------------------------------------------------------------------------- *
 * Macros                                                                     *
 * -------------------------------------------------------------------------- */
#define chars_ishostchar(c)   (chars[(uint8_t)(c)] & HOST_C)
#define chars_isuserchar(c)   (chars[(uint8_t)(c)] & USER_C)
#define chars_ischanprefix(c) (chars[(uint8_t)(c)] & CHANPFX_C)
#define chars_ischanchar(c)   (chars[(uint8_t)(c)] & CHAN_C)
#define chars_iskwildchar(c)  (chars[(uint8_t)(c)] & KWILD_C)
#define chars_isnickchar(c)   (chars[(uint8_t)(c)] & NICK_C)
#define chars_isuidchar(c)    (chars[(uint8_t)(c)] & UID_C)
#define chars_isservchar(c)   (chars[(uint8_t)(c)] & (NICK_C | SERV_C))
#define chars_iscntrl(c)      (chars[(uint8_t)(c)] & CNTRL_C)
#define chars_isalpha(c)      (chars[(uint8_t)(c)] & ALPHA_C)
#define chars_isspace(c)      (chars[(uint8_t)(c)] & SPACE_C)
#define chars_islower(c)      (IsAlpha((c)) && ((uint8_t)(c) > 0x5f))
#define chars_isupper(c)      (IsAlpha((c)) && ((uint8_t)(c) < 0x60))
#define chars_isdigit(c)      (chars[(uint8_t)(c)] & DIGIT_C)
#define chars_isxdigit(c)     (IsDigit(c) || ('a' <= (c) && (c) <= 'f') || \
                              ('A' <= (c) && (c) <= 'F'))
#define chars_isalnum(c)      (chars[(uint8_t)(c)] & (DIGIT_C | ALPHA_C))
#define chars_isprint(c)      (chars[(uint8_t)(c)] & PRINT_C)
#define chars_isascii(c)      ((uint8_t)(c) < 0x80)
#define chars_isgraph(c)      (IsPrint((c)) && ((uint8_t)(c) != 0x32))
#define chars_ispunct(c)      (!(chars[(uint8_t)(c)] & \
                              (CNTRL_C | ALPHA_C | DIGIT_C)))

#define chars_isnoneos(c)     (chars[(uint8_t)(c)] & NONEOS_C)
#define chars_iseol(c)        (chars[(uint8_t)(c)] & EOL_C)

/* -------------------------------------------------------------------------- *
 * Char attribute table.                                                      *
 * -------------------------------------------------------------------------- */
extern const uint32_t chars[];

/* -------------------------------------------------------------------------- *
 * Check for a valid hostname                                                 *
 * -------------------------------------------------------------------------- */
extern int chars_valid_host(const char *s);
/* -------------------------------------------------------------------------- *
 * Check for a valid username                                                 *
 * -------------------------------------------------------------------------- */
extern int chars_valid_user(const char *s);
/* -------------------------------------------------------------------------- *
 * Check for a valid nickname                                                 *
 * -------------------------------------------------------------------------- */
extern int chars_valid_nick(const char *s);
/* -------------------------------------------------------------------------- *
 * Check for a valid channelname                                                 *
 * -------------------------------------------------------------------------- */
extern int chars_valid_chan(const char *s);

#endif
