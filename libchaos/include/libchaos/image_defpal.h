/* cgircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2004-2005  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: image_defpal.h,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

const struct color image_defpal[256] = {
  /* ANSI Colors */
  { 0x00, 0x00, 0x00 },         /* black */
  { 0x80, 0x80, 0x80 },         /* red */
  { 0x00, 0x00, 0x80 },         /* green */
  { 0x80, 0x80, 0x00 },         /* brown */
  { 0x00, 0x00, 0x80 },         /* blue */
  { 0x80, 0x00, 0x80 },         /* magenta */
  { 0x00, 0x80, 0x80 },         /* cyan */
  { 0x80, 0x80, 0x80 },         /* light gray */

  { 0x40, 0x40, 0x40 },         /* dark gray */
  { 0xff, 0x00, 0x00 },         /* light red */
  { 0x00, 0xff, 0x00 },         /* light green */
  { 0xff, 0xff, 0x00 },         /* light yellow */
  { 0x00, 0x00, 0xff },         /* light blue */
  { 0xff, 0x00, 0xff },         /* light magenta */
  { 0x00, 0xff, 0xff },         /* light cyan */
  { 0xff, 0xff, 0xff },         /* white */

  /* Black-White Gradient */
  { 0x00, 0x00, 0x00 },
  { 0x11, 0x11, 0x11 },
  { 0x22, 0x22, 0x22 },
  { 0x33, 0x33, 0x33 },
  { 0x44, 0x44, 0x44 },
  { 0x55, 0x55, 0x55 },
  { 0x66, 0x66, 0x66 },
  { 0x77, 0x77, 0x77 },
  { 0x88, 0x88, 0x88 },
  { 0x99, 0x99, 0x99 },
  { 0xaa, 0xaa, 0xaa },
  { 0xbb, 0xbb, 0xbb },
  { 0xcc, 0xcc, 0xcc },
  { 0xdd, 0xdd, 0xdd },
  { 0xee, 0xee, 0xee },
  { 0xff, 0xff, 0xff },

  /* Websafe palette */
  { 0xff, 0xff, 0xff },
  { 0xff, 0xff, 0xcc },
  { 0xff, 0xff, 0x99 },
  { 0xff, 0xff, 0x66 },
  { 0xff, 0xff, 0x33 },
  { 0xff, 0xff, 0x00 },
  { 0xff, 0xcc, 0xff },
  { 0xff, 0xcc, 0xcc },
  { 0xff, 0xcc, 0x99 },
  { 0xff, 0xcc, 0x66 },
  { 0xff, 0xcc, 0x33 },
  { 0xff, 0xcc, 0x00 },
  { 0xff, 0x99, 0xff },
  { 0xff, 0x99, 0xcc },
  { 0xff, 0x99, 0x99 },
  { 0xff, 0x99, 0x66 },
  { 0xff, 0x99, 0x33 },
  { 0xff, 0x99, 0x00 },
  { 0xff, 0x66, 0xff },
  { 0xff, 0x66, 0xcc },
  { 0xff, 0x66, 0x99 },
  { 0xff, 0x66, 0x66 },
  { 0xff, 0x66, 0x33 },
  { 0xff, 0x66, 0x00 },
  { 0xff, 0x33, 0xff },
  { 0xff, 0x33, 0xcc },
  { 0xff, 0x33, 0x99 },
  { 0xff, 0x33, 0x66 },
  { 0xff, 0x33, 0x33 },
  { 0xff, 0x33, 0x00 },
  { 0xff, 0x00, 0xff },
  { 0xff, 0x00, 0xcc },
  { 0xff, 0x00, 0x99 },
  { 0xff, 0x00, 0x66 },
  { 0xff, 0x00, 0x33 },
  { 0xff, 0x00, 0x00 },
  { 0xcc, 0xff, 0xff },
  { 0xcc, 0xff, 0xcc },
  { 0xcc, 0xff, 0x99 },
  { 0xcc, 0xff, 0x66 },
  { 0xcc, 0xff, 0x33 },
  { 0xcc, 0xff, 0x00 },
  { 0xcc, 0xcc, 0xff },
  { 0xcc, 0xcc, 0xcc },
  { 0xcc, 0xcc, 0x99 },
  { 0xcc, 0xcc, 0x66 },
  { 0xcc, 0xcc, 0x33 },
  { 0xcc, 0xcc, 0x00 },
  { 0xcc, 0x99, 0xff },
  { 0xcc, 0x99, 0xcc },
  { 0xcc, 0x99, 0x99 },
  { 0xcc, 0x99, 0x66 },
  { 0xcc, 0x99, 0x33 },
  { 0xcc, 0x99, 0x00 },
  { 0xcc, 0x66, 0xff },
  { 0xcc, 0x66, 0xcc },
  { 0xcc, 0x66, 0x99 },
  { 0xcc, 0x66, 0x66 },
  { 0xcc, 0x66, 0x33 },
  { 0xcc, 0x66, 0x00 },
  { 0xcc, 0x33, 0xff },
  { 0xcc, 0x33, 0xcc },
  { 0xcc, 0x33, 0x99 },
  { 0xcc, 0x33, 0x66 },
  { 0xcc, 0x33, 0x33 },
  { 0xcc, 0x33, 0x00 },
  { 0xcc, 0x00, 0xff },
  { 0xcc, 0x00, 0xcc },
  { 0xcc, 0x00, 0x99 },
  { 0xcc, 0x00, 0x66 },
  { 0xcc, 0x00, 0x33 },
  { 0xcc, 0x00, 0x00 },
  { 0x99, 0xff, 0xff },
  { 0x99, 0xff, 0xcc },
  { 0x99, 0xff, 0x99 },
  { 0x99, 0xff, 0x66 },
  { 0x99, 0xff, 0x33 },
  { 0x99, 0xff, 0x00 },
  { 0x99, 0xcc, 0xff },
  { 0x99, 0xcc, 0xcc },
  { 0x99, 0xcc, 0x99 },
  { 0x99, 0xcc, 0x66 },
  { 0x99, 0xcc, 0x33 },
  { 0x99, 0xcc, 0x00 },
  { 0x99, 0x99, 0xff },
  { 0x99, 0x99, 0xcc },
  { 0x99, 0x99, 0x99 },
  { 0x99, 0x99, 0x66 },
  { 0x99, 0x99, 0x33 },
  { 0x99, 0x99, 0x00 },
  { 0x99, 0x66, 0xff },
  { 0x99, 0x66, 0xcc },
  { 0x99, 0x66, 0x99 },
  { 0x99, 0x66, 0x66 },
  { 0x99, 0x66, 0x33 },
  { 0x99, 0x66, 0x00 },
  { 0x99, 0x33, 0xff },
  { 0x99, 0x33, 0xcc },
  { 0x99, 0x33, 0x99 },
  { 0x99, 0x33, 0x66 },
  { 0x99, 0x33, 0x33 },
  { 0x99, 0x33, 0x00 },
  { 0x99, 0x00, 0xff },
  { 0x99, 0x00, 0xcc },
  { 0x99, 0x00, 0x99 },
  { 0x99, 0x00, 0x66 },
  { 0x99, 0x00, 0x33 },
  { 0x99, 0x00, 0x00 },
  { 0x66, 0xff, 0xff },
  { 0x66, 0xff, 0xcc },
  { 0x66, 0xff, 0x99 },
  { 0x66, 0xff, 0x66 },
  { 0x66, 0xff, 0x33 },
  { 0x66, 0xff, 0x00 },
  { 0x66, 0xcc, 0xff },
  { 0x66, 0xcc, 0xcc },
  { 0x66, 0xcc, 0x99 },
  { 0x66, 0xcc, 0x66 },
  { 0x66, 0xcc, 0x33 },
  { 0x66, 0xcc, 0x00 },
  { 0x66, 0x99, 0xff },
  { 0x66, 0x99, 0xcc },
  { 0x66, 0x99, 0x99 },
  { 0x66, 0x99, 0x66 },
  { 0x66, 0x99, 0x33 },
  { 0x66, 0x99, 0x00 },
  { 0x66, 0x66, 0xff },
  { 0x66, 0x66, 0xcc },
  { 0x66, 0x66, 0x99 },
  { 0x66, 0x66, 0x66 },
  { 0x66, 0x66, 0x33 },
  { 0x66, 0x66, 0x00 },
  { 0x66, 0x33, 0xff },
  { 0x66, 0x33, 0xcc },
  { 0x66, 0x33, 0x99 },
  { 0x66, 0x33, 0x66 },
  { 0x66, 0x33, 0x33 },
  { 0x66, 0x33, 0x00 },
  { 0x66, 0x00, 0xff },
  { 0x66, 0x00, 0xcc },
  { 0x66, 0x00, 0x99 },
  { 0x66, 0x00, 0x66 },
  { 0x66, 0x00, 0x33 },
  { 0x66, 0x00, 0x00 },
  { 0x33, 0xff, 0xff },
  { 0x33, 0xff, 0xcc },
  { 0x33, 0xff, 0x99 },
  { 0x33, 0xff, 0x66 },
  { 0x33, 0xff, 0x33 },
  { 0x33, 0xff, 0x00 },
  { 0x33, 0xcc, 0xff },
  { 0x33, 0xcc, 0xcc },
  { 0x33, 0xcc, 0x99 },
  { 0x33, 0xcc, 0x66 },
  { 0x33, 0xcc, 0x33 },
  { 0x33, 0xcc, 0x00 },
  { 0x33, 0x99, 0xff },
  { 0x33, 0x99, 0xcc },
  { 0x33, 0x99, 0x99 },
  { 0x33, 0x99, 0x66 },
  { 0x33, 0x99, 0x33 },
  { 0x33, 0x99, 0x00 },
  { 0x33, 0x66, 0xff },
  { 0x33, 0x66, 0xcc },
  { 0x33, 0x66, 0x99 },
  { 0x33, 0x66, 0x66 },
  { 0x33, 0x66, 0x33 },
  { 0x33, 0x66, 0x00 },
  { 0x33, 0x33, 0xff },
  { 0x33, 0x33, 0xcc },
  { 0x33, 0x33, 0x99 },
  { 0x33, 0x33, 0x66 },
  { 0x33, 0x33, 0x33 },
  { 0x33, 0x33, 0x00 },
  { 0x33, 0x00, 0xff },
  { 0x33, 0x00, 0xcc },
  { 0x33, 0x00, 0x99 },
  { 0x33, 0x00, 0x66 },
  { 0x33, 0x00, 0x33 },
  { 0x33, 0x00, 0x00 },
  { 0x00, 0xff, 0xff },
  { 0x00, 0xff, 0xcc },
  { 0x00, 0xff, 0x99 },
  { 0x00, 0xff, 0x66 },
  { 0x00, 0xff, 0x33 },
  { 0x00, 0xff, 0x00 },
  { 0x00, 0xcc, 0xff },
  { 0x00, 0xcc, 0xcc },
  { 0x00, 0xcc, 0x99 },
  { 0x00, 0xcc, 0x66 },
  { 0x00, 0xcc, 0x33 },
  { 0x00, 0xcc, 0x00 },
  { 0x00, 0x99, 0xff },
  { 0x00, 0x99, 0xcc },
  { 0x00, 0x99, 0x99 },
  { 0x00, 0x99, 0x66 },
  { 0x00, 0x99, 0x33 },
  { 0x00, 0x99, 0x00 },
  { 0x00, 0x66, 0xff },
  { 0x00, 0x66, 0xcc },
  { 0x00, 0x66, 0x99 },
  { 0x00, 0x66, 0x66 },
  { 0x00, 0x66, 0x33 },
  { 0x00, 0x66, 0x00 },
  { 0x00, 0x33, 0xff },
  { 0x00, 0x33, 0xcc },
  { 0x00, 0x33, 0x99 },
  { 0x00, 0x33, 0x66 },
  { 0x00, 0x33, 0x33 },
  { 0x00, 0x33, 0x00 },
  { 0x00, 0x00, 0xff },
  { 0x00, 0x00, 0xcc },
  { 0x00, 0x00, 0x99 },
  { 0x00, 0x00, 0x66 },
  { 0x00, 0x00, 0x33 },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 }
};
