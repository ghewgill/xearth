/*
 * dither.c
 * kirk johnson
 * july 1993
 *
 * RCS $Id: dither.c,v 1.9 1994/05/22 02:45:47 tuna Exp $
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
#include "kljcpyrt.h"

static void dither_row_ltor();
static void dither_row_rtol();
static void mono_dither_row_ltor();
static void mono_dither_row_rtol();

static u_char level[256];
static u_char grn_idx[256];
static u_char blu_idx[256];
static short *curr;
static short *next;
static int    even_row;

int     dither_ncolors;
u_char *dither_colormap;


void dither_setup(ncolors)
     int ncolors;
{
  int i;
  int val;
  int half;

  half = (ncolors - 2) / 2;
  ncolors = half*2 + 2;
  dither_ncolors = ncolors;

  dither_colormap = (u_char *) malloc((unsigned) ncolors*3);
  assert(dither_colormap != NULL);
  xearth_bzero((char *) dither_colormap, ncolors*3);

  level[0] = 0;
  for (i=1; i<=half; i++)
  {
    val = (i * 255) / half;

    dither_colormap[i*3+0] = 0;
    dither_colormap[i*3+1] = val;
    dither_colormap[i*3+2] = 0;
    level[i] = val;

    i += half;
    dither_colormap[i*3+0] = 0;
    dither_colormap[i*3+1] = 0;
    dither_colormap[i*3+2] = val;
    level[i] = val;
    i -= half;
  }

  dither_colormap[(ncolors-1)*3+0] = 255;
  dither_colormap[(ncolors-1)*3+1] = 255;
  dither_colormap[(ncolors-1)*3+2] = 255;
  level[ncolors-1] = 255;

  for (i=0; i<256; i++)
  {
    val = (i * half + 127) / 255;

    grn_idx[i] = val;

    if (val == 0)
      blu_idx[i] = val;
    else
      blu_idx[i] = val + half;
  }

  curr = (short *) malloc((unsigned) sizeof(short) * 2 * (wdth+2));
  assert(curr != NULL);
  xearth_bzero((char *) curr, sizeof(short) * 2 * (wdth+2));
  curr += 2;

  next = (short *) malloc((unsigned) sizeof(short) * 2 * (wdth+2));
  assert(next != NULL);
  xearth_bzero((char *) next, sizeof(short) * 2 * (wdth+2));
  next += 2;

  even_row = 1;
}


void dither_row(row, rslt)
     u_char *row;
     u_char *rslt;
{
  if (even_row)
    dither_row_ltor(row, rslt);
  else
    dither_row_rtol(row, rslt);

  even_row = !even_row;
}


void dither_cleanup()
{
  free(curr-2);
  free(next-2);
  free(dither_colormap);
}


static void dither_row_ltor(row, rslt)
     u_char *row;
     u_char *rslt;
{
  int     i, i_lim;
  int     grn, g_tmp;
  int     blu, b_tmp;
  int     idx;
  u_char *rowtmp;
  short  *currtmp;
  short  *nexttmp;

  rowtmp  = row;
  currtmp = curr;
  nexttmp = next;

  /* use i_lim to encourage compilers to register loop limit
   */
  i_lim = wdth;
  for (i=0; i<i_lim; i++)
  {
    grn = rowtmp[1];
    blu = rowtmp[2];

    if ((grn == 0) && (blu == 0))
    {
      rslt[i] = 0;
    }
    else if ((grn == 255) && (blu == 255))
    {
      rslt[i] = dither_ncolors - 1;
    }
    else
    {
      grn += currtmp[0];
      blu += currtmp[1];

      if (grn > blu)
      {
	if (grn < 0)
	  grn = 0;
	else if (grn > 255)
	  grn = 255;

	idx  = grn_idx[grn];
	grn -= level[idx];
      }
      else
      {
	if (blu < 0)
	  blu = 0;
	else if (blu > 255)
	  blu = 255;

	idx  = blu_idx[blu];
	blu -= level[idx];
      }

      rslt[i] = idx;

      g_tmp = (grn<0) ? 7 : 8;
      b_tmp = (blu<0) ? 7 : 8;

      currtmp[ 2] += ((grn * 7) + g_tmp) >> 4;
      currtmp[ 3] += ((blu * 7) + b_tmp) >> 4;
      nexttmp[ 2] += ((grn * 1) + g_tmp) >> 4;
      nexttmp[ 3] += ((blu * 1) + b_tmp) >> 4;
      nexttmp[ 0] += ((grn * 5) + g_tmp) >> 4;
      nexttmp[ 1] += ((blu * 5) + b_tmp) >> 4;
      nexttmp[-2] += ((grn * 3) + g_tmp) >> 4;
      nexttmp[-1] += ((blu * 3) + b_tmp) >> 4;
    }

    rowtmp  += 3;
    currtmp += 2;
    nexttmp += 2;
  }

  currtmp = curr;
  curr    = next;
  next    = currtmp;
  xearth_bzero((char *) next, sizeof(short) * 2 * wdth);
}


static void dither_row_rtol(row, rslt)
     u_char *row;
     u_char *rslt;
{
  int     i;
  int     grn, g_tmp;
  int     blu, b_tmp;
  int     idx;
  u_char *rowtmp;
  short  *currtmp;
  short  *nexttmp;

  rowtmp  = row  + 3*(wdth-1);
  currtmp = curr + 2*(wdth-1);
  nexttmp = next + 2*(wdth-1);

  for (i=(wdth-1); i>=0; i--)
  {
    grn = rowtmp[1];
    blu = rowtmp[2];

    if ((grn == 0) && (blu == 0))
    {
      rslt[i] = 0;
    }
    else if ((grn == 255) && (blu == 255))
    {
      rslt[i] = dither_ncolors - 1;
    }
    else
    {
      grn += currtmp[0];
      blu += currtmp[1];

      if (grn > blu)
      {
	if (grn < 0)
	  grn = 0;
	else if (grn > 255)
	  grn = 255;

	idx  = grn_idx[grn];
	grn -= level[idx];
      }
      else
      {
	if (blu < 0)
	  blu = 0;
	else if (blu > 255)
	  blu = 255;

	idx  = blu_idx[blu];
	blu -= level[idx];
      }

      rslt[i] = idx;

      g_tmp = (grn<0) ? 7 : 8;
      b_tmp = (blu<0) ? 7 : 8;

      currtmp[-2] += ((grn * 7) + g_tmp) >> 4;
      currtmp[-1] += ((blu * 7) + b_tmp) >> 4;
      nexttmp[-2] += ((grn * 1) + g_tmp) >> 4;
      nexttmp[-1] += ((blu * 1) + b_tmp) >> 4;
      nexttmp[ 0] += ((grn * 5) + g_tmp) >> 4;
      nexttmp[ 1] += ((blu * 5) + b_tmp) >> 4;
      nexttmp[ 2] += ((grn * 3) + g_tmp) >> 4;
      nexttmp[ 3] += ((blu * 3) + b_tmp) >> 4;
    }

    rowtmp  -= 3;
    currtmp -= 2;
    nexttmp -= 2;
  }

  currtmp = curr;
  curr    = next;
  next    = currtmp;
  xearth_bzero((char *) next, sizeof(short) * 2 * wdth);
}


void mono_dither_setup()
{
  curr = (short *) malloc((unsigned) sizeof(short) * (wdth+2));
  assert(curr != NULL);
  xearth_bzero((char *) curr, sizeof(short) * (wdth+2));
  curr += 1;

  next = (short *) malloc((unsigned) sizeof(short) * (wdth+2));
  assert(next != NULL);
  xearth_bzero((char *) next, sizeof(short) * (wdth+2));
  next += 1;

  even_row = 1;
}


void mono_dither_row(row)
     u_char *row;
{
  if (even_row)
    mono_dither_row_ltor(row);
  else
    mono_dither_row_rtol(row);

  even_row = !even_row;
}


void mono_dither_cleanup()
{
  free(curr-1);
  free(next-1);
}


static void mono_dither_row_ltor(row)
     u_char *row;
{
  int     i, i_lim;
  int     val, tmp;
  u_char *rowtmp;
  short  *currtmp;
  short  *nexttmp;

  rowtmp  = row;
  currtmp = curr;
  nexttmp = next;

  /* use i_lim to encourage compilers to register loop limit
   */
  i_lim = wdth;
  for (i=0; i<i_lim; i++)
  {
    val = rowtmp[0];

    if (val == 0)
    {
      rowtmp[0] = 0;
    }
    else if (val == 255)
    {
      rowtmp[0] = 1;
    }
    else
    {
      val += currtmp[0];

      if (val > 127)
      {
	rowtmp[0] = 1;
	val -= 255;
      }
      else
      {
	rowtmp[0] = 0;
      }

      tmp = (val < 0) ? 7 : 8;
      currtmp[ 1] += ((val * 7) + tmp) >> 4;
      nexttmp[ 1] += ((val * 1) + tmp) >> 4;
      nexttmp[ 0] += ((val * 5) + tmp) >> 4;
      nexttmp[-1] += ((val * 3) + tmp) >> 4;
    }

    rowtmp  += 1;
    currtmp += 1;
    nexttmp += 1;
  }

  currtmp = curr;
  curr    = next;
  next    = currtmp;
  xearth_bzero((char *) next, sizeof(short) * wdth);
}


static void mono_dither_row_rtol(row)
     u_char *row;
{
  int     i;
  int     val, tmp;
  u_char *rowtmp;
  short  *currtmp;
  short  *nexttmp;

  rowtmp  = row  + (wdth-1);
  currtmp = curr + (wdth-1);
  nexttmp = next + (wdth-1);

  for (i=(wdth-1); i>=0; i--)
  {
    val = rowtmp[0];

    if (val == 0)
    {
      rowtmp[0] = 0;
    }
    else if (val == 255)
    {
      rowtmp[0] = 1;
    }
    else
    {
      val += currtmp[0];

      if (val > 127)
      {
	rowtmp[0] = 1;
	val -= 255;
      }
      else
      {
	rowtmp[0] = 0;
      }

      tmp = (val < 0) ? 7 : 8;
      currtmp[-1] += ((val * 7) + tmp) >> 4;
      nexttmp[-1] += ((val * 1) + tmp) >> 4;
      nexttmp[ 0] += ((val * 5) + tmp) >> 4;
      nexttmp[ 1] += ((val * 3) + tmp) >> 4;
    }

    rowtmp  -= 1;
    currtmp -= 1;
    nexttmp -= 1;
  }

  currtmp = curr;
  curr    = next;
  next    = currtmp;
  xearth_bzero((char *) next, sizeof(short) * wdth);
}
