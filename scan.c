/*
 * scan.c
 * kirk johnson
 * august 1993
 *
 * RCS $Id: scan.c,v 1.6 1994/05/23 23:03:26 tuna Exp $
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

#define MAP_DATA_SCALE  (30000)

#define XingTypeEntry (0)
#define XingTypeExit  (1)

typedef struct
{
  int    type;
  int    cidx;
  double x, y;
  double angle;
} EdgeXing;

void       scan_map();
void       scan_outline();
void       scan_curves();
double    *extract_curve();
void       scan_along_curve();
void       handle_xings();
void       scan_arc();
void       scan();
void       get_scanbits();

double cos_view_lat;
double sin_view_lat;
double cos_view_lon;
double sin_view_lon;

double proj_scale;
double proj_xofs;
double proj_yofs;
double inv_proj_scale;

int     first_scan = 1;
ExtArr  scanbits;
ExtArr  edgexings;
int     min_y, max_y;
ExtArr *scanbuf;


static int double_comp(a, b)
     void *a;
     void *b;
{
  double val_a;
  double val_b;
  int    rslt;

  val_a = *((double *) a);
  val_b = *((double *) b);

  if (val_a < val_b)
    rslt = -1;
  else if (val_a > val_b)
    rslt = 1;
  else
    rslt = 0;

  return rslt;
}


static int scanbit_comp(a, b)
     void *a;
     void *b;
{
  return (((ScanBit *) a)->y - ((ScanBit *) b)->y);
}


static int edgexing_comp(a, b)
     void *a;
     void *b;
{
  double angle_a;
  double angle_b;
  int    rslt;

  angle_a = ((EdgeXing *) a)->angle;
  angle_b = ((EdgeXing *) b)->angle;

  if (angle_a < angle_b)
    rslt = -1;
  else if (angle_a > angle_b)
    rslt = 1;
  else
    rslt = 0;

  return rslt;
}


void scan_map()
{
  int    i;
  double inv_mag;

  cos_view_lat = cos(view_lat * (M_PI/180));
  sin_view_lat = sin(view_lat * (M_PI/180));
  cos_view_lon = cos(view_lon * (M_PI/180));
  sin_view_lon = sin(view_lon * (M_PI/180));

  inv_mag        = 1 / view_mag;
  proj_scale     = ((hght < wdth)
		    ? (hght / (2 * inv_mag))
		    : (wdth / (2 * inv_mag)));
  proj_xofs      = (double) wdth / 2 + shift_x;
  proj_yofs      = (double) hght / 2 + shift_y;
  inv_proj_scale = 1 / proj_scale;

  /* the first time through, allocate scanbits.
   * on subsequent passes, simply reset it.
   */
  if (first_scan)
    scanbits = extarr_alloc(sizeof(ScanBit));
  else
    scanbits->count  = 0;

  edgexings = extarr_alloc(sizeof(EdgeXing));

  scanbuf = (ExtArr *) malloc((unsigned) sizeof(ExtArr) * hght);
  assert(scanbuf != NULL);
  for (i=0; i<hght; i++)
    scanbuf[i] = extarr_alloc(sizeof(double));

  scan_outline();
  scan_curves();

  extarr_free(edgexings);
  for (i=0; i<hght; i++)
    extarr_free(scanbuf[i]);
  free(scanbuf);

  qsort(scanbits->body, scanbits->count, sizeof(ScanBit), scanbit_comp);

  first_scan = 0;
}


void scan_outline()
{
  min_y = hght;
  max_y = -1;
  scan_arc(1.0, 0.0, 0.0, 1.0, 0.0, 2*M_PI);
  get_scanbits(64);
}


void scan_curves()
{
  int     i;
  int     cidx;
  int     npts;
  int     val;
  short  *raw;
  double *pos;
  double *prev;
  double *curr;

  cidx = 0;
  raw  = map_data;
  while (1)
  {
    npts = raw[0];
    if (npts == 0) break;
    val  = raw[1];
    raw += 2;

    pos   = extract_curve(npts, raw);
    prev  = pos + (npts-1)*3;
    curr  = pos;
    min_y = hght;
    max_y = -1;

    for (i=0; i<npts; i++)
    {
      scan_along_curve(prev, curr, cidx);
      prev  = curr;
      curr += 3;
    }

    free(pos);
    if (edgexings->count > 0)
      handle_xings();
    if (min_y <= max_y)
      get_scanbits(val);

    cidx += 1;
    raw  += 3*npts;
  }
}


double *extract_curve(npts, data)
     int    npts;
     short *data;
{
  int     i;
  double  tmp1;
  double  tmp2;
  double *pos;
  double *rslt;

  rslt = (double *) malloc((unsigned) sizeof(double) * 3 * npts);
  assert(rslt != NULL);

  pos = rslt;
  for (i=0; i<npts; i++)
  {
    pos[0] = data[0] * (1.0/MAP_DATA_SCALE);
    pos[1] = data[1] * (1.0/MAP_DATA_SCALE);
    pos[2] = data[2] * (1.0/MAP_DATA_SCALE);

    tmp1 = (cos_view_lon * pos[0]) - (sin_view_lon * pos[2]);
    tmp2 = (sin_view_lon * pos[0]) + (cos_view_lon * pos[2]);
    pos[0] = tmp1;
    pos[2] = tmp2;

    tmp1 = (cos_view_lat * pos[1]) - (sin_view_lat * pos[2]);
    tmp2 = (sin_view_lat * pos[1]) + (cos_view_lat * pos[2]);
    pos[1] = tmp1;
    pos[2] = tmp2;

    data += 3;
    pos  += 3;
  }

  return rslt;
}


void scan_along_curve(prev, curr, cidx)
     double *prev;
     double *curr;
     int     cidx;
{
  double    tmp;
  double    extra[3];
  EdgeXing *xing;

  if (prev[2] <= 0)             /* prev not visible */
  {
    if (curr[2] <= 0)
      return;                   /* neither point visible */

    tmp = curr[2] / (curr[2] - prev[2]);
    extra[0] = curr[0] - tmp * (curr[0] - prev[0]);
    extra[1] = curr[1] - tmp * (curr[1] - prev[1]);
    extra[2] = 0;

    tmp = sqrt(extra[0]*extra[0] + extra[1]*extra[1]);
    extra[0] /= tmp;
    extra[1] /= tmp;

    /* extra[] is an edge crossing (entry point) */
    xing = (EdgeXing *) extarr_next(edgexings);
    xing->type  = XingTypeEntry;
    xing->cidx  = cidx;
    xing->x     = extra[0];
    xing->y     = extra[1];
    xing->angle = atan2(extra[1], extra[0]);

    prev = extra;
  }
  else if (curr[2] <= 0)        /* curr not visible */
  {
    tmp = curr[2] / (curr[2] - prev[2]);
    extra[0] = curr[0] - tmp * (curr[0] - prev[0]);
    extra[1] = curr[1] - tmp * (curr[1] - prev[1]);
    extra[2] = 0;

    tmp = sqrt(extra[0]*extra[0] + extra[1]*extra[1]);
    extra[0] /= tmp;
    extra[1] /= tmp;

    /* extra[] is an edge crossing (exit point) */
    xing = (EdgeXing *) extarr_next(edgexings);
    xing->type  = XingTypeExit;
    xing->cidx  = cidx;
    xing->x     = extra[0];
    xing->y     = extra[1];
    xing->angle = atan2(extra[1], extra[0]);

    curr = extra;
  }

  scan(XPROJECT(prev[0]), YPROJECT(prev[1]),
       XPROJECT(curr[0]), YPROJECT(curr[1]));
}


void handle_xings()
{
  int       i;
  unsigned  nxings;
  EdgeXing *xings;
  EdgeXing *from;
  EdgeXing *to;

  xings  = (EdgeXing *) edgexings->body;
  nxings = edgexings->count;

  assert((nxings % 2) == 0);
  qsort(xings, nxings, sizeof(EdgeXing), edgexing_comp);

  if (xings[0].type == XingTypeExit)
  {
    for (i=0; i<nxings; i+=2)
    {
      from = &(xings[i]);
      to   = &(xings[i+1]);

      if ((from->type != XingTypeExit) ||
	  (to->type != XingTypeEntry))
      {
	fprintf(stderr, "%s:%d incorrect edgexing sequence ",
		__FILE__, __LINE__);
	fprintf(stderr, "(from->cidx %d, to->cidx %d)\n",
		from->cidx, to->cidx);
	exit(1);
      }
      scan_arc(from->x, from->y, from->angle, to->x, to->y, to->angle);
    }
  }
  else
  {
    from = &(xings[nxings-1]);
    to   = &(xings[0]);

    if ((from->type != XingTypeExit) ||
	(to->type != XingTypeEntry) ||
	(from->angle < to->angle))
    {
      fprintf(stderr, "%s:%d incorrect edgexing sequence ",
	      __FILE__, __LINE__);
      fprintf(stderr, "(from->cidx %d, to->cidx %d)\n",
	      from->cidx, to->cidx);
      exit(1);
    }
    scan_arc(from->x, from->y, from->angle, to->x, to->y, to->angle+2*M_PI);

    for (i=1; i<(nxings-1); i+=2)
    {
      from = &(xings[i]);
      to   = &(xings[i+1]);

      if ((from->type != XingTypeExit) ||
	  (to->type != XingTypeEntry))
      {
	fprintf(stderr, "%s:%d incorrect edgexing sequence ",
		__FILE__, __LINE__);
	fprintf(stderr, "(from->cidx %d, to->cidx %d)\n",
		from->cidx, to->cidx);
	exit(1);
      }
      scan_arc(from->x, from->y, from->angle, to->x, to->y, to->angle);
    }
  }

  edgexings->count = 0;
}


void scan_arc(x0, y0, a0, x1, y1, a1)
     double x0, y0, a0;
     double x1, y1, a1;
{
  int    i;
  int    lo, hi;
  double angle, step;
  double prev_x, prev_y;
  double curr_x, curr_y;

  assert(a0 < a1);

  step = M_PI / 50;
  lo   = ceil(a0 / step);
  hi   = floor(a1 / step);

  prev_x = XPROJECT(x0);
  prev_y = YPROJECT(y0);

  for (i=lo; i<=hi; i++)
  {
    angle  = i * step;
    curr_x = XPROJECT(cos(angle));
    curr_y = YPROJECT(sin(angle));
    scan(prev_x, prev_y, curr_x, curr_y);

    prev_x = curr_x;
    prev_y = curr_y;
  }

  curr_x = XPROJECT(x1);
  curr_y = YPROJECT(y1);
  scan(prev_x, prev_y, curr_x, curr_y);
}


void scan(x0, y0, x1, y1)
     double x0, y0;
     double x1, y1;
{
  int    i;
  int    lo_y, hi_y;
  double x_value;
  double x_delta;

  if (y0 < y1)
  {
    lo_y = ceil(y0 - 0.5);
    hi_y = floor(y1 - 0.5);

    if (hi_y == (y1 - 0.5))
      hi_y -= 1;
  }
  else
  {
    lo_y = ceil(y1 - 0.5);
    hi_y = floor(y0 - 0.5);

    if (hi_y == (y0 - 0.5))
      hi_y -= 1;
  }

  if (lo_y < 0)     lo_y = 0;
  if (hi_y >= hght) hi_y = hght-1;

  if (lo_y > hi_y)
    return;                     /* no scan lines crossed */

  if (lo_y < min_y) min_y = lo_y;
  if (hi_y > max_y) max_y = hi_y;

  x_value = x0 - (x0 - x1) * (y0 - (lo_y + 0.5)) / (y0 - y1);
  x_delta = (x0 - x1) / (y0 - y1);

  for (i=lo_y; i<=hi_y; i++)
  {
    *((double *) extarr_next(scanbuf[i])) = x_value;
    x_value += x_delta;
  }
}


void get_scanbits(val)
     int val;
{
  int      i, j;
  int      lo_x, hi_x;
  unsigned nvals;
  double  *vals;
  ScanBit *scanbit;

  for (i=min_y; i<=max_y; i++)
  {
    vals  = (double *) scanbuf[i]->body;
    nvals = scanbuf[i]->count;
    assert((nvals % 2) == 0);
    qsort(vals, nvals, sizeof(double), double_comp);

    for (j=0; j<nvals; j+=2)
    {
      lo_x = ceil(vals[j] - 0.5);
      hi_x = floor(vals[j+1] - 0.5);

      if (lo_x < 0)     lo_x = 0;
      if (hi_x >= wdth) hi_x = wdth-1;

      if (lo_x <= hi_x)
      {
        scanbit = (ScanBit *) extarr_next(scanbits);
        scanbit->y    = i;
        scanbit->lo_x = lo_x;
        scanbit->hi_x = hi_x;
        scanbit->val  = val;
      }
    }

    scanbuf[i]->count = 0;
  }
}
