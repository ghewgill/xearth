/*
 * png.c
 * greg hewgill
 * august 2009
 *
 * Copyright (C) 1989, 1990, 1993-1995, 1999 Kirk Lauritz Johnson
 *
 * Parts of the source code (as marked) are:
 *   Copyright (C) 1989, 1990, 1991 by Jim Frost
 *   Copyright (C) 1992 by Jamie Zawinski <jwz@lucid.com>
 *
 * Permission to use, copy, modify and freely distribute xearth for
 * non-commercial and not-for-profit purposes is hereby granted
 * without fee, provided that both the above copyright notice and this
 * permission notice appear in all copies and in supporting
 * documentation.
 *
 * Unisys Corporation holds worldwide patent rights on the Lempel Zev
 * Welch (LZW) compression technique employed in the CompuServe GIF
 * image file format as well as in other formats. Unisys has made it
 * clear, however, that it does not require licensing or fees to be
 * paid for freely distributed, non-commercial applications (such as
 * xearth) that employ LZW/GIF technology. Those wishing further
 * information about licensing the LZW patent should contact Unisys
 * directly at (lzw_info@unisys.com) or by writing to
 *
 *   Unisys Corporation
 *   Welch Licensing Department
 *   M/S-C1SW19
 *   P.O. Box 500
 *   Blue Bell, PA 19424
 *
 * The author makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
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

#include "xearth.h"
#include "kljcpyrt.h"

#include <gd.h>

static void png_setup _P((void));
static int  png_row _P((u_char *));
static void png_cleanup _P((FILE *));

static u16or32 *dith;
static gdImagePtr img;
static int y;
static int colors[256];


void png_output()
{
  compute_positions();
  scan_map();
  do_dots();
  png_setup();
  render(png_row);
  png_cleanup(stdout);
}


static void png_setup()
{
  int  i;

  if (num_colors > 256)
    fatal("number of colors must be <= 256 with PNG output");

  img = gdImageCreate(wdth, hght);

  dither_setup(num_colors);
  dith = (u16or32 *) malloc((unsigned) sizeof(u16or32) * wdth);
  assert(dith != NULL);

  for (i=0; i<dither_ncolors; i++)
  {
    colors[i] = gdImageColorAllocate(img,
                                     dither_colormap[i*3+0],
                                     dither_colormap[i*3+1],
                                     dither_colormap[i*3+2]);
  }

  y = 0;
}


static int png_row(row)
     u_char *row;
{
  int      i, i_lim;
  u16or32 *tmp;

  tmp = dith;
  dither_row(row, tmp);

  /* use i_lim to encourage compilers to register loop limit
   */
  i_lim = wdth;
  for (i=0; i<i_lim; i++)
    gdImageSetPixel(img, i, y, colors[tmp[i]]);

  y += 1;

  return 0;
}


static void png_cleanup(s)
     FILE *s;
{
  gdImagePng(img, s);
  gdImageDestroy(img);

  dither_cleanup();
  free(dith);
}
