/*
 * scan.c
 * kirk johnson
 * august 1993
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

#define MAP_DATA_SCALE (30000)

#define XingTypeEntry (0)
#define XingTypeExit  (1)

typedef struct
{
  int    type;
  int    cidx;
  double x, y;
  double angle;
} EdgeXing;

void    scan_map _P((void));
void    orth_scan_outline _P((void));
void    orth_scan_curves _P((void));
double *orth_extract_curve _P((int, short *));
void    orth_scan_along_curve _P((double *, double *, int));
void    orth_find_edge_xing _P((double *, double *, double *));
void    orth_handle_xings _P((void));
void    orth_scan_arc _P((double, double, double, double, double, double));
void    merc_scan_outline _P((void));
void    merc_scan_curves _P((void));
double *merc_extract_curve _P((int, short *));
void    merc_scan_along_curve _P((double *, double *, int));
double  merc_find_edge_xing _P((double *, double *));
void    merc_handle_xings _P((void));
void    merc_scan_edge _P((EdgeXing *, EdgeXing *));
void    cyl_scan_outline _P((void));
void    cyl_scan_curves _P((void));
double *cyl_extract_curve _P((int, short *));
void    cyl_scan_along_curve _P((double *, double *, int));
double  cyl_find_edge_xing _P((double *, double *));
void    cyl_handle_xings _P((void));
void    cyl_scan_edge _P((EdgeXing *, EdgeXing *));
void    xing_error _P((const char *, int, int, int, EdgeXing *));
void    scan _P((double, double, double, double));
void    get_scanbits _P((int));

static int double_comp _P((const void *, const void *));
static int scanbit_comp _P((const void *, const void *));
static int orth_edgexing_comp _P((const void *, const void *));
static int merc_edgexing_comp _P((const void *, const void *));
static int cyl_edgexing_comp _P((const void *, const void *));

ViewPosInfo view_pos_info;
ProjInfo    proj_info;

int     first_scan = 1;
ExtArr  scanbits;
ExtArr  edgexings;
int     min_y, max_y;
ExtArr *scanbuf;


static int double_comp(a, b)
     const void *a;
     const void *b;
{
  double val_a;
  double val_b;
  int    rslt;

  val_a = *((const double *) a);
  val_b = *((const double *) b);

  if (val_a < val_b)
    rslt = -1;
  else if (val_a > val_b)
    rslt = 1;
  else
    rslt = 0;

  return rslt;
}


static int scanbit_comp(a, b)
     const void *a;
     const void *b;
{
  return (((const ScanBit *) a)->y - ((const ScanBit *) b)->y);
}


static int orth_edgexing_comp(a, b)
     const void *a;
     const void *b;
{
  double angle_a;
  double angle_b;
  int    rslt;

  angle_a = ((const EdgeXing *) a)->angle;
  angle_b = ((const EdgeXing *) b)->angle;

  if (angle_a < angle_b)
    rslt = -1;
  else if (angle_a > angle_b)
    rslt = 1;
  else
    rslt = 0;

  return rslt;
}


static int merc_edgexing_comp(a, b)
     const void *a;
     const void *b;
{
  double val_a;
  double val_b;
  int    rslt;

  val_a = ((const EdgeXing *) a)->angle;
  val_b = ((const EdgeXing *) b)->angle;

  if (val_a < val_b)
  {
    rslt = -1;
  }
  else if (val_a > val_b)
  {
    rslt = 1;
  }
  else if (val_a == 0)
  {
    val_a = ((const EdgeXing *) a)->y;
    val_b = ((const EdgeXing *) b)->y;

    if (val_a < val_b)
      rslt = -1;
    else if (val_a > val_b)
      rslt = 1;
    else
      rslt = 0;
  }
  else if (val_a == 2)
  {
    val_a = ((const EdgeXing *) a)->y;
    val_b = ((const EdgeXing *) b)->y;

    if (val_a > val_b)
      rslt = -1;
    else if (val_a < val_b)
      rslt = 1;
    else
      rslt = 0;
  }
  else
  {
    /* keep lint happy */
    rslt = 0;
    assert(0);
  }

  return rslt;
}


static int cyl_edgexing_comp(a, b)
     const void *a;
     const void *b;
{
  double val_a;
  double val_b;
  int    rslt;

  val_a = ((const EdgeXing *) a)->angle;
  val_b = ((const EdgeXing *) b)->angle;

  if (val_a < val_b)
  {
    rslt = -1;
  }
  else if (val_a > val_b)
  {
    rslt = 1;
  }
  else if (val_a == 0)
  {
    val_a = ((const EdgeXing *) a)->y;
    val_b = ((const EdgeXing *) b)->y;

    if (val_a < val_b)
      rslt = -1;
    else if (val_a > val_b)
      rslt = 1;
    else
      rslt = 0;
  }
  else if (val_a == 2)
  {
    val_a = ((const EdgeXing *) a)->y;
    val_b = ((const EdgeXing *) b)->y;

    if (val_a > val_b)
      rslt = -1;
    else if (val_a < val_b)
      rslt = 1;
    else
      rslt = 0;
  }
  else
  {
    /* keep lint happy */
    rslt = 0;
    assert(0);
  }

  return rslt;
}


void scan_map()
{
  int          i;
  ViewPosInfo *vpi;
  ProjInfo    *pi;

  vpi = &view_pos_info;
  vpi->cos_lat = cos(view_lat * (M_PI/180));
  vpi->sin_lat = sin(view_lat * (M_PI/180));
  vpi->cos_lon = cos(view_lon * (M_PI/180));
  vpi->sin_lon = sin(view_lon * (M_PI/180));
  vpi->cos_rot = cos(view_rot * (M_PI/180));
  vpi->sin_rot = sin(view_rot * (M_PI/180));

  pi = &proj_info;
  if (proj_type == ProjTypeOrthographic)
  {
    pi->proj_scale = ((hght < wdth) ? hght : wdth) * (view_mag / 2) * 0.99;
  }
  else
  {
    /* proj_type is either ProjTypeMercator or ProjTypeCylindrical
     */
    pi->proj_scale = (view_mag * wdth) / (2 * M_PI);
  }

  pi->proj_xofs      = (double) wdth / 2 + shift_x;
  pi->proj_yofs      = (double) hght / 2 + shift_y;
  pi->inv_proj_scale = 1 / pi->proj_scale;

  /* the first time through, allocate scanbits and edgexings;
   * on subsequent passes, simply reset them.
   */
  if (first_scan)
  {
    scanbits  = extarr_alloc(sizeof(ScanBit));
    edgexings = extarr_alloc(sizeof(EdgeXing));
  }
  else
  {
    scanbits->count  = 0;
    edgexings->count = 0;
  }

  /* maybe only allocate these once and reset them on
   * subsequent passes (like scanbits and edgexings)?
   */
  scanbuf = (ExtArr *) malloc((unsigned) sizeof(ExtArr) * hght);
  assert(scanbuf != NULL);
  for (i=0; i<hght; i++)
    scanbuf[i] = extarr_alloc(sizeof(double));

  if (proj_type == ProjTypeOrthographic)
  {
    orth_scan_outline();
    orth_scan_curves();
  }
  else if (proj_type == ProjTypeMercator)
  {
    merc_scan_outline();
    merc_scan_curves();
  }
  else /* (proj_type == ProjTypeCylindrical) */
  {
    cyl_scan_outline();
    cyl_scan_curves();
  }

  for (i=0; i<hght; i++)
    extarr_free(scanbuf[i]);
  free(scanbuf);

  qsort(scanbits->body, scanbits->count, sizeof(ScanBit), scanbit_comp);

  first_scan = 0;
}


void orth_scan_outline()
{
  min_y = hght;
  max_y = -1;

  orth_scan_arc(1.0, 0.0, 0.0, 1.0, 0.0, (2*M_PI));

  get_scanbits(64);
}


void orth_scan_curves()
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

    pos   = orth_extract_curve(npts, raw);
    prev  = pos + (npts-1)*3;
    curr  = pos;
    min_y = hght;
    max_y = -1;

    for (i=0; i<npts; i++)
    {
      orth_scan_along_curve(prev, curr, cidx);
      prev  = curr;
      curr += 3;
    }

    free(pos);
    if (edgexings->count > 0)
      orth_handle_xings();
    if (min_y <= max_y)
      get_scanbits(val);

    cidx += 1;
    raw  += 3*npts;
  }
}


double *orth_extract_curve(npts, data)
     int    npts;
     short *data;
{
  int     i;
  int     x, y, z;
  double  scale;
  double *pos;
  double *rslt;

  rslt = (double *) malloc((unsigned) sizeof(double) * 3 * npts);
  assert(rslt != NULL);

  x     = 0;
  y     = 0;
  z     = 0;
  scale = 1.0 / MAP_DATA_SCALE;
  pos   = rslt;

  for (i=0; i<npts; i++)
  {
    x += data[0];
    y += data[1];
    z += data[2];

    pos[0] = x * scale;
    pos[1] = y * scale;
    pos[2] = z * scale;

    XFORM_ROTATE(pos, view_pos_info);

    data += 3;
    pos  += 3;
  }

  return rslt;
}


void orth_scan_along_curve(prev, curr, cidx)
     double *prev;
     double *curr;
     int     cidx;
{
  double    extra[3];
  EdgeXing *xing;

  if (prev[2] <= 0)             /* prev not visible */
  {
    if (curr[2] <= 0)
      return;                   /* neither point visible */

    orth_find_edge_xing(prev, curr, extra);

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
    orth_find_edge_xing(prev, curr, extra);

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


void orth_find_edge_xing(prev, curr, rslt)
     double *prev;
     double *curr;
     double *rslt;
{
  double tmp;
  double r0, r1;

  tmp = curr[2] / (curr[2] - prev[2]);
  r0 = curr[0] - tmp * (curr[0] - prev[0]);
  r1 = curr[1] - tmp * (curr[1] - prev[1]);

  tmp = sqrt((r0*r0) + (r1*r1));
  rslt[0] = r0 / tmp;
  rslt[1] = r1 / tmp;
  rslt[2] = 0;
}


void orth_handle_xings()
{
  int       i;
  int       nxings;
  EdgeXing *xings;
  EdgeXing *from;
  EdgeXing *to;

  xings  = (EdgeXing *) edgexings->body;
  nxings = edgexings->count;

  assert((nxings % 2) == 0);
  qsort(xings, (unsigned) nxings, sizeof(EdgeXing), orth_edgexing_comp);

  if (xings[0].type == XingTypeExit)
  {
    for (i=0; i<nxings; i+=2)
    {
      from = &(xings[i]);
      to   = &(xings[i+1]);

      if ((from->type != XingTypeExit) ||
          (to->type != XingTypeEntry))
        xing_error(__FILE__, __LINE__, i, nxings, xings);

      orth_scan_arc(from->x, from->y, from->angle,
                    to->x, to->y, to->angle);
    }
  }
  else
  {
    from = &(xings[nxings-1]);
    to   = &(xings[0]);

    if ((from->type != XingTypeExit) ||
        (to->type != XingTypeEntry) ||
        (from->angle < to->angle))
      xing_error(__FILE__, __LINE__, nxings-1, nxings, xings);

    orth_scan_arc(from->x, from->y, from->angle,
                  to->x, to->y, to->angle+(2*M_PI));

    for (i=1; i<(nxings-1); i+=2)
    {
      from = &(xings[i]);
      to   = &(xings[i+1]);

      if ((from->type != XingTypeExit) ||
          (to->type != XingTypeEntry))
        xing_error(__FILE__, __LINE__, i, nxings, xings);

      orth_scan_arc(from->x, from->y, from->angle,
                    to->x, to->y, to->angle);
    }
  }

  edgexings->count = 0;
}


void orth_scan_arc(x_0, y_0, a_0, x_1, y_1, a_1)
     double x_0, y_0, a_0;
     double x_1, y_1, a_1;
{
  int    i;
  int    lo, hi;
  double angle, step;
  double prev_x, prev_y;
  double curr_x, curr_y;
  double c_step, s_step;
  double arc_x, arc_y;
  double tmp;

  assert(a_0 < a_1);

  step = proj_info.inv_proj_scale * 10;
  if (step > 0.05) step = 0.05;
  lo = ceil(a_0 / step);
  hi = floor(a_1 / step);

  prev_x = XPROJECT(x_0);
  prev_y = YPROJECT(y_0);

  if (lo <= hi)
  {
    c_step = cos(step);
    s_step = sin(step);

    angle = lo * step;
    arc_x = cos(angle);
    arc_y = sin(angle);

    for (i=lo; i<=hi; i++)
    {
      curr_x = XPROJECT(arc_x);
      curr_y = YPROJECT(arc_y);
      scan(prev_x, prev_y, curr_x, curr_y);

      /* instead of repeatedly calling cos() and sin() to get the next
       * values for arc_x and arc_y, simply rotate the existing values
       */
      tmp   = (c_step * arc_x) - (s_step * arc_y);
      arc_y = (s_step * arc_x) + (c_step * arc_y);
      arc_x = tmp;

      prev_x = curr_x;
      prev_y = curr_y;
    }
  }

  curr_x = XPROJECT(x_1);
  curr_y = YPROJECT(y_1);
  scan(prev_x, prev_y, curr_x, curr_y);
}


void merc_scan_outline()
{
  double left, right;
  double top, bottom;

  min_y = hght;
  max_y = -1;

  left   = XPROJECT(-M_PI);
  right  = XPROJECT(M_PI);
  top    = YPROJECT(BigNumber);
  bottom = YPROJECT(-BigNumber);

  scan(right, top, left, top);
  scan(left, top, left, bottom);
  scan(left, bottom, right, bottom);
  scan(right, bottom, right, top);

  get_scanbits(64);
}


void merc_scan_curves()
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

    pos   = merc_extract_curve(npts, raw);
    prev  = pos + (npts-1)*5;
    curr  = pos;
    min_y = hght;
    max_y = -1;

    for (i=0; i<npts; i++)
    {
      merc_scan_along_curve(prev, curr, cidx);
      prev  = curr;
      curr += 5;
    }

    free(pos);
    if (edgexings->count > 0)
      merc_handle_xings();
    if (min_y <= max_y)
      get_scanbits(val);

    cidx += 1;
    raw  += 3*npts;
  }
}


double *merc_extract_curve(npts, data)
     int    npts;
     short *data;
{
  int     i;
  int     x, y, z;
  double  scale;
  double *pos;
  double *rslt;

  rslt = (double *) malloc((unsigned) sizeof(double) * 5 * npts);
  assert(rslt != NULL);

  x     = 0;
  y     = 0;
  z     = 0;
  scale = 1.0 / MAP_DATA_SCALE;
  pos   = rslt;

  for (i=0; i<npts; i++)
  {
    x += data[0];
    y += data[1];
    z += data[2];

    pos[0] = x * scale;
    pos[1] = y * scale;
    pos[2] = z * scale;

    XFORM_ROTATE(pos, view_pos_info);

    /* apply mercator projection
     */
    pos[3] = MERCATOR_X(pos[0], pos[2]);
    pos[4] = MERCATOR_Y(pos[1]);

    data += 3;
    pos  += 5;
  }

  return rslt;
}


void merc_scan_along_curve(prev, curr, cidx)
     double *prev;
     double *curr;
     int     cidx;
{
  double    px, py;
  double    cx, cy;
  double    dx;
  double    mx, my;
  EdgeXing *xing;

  px = prev[3];
  cx = curr[3];
  py = prev[4];
  cy = curr[4];
  dx = cx - px;

  if (dx > 0)
  {
    /* curr to the right of prev
     */

    if (dx > ((2*M_PI) - dx))
    {
      /* vertical edge crossing to the left of prev
       */

      /* find exit point (left edge) */
      mx = - M_PI;
      my = merc_find_edge_xing(prev, curr);

      /* scan from prev to exit point */
      scan(XPROJECT(px), YPROJECT(py), XPROJECT(mx), YPROJECT(my));

      /* (mx, my) is an edge crossing (exit point) */
      xing = (EdgeXing *) extarr_next(edgexings);
      xing->type  = XingTypeExit;
      xing->cidx  = cidx;
      xing->x     = mx;
      xing->y     = my;
      xing->angle = 2; /* left edge */

      /* scan from entry point (right edge) to curr */
      mx = M_PI;
      scan(XPROJECT(mx), YPROJECT(my), XPROJECT(cx), YPROJECT(cy));

      /* (mx, my) is an edge crossing (entry point) */
      xing = (EdgeXing *) extarr_next(edgexings);
      xing->type  = XingTypeEntry;
      xing->cidx  = cidx;
      xing->x     = mx;
      xing->y     = my;
      xing->angle = 0; /* right edge */
    }
    else
    {
      /* no vertical edge crossing
       */
      scan(XPROJECT(px), YPROJECT(py), XPROJECT(cx), YPROJECT(cy));
    }
  }
  else
  {
    /* curr to the left of prev
     */
    dx = - dx;

    if (dx > ((2*M_PI) - dx))
    {
      /* vertical edge crossing to the right of prev
       */

      /* find exit point (right edge) */
      mx = M_PI;
      my = merc_find_edge_xing(prev, curr);

      /* scan from prev to exit point */
      scan(XPROJECT(px), YPROJECT(py), XPROJECT(mx), YPROJECT(my));

      /* (mx, my) is an edge crossing (exit point) */
      xing = (EdgeXing *) extarr_next(edgexings);
      xing->type  = XingTypeExit;
      xing->cidx  = cidx;
      xing->x     = mx;
      xing->y     = my;
      xing->angle = 0; /* right edge */

      /* scan from entry point (left edge) to curr */
      mx = - M_PI;
      scan(XPROJECT(mx), YPROJECT(my), XPROJECT(cx), YPROJECT(cy));

      /* (mx, my) is an edge crossing (entry point) */
      xing = (EdgeXing *) extarr_next(edgexings);
      xing->type  = XingTypeEntry;
      xing->cidx  = cidx;
      xing->x     = mx;
      xing->y     = my;
      xing->angle = 2; /* left edge */
    }
    else
    {
      /* no vertical edge crossing
       */
      scan(XPROJECT(px), YPROJECT(py), XPROJECT(cx), YPROJECT(cy));
    }
  }
}


double merc_find_edge_xing(prev, curr)
     double *prev;
     double *curr;
{
  double ratio;
  double scale;
  double z1, z2;
  double rslt;

  if (curr[0] != 0)
  {
    ratio = (prev[0] / curr[0]);
    z1 = prev[1] - (ratio * curr[1]);
    z2 = prev[2] - (ratio * curr[2]);
  }
  else
  {
    z1 = curr[1];
    z2 = curr[2];
  }

  scale = ((z2 > 0) ? -1 : 1) / sqrt((z1*z1) + (z2*z2));
  z1 *= scale;

  rslt = MERCATOR_Y(z1);

  return rslt;
}


void merc_handle_xings()
{
  int       i;
  int       nxings;
  EdgeXing *xings;
  EdgeXing *from;
  EdgeXing *to;

  xings  = (EdgeXing *) edgexings->body;
  nxings = edgexings->count;

  assert((nxings % 2) == 0);
  qsort(xings, (unsigned) nxings, sizeof(EdgeXing), merc_edgexing_comp);

  if (xings[0].type == XingTypeExit)
  {
    for (i=0; i<nxings; i+=2)
    {
      from = &(xings[i]);
      to   = &(xings[i+1]);

      if ((from->type != XingTypeExit) ||
          (to->type != XingTypeEntry))
        xing_error(__FILE__, __LINE__, i, nxings, xings);

      merc_scan_edge(from, to);
    }
  }
  else
  {
    from = &(xings[nxings-1]);
    to   = &(xings[0]);

    if ((from->type != XingTypeExit) ||
        (to->type != XingTypeEntry) ||
        (from->angle < to->angle))
      xing_error(__FILE__, __LINE__, nxings-1, nxings, xings);

    merc_scan_edge(from, to);

    for (i=1; i<(nxings-1); i+=2)
    {
      from = &(xings[i]);
      to   = &(xings[i+1]);

      if ((from->type != XingTypeExit) ||
          (to->type != XingTypeEntry))
        xing_error(__FILE__, __LINE__, i, nxings, xings);

      merc_scan_edge(from, to);
    }
  }

  edgexings->count = 0;
}


void merc_scan_edge(from, to)
     EdgeXing *from;
     EdgeXing *to;
{
  int    s0, s1, s_new;
  double x_0, x_1, x_new;
  double y_0, y_1, y_new;

  s0 = from->angle;
  x_0 = XPROJECT(from->x);
  y_0 = YPROJECT(from->y);

  s1 = to->angle;
  x_1 = XPROJECT(to->x);
  y_1 = YPROJECT(to->y);

  while (s0 != s1)
  {
    switch (s0)
    {
    case 0:
      x_new = XPROJECT(M_PI);
      y_new = YPROJECT(BigNumber);
      s_new = 1;
      break;

    case 1:
      x_new = XPROJECT(-M_PI);
      y_new = YPROJECT(BigNumber);
      s_new = 2;
      break;

    case 2:
      x_new = XPROJECT(-M_PI);
      y_new = YPROJECT(-BigNumber);
      s_new = 3;
      break;

    case 3:
      x_new = XPROJECT(M_PI);
      y_new = YPROJECT(-BigNumber);
      s_new = 0;
      break;

    default:
      /* keep lint happy */
      x_new = 0;
      y_new = 0;
      s_new = 0;
      assert(0);
    }

    scan(x_0, y_0, x_new, y_new);
    x_0 = x_new;
    y_0 = y_new;
    s0 = s_new;
  }

  scan(x_0, y_0, x_1, y_1);
}


void cyl_scan_outline()
{
  double left, right;
  double top, bottom;

  min_y = hght;
  max_y = -1;

  left   = XPROJECT(-M_PI);
  right  = XPROJECT(M_PI);
  top    = YPROJECT(BigNumber);
  bottom = YPROJECT(-BigNumber);

  scan(right, top, left, top);
  scan(left, top, left, bottom);
  scan(left, bottom, right, bottom);
  scan(right, bottom, right, top);

  get_scanbits(64);
}


void cyl_scan_curves()
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

    pos   = cyl_extract_curve(npts, raw);
    prev  = pos + (npts-1)*5;
    curr  = pos;
    min_y = hght;
    max_y = -1;

    for (i=0; i<npts; i++)
    {
      cyl_scan_along_curve(prev, curr, cidx);
      prev  = curr;
      curr += 5;
    }

    free(pos);
    if (edgexings->count > 0)
      cyl_handle_xings();
    if (min_y <= max_y)
      get_scanbits(val);

    cidx += 1;
    raw  += 3*npts;
  }
}


double *cyl_extract_curve(npts, data)
     int    npts;
     short *data;
{
  int     i;
  int     x, y, z;
  double  scale;
  double *pos;
  double *rslt;

  rslt = (double *) malloc((unsigned) sizeof(double) * 5 * npts);
  assert(rslt != NULL);

  x     = 0;
  y     = 0;
  z     = 0;
  scale = 1.0 / MAP_DATA_SCALE;
  pos   = rslt;

  for (i=0; i<npts; i++)
  {
    x += data[0];
    y += data[1];
    z += data[2];

    pos[0] = x * scale;
    pos[1] = y * scale;
    pos[2] = z * scale;

    XFORM_ROTATE(pos, view_pos_info);

    /* apply cylindrical projection
     */
    pos[3] = CYLINDRICAL_X(pos[0], pos[2]);
    pos[4] = CYLINDRICAL_Y(pos[1]);

    data += 3;
    pos  += 5;
  }

  return rslt;
}


void cyl_scan_along_curve(prev, curr, cidx)
     double *prev;
     double *curr;
     int     cidx;
{
  double    px, py;
  double    cx, cy;
  double    dx;
  double    mx, my;
  EdgeXing *xing;

  px = prev[3];
  cx = curr[3];
  py = prev[4];
  cy = curr[4];
  dx = cx - px;

  if (dx > 0)
  {
    /* curr to the right of prev
     */

    if (dx > ((2*M_PI) - dx))
    {
      /* vertical edge crossing to the left of prev
       */

      /* find exit point (left edge) */
      mx = - M_PI;
      my = cyl_find_edge_xing(prev, curr);

      /* scan from prev to exit point */
      scan(XPROJECT(px), YPROJECT(py), XPROJECT(mx), YPROJECT(my));

      /* (mx, my) is an edge crossing (exit point) */
      xing = (EdgeXing *) extarr_next(edgexings);
      xing->type  = XingTypeExit;
      xing->cidx  = cidx;
      xing->x     = mx;
      xing->y     = my;
      xing->angle = 2; /* left edge */

      /* scan from entry point (right edge) to curr */
      mx = M_PI;
      scan(XPROJECT(mx), YPROJECT(my), XPROJECT(cx), YPROJECT(cy));

      /* (mx, my) is an edge crossing (entry point) */
      xing = (EdgeXing *) extarr_next(edgexings);
      xing->type  = XingTypeEntry;
      xing->cidx  = cidx;
      xing->x     = mx;
      xing->y     = my;
      xing->angle = 0; /* right edge */
    }
    else
    {
      /* no vertical edge crossing
       */
      scan(XPROJECT(px), YPROJECT(py), XPROJECT(cx), YPROJECT(cy));
    }
  }
  else
  {
    /* curr to the left of prev
     */
    dx = - dx;

    if (dx > ((2*M_PI) - dx))
    {
      /* vertical edge crossing to the right of prev
       */

      /* find exit point (right edge) */
      mx = M_PI;
      my = cyl_find_edge_xing(prev, curr);

      /* scan from prev to exit point */
      scan(XPROJECT(px), YPROJECT(py), XPROJECT(mx), YPROJECT(my));

      /* (mx, my) is an edge crossing (exit point) */
      xing = (EdgeXing *) extarr_next(edgexings);
      xing->type  = XingTypeExit;
      xing->cidx  = cidx;
      xing->x     = mx;
      xing->y     = my;
      xing->angle = 0; /* right edge */

      /* scan from entry point (left edge) to curr */
      mx = - M_PI;
      scan(XPROJECT(mx), YPROJECT(my), XPROJECT(cx), YPROJECT(cy));

      /* (mx, my) is an edge crossing (entry point) */
      xing = (EdgeXing *) extarr_next(edgexings);
      xing->type  = XingTypeEntry;
      xing->cidx  = cidx;
      xing->x     = mx;
      xing->y     = my;
      xing->angle = 2; /* left edge */
    }
    else
    {
      /* no vertical edge crossing
       */
      scan(XPROJECT(px), YPROJECT(py), XPROJECT(cx), YPROJECT(cy));
    }
  }
}


double cyl_find_edge_xing(prev, curr)
     double *prev;
     double *curr;
{
  double ratio;
  double scale;
  double z1, z2;
  double rslt;

  if (curr[0] != 0)
  {
    ratio = (prev[0] / curr[0]);
    z1 = prev[1] - (ratio * curr[1]);
    z2 = prev[2] - (ratio * curr[2]);
  }
  else
  {
    z1 = curr[1];
    z2 = curr[2];
  }

  scale = ((z2 > 0) ? -1 : 1) / sqrt((z1*z1) + (z2*z2));
  z1 *= scale;

  rslt = CYLINDRICAL_Y(z1);

  return rslt;
}


void cyl_handle_xings()
{
  int       i;
  int       nxings;
  EdgeXing *xings;
  EdgeXing *from;
  EdgeXing *to;

  xings  = (EdgeXing *) edgexings->body;
  nxings = edgexings->count;

  assert((nxings % 2) == 0);
  qsort(xings, (unsigned) nxings, sizeof(EdgeXing), cyl_edgexing_comp);

  if (xings[0].type == XingTypeExit)
  {
    for (i=0; i<nxings; i+=2)
    {
      from = &(xings[i]);
      to   = &(xings[i+1]);

      if ((from->type != XingTypeExit) ||
          (to->type != XingTypeEntry))
        xing_error(__FILE__, __LINE__, i, nxings, xings);

      cyl_scan_edge(from, to);
    }
  }
  else
  {
    from = &(xings[nxings-1]);
    to   = &(xings[0]);

    if ((from->type != XingTypeExit) ||
        (to->type != XingTypeEntry) ||
        (from->angle < to->angle))
      xing_error(__FILE__, __LINE__, nxings-1, nxings, xings);

    cyl_scan_edge(from, to);

    for (i=1; i<(nxings-1); i+=2)
    {
      from = &(xings[i]);
      to   = &(xings[i+1]);

      if ((from->type != XingTypeExit) ||
          (to->type != XingTypeEntry))
        xing_error(__FILE__, __LINE__, i, nxings, xings);

      cyl_scan_edge(from, to);
    }
  }

  edgexings->count = 0;
}


void cyl_scan_edge(from, to)
     EdgeXing *from;
     EdgeXing *to;
{
  int    s0, s1, s_new;
  double x_0, x_1, x_new;
  double y_0, y_1, y_new;

  s0 = from->angle;
  x_0 = XPROJECT(from->x);
  y_0 = YPROJECT(from->y);

  s1 = to->angle;
  x_1 = XPROJECT(to->x);
  y_1 = YPROJECT(to->y);

  while (s0 != s1)
  {
    switch (s0)
    {
    case 0:
      x_new = XPROJECT(M_PI);
      y_new = YPROJECT(BigNumber);
      s_new = 1;
      break;

    case 1:
      x_new = XPROJECT(-M_PI);
      y_new = YPROJECT(BigNumber);
      s_new = 2;
      break;

    case 2:
      x_new = XPROJECT(-M_PI);
      y_new = YPROJECT(-BigNumber);
      s_new = 3;
      break;

    case 3:
      x_new = XPROJECT(M_PI);
      y_new = YPROJECT(-BigNumber);
      s_new = 0;
      break;

    default:
      /* keep lint happy */
      x_new = 0;
      y_new = 0;
      s_new = 0;
      assert(0);
    }

    scan(x_0, y_0, x_new, y_new);
    x_0 = x_new;
    y_0 = y_new;
    s0 = s_new;
  }

  scan(x_0, y_0, x_1, y_1);
}


void xing_error(file, line, idx, nxings, xings)
     const char *file;
     int         line;
     int         idx;
     int         nxings;
     EdgeXing   *xings;
{
  fflush(stdout);
  fprintf(stderr, "xearth %s: incorrect edgexing sequence (%s:%d)\n",
          VersionString, file, line);
  fprintf(stderr, " (cidx %d) (view_lat %.16f) (view_lon %.16f)\n",
          xings[idx].cidx, view_lat, view_lon);
  fprintf(stderr, "\n");
  exit(1);
}


void scan(x_0, y_0, x_1, y_1)
     double x_0, y_0;
     double x_1, y_1;
{
  int    i;
  int    lo_y, hi_y;
  double x_value;
  double x_delta;

  if (y_0 < y_1)
  {
    lo_y = ceil(y_0 - 0.5);
    hi_y = floor(y_1 - 0.5);

    if (hi_y == (y_1 - 0.5))
      hi_y -= 1;
  }
  else
  {
    lo_y = ceil(y_1 - 0.5);
    hi_y = floor(y_0 - 0.5);

    if (hi_y == (y_0 - 0.5))
      hi_y -= 1;
  }

  if (lo_y < 0)     lo_y = 0;
  if (hi_y >= hght) hi_y = hght-1;

  if (lo_y > hi_y)
    return;                     /* no scan lines crossed */

  if (lo_y < min_y) min_y = lo_y;
  if (hi_y > max_y) max_y = hi_y;

  x_delta = (x_1 - x_0) / (y_1 - y_0);
  x_value = x_0 + x_delta * ((lo_y + 0.5) - y_0);

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
