/*
 * x11.c
 * kirk johnson
 * july 1993
 *
 * RCS $Id: x11.c,v 1.15 1994/06/01 18:03:35 tuna Exp $
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
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include "kljcpyrt.h"

#define MONO_1  (0)
#define MONO_8  (1)
#define COLOR_8 (2)

#define RETAIN_PROP_NAME "_XSETROOT_ID"

typedef struct
{
  float lat;			/* marker latitude  */
  float lon;			/* marker longitude */
  char *label;			/* marker label     */
} MarkerInfo;

static XFontStruct *load_x_font();
static void         get_viewing_position();
static void         get_sun_position();
static void         get_size();
static void         get_shift();
static void         draw_label();
static void         mark_location();
static void         draw_outlined_string();
static Window       GetVRoot();
static void         updateProperty();
static void         preserveResource();
static void         freePrevious();
static int          xkill_handler();

static MarkerInfo marker_info[] =
{
{  61.2, -149.9, "Anchorage" },
{  38.0,   23.7, "Athens" },
{  39.9,  116.5, "Beijing" },
{  52.5,   13.4, "Berlin" },
{  19.0,   72.8, "Bombay" },
{  42.4,  -71.1, "Boston" },
{  50.8,    4.4, "Brussels" },
{ -34.6,  -58.1, "Buenos Aires" },
{  31.3,   31.1, "Cairo" },
{ -33.9,   18.4, "Cape Town" },
{  41.9,  -87.7, "Chicago" },
{  32.8,  -96.8, "Dallas" },
{  39.7, -105.0, "Denver" },
{  24.5,   54.4, "Dubai" },
{  60.2,   25.0, "Helsinki" },
{  21.3, -157.9, "Honolulu" },
{  -6.2,  106.8, "Jakarta" },
{   3.1,  101.7, "Kuala Lumpur" },
{ -12.1,  -77.0, "Lima" },
{  51.5,   -0.1, "London" },
{  55.8,   37.7, "Moscow" },
{  -1.3,   36.8, "Nairobi" },
{  59.9,   10.7, "Oslo" },
{  48.9,    2.4, "Paris" },
{  64.2,  -22.0, "Reykjavik" },
{ -22.9,  -43.2, "Rio de Janeiro" },
{  41.5,   12.4, "Rome" },
{  37.8, -122.4, "San Francisco" },
{   9.8,  -83.4, "San Jose" },
{  42.7,   23.3, "Sofia" },
{  59.6,   18.1, "Stockholm" },
{ -33.9,  151.2, "Sydney" },
{  32.1,   34.8, "Tel Aviv" },
{  35.7,  139.7, "Tokyo" },
{  43.7,  -79.4, "Toronto" },
{  49.3, -123.1, "Vancouver" },
{  48.2,   16.4, "Vienna" },
{ -41.3,  174.8, "Wellington" },
{     0,      0, NULL },
};

static u_char *dith;
static u_char *xbuf;
static int     idx;
static XImage *xim;
static Pixmap  work_pix;
static Pixmap  disp_pix;
static int   (*orig_error_handler)();

#ifdef DEBUG
static int frame = 0;
#endif /* DEBUG */

char        *progclass;
Widget       app_shell;
XtAppContext app_context;
XrmDatabase  db;

Display     *dsply;             /* display connection  */
int          scrn;              /* screen number       */
Window       root;              /* root window         */
Window       vroot;		/* virtual root window */
Colormap     cmap;              /* default colormap    */
Visual      *visl;              /* default visual      */
int          dpth;              /* default depth       */
Pixel        white;             /* white pixel         */
Pixel        black;             /* black pixel         */
Pixel        hlight;		/* highlight pixel     */
GC           gc;                /* graphics context    */
Pixel       *pels;              /* allocated colors    */
char        *font_name;		/* text font name      */
XFontStruct *font;              /* basic text font     */

int          mono;		/* render in mono?     */
int          x_type;		/* type of rendering   */


static char *defaults[] =
{
  "*pos:      sunrel 0 0",
  "*shift:    0 0",
  "*mag:      0.99",
  "*shade:    on",
  "*label:    off",
  "*markers:  on",
  "*wait:     300",
  "*timewarp: 1",
  "*day:      100",
  "*night:    10",
  "*twopix:   on",
  "*ncolors:  64",
  "*fork:     off",
  "*nice:     0",
  "*stars:    on",
  "*starfreq: 0.002",
  "*grid:     off",
  "*grid1:    6",
  "*grid2:    15",
  "*gamma:    1.0",
  "*font:     variable",
  NULL
};

static XrmOptionDescRec options[] =
{
{ "-pos",       ".pos",      XrmoptionSepArg, 0     },
{ "-mag",       ".mag",      XrmoptionSepArg, 0     },
{ "-shade",     ".shade",    XrmoptionNoArg,  "on"  },
{ "-noshade",   ".shade",    XrmoptionNoArg,  "off" },
{ "-sunpos",    ".sunpos",   XrmoptionSepArg, 0     },
{ "-size",      ".size",     XrmoptionSepArg, 0     },
{ "-shift",     ".shift",    XrmoptionSepArg, 0     },
{ "-label",     ".label",    XrmoptionNoArg,  "on"  },
{ "-nolabel",   ".label",    XrmoptionNoArg,  "off" },
{ "-markers",   ".markers",  XrmoptionNoArg,  "on"  },
{ "-nomarkers", ".markers",  XrmoptionNoArg,  "off" },
{ "-wait",      ".wait",     XrmoptionSepArg, 0     },
{ "-timewarp",  ".timewarp", XrmoptionSepArg, 0     },
{ "-day",       ".day",      XrmoptionSepArg, 0     },
{ "-night",     ".night",    XrmoptionSepArg, 0     },
{ "-onepix",    ".twopix",   XrmoptionNoArg,  "off" },
{ "-twopix",    ".twopix",   XrmoptionNoArg,  "on"  },
{ "-ncolors",   ".ncolors",  XrmoptionSepArg, 0     },
{ "-fork",      ".fork",     XrmoptionNoArg,  "on"  },
{ "-nofork",    ".fork",     XrmoptionNoArg,  "off" },
{ "-nice",      ".nice",     XrmoptionSepArg, 0     },
{ "-version",   ".version",  XrmoptionNoArg,  "on"  },
{ "-stars",     ".stars",    XrmoptionNoArg,  "on"  },
{ "-nostars",   ".stars",    XrmoptionNoArg,  "off" },
{ "-starfreq",  ".starfreq", XrmoptionSepArg, 0     },
{ "-grid",      ".grid",     XrmoptionNoArg,  "on"  },
{ "-nogrid",    ".grid",     XrmoptionNoArg,  "off" },
{ "-grid1",     ".grid1",    XrmoptionSepArg, 0     },
{ "-grid2",     ".grid2",    XrmoptionSepArg, 0     },
{ "-time",      ".time",     XrmoptionSepArg, 0     },
{ "-gamma",     ".gamma",    XrmoptionSepArg, 0     },
{ "-font",      ".font",     XrmoptionSepArg, 0     },
{ "-mono",      ".mono",     XrmoptionNoArg,  "on"  },
{ "-nomono",    ".mono",     XrmoptionNoArg,  "off" },
};


void command_line_x(argc, argv)
     int   argc;
     char *argv[];
{
  int     i;
  XColor  xc, junk;
  u_char *tmp;
  double  inv_xgamma;

  progname  = argv[0];
  progclass = "XEarth";
  app_shell = XtAppInitialize(&app_context, progclass,
			      options, XtNumber(options),
			      &argc, argv, defaults, 0, 0);
  if (argc > 1) usage(NULL);

  dsply = XtDisplay(app_shell);
  scrn  = DefaultScreen(dsply);
  db    = XtDatabase(dsply);

  XtGetApplicationNameAndClass(dsply, &progname, &progclass);

  root  = RootWindow(dsply, scrn);
  vroot = GetVRoot(dsply);
  cmap  = DefaultColormap(dsply, scrn);
  visl  = DefaultVisual(dsply, scrn);
  dpth  = DefaultDepth(dsply, scrn);

  if (get_boolean_resource("version", "Version"))
    version_info();

  wdth = DisplayWidth(dsply, scrn);
  hght = DisplayHeight(dsply, scrn);

  /* process complex resources
   */
  get_viewing_position();
  get_sun_position();
  get_size();
  get_shift();

  /* process simple resources
   */
  view_mag        = get_float_resource("mag", "Mag");
  do_shade        = get_boolean_resource("shade", "Shade");
  do_label        = get_boolean_resource("label", "Label");
  do_markers      = get_boolean_resource("markers", "Markers");
  wait_time       = get_integer_resource("wait", "Wait");
  time_warp       = get_float_resource("timewarp", "Timewarp");
  day             = get_integer_resource("day", "Day");
  night           = get_integer_resource("night", "Night");
  use_two_pixmaps = get_boolean_resource("twopix", "Twopix");
  num_colors      = get_integer_resource("ncolors", "Ncolors");
  do_fork         = get_boolean_resource("fork", "Fork");
  priority        = get_integer_resource("nice", "Nice");
  do_stars        = get_boolean_resource("stars", "Stars");
  star_freq       = get_float_resource("starfreq", "Starfreq");
  do_grid         = get_boolean_resource("grid", "Grid");
  grid_big        = get_integer_resource("grid1", "Grid1");
  grid_small      = get_integer_resource("grid2", "Grid2");
  fixed_time      = get_integer_resource("time", "Time");
  xgamma          = get_float_resource("gamma", "Gamma");
  font_name       = get_string_resource("font", "Font");
  mono            = get_boolean_resource("mono", "Mono");

  /* things to add:
   *
   *  differentiate between label and marker fonts
   *  allow user to specify label, marker, star, grid colors
   *  allow user to specify # of markers and positions
   */

  /* various sanity checks on simple resources
   */
  if (view_mag <= 0)
    fatal("viewing magnification must be positive");
  if (wait_time < 0)
    fatal("arg to -wait must be non-negative");
  if (time_warp <= 0)
    fatal("arg to -timewarp must be positive");
  if (num_colors < 3)
    fatal("arg to -ncolors must be >= 3");
  if ((star_freq < 0) || (star_freq > 1))
    fatal("arg to -starfreq must be between 0 and 1");
  if (grid_big <= 0)
    fatal("arg to -grid1 must be positive");
  if (grid_small <= 0)
    fatal("arg to -grid2 must be positive");
  if ((day > 100) || (day < 0))
    fatal("arg to -day must be between 0 and 100");
  if ((night > 100) || (night < 0))
    fatal("arg to -night must be between 0 and 100");
  if (xgamma <= 0)
    fatal("arg to -gamma must be positive");

  white = WhitePixel(dsply, scrn);
  black = BlackPixel(dsply, scrn);
  gc = XCreateGC(dsply, root, 0, NULL);
  XSetState(dsply, gc, white, black, GXcopy, AllPlanes);
  hlight = white;

  /* i _think_ the eight-bit code will work on any eight-bit display,
   * but i'm not absolutely certain; ditto with the one-bit code.
   * other depths definitely won't work until i add code to support
   * them.
   */

  switch (dpth)
  {
  case 1:
    x_type = MONO_1;
    break;

  case 8:
    x_type = mono ? MONO_8 : COLOR_8;
    break;

  default:
    fatal("this verison of xearth only supports 1- and 8-bit displays");
  }

  switch (x_type)
  {
  case MONO_1:
  case MONO_8:
    mono_dither_setup();
    break;

  case COLOR_8:
    if (XAllocNamedColor(dsply, cmap, "red", &xc, &junk) != 0)
      hlight = xc.pixel;

    dither_setup(num_colors);
    pels = (Pixel *) malloc((unsigned) sizeof(Pixel) * dither_ncolors);
    assert(pels != NULL);

    tmp = dither_colormap;
    inv_xgamma = 1.0 / xgamma;
    for (i=0; i<dither_ncolors; i++)
    {
      xc.red   = ((1<<16)-1) * pow(((double) tmp[0] / 255), inv_xgamma);
      xc.green = ((1<<16)-1) * pow(((double) tmp[1] / 255), inv_xgamma);
      xc.blue  = ((1<<16)-1) * pow(((double) tmp[2] / 255), inv_xgamma);

      if (XAllocColor(dsply, cmap, &xc) == 0)
	fatal("unable to allocate enough colors");
      pels[i] = xc.pixel;

      tmp += 3;
    }
    break;

  default:
    assert(0);
  }

  work_pix = XCreatePixmap(dsply, root, (unsigned) wdth,
			   (unsigned) hght, (unsigned) dpth);
  if (use_two_pixmaps)
    disp_pix = XCreatePixmap(dsply, root, (unsigned) wdth,
			     (unsigned) hght, (unsigned) dpth);

  font = load_x_font(dsply, font_name);

  /* try to free any resources retained by any previous clients that
   * scribbled in the root window (also deletes the _XSETROOT_ID
   * property from the root window, if it was there)
   */
  freePrevious(dsply, vroot);

  /* 18 may 1994
   *
   * setting the _XSETROOT_ID property is dangerous if xearth might be
   * killed by other means (e.g., from the shell), because some other
   * client might allocate an resource with the same resource ID that
   * xearth had stored in the _XSETROOT_ID property, so subsequent
   * attempts to free any resources retained by a client that had
   * scribbled on the root window via XKillClient() may end up killing
   * the wrong client.
   *
   * this possibility could be eliminated by setting the closedown
   * mode for the display connection to RetainPermanent, but this
   * seemed to be causing core dumps in an R5pl26 server -- i submited
   * a bug report to the X consortium about this. i _think_ the server
   * core dumps were related to the fact that xearth can sleep for a
   * _long_ time between protocol requests, perhaps longer than it
   * takes for one server to die (e.g., when somebody logs out) and a
   * new server to be restarted, and somehow exercising the display
   * connection from the old server was causing the new one to crash?
   *
   * possible fixes:
   *
   * - replace the big sleep() with a loop that interleaves sleep(1)
   *   and, say, calls to XNoOp() to test the display connection;
   *   presumably one second is short enough to avoid the possibility
   *   of one server dying and another restarting before a call to
   *   XNoOp() catches the fact that the connection to the old server
   *   died.
   *
   * - use RetainTemporary mode instead of RetainPermanent? need to
   *   check the X documentation and figure out exactly what this
   *   would mean.
   *
   * it would be nice to install the _XSETROOT_ID property so xearth
   * interoperates gracefully with other things that try to scribble
   * on the root window (e.g., xsetroot, xloadimage, xv), but until i
   * figure out a fix to the problems described above, probably best
   * not to bother.
   */
  /* preserveResource(dsply, vroot); */
}


static XFontStruct *load_x_font(dpy, font_name)
     Display *dpy;
     char    *font_name;
{
  XFontStruct *rslt;

  rslt = XLoadQueryFont(dpy, font_name);
  if (rslt == NULL)
  {
    rslt = XQueryFont(dpy, XGContextFromGC(gc));
    if (rslt == NULL)
      fatal("completely unable to load fonts");
    else
      warning("unable to load font, reverting to default");
  }
  else
  {
    XSetFont(dpy, gc, rslt->fid);
  }

  return rslt;
}


/* fetch and decode 'pos' resource (viewing position specifier)
 */
static void get_viewing_position()
{
  char *res;

  res = get_string_resource("pos", "Pos");
  if (res != NULL)
  {
    decode_viewing_pos(res);
    free(res);
  }
}


/* fetch and decode 'sunpos' resource (sun position specifier)
 */
static void get_sun_position()
{
  char *res;

  res = get_string_resource("sunpos", "Sunpos");
  if (res != NULL)
  {
    decode_sun_pos(res);
    free(res);
  }
}


/* fetch and decode 'size' resource (size specifier)
 */
static void get_size()
{
  char *res;

  res = get_string_resource("size", "Size");
  if (res != NULL)
  {
    decode_size(res);
    free(res);
  }
}


/* fetch and decode 'shift' resource (shift specifier)
 */
static void get_shift()
{
  char *res;

  res = get_string_resource("shift", "Shift");
  if (res != NULL)
  {
    decode_shift(res);
    free(res);
  }
}


void x11_setup()
{
  int dith_size;
  int xbuf_size;

  switch (x_type)
  {
  case MONO_1:
    dith_size = wdth + 7;
    xbuf_size = dith_size >> 3;
    break;

  case MONO_8:
  case COLOR_8:
    dith_size = wdth;
    xbuf_size = dith_size;
    break;

  default:
    /* keep lint happy */
    dith_size = 0;
    xbuf_size = 0;
    assert(0);
  }

  dith = (u_char *) malloc((unsigned) dith_size);
  assert(dith != NULL);

  xbuf = (u_char *) malloc((unsigned) xbuf_size);
  assert(xbuf != NULL);

  xim = XCreateImage(dsply, visl, (unsigned) dpth, ZPixmap, 0,
		     (char *) xbuf, (unsigned) wdth, 1, 8,
		     xbuf_size);

  if (x_type == MONO_1)
  {
    /* force MSBFirst bitmap_bit_order and byte_order
     */
    xim->bitmap_bit_order = MSBFirst;
    xim->byte_order       = MSBFirst;
  }

  idx = 0;
}


void x11_row(row)
     u_char *row;
{
  int     i, i_lim;
  int     val;
  u_char *tmp;

  switch (x_type)
  {
  case MONO_1:
  case MONO_8:
  {
    /* convert row to gray scale
     */
    tmp   = dith;
    i_lim = wdth;
    for (i=0; i<i_lim; i++)
    {
      tmp[i] = ((2 * row[0]) + (5 * row[1]) + row[2]) >> 3;
      row += 3;
    }

    /* dither to 0s (black) and 1s (white)
     */
    mono_dither_row(tmp);

    if (x_type == MONO_1)
    {
      /* pack pixels into bytes (assuming the XImage uses MSBFirst
       * bitmap_bit_order and MSBFirst byte_order)
       */
      for (i=0; i<i_lim; i+=8)
      {
	val = ((tmp[0] << 7) | (tmp[1] << 6) | (tmp[2] << 5) |
	       (tmp[3] << 4) | (tmp[4] << 3) | (tmp[5] << 2) |
	       (tmp[6] << 1) | (tmp[7] << 0));

	/* if white is pixel 0, need to toggle the bits
	 */
	xbuf[i>>3] = (white == 0) ? (~ val) : val;
	tmp += 8;
      }
    }
    else
    {
      /* convert to black and white pixels
       */
      for (i=0; i<i_lim; i++)
	xbuf[i] = tmp[i] ? white : black;
    }
    break;
  }

  case COLOR_8:
  {
    /* dither to colormap
     */
    dither_row(row, dith);

    i_lim = wdth;
    for (i=0; i<i_lim; i++)
      xbuf[i] = pels[dith[i]];
    break;
  }

  default:
    assert(0);
  }

  XPutImage(dsply, work_pix, gc, xim, 0, 0, 0, idx, (unsigned) wdth, 1);
  idx += 1;
}


void x11_cleanup()
{
  MarkerInfo *minfo;
  Display    *dpy;
  Pixmap      tmp;

  XDestroyImage(xim);
  free(dith);

  dpy = dsply;

  if (do_markers)
  {
    minfo = marker_info;
    while (minfo->label != NULL)
    {
      mark_location(dpy, minfo->lat, minfo->lon, minfo->label);
      minfo += 1;
    }
  }

  if (do_label) draw_label(dpy);

  XSetWindowBackgroundPixmap(dpy, vroot, work_pix);
  XClearWindow(dpy, vroot);
  XSync(dpy, True);

  if (use_two_pixmaps)
  {
    tmp      = work_pix;
    work_pix = disp_pix;
    disp_pix = tmp;
  }
}


static void draw_label(dpy)
     Display *dpy;
{
  int         dy;
  int         x, y;
  int         len;
  int         direction;
  int         ascent;
  int         descent;
  char        buf[128];
  XCharStruct extents;

  dy = font->ascent + font->descent + 1;

  strftime(buf, sizeof(buf), "%d %h %y %H:%M %Z", localtime(&current_time));
  len = strlen(buf);
  XTextExtents(font, buf, len, &direction, &ascent, &descent, &extents);
  x = wdth - (extents.rbearing + 10);
  y = hght - (2*dy + 10);
  draw_outlined_string(dpy, work_pix, white, black, x, y, buf, len);

  sprintf(buf, "view %.1f %c %.1f %c",
          fabs(view_lat), ((view_lat < 0) ? 'S' : 'N'),
          fabs(view_lon), ((view_lon < 0) ? 'W' : 'E'));
  len = strlen(buf);
  XTextExtents(font, buf, len, &direction, &ascent, &descent, &extents);
  x = wdth - (extents.rbearing + 10);
  y = hght - (1*dy + 10);
  draw_outlined_string(dpy, work_pix, white, black, x, y, buf, len);

  sprintf(buf, "sun %.1f %c %.1f %c",
          fabs(sun_lat), ((sun_lat < 0) ? 'S' : 'N'),
          fabs(sun_lon), ((sun_lon < 0) ? 'W' : 'E'));
  len = strlen(buf);
  XTextExtents(font, buf, len, &direction, &ascent, &descent, &extents);
  x = wdth - (extents.rbearing + 10);
  y = hght - 10;
  draw_outlined_string(dpy, work_pix, white, black, x, y, buf, len);

#ifdef DEBUG
  frame += 1;
  sprintf(buf, "frame %d", frame);
  len = strlen(buf);
  XTextExtents(font, buf, len, &direction, &ascent, &descent, &extents);
  x = wdth - (extents.rbearing + 10);
  y = hght - (3*dy + 10);
  draw_outlined_string(dpy, work_pix, white, black, x, y, buf, len);
#endif /* DEBUG */
}


static void mark_location(dpy, lat, lon, text)
     Display *dpy;
     double   lat;
     double   lon;
     char    *text;
{
  int    x, y;
  int    len;
  double pos[3];
  double tmp1, tmp2;

  lat *= (M_PI/180);
  lon *= (M_PI/180);

  pos[0] = sin(lon) * cos(lat);
  pos[1] = sin(lat);
  pos[2] = cos(lon) * cos(lat);

  tmp1 = (cos_view_lon * pos[0]) - (sin_view_lon * pos[2]);
  tmp2 = (sin_view_lon * pos[0]) + (cos_view_lon * pos[2]);
  pos[0] = tmp1;
  pos[2] = tmp2;

  tmp1 = (cos_view_lat * pos[1]) - (sin_view_lat * pos[2]);
  tmp2 = (sin_view_lat * pos[1]) + (cos_view_lat * pos[2]);
  pos[1] = tmp1;
  pos[2] = tmp2;

  if (pos[2] <= 0) return;

  x = XPROJECT(pos[0]);
  y = YPROJECT(pos[1]);

  XSetForeground(dpy, gc, black);
  XDrawArc(dpy, work_pix, gc, x-3, y-3, 6, 6, 0, 360*64);
  XDrawArc(dpy, work_pix, gc, x-1, y-1, 2, 2, 0, 360*64);
  XSetForeground(dpy, gc, hlight);
  XDrawArc(dpy, work_pix, gc, x-2, y-2, 4, 4, 0, 360*64);

  if (text != NULL)
  {
    x  += 4;
    y  += (font->ascent + font->descent) / 3;
    len = strlen(text);
    draw_outlined_string(dpy, work_pix, hlight, black, x, y, text, len);
  }

  XSetForeground(dpy, gc, white);
}


static void draw_outlined_string(dpy, pix, fg, bg, x, y, text, len)
     Display *dpy;
     Pixmap   pix;
     Pixel    fg;
     Pixel    bg;
     int      x;
     int      y;
     char    *text;
     int      len;
{
  XSetForeground(dpy, gc, bg);
  XDrawString(dpy, pix, gc, x+1, y, text, len);
  XDrawString(dpy, pix, gc, x-1, y, text, len);
  XDrawString(dpy, pix, gc, x, y+1, text, len);
  XDrawString(dpy, pix, gc, x, y-1, text, len);
  XSetForeground(dpy, gc, fg);
  XDrawString(dpy, pix, gc, x, y, text, len);
}


/* Function Name: GetVRoot
 * Description: Gets the root window, even if it's a virtual root
 * Arguments: the display and the screen
 * Returns: the root window for the client
 *
 * (taken nearly verbatim from the june 1993 comp.windows.x FAQ, item 148)
 */
static Window GetVRoot(dpy)
     Display *dpy;
{
  int          i;
  Window       rootReturn, parentReturn, *children;
  unsigned int numChildren;
  Atom         __SWM_VROOT = None;
  Window       rslt = root;

  __SWM_VROOT = XInternAtom(dpy, "__SWM_VROOT", False);
  XQueryTree(dpy, root, &rootReturn, &parentReturn, &children, &numChildren);
  for (i=0; i<numChildren; i++)
  {
    Atom          actual_type;
    int           actual_format;
    unsigned long nitems, bytesafter;
    Window       *newRoot = NULL;

    /* item 148 in the FAQ neglects to mention that there is a race
     * condition here; consider a child of the root window that
     * existed when XQueryTree() was called, but has disappeared
     * before XGetWindowProperty() gets called for that window ...
     */
    if ((XGetWindowProperty(dpy, children[i], __SWM_VROOT, 0, 1,
                            False, XA_WINDOW, &actual_type,
                            &actual_format, &nitems, &bytesafter,
                            (unsigned char **) &newRoot) == Success)
        && newRoot)
    {
      rslt = *newRoot;
      break;
    }
  }

  /* item 148 in the FAQ also neglects to mention that we probably
   * want to free the list of children after we're done with it ...
   */
  XFree((void *) children);

  return rslt;
}


/*
 * the following code is lifted nearly verbatim from jim frost's
 * xloadimage code (version 3.00). that code includes a note
 * indicating that the changes to allow proper freeing of previously
 * allocated resources made by Deron Dann Johnson (dj@eng.sun.com),
 * thus he may well be the author of this code.
 *
 * Copyright (C) 1989, 1990, 1991 by Jim Frost.
 *
 * xkill_handler() and the XSetErrorHandler() code in freePrevious()
 * were not in the original xloadimage code; this is new as of xearth
 * version 0.91.
 */

static void updateProperty(dpy, w, name, type, format, data, nelem)
     Display *dpy;
     Window   w;
     char    *name;
     Atom     type;
     int      format;
     int      data;
     int      nelem;
{
  /* intern the property name */
  Atom atom = XInternAtom(dpy, name, 0);

  /* create or replace the property */
  XChangeProperty(dpy, w, atom, type, format, PropModeReplace,
                  (unsigned char *)&data, nelem);
}


/* Sets the close-down mode of the client to 'RetainPermanent'
 * so all client resources will be preserved after the client
 * exits.  Puts a property on the default root window containing
 * an XID of the client so that the resources can later be killed.
 */
static void preserveResource(dpy, w)
     Display *dpy;
     Window   w;
{
  /* create dummy resource */
  Pixmap pm = XCreatePixmap(dpy, w, 1, 1, 1);

  /* create/replace the property */
  updateProperty(dpy, w, RETAIN_PROP_NAME, XA_PIXMAP, 32, (int)pm, 1);

  /* retain all client resources until explicitly killed */
  XSetCloseDownMode(dpy, RetainPermanent);
}


/* Flushes any resources previously retained by the client,
 * if any exist.
 */
static void freePrevious(dpy, w)
     Display *dpy;
     Window   w;
{
  Pixmap       *pm;
  Atom          actual_type;
  int           format;
  unsigned long nitems;
  unsigned long bytes_after;

  /* intern the property name */
  Atom atom = XInternAtom(dpy, RETAIN_PROP_NAME, 0);

  /* look for existing resource allocation */
  if ((XGetWindowProperty(dpy, w, atom, 0, 1, 1 /*delete*/,
                          AnyPropertyType, &actual_type,
			  &format, &nitems, &bytes_after,
			  (unsigned char **) &pm) == Success) &&
      (nitems == 1))
    if ((actual_type == XA_PIXMAP) && (format == 32) &&
        (nitems == 1) && (bytes_after == 0))
    {
      /* blast it away, but first provide new X error handler in case
       * the client that installed the RETAIN_PROP_NAME (_XSETROOT_ID)
       * property on the root window has already terminated
       */
      orig_error_handler = XSetErrorHandler(xkill_handler);
      XKillClient(dpy, (XID) *pm);
      XSync(dpy, False);
      XSetErrorHandler(orig_error_handler);
      XFree((void *) pm);
    }
    else if (actual_type != None)
    {
      fprintf(stderr,
              "%s: warning: invalid format encountered for property %s\n",
              RETAIN_PROP_NAME, progname);
    }
}


static int xkill_handler(dpy, xev)
     Display     *dpy;
     XErrorEvent *xev;
{
  /* ignore any BadValue errors from the call to XKillClient() in 
   * freePrevious(); they should only happen if the client that
   * installed the RETAIN_PROP_NAME (_XSETROOT_ID) property on the
   * root window has already terminated
   */
  if ((xev->error_code == BadValue) &&
      (xev->request_code == X_KillClient))
  {
    fprintf(stderr, "ignoring BadValue error from XKillClient()\n");
    fflush(stderr);
    return 0;
  }

  /* pass any other errors get on to the original error handler
   */
  return orig_error_handler(dpy, xev);
}
