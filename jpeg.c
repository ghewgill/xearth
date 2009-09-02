/*
 * jpeg.c
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

static void jpeg_setup _P((void));
static int  jpeg_row _P((u_char *));
static void jpeg_cleanup _P((FILE *));

static gdImagePtr img;
static int y;


void jpeg_output()
{
  compute_positions();
  scan_map();
  do_dots();
  jpeg_setup();
  render(jpeg_row);
  jpeg_cleanup(stdout);
}


static void jpeg_setup()
{
  img = gdImageCreateTrueColor(wdth, hght);

  y = 0;
}


static int jpeg_row(row)
     u_char *row;
{
  int      i;

  for (i=0; i<wdth; i++)
    gdImageSetPixel(img, i, y, gdTrueColor(row[i*3+0], row[i*3+1], row[i*3+2]));

  y += 1;

  return 0;
}


static void jpeg_cleanup(s)
     FILE *s;
{
  gdImageJpeg(img, s, 90);
  gdImageDestroy(img);
}
