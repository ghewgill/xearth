/*
 * gif.c
 * kirk johnson
 * july 1993
 *
 * RCS $Id: gif.c,v 1.5 1994/05/20 01:37:40 tuna Exp $
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

#include "xearth.h"
#include "giflib.h"
#include "kljcpyrt.h"

static u_char *dith;


void gif_setup(s)
     FILE *s;
{
  int  i;
  int  rtn;
  BYTE cmap[3][256];

  dither_setup(num_colors);
  dith = (u_char *) malloc((unsigned) wdth);
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


void gif_row(row)
     u_char *row;
{
  int i;

  dither_row(row, dith);

  for (i=0; i<wdth; i++)
    gifout_put_pixel(dith[i]);
}


void gif_cleanup()
{
  int rtn;

  rtn = gifout_close_image();
  assert(rtn == GIFLIB_SUCCESS);

  rtn = gifout_close_file();
  assert(rtn == GIFLIB_SUCCESS);

  dither_cleanup();
  free(dith);
}
