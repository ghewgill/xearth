/*
 * xearth.h
 * kirk johnson
 * july 1993
 *
 * RCS $Id: xearth.h,v 1.15 1994/06/01 18:04:35 tuna Exp $
 *
 * Copyright (C) 1989, 1990, 1993, 1994 Kirk Lauritz Johnson
 *
 * Parts of the source code (as marked) are:
 *   Copyright (C) 1989, 1990, 1991 by Jim Frost
 *   Copyright (C) 1992 by Jamie Zawinski <jwz@lucid.com>
 *
 * Permission to use, copy, modify, distribute, and sell this
 * software and its documentation for any purpose is hereby granted
 * without fee, provided that the above copyright notice appear in
 * all copies and that both that copyright notice and this
 * permission notice appear in supporting documentation. The author
 * makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _XEARTH_H_
#define _XEARTH_H_

#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include "extarr.h"
#include "kljcpyrt.h"

#define VersionString "0.92"

/* if NO_RANDOM is defined, use lrand48() instead of random()
 */
#ifdef NO_RANDOM
#define random() lrand48()
#endif /* NO_RANDOM */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif /* !M_PI */

/* rotational period of the earth (seconds)
 */
#define EarthPeriod (86400)

/* types of pixels
 */
#define PixTypeSpace     (0)
#define PixTypeLand      (1)
#define PixTypeWater     (2)
#define PixTypeStar      (3)
#define PixTypeGridLand  (4)
#define PixTypeGridWater (5)

/* types of dots
 */
#define DotTypeStar (0)
#define DotTypeGrid (1)

#define ViewPosTypeFixed (0)
#define ViewPosTypeSun   (1)
#define ViewPosTypeOrbit (2)

#define XPROJECT(x)     ((proj_scale*(x))+proj_xofs)
#define YPROJECT(y)     (proj_yofs-(proj_scale*(y)))
#define INV_XPROJECT(x) (((x)-proj_xofs)*inv_proj_scale)
#define INV_YPROJECT(y) ((proj_yofs-(y))*inv_proj_scale)

typedef struct
{
  short y;
  short lo_x;
  short hi_x;
  short val;
} ScanBit;

typedef struct
{
  short  x;
  short  y;
  u_char type;
} ScanDot;

/* dither.c */
extern int     dither_ncolors;
extern u_char *dither_colormap;
extern void    dither_setup();
extern void    dither_row();
extern void    dither_cleanup();
extern void    mono_dither_setup();
extern void    mono_dither_row();
extern void    mono_dither_cleanup();

/* gif.c */
extern void gif_setup();
extern void gif_row();
extern void gif_cleanup();

/* mapdata.c */
extern short map_data[];

/* ppm.c */
extern void ppm_setup();
extern void ppm_row();

/* render.c */
extern void render_no_shading();
extern void render_with_shading();
extern void do_dots();

/* resources.c */
extern char  *get_string_resource();
extern int    get_boolean_resource();
extern int    get_integer_resource();
extern double get_float_resource();
extern int    get_pixel_resource();

/* scan.c */
extern double cos_view_lat;
extern double sin_view_lat;
extern double cos_view_lon;
extern double sin_view_lon;
extern double proj_scale;
extern double proj_xofs;
extern double proj_yofs;
extern double inv_proj_scale;
extern ExtArr scanbits;
extern void   scan_map();

/* sunpos.c */
extern void sun_position();

/* x11.c */
extern void command_line_x();
extern void x11_resources();
extern void x11_setup();
extern void x11_row();
extern void x11_cleanup();

/* xearth.c */
extern char  *progname;
extern int    wdth;
extern int    hght;
extern int    shift_x;
extern int    shift_y;
extern char  *dpyname;
extern double view_lon;
extern double view_lat;
extern double view_mag;
extern double sun_lon;
extern int    do_shade;
extern double sun_lat;
extern int    do_stars;
extern double star_freq;
extern int    do_grid;
extern int    grid_big;
extern int    grid_small;
extern int    do_label;
extern int    do_markers;
extern int    wait_time;
extern double time_warp;
extern int    fixed_time;
extern int    day;
extern int    night;
extern double xgamma;
extern int    use_two_pixmaps;
extern int    num_colors;
extern int    do_fork;
extern int    priority;
extern time_t current_time;
extern void   decode_viewing_pos();
extern void   decode_sun_pos();
extern void   decode_size();
extern void   decode_shift();
extern void   xearth_bzero();
extern void   version_info();
extern void   usage();
extern void   warning();
extern void   fatal();

#ifdef USE_EXACT_SQRT

#define SQRT(x) (((x) <= 0.0) ? (0.0) : (sqrt(x)))

#else

/*
 * brute force approximation for sqrt() over [0,1]
 *  - two quadratic regions
 *  - returns zero for args less than zero
 */
#define SQRT(x)                                             \
  (((x) > 0.13)                                             \
   ? ((((-0.3751672414*(x))+1.153263483)*(x))+0.2219037586) \
   : (((x) > 0.0)                                           \
      ? ((((-9.637346154*(x))+3.56143)*(x))+0.065372935)    \
      : (0.0)))

#endif /* USE_EXACT_SQRT */

#ifndef isupper
# define isupper(c)  ((c) >= 'A' && (c) <= 'Z')
#endif
#ifndef _tolower
# define _tolower(c)  ((c) - 'A' + 'a')
#endif

#endif
