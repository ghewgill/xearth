/*
 * x11.c
 * kirk johnson
 * july 1993
 *
 * RCS $Id: x11.c,v 1.35 1995/09/25 01:12:41 tuna Exp $
 *
 * Copyright (C) 1989, 1990, 1993, 1994, 1995 Kirk Lauritz Johnson
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
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include "kljcpyrt.h"

#define RETAIN_PROP_NAME "_XSETROOT_ID"

#define MONO_1   (0)
#define MONO_8   (1)
#define COLOR_8  (2)
#define MONO_16  (3)
#define COLOR_16 (4)
#define MONO_32  (5)
#define COLOR_32 (6)

#define LABEL_LEFT_FLUSH (1<<0)
#define LABEL_TOP_FLUSH  (1<<1)

static XFontStruct *load_x_font _P((Display *, char *));
static void         get_proj_type _P((void));
static void         get_viewing_position _P((void));
static void         get_sun_position _P((void));
static void         get_size _P((void));
static void         get_shift _P((void));
static void         get_labelpos _P((void));
static void         x11_setup _P((void));
static void         pack_mono_1 _P((u16or32 *, u_char *));
static void         pack_8 _P((u16or32 *, Pixel *, u_char *));
static void         pack_16 _P((u16or32 *, Pixel *, u_char *));
static void         pack_32 _P((u16or32 *, Pixel *, u_char *));
static void         x11_row _P((u_char *));
static void         x11_cleanup _P((void));
static void         draw_label _P((Display *));
static void         mark_location _P((Display *, MarkerInfo *));
static void         draw_outlined_string _P((Display *, Pixmap, Pixel, Pixel,
                      int, int, char *, int));
static Window       GetVRoot _P((Display *));
static void         updateProperty _P((Display *, Window, const char *, Atom,
                      int, int, int));
static void         preserveResource _P((Display *, Window));
static void         freePrevious _P((Display *, Window));
static int          xkill_handler _P((Display *, XErrorEvent *));

static u16or32 *dith;
static u_char  *xbuf;
static int      idx;
static XImage  *xim;
static Pixmap   work_pix;
static Pixmap   disp_pix;
static int    (*orig_error_handler) _P((Display *, XErrorEvent *));

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
Window       vroot;             /* virtual root window */
Colormap     cmap;              /* default colormap    */
Visual      *visl;              /* default visual      */
int          dpth;              /* default depth       */
Pixel        white;             /* white pixel         */
Pixel        black;             /* black pixel         */
Pixel        hlight;            /* highlight pixel     */
GC           gc;                /* graphics context    */
Pixel       *pels;              /* allocated colors    */
char        *font_name;         /* text font name      */
XFontStruct *font;              /* basic text font     */

static int   do_once;           /* only render once?   */
static int   mono;              /* render in mono?     */
static int   x_type;            /* type of rendering   */

static int   label_xvalue;      /* label x position    */
static int   label_yvalue;      /* label y position    */
static int   label_orient;      /* label orientation   */


static char *defaults[] =
{
  "*proj:       orthographic",
  "*pos:        sunrel 0 0",
  "*rot:        0",
  "*shift:      0 0",
  "*mag:        1.0",
  "*shade:      on",
  "*label:      off",
  "*labelpos:   -5-5",
  "*markers:    on",
  "*markerfile: built-in",
  "*wait:       300",
  "*timewarp:   1",
  "*day:        100",
  "*night:      5",
  "*term:       1",
  "*twopix:     on",
  "*ncolors:    64",
  "*fork:       off",
  "*once:       off",
  "*nice:       0",
  "*stars:      on",
  "*starfreq:   0.002",
  "*bigstars:   0",
  "*grid:       off",
  "*grid1:      6",
  "*grid2:      15",
  "*gamma:      1.0",
  "*font:       variable",
  NULL
};

static XrmOptionDescRec options[] =
{
{ "-proj",        ".proj",        XrmoptionSepArg, 0     },
{ "-pos",         ".pos",         XrmoptionSepArg, 0     },
{ "-rot",         ".rot",         XrmoptionSepArg, 0     },
{ "-mag",         ".mag",         XrmoptionSepArg, 0     },
{ "-shade",       ".shade",       XrmoptionNoArg,  "on"  },
{ "-noshade",     ".shade",       XrmoptionNoArg,  "off" },
{ "-sunpos",      ".sunpos",      XrmoptionSepArg, 0     },
{ "-size",        ".size",        XrmoptionSepArg, 0     },
{ "-shift",       ".shift",       XrmoptionSepArg, 0     },
{ "-label",       ".label",       XrmoptionNoArg,  "on"  },
{ "-nolabel",     ".label",       XrmoptionNoArg,  "off" },
{ "-labelpos",    ".labelpos",    XrmoptionSepArg, 0     },
{ "-markers",     ".markers",     XrmoptionNoArg,  "on"  },
{ "-nomarkers",   ".markers",     XrmoptionNoArg,  "off" },
{ "-markerfile",  ".markerfile",  XrmoptionSepArg, 0     },
{ "-showmarkers", ".showmarkers", XrmoptionNoArg,  "on"  },
{ "-wait",        ".wait",        XrmoptionSepArg, 0     },
{ "-timewarp",    ".timewarp",    XrmoptionSepArg, 0     },
{ "-day",         ".day",         XrmoptionSepArg, 0     },
{ "-night",       ".night",       XrmoptionSepArg, 0     },
{ "-term",        ".term",        XrmoptionSepArg, 0     },
{ "-onepix",      ".twopix",      XrmoptionNoArg,  "off" },
{ "-twopix",      ".twopix",      XrmoptionNoArg,  "on"  },
{ "-ncolors",     ".ncolors",     XrmoptionSepArg, 0     },
{ "-fork",        ".fork",        XrmoptionNoArg,  "on"  },
{ "-nofork",      ".fork",        XrmoptionNoArg,  "off" },
{ "-once",        ".once",        XrmoptionNoArg,  "on"  },
{ "-noonce",      ".once",        XrmoptionNoArg,  "off" },
{ "-nice",        ".nice",        XrmoptionSepArg, 0     },
{ "-version",     ".version",     XrmoptionNoArg,  "on"  },
{ "-stars",       ".stars",       XrmoptionNoArg,  "on"  },
{ "-nostars",     ".stars",       XrmoptionNoArg,  "off" },
{ "-starfreq",    ".starfreq",    XrmoptionSepArg, 0     },
{ "-bigstars",    ".bigstars",    XrmoptionSepArg, 0     },
{ "-grid",        ".grid",        XrmoptionNoArg,  "on"  },
{ "-nogrid",      ".grid",        XrmoptionNoArg,  "off" },
{ "-grid1",       ".grid1",       XrmoptionSepArg, 0     },
{ "-grid2",       ".grid2",       XrmoptionSepArg, 0     },
{ "-time",        ".time",        XrmoptionSepArg, 0     },
{ "-gamma",       ".gamma",       XrmoptionSepArg, 0     },
{ "-font",        ".font",        XrmoptionSepArg, 0     },
{ "-mono",        ".mono",        XrmoptionNoArg,  "on"  },
{ "-nomono",      ".mono",        XrmoptionNoArg,  "off" },
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

  if (get_boolean_resource("showmarkers", "Showmarkers"))
  {
    markerfile = get_string_resource("markerfile", "Markerfile");
    show_marker_info(markerfile);
  }

  wdth = DisplayWidth(dsply, scrn);
  hght = DisplayHeight(dsply, scrn);

  /* process complex resources
   */
  get_proj_type();
  get_viewing_position();
  get_sun_position();
  get_size();
  get_shift();
  get_labelpos();

  /* process simple resources
   */
  view_rot        = get_float_resource("rot", "Rot");
  view_mag        = get_float_resource("mag", "Mag");
  do_shade        = get_boolean_resource("shade", "Shade");
  do_label        = get_boolean_resource("label", "Label");
  do_markers      = get_boolean_resource("markers", "Markers");
  markerfile      = get_string_resource("markerfile", "Markerfile");
  wait_time       = get_integer_resource("wait", "Wait");
  time_warp       = get_float_resource("timewarp", "Timewarp");
  day             = get_integer_resource("day", "Day");
  night           = get_integer_resource("night", "Night");
  terminator      = get_integer_resource("term", "Term");
  use_two_pixmaps = get_boolean_resource("twopix", "Twopix");
  num_colors      = get_integer_resource("ncolors", "Ncolors");
  do_fork         = get_boolean_resource("fork", "Fork");
  do_once         = get_boolean_resource("once", "Once");
  priority        = get_integer_resource("nice", "Nice");
  do_stars        = get_boolean_resource("stars", "Stars");
  star_freq       = get_float_resource("starfreq", "Starfreq");
  big_stars       = get_integer_resource("bigstars", "Bigstars");
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
   */

  /* various sanity checks on simple resources
   */
  if ((view_rot < -180) || (view_rot > 360))
    fatal("viewing rotation must be between -180 and 360");
  if (view_mag <= 0)
    fatal("viewing magnification must be positive");
  if (wait_time < 0)
    fatal("arg to -wait must be non-negative");
  if (time_warp <= 0)
    fatal("arg to -timewarp must be positive");
  if ((num_colors < 3) || (num_colors > 1024))
    fatal("arg to -ncolors must be between 3 and 1024");
  if ((star_freq < 0) || (star_freq > 1))
    fatal("arg to -starfreq must be between 0 and 1");
  if ((big_stars < 0) || (big_stars > 100))
    fatal("arg to -bigstars must be between 0 and 100");
  if (grid_big <= 0)
    fatal("arg to -grid1 must be positive");
  if (grid_small <= 0)
    fatal("arg to -grid2 must be positive");
  if ((day > 100) || (day < 0))
    fatal("arg to -day must be between 0 and 100");
  if ((night > 100) || (night < 0))
    fatal("arg to -night must be between 0 and 100");
  if ((terminator > 100) || (terminator < 0))
    fatal("arg to -term must be between 0 and 100");
  if (xgamma <= 0)
    fatal("arg to -gamma must be positive");

  /* if we're only rendering once, make sure we don't
   * waste memory by allocating two pixmaps
   */
  if (do_once)
    use_two_pixmaps = 0;

  white = WhitePixel(dsply, scrn);
  black = BlackPixel(dsply, scrn);
  gc = XCreateGC(dsply, root, 0, NULL);
  XSetState(dsply, gc, white, black, GXcopy, AllPlanes);
  hlight = white;

  switch (dpth)
  {
  case 1:
    /* try to pack ximage data 1 bit/pixel */
    x_type = MONO_1;
    break;

  case 8:
    /* try to pack ximage data 8 bits/pixel */
    x_type = mono ? MONO_8 : COLOR_8;
    break;

  case 12:
  case 15:
  case 16:
    /* try to pack ximage data 16 bits/pixel */
    x_type = mono ? MONO_16 : COLOR_16;
    break;

  case 24:
    /* try to pack ximage data 32 bits/pixel */
    x_type = mono ? MONO_32 : COLOR_32;
    break;

  default:
    fflush(stdout);
    fprintf(stderr,
            "xearth %s: fatal - unsupported display depth %d\n",
            VersionString, dpth);
    fprintf(stderr,
            "  (supported depths: 1, 8, 15, 16, and 24 bits)\n");
    exit(1);
  }

  switch (x_type)
  {
  case MONO_1:
  case MONO_8:
  case MONO_16:
  case MONO_32:
    mono_dither_setup();
    pels = (Pixel *) malloc((unsigned) sizeof(Pixel) * 2);
    assert(pels != NULL);
    pels[0] = black;
    pels[1] = white;
    break;

  case COLOR_8:
  case COLOR_16:
  case COLOR_32:
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
   * client might allocate a resource with the same resource ID that
   * xearth had stored in the _XSETROOT_ID property, so subsequent
   * attempts to free any resources retained by a client that had
   * scribbled on the root window via XKillClient() may end up killing
   * the wrong client.
   *
   * this possibility could be eliminated by setting the closedown
   * mode for the display connection to RetainPermanent, but this
   * seemed to be causing core dumps in an R5pl26 server -- i submitted
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


void x11_output()
{
  while (1)
  {
    compute_positions();

    /* if we were really clever, we'd only
     * do this if the position has changed
     */
    scan_map();
    do_dots();

    /* for now, go ahead and reload the marker info every time
     * we redraw, but maybe change this in the future?
     */
    load_marker_info(markerfile);

    x11_setup();
    render(x11_row);
    x11_cleanup();

    if (do_once)
    {
      preserveResource(dsply, vroot);
      XSync(dsply, True);
      return;
    }

    /* sleep for designated wait_time
     */
    sleep((unsigned) wait_time);
  }
}


static XFontStruct *load_x_font(dpy, fontname)
     Display *dpy;
     char    *fontname;
{
  XFontStruct *rslt;

  rslt = XLoadQueryFont(dpy, fontname);
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


/* fetch and decode 'proj' resource (projection type)
 */
static void get_proj_type()
{
  char *res;

  res = get_string_resource("proj", "Proj");
  if (res != NULL)
  {
    decode_proj_type(res);
    free(res);
  }
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


/* fetch and decode 'labelpos' resource (label position)
 */
static void get_labelpos()
{
  char    *res;
  int      mask;
  int      x, y;
  unsigned w, h;

  /* it's somewhat brute-force ugly to hard-code these here,
   * duplicating information contained in defaults[], but such it is.
   */
  label_orient = 0;
  label_xvalue = wdth - 5;
  label_yvalue = hght - 5;

  res = get_string_resource("labelpos", "Labelpos");
  if (res != NULL)
  {
    mask = XParseGeometry(res, &x, &y, &w, &h);

    if (mask & (WidthValue | HeightValue))
      warning("width and height ignored in label position");

    if ((mask & XValue) && (mask & YValue))
    {
      if (mask & XNegative)
      {
        label_xvalue = wdth + x;
      }
      else
      {
        label_xvalue  = x;
        label_orient |= LABEL_LEFT_FLUSH;
      }

      if (mask & YNegative)
      {
        label_yvalue = hght + y;
      }
      else
      {
        label_yvalue  = y;
        label_orient |= LABEL_TOP_FLUSH;
      }
    }
    else
    {
      warning("label position must specify x and y offsets");
    }

    free(res);
  }
}


static void x11_setup()
{
  int dith_size;
  int xbuf_size;
  int bits_per_pixel;

  switch (x_type)
  {
  case MONO_1:
    dith_size      = wdth + 7;
    xbuf_size      = dith_size >> 3;
    bits_per_pixel = 1;
    break;

  case MONO_8:
  case COLOR_8:
    dith_size      = wdth;
    xbuf_size      = dith_size;
    bits_per_pixel = 8;
    break;

  case MONO_16:
  case COLOR_16:
    dith_size      = wdth;
    xbuf_size      = dith_size * 2;
    bits_per_pixel = 16;
    break;

  case MONO_32:
  case COLOR_32:
    dith_size      = wdth;
    xbuf_size      = dith_size * 4;
    bits_per_pixel = 32;
    break;

  default:
    /* keep lint happy */
    dith_size      = 0;
    xbuf_size      = 0;
    bits_per_pixel = 0;
    assert(0);
  }

  dith = (u16or32 *) malloc((unsigned) sizeof(u16or32) * dith_size);
  assert(dith != NULL);

  xbuf = (u_char *) malloc((unsigned) xbuf_size);
  assert(xbuf != NULL);

  xim = XCreateImage(dsply, visl, (unsigned) dpth, ZPixmap, 0,
                     (char *) xbuf, (unsigned) wdth, 1, 8,
                     xbuf_size);

  if (xim->bits_per_pixel != bits_per_pixel)
  {
    fflush(stdout);
    fprintf(stderr,
            "xearth %s: fatal - unexpected bits/pixel for depth %d\n",
            VersionString, dpth);
    fprintf(stderr,
            "  (expected %d bits/pixel, actual value is %d)\n",
            bits_per_pixel, xim->bits_per_pixel);
    exit(1);
  }

  if (x_type == MONO_1)
  {
    /* force MSBFirst bitmap_bit_order and byte_order
     */
    xim->bitmap_bit_order = MSBFirst;
    xim->byte_order       = MSBFirst;
  }

  idx = 0;
}


/* pack pixels into ximage format (assuming bits_per_pixel == 1,
 * bitmap_bit_order == MSBFirst, and byte_order == MSBFirst)
 */
static void pack_mono_1(src, dst)
     u16or32 *src;
     u_char  *dst;
{
  int      i, i_lim;
  unsigned val;

  i_lim = wdth;
  for (i=0; i<i_lim; i+=8)
  {
    val = ((src[0] << 7) | (src[1] << 6) | (src[2] << 5) |
           (src[3] << 4) | (src[4] << 3) | (src[5] << 2) |
           (src[6] << 1) | (src[7] << 0));

    /* if white is pixel 0, need to toggle the bits
     */
    dst[i>>3] = (white == 0) ? (~ val) : val;
    src += 8;
  }
}


/* pack pixels into ximage format (assuming bits_per_pixel == 8)
 */
static void pack_8(src, map, dst)
     u16or32 *src;
     Pixel   *map;
     u_char  *dst;
{
  int      i, i_lim;
  unsigned val;

  i_lim = wdth;
  for (i=0; i<i_lim; i++)
  {
    val = map[src[i]];
    dst[i] = val;
  }
}


/* pack pixels into ximage format (assuming bits_per_pixel == 16)
 */
static void pack_16(src, map, dst)
     u16or32 *src;
     Pixel   *map;
     u_char  *dst;
{
  int      i, i_lim;
  unsigned val;

  i_lim = wdth;

  if (xim->byte_order == MSBFirst)
  {
    for (i=0; i<i_lim; i++)
    {
      val    = map[src[i]];
      dst[0] = (val >> 8) & 0xff;
      dst[1] = val & 0xff;
      dst   += 2;
    }
  }
  else /* (xim->byte_order == LSBFirst) */
  {
    for (i=0; i<i_lim; i++)
    {
      val    = map[src[i]];
      dst[0] = val & 0xff;
      dst[1] = (val >> 8) & 0xff;
      dst   += 2;
    }
  }
}


/* pack pixels into ximage format (assuming bits_per_pixel == 32)
 */
static void pack_32(src, map, dst)
     u16or32 *src;
     Pixel   *map;
     u_char  *dst;
{
  int      i, i_lim;
  unsigned val;

  i_lim = wdth;

  if (xim->byte_order == MSBFirst)
  {
    for (i=0; i<i_lim; i++)
    {
      val    = map[src[i]];
      dst[0] = (val >> 24) & 0xff;
      dst[1] = (val >> 16) & 0xff;
      dst[2] = (val >> 8) & 0xff;
      dst[3] = val & 0xff;
      dst   += 4;
    }
  }
  else /* (xim->byte_order == LSBFirst) */
  {
    for (i=0; i<i_lim; i++)
    {
      val    = map[src[i]];
      dst[0] = val & 0xff;
      dst[1] = (val >> 8) & 0xff;
      dst[2] = (val >> 16) & 0xff;
      dst[3] = (val >> 24) & 0xff;
      dst   += 4;
    }
  }
}


static void x11_row(row)
     u_char *row;
{
  switch (x_type)
  {
  case MONO_1:
    mono_dither_row(row, dith);
    pack_mono_1(dith, xbuf);
    break;

  case MONO_8:
    mono_dither_row(row, dith);
    pack_8(dith, pels, xbuf);
    break;

  case MONO_16:
    mono_dither_row(row, dith);
    pack_16(dith, pels, xbuf);
    break;

  case MONO_32:
    mono_dither_row(row, dith);
    pack_32(dith, pels, xbuf);
    break;

  case COLOR_8:
    dither_row(row, dith);
    pack_8(dith, pels, xbuf);
    break;

  case COLOR_16:
    dither_row(row, dith);
    pack_16(dith, pels, xbuf);
    break;

  case COLOR_32:
    dither_row(row, dith);
    pack_32(dith, pels, xbuf);
    break;

  default:
    assert(0);
  }

  XPutImage(dsply, work_pix, gc, xim, 0, 0, 0, idx, (unsigned) wdth, 1);
  idx += 1;
}


static void x11_cleanup()
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
      mark_location(dpy, minfo);
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

  if (label_orient & LABEL_TOP_FLUSH)
  {
    y = label_yvalue + font->ascent;
  }
  else
  {
    y = label_yvalue - font->descent;
#ifdef DEBUG
    y -= 3 * dy;                /* 4 lines of text */
#else
    y -= 2 * dy;                /* 3 lines of text */
#endif
  }

#ifdef DEBUG
  frame += 1;
  sprintf(buf, "frame %d", frame);
  len = strlen(buf);
  XTextExtents(font, buf, len, &direction, &ascent, &descent, &extents);
  if (label_orient & LABEL_LEFT_FLUSH)
    x = label_xvalue - extents.lbearing;
  else
    x = label_xvalue - extents.rbearing;
  draw_outlined_string(dpy, work_pix, white, black, x, y, buf, len);
  y += dy;
#endif /* DEBUG */

  strftime(buf, sizeof(buf), "%d %b %y %H:%M %Z", localtime(&current_time));
  len = strlen(buf);
  XTextExtents(font, buf, len, &direction, &ascent, &descent, &extents);
  if (label_orient & LABEL_LEFT_FLUSH)
    x = label_xvalue - extents.lbearing;
  else
    x = label_xvalue - extents.rbearing;
  draw_outlined_string(dpy, work_pix, white, black, x, y, buf, len);
  y += dy;

  sprintf(buf, "view %.1f %c %.1f %c",
          fabs(view_lat), ((view_lat < 0) ? 'S' : 'N'),
          fabs(view_lon), ((view_lon < 0) ? 'W' : 'E'));
  len = strlen(buf);
  XTextExtents(font, buf, len, &direction, &ascent, &descent, &extents);
  if (label_orient & LABEL_LEFT_FLUSH)
    x = label_xvalue - extents.lbearing;
  else
    x = label_xvalue - extents.rbearing;
  draw_outlined_string(dpy, work_pix, white, black, x, y, buf, len);
  y += dy;

  sprintf(buf, "sun %.1f %c %.1f %c",
          fabs(sun_lat), ((sun_lat < 0) ? 'S' : 'N'),
          fabs(sun_lon), ((sun_lon < 0) ? 'W' : 'E'));
  len = strlen(buf);
  XTextExtents(font, buf, len, &direction, &ascent, &descent, &extents);
  if (label_orient & LABEL_LEFT_FLUSH)
    x = label_xvalue - extents.lbearing;
  else
    x = label_xvalue - extents.rbearing;
  draw_outlined_string(dpy, work_pix, white, black, x, y, buf, len);
  y += dy;
}


static void mark_location(dpy, info)
     Display    *dpy;
     MarkerInfo *info;
{
  int         x, y;
  int         len;
  double      lat, lon;
  double      pos[3];
  char       *text;
  int         direction;
  int         ascent;
  int         descent;
  XCharStruct extents;

  lat = info->lat * (M_PI/180);
  lon = info->lon * (M_PI/180);

  pos[0] = sin(lon) * cos(lat);
  pos[1] = sin(lat);
  pos[2] = cos(lon) * cos(lat);

  XFORM_ROTATE(pos, view_pos_info);

  if (proj_type == ProjTypeOrthographic)
  {
    /* if the marker isn't visible, return immediately
     */
    if (pos[2] <= 0) return;
  }
  else /* (proj_type == ProjTypeMercator) */
  {
    /* apply mercator projection
     */
    pos[0] = MERCATOR_X(pos[0], pos[2]);
    pos[1] = MERCATOR_Y(pos[1]);
  }

  x = XPROJECT(pos[0]);
  y = YPROJECT(pos[1]);

  XSetForeground(dpy, gc, black);
  XDrawArc(dpy, work_pix, gc, x-3, y-3, 6, 6, 0, 360*64);
  XDrawArc(dpy, work_pix, gc, x-1, y-1, 2, 2, 0, 360*64);
  XSetForeground(dpy, gc, hlight);
  XDrawArc(dpy, work_pix, gc, x-2, y-2, 4, 4, 0, 360*64);

  text = info->label;
  if (text != NULL)
  {
    len = strlen(text);
    XTextExtents(font, text, len, &direction, &ascent, &descent, &extents);

    switch (info->align)
    {
    case MarkerAlignLeft:
      x -= (extents.rbearing + 4);
      y += (font->ascent + font->descent) / 3;
      break;

    case MarkerAlignRight:
    case MarkerAlignDefault:
      x += (extents.lbearing + 3);
      y += (font->ascent + font->descent) / 3;
      break;

    case MarkerAlignAbove:
      x -= (extents.rbearing - extents.lbearing) / 2;
      y -= (extents.descent + 4);
      break;

    case MarkerAlignBelow:
      x -= (extents.rbearing - extents.lbearing) / 2;
      y += (extents.ascent + 5);
      break;

    default:
      assert(0);
    }

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
     Display    *dpy;
     Window      w;
     const char *name;
     Atom        type;
     int         format;
     int         data;
     int         nelem;
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
