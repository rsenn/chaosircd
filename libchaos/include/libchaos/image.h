/* chaosircd - pi-networks irc server
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
 * $Id: image.h,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

#ifndef LIB_IMAGE_H
#define LIB_IMAGE_H

/* ------------------------------------------------------------------------ *
 * Library headers                                                            *
 * ------------------------------------------------------------------------ */
#include "libchaos/defs.h"
#include "libchaos/dlink.h"

/* ------------------------------------------------------------------------ *
 * Constants                                                                  *
 * ------------------------------------------------------------------------ */
#define IMAGE_TYPE_8  0
#define IMAGE_TYPE_32 1

#define IMAGE_ALIGN_LEFT   0
#define IMAGE_ALIGN_CENTER 1
#define IMAGE_ALIGN_RIGHT  2

#if ENDIAN == ENDIAN_LIL
#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000
#define RSHIFT 0
#define GSHIFT 8
#define BSHIFT 16
#define ASHIFT 24
#else
#define RMASK 0xff000000
#define GMASK 0x00ff0000
#define BMASK 0x0000ff00
#define AMASK 0x000000ff
#define RSHIFT 24
#define GSHIFT 16
#define BSHIFT 8
#define ASHIFT 0
#endif

/* ------------------------------------------------------------------------ *
 * Image block structure.                                                     *
 * ------------------------------------------------------------------------ */
#ifndef COLOR_TYPE
struct color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};
#define COLOR_TYPE
#endif /* COLOR_TYPE */

#ifndef PALETTE_TYPE
struct palette {
  int           count;
  int           bpp;
  struct color *colors;
};
#define PALETTE_TYPE
#endif /* PALETTE_TYPE */

/* octtree */
struct ctree {
  int           key;
  struct color  color;
  uint32_t      count;
  struct ctree *children[8];
  struct ctree *parent;
};

struct font {
  uint16_t w;
  uint16_t h;
  char     data[0];
};

struct rect {
  int16_t  x;
  int16_t  y;
  uint16_t w;
  uint16_t h;
};

struct image {
  struct node         node;                 /* linking node for image_list */
  uint32_t            id;
  uint32_t            refcount;             /* times this block is referenced */
  uint32_t            hash;
  uint16_t            w;
  uint16_t            h;
  uint32_t            pitch;
  int                 colorkey;
  int                 type;
  int                 bpp;
  int                 opp;
  union {
    uint32_t         *data32;
    uint8_t          *data8;
    void             *data;
  }                   pixel;
  struct list         lines;
  struct rect         rect;
  char                name[64];    /* user-definable name */
  struct color        palette[256];
};

/* ------------------------------------------------------------------------ *
 * Global variables                                                           *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)             image_log;
CHAOS_API(struct sheap)    image_heap;       /* heap containing image blocks */
CHAOS_API(struct dheap)    image_data_heap;  /* heap containing the actual images */
CHAOS_API(struct list)     image_list;       /* list linking image blocks */
CHAOS_API(struct timer *)  image_timer;
CHAOS_API(uint32_t)        image_id;
CHAOS_API(int)             image_dirty;
CHAOS_API(struct font)     image_font_6x10;
CHAOS_API(struct font)     image_font_8x13;
CHAOS_API(struct font)     image_font_8x13b;
/* ------------------------------------------------------------------------ *
 * Initialize image heap and add garbage collect timer.                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_init              (void);

/* ------------------------------------------------------------------------ *
 * Destroy imageer heap and cancel timer.                                     *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_shutdown          (void);

/* ------------------------------------------------------------------------ *
 * Garbage collect                                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)             image_collect           (void);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_default           (struct image   *iptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct image *)  image_new               (int             type,
                                                    uint16_t        width,
                                                    uint16_t        height);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_8to32             (struct image   *iptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_32to8             (struct image   *iptr,
                                                    struct palette *palette,
                                                    int             colorkey);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_convert           (struct image   *iptr,
                                                    int             type);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_blit_32to32       (struct image   *src,
                                                    struct rect    *srect,
                                                    struct image   *dst,
                                                    struct rect    *drect);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct palette *)image_quantize          (struct image   *iptr,
                                                    int             maxcolors,
                                                    int            *ckey);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putpixel          (struct image   *iptr,
                                                    int16_t         x,
                                                    int16_t         y,
                                                    uint32_t        c);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_clear             (struct image   *iptr,
                                                    uint32_t        c);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putindex          (struct image   *iptr,
                                                    int16_t         x,
                                                    int16_t         y,
                                                    uint8_t         i);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putcolor          (struct image   *iptr,
                                                    int16_t         x,
                                                    int16_t         y,
                                                    struct color   *color);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_puthline          (struct image   *iptr,
                                                    int16_t         x1,
                                                    int16_t         x2,
                                                    int16_t         y,
                                                    uint32_t        c);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putvline          (struct image   *iptr,
                                                    int16_t         x,
                                                    int16_t         y1,
                                                    int16_t         y2,
                                                    uint32_t        c);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putline           (struct image   *iptr,
                                                    int16_t         x1,
                                                    int16_t         y1,
                                                    int16_t         x2,
                                                    int16_t         y2,
                                                    uint32_t        c);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putrect           (struct image   *iptr,
                                                    struct rect    *rect,
                                                    uint32_t        c);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putfrect          (struct image   *iptr,
                                                    struct rect    *rect,
                                                    uint32_t        c);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putcircle         (struct image   *iptr,
                                                    int16_t         x,
                                                    int16_t         y,
                                                    int             rad,
                                                    int             steps,
                                                    uint32_t        c);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putellipse        (struct image   *iptr,
                                                    int16_t         x,
                                                    int16_t         y,
                                                    int             xrad,
                                                    int             yrad,
                                                    int             steps,
                                                    uint32_t        c);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putchar           (struct image   *iptr,
                                                    struct font    *ifptr,
                                                    uint16_t        x,
                                                    uint16_t        y,
                                                    uint32_t        c,
                                                    char            a);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putstr            (struct image   *iptr,
                                                    struct font    *ifptr,
                                                    uint16_t        x,
                                                    uint16_t        y,
                                                    uint32_t        c,
                                                    int             align,
                                                    char           *s);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_putnum            (struct image   *iptr,
                                                    struct font    *ifptr,
                                                    uint16_t        x,
                                                    uint16_t        y,
                                                    uint32_t        c,
                                                    int             align,
                                                    int             num);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(int)             image_save_gif          (struct image   *iptr,
                                                    const char     *name);
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct image *)  image_load_gif          (const char     *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_delete            (struct image   *iptr);

/* ------------------------------------------------------------------------ *
 * Loose all references                                                       *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_release           (struct image   *iptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_set_name          (struct image   *iptr,
                                                    const char     *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(const char *)    image_get_name          (struct image   *iptr);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct image *)  image_find_name         (const char     *name);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct image *)  image_find_id           (uint32_t        id);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_color_hsv2rgb     (uint8_t        *hue,
                                                    uint8_t        *sat,
                                                    uint8_t        *val);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_color_set         (struct image   *iptr,
                                                    uint8_t         index,
                                                    struct color   *color);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_color_setrgb      (struct image   *iptr,
                                                    uint8_t         index,
                                                    uint8_t         red,
                                                    uint8_t         green,
                                                    uint8_t         blue);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_color_sethtml     (struct image   *iptr,
                                                    uint8_t         index,
                                                    const char     *html);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(char *)          image_color_str         (struct color    color);


/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_color_parse       (struct color   *color,
                                                    const char     *str);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_color_dump        (struct color   *color);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_rect_dump         (struct rect    *rect);

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_palette_set       (struct image   *iptr,
                                                    struct palette *pal);

/* ------------------------------------------------------------------------ *
 * Create an empty palette                                                    *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct palette *)image_palette_new       (uint32_t        ncolors);

/* ------------------------------------------------------------------------ *
 * Create a greyscale palette                                                 *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct palette *)image_palette_greyscale (uint32_t        ncolors,
                                                    int             colorkey);

/* ------------------------------------------------------------------------ *
 * Create an initialised palette                                              *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct palette *)image_palette_make      (uint32_t        ncolors,
                                                    struct color   *colors);

/* ------------------------------------------------------------------------ *
 * Copy a palette                                                             *
 * ------------------------------------------------------------------------ */
CHAOS_API(struct palette *)image_palette_copy      (struct palette *pal);

/* ------------------------------------------------------------------------ *
 * Free a palette                                                             *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_palette_free      (struct palette *palette);

/* ------------------------------------------------------------------------ *
 * Returns index of the closest match                                         *
 * ------------------------------------------------------------------------ */
CHAOS_API(uint8_t)         image_palette_match     (struct palette *palette,
                                                    struct color   *c,
                                                    int             colorkey);

  /* ------------------------------------------------------------------------ *
 * Dump imageers and image heap.                                            *
 * ------------------------------------------------------------------------ */
CHAOS_API(void)            image_dump              (struct image  *iptr);

#endif /* LIB_IMAGE_H */
