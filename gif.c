/*
 * gif.c
 * kirk johnson
 * july 1993
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
#include "giflib.h"
#include "kljcpyrt.h"

static void gif_setup _P((FILE *));
static int  gif_row _P((u_char *));
static void gif_cleanup _P((void));

static u16or32 *dith;


void gif_output()
{
  compute_positions();
  scan_map();
  do_dots();
  gif_setup(stdout);
  render(gif_row);
  gif_cleanup();
}


static void gif_setup(s)
     FILE *s;
{
  int  i;
  int  rtn;
  BYTE cmap[3][256];

  if (num_colors > 256)
    fatal("number of colors must be <= 256 with GIF output");

  dither_setup(num_colors);
  dith = (u16or32 *) malloc((unsigned) sizeof(u16or32) * wdth);
  assert(dith != NULL);

  for (i=0; i<dither_ncolors; i++)
  {
    cmap[0][i] = dither_colormap[i*3+0];
    cmap[1][i] = dither_colormap[i*3+1];
    cmap[2][i] = dither_colormap[i*3+2];
  }

  rtn = gifout_open_file(s, wdth, hght, dither_ncolors, cmap, 0);
  assert(rtn == GIFLIB_SUCCESS);

  rtn = gifout_open_image(0, 0, wdth, hght);
  assert(rtn == GIFLIB_SUCCESS);
}


static int gif_row(row)
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
    gifout_put_pixel((int) tmp[i]);

  return 0;
}


static void gif_cleanup()
{
  int rtn;

  rtn = gifout_close_image();
  assert(rtn == GIFLIB_SUCCESS);

  rtn = gifout_close_file();
  assert(rtn == GIFLIB_SUCCESS);

  dither_cleanup();
  free(dith);
}
