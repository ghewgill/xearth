/*
 * render.c
 * kirk johnson
 * october 1993
 *
 * RCS $Id: render.c,v 1.7 1994/05/23 23:03:01 tuna Exp $
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

static void new_stars();
static void new_grid();

static int      scanbitcnt;
static ScanBit *scanbit;
static u_char   scan_to_pix[256];

static ExtArr   dots = NULL;
static int      dotcnt;
static ScanDot *dot;


static int dot_comp(a, b)
     void *a;
     void *b;
{
  return (((ScanDot *) a)->y - ((ScanDot *) b)->y);
}


static void render_rows_setup()
{
  int i;

  scanbitcnt = scanbits->count;
  scanbit    = (ScanBit *) scanbits->body;
  dotcnt     = dots->count;
  dot        = (ScanDot *) dots->body;

  /* precompute table for translating between
   * scan buffer values and pixel types
   */
  for (i=0; i<256; i++)
    if (i == 0)
      scan_to_pix[i] = PixTypeSpace;
    else if (i > 64)
      scan_to_pix[i] = PixTypeLand;
    else
      scan_to_pix[i] = PixTypeWater;
}


static void render_next_row(buf, idx)
     char *buf;
     int   idx;
{
  int      i, i_lim;
  int      tmp;
  int      _scanbitcnt;
  ScanBit *_scanbit;

  xearth_bzero((char *) buf, wdth);

  /* explicitly copy scanbitcnt and scanbit to local variables
   * to help compilers figure out that they can be registered
   */
  _scanbitcnt = scanbitcnt;
  _scanbit    = scanbit;

  while ((_scanbitcnt > 0) && (_scanbit->y == idx))
  {
    /* use i_lim to encourage compilers to register loop limit
     */
    i_lim = _scanbit->hi_x;
    for (i=_scanbit->lo_x; i<=i_lim; i++)
      buf[i] += _scanbit->val;

    _scanbit    += 1;
    _scanbitcnt -= 1;
  }

  /* copy changes to scanbitcnt and scanbit out to memory
   */
  scanbitcnt = _scanbitcnt;
  scanbit    = _scanbit;

  /* use i_lim to encourage compilers to register loop limit
   */
  i_lim = wdth;
  for (i=0; i<i_lim; i++)
    buf[i] = scan_to_pix[(u_char) buf[i]];

  while ((dotcnt > 0) && (dot->y == idx))
  {
    tmp = dot->x;

    if (dot->type == DotTypeStar)
    {
      if (buf[tmp] == PixTypeSpace)
	buf[tmp] = PixTypeStar;
    }
    else
    {
      switch (buf[tmp])
      {
      case PixTypeLand:
	buf[tmp] = PixTypeGridLand;
	break;

      case PixTypeWater:
	buf[tmp] = PixTypeGridWater;
	break;
      }
    }

    dot    += 1;
    dotcnt -= 1;
  }
}


void render_no_shading(rowfunc)
     void (*rowfunc)();
{
  int     i, i_lim;
  int     j, j_lim;
  char   *scanbuf;
  u_char *row;
  u_char *tmp;

  scanbuf = (char *) malloc((unsigned) wdth);
  assert(scanbuf != NULL);
  row = (u_char *) malloc((unsigned) wdth*3);
  assert(row != NULL);
  render_rows_setup();

  /* main render loop
   * (use i_lim and j_lim to encourage compilers to register loop limits)
   */
  i_lim = hght;
  j_lim = wdth;
  for (i=0; i<i_lim; i++)
  {
    render_next_row(scanbuf, i);

    tmp = row;
    for (j=0; j<j_lim; j++)
    {
      switch (scanbuf[j])
      {
      case PixTypeSpace:	/* black */
	tmp[0] = 0;
	tmp[1] = 0;
	tmp[2] = 0;
	break;

      case PixTypeStar:		/* white */
      case PixTypeGridLand:
      case PixTypeGridWater:
        tmp[0] = 255;
        tmp[1] = 255;
        tmp[2] = 255;
	break;

      case PixTypeLand:		/* green */
        tmp[0] = 0;
        tmp[1] = 255;
        tmp[2] = 0;
	break;

      case PixTypeWater:	/* blue */
        tmp[0] = 0;
        tmp[1] = 0;
        tmp[2] = 255;
	break;

      default:
	assert(0);
      }

      tmp += 3;
    }

    rowfunc(row);
  }

  free(scanbuf);
  free(row);
}


void render_with_shading(rowfunc)
     void (*rowfunc)();
{
  int     i, i_lim;
  int     j, j_lim;
  int     base_val;
  double  delta_val;
  int     val;
  char   *scanbuf;
  u_char *row;
  u_char *tmp;
  double  tmp1, tmp2;
  double  x, y, z;
  double  sol[3];
  double  scale;
  double *inv_x;

  scanbuf = (char *) malloc((unsigned) wdth);
  assert(scanbuf != NULL);
  row = (u_char *) malloc((unsigned) wdth*3);
  assert(row != NULL);
  render_rows_setup();

  sol[0] = sin(sun_lon * (M_PI/180)) * cos(sun_lat * (M_PI/180));
  sol[1] = sin(sun_lat * (M_PI/180));
  sol[2] = cos(sun_lon * (M_PI/180)) * cos(sun_lat * (M_PI/180));

  tmp1 = (cos_view_lon * sol[0]) - (sin_view_lon * sol[2]);
  tmp2 = (sin_view_lon * sol[0]) + (cos_view_lon * sol[2]);
  sol[0] = tmp1;
  sol[2] = tmp2;

  tmp1 = (cos_view_lat * sol[1]) - (sin_view_lat * sol[2]);
  tmp2 = (sin_view_lat * sol[1]) + (cos_view_lat * sol[2]);
  sol[1] = tmp1;
  sol[2] = tmp2;

  /* precompute INV_XPROJECT() values
   * (use i_lim to encourage compilers to register loop limit)
   */
  inv_x = (double *) malloc((unsigned) sizeof(double) * wdth);
  assert(inv_x != NULL);
  i_lim = wdth;
  for (i=0; i<i_lim; i++)
    inv_x[i] = INV_XPROJECT(i);

  /* precompute shading parameters */
  base_val  = (night * 255.99) / 100;
  delta_val = (day * 255.99) / 100 - base_val;

  /* main render loop
   * (use i_lim and j_lim to encourage compilers to register loop limits)
   */
  i_lim = hght;
  j_lim = wdth;
  for (i=0; i<i_lim; i++)
  {
    y = INV_YPROJECT(i);
    render_next_row(scanbuf, i);

    tmp = row;
    for (j=0; j<j_lim; j++)
    {
      switch (scanbuf[j])
      {
      case PixTypeSpace:	/* black */
	tmp[0] = 0;
	tmp[1] = 0;
	tmp[2] = 0;
	break;

      case PixTypeStar:		/* white */
      case PixTypeGridLand:
      case PixTypeGridWater:
        tmp[0] = 255;
        tmp[1] = 255;
        tmp[2] = 255;
	break;

      case PixTypeLand:		/* green */
      case PixTypeWater:
        x = inv_x[j];
        z = 1 - x*x - y*y;
        z = SQRT(z);
        scale = x*sol[0] + y*sol[1] + z*sol[2];
        if (scale < 0)
        {
          val = base_val;
        }
        else
        {
	  val = base_val + scale*delta_val;
	  if (val > 255) val = 255;
        }
        assert(val >= 0);

        if (scanbuf[j] == 1)
        {
          /* land (green) */
          tmp[0] = 0;
          tmp[1] = val;
          tmp[2] = 0;
        }
        else
        {
          /* water (blue) */
          tmp[0] = 0;
          tmp[1] = 0;
          tmp[2] = val;
        }
	break;

      default:
	assert(0);
      }

      tmp += 3;
    }

    rowfunc(row);
  }

  free(scanbuf);
  free(row);
  free(inv_x);
}


void do_dots()
{
  if (dots == NULL)
    dots = extarr_alloc(sizeof(ScanDot));
  else
    dots->count = 0;

  if (do_stars) new_stars(star_freq);
  if (do_grid) new_grid(grid_big, grid_small);

  qsort(dots->body, dots->count, sizeof(ScanDot), dot_comp);
}


static void new_stars(freq)
     double freq;
{
  int      i;
  int      x, y;
  int      max_stars;
  ScanDot *newdot;

  max_stars = wdth * hght * freq;

  for (i=0; i<max_stars; i++)
  {
    x = random() % wdth;
    y = random() % hght;

    newdot = (ScanDot *) extarr_next(dots);
    newdot->x    = x;
    newdot->y    = y;
    newdot->type = DotTypeStar;
  }
}


static void new_grid(big, small)
     int big;
     int small;
{
  int      i, j;
  int      x, y;
  int      cnt;
  double   lat, lon;
  double   lat_scale, lon_scale;
  double   cos_lat, sin_lat;
  double   cos_lon, sin_lon;
  double   pos[3];
  double   tmp1, tmp2;
  ScanDot *newdot;

  /* lines of longitude
   */
  lon_scale = M_PI / (2 * big);
  lat_scale = M_PI / (2 * big * small);
  for (i=(-2*big); i<(2*big); i++)
  {
    lon     = i * lon_scale;
    cos_lon = cos(lon);
    sin_lon = sin(lon);

    for (j=(-(big*small)+1); j<(big*small); j++)
    {
      lat     = j * lat_scale;
      cos_lat = cos(lat);
      sin_lat = sin(lat);

      pos[0] = sin_lon * cos_lat;
      pos[1] = sin_lat;
      pos[2] = cos_lon * cos_lat;

      tmp1 = (cos_view_lon * pos[0]) - (sin_view_lon * pos[2]);
      tmp2 = (sin_view_lon * pos[0]) + (cos_view_lon * pos[2]);
      pos[0] = tmp1;
      pos[2] = tmp2;

      tmp1 = (cos_view_lat * pos[1]) - (sin_view_lat * pos[2]);
      tmp2 = (sin_view_lat * pos[1]) + (cos_view_lat * pos[2]);
      pos[1] = tmp1;
      pos[2] = tmp2;

      if (pos[2] >= 0)
      {
	x = XPROJECT(pos[0]);
	y = YPROJECT(pos[1]);
	if ((x >= 0) && (x < wdth) && (y >= 0) && (y < hght))
	{
	  newdot = (ScanDot *) extarr_next(dots);
	  newdot->x    = x;
	  newdot->y    = y;
	  newdot->type = DotTypeGrid;
	}
      }
    }
  }

  /* lines of latitude
   */
  lat_scale = M_PI / (2 * big);
  for (i=(1-big); i<big; i++)
  {
    lat       = i * lat_scale;
    cos_lat   = cos(lat);
    sin_lat   = sin(lat);
    cnt       = 2 * ((int) ((cos_lat * small) + 0.5)) * big;
    lon_scale = M_PI / cnt;

    for (j=(-cnt); j<cnt; j++)
    {
      lon     = j * lon_scale;
      cos_lon = cos(lon);
      sin_lon = sin(lon);

      pos[0] = sin_lon * cos_lat;
      pos[1] = sin_lat;
      pos[2] = cos_lon * cos_lat;

      tmp1 = (cos_view_lon * pos[0]) - (sin_view_lon * pos[2]);
      tmp2 = (sin_view_lon * pos[0]) + (cos_view_lon * pos[2]);
      pos[0] = tmp1;
      pos[2] = tmp2;

      tmp1 = (cos_view_lat * pos[1]) - (sin_view_lat * pos[2]);
      tmp2 = (sin_view_lat * pos[1]) + (cos_view_lat * pos[2]);
      pos[1] = tmp1;
      pos[2] = tmp2;

      if (pos[2] >= 0)
      {
	x = XPROJECT(pos[0]);
	y = YPROJECT(pos[1]);
	if ((x >= 0) && (x < wdth) && (y >= 0) && (y < hght))
	{
	  newdot = (ScanDot *) extarr_next(dots);
	  newdot->x    = x;
	  newdot->y    = y;
	  newdot->type = DotTypeGrid;
	}
      }
    }
  }
}
