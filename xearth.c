/*
 * xearth.c
 * kirk johnson
 * july 1993
 *
 * RCS $Id: xearth.c,v 1.14 1994/05/25 06:04:04 tuna Exp $
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

#ifndef NO_SETPRIORITY
/* apparently some systems (possibly mt.xinu 4.3bsd?) may need 
 * <sys/time.h> to be included here before <sys/resource.h>.
 */
#include <sys/resource.h>	/* for setpriority() */
#endif

#define ModeX   (0)             /* possible output_mode values */
#define ModePPM (1)
#define ModeGIF (2)

void set_priority();
void output();
void compute_positions();
void sun_relative_position();
void simple_orbit();
void set_defaults();
int  using_x();
void command_line();

char    *progname;              /* program name                */
int      output_mode;           /* output mode (X, PPM, ...)   */
double   view_lon;              /* viewing position longitude  */
double   view_lat;              /* viewing position latitude   */
int      view_pos_type;		/* type of viewing position    */
double   sun_rel_lon;		/* view lon, relative to sun   */
double   sun_rel_lat;		/* view lat, relative to sun   */
double   orbit_period;		/* orbit period (seconds)      */
double   orbit_inclin;		/* orbit inclination (degrees) */
double   view_mag;		/* viewing magnification       */
int      do_shade;		/* render with shading?        */
double   sun_lon;               /* sun position longitude      */
double   sun_lat;               /* sun position latitude       */
int      compute_sun_pos;       /* compute sun's position?     */
int      wdth;                  /* image width (pixels)        */
int      hght;                  /* image height (pixels)       */
int      shift_x;		/* image shift (x, pixels)     */
int      shift_y;		/* image shift (y, pixels)     */
int      do_stars;		/* show stars in background?   */
double   star_freq;		/* frequency of stars          */
int      do_grid;		/* show lon/lat grid?          */
int      grid_big;		/* lon/lat grid line spacing   */
int      grid_small;		/* dot spacing along grids     */
int      do_label;              /* label image                 */
int      do_markers;		/* display markers (X only)    */
int      wait_time;             /* wait time between redraw    */
double   time_warp;		/* passage of time multiplier  */
int      fixed_time;		/* fixed viewing time (ssue)   */
int      day;                   /* day side brightness (%)     */
int      night;                 /* night side brightness (%)   */
double   xgamma;		/* gamma correction (X only)   */
int      use_two_pixmaps;	/* use two pixmaps? (X only)   */
int      num_colors;		/* number of colors to use     */
int      do_fork;		/* fork child process?         */
int      priority;		/* desired process priority    */

time_t start_time = 0;
time_t current_time;


int main(argc, argv)
     int   argc;
     char *argv[];
{
  set_defaults();

  if (using_x(argc, argv))
    command_line_x(argc, argv);
  else
    command_line(argc, argv);

  if (priority != 0)
    set_priority(priority);

  output();

  exit(0);
}


#ifdef NO_SETPRIORITY

/* setpriority()-like functionality for System V
 * derivates that only provide nice()
 */
void set_priority(new)
     int new;
{
  int old;
  extern int errno;

  /* determine current priority of the process
   */
  errno = 0;
  old   = nice(0);
  if ((old == -1) && (errno != 0))
  {
    perror("nice");
    exit(-1);
  }

  /* try to change priority to new
   */
  new = nice(new - old);
  if ((new == -1) && (errno != 0))
  {
    perror("nice");
    exit(-1);
  }
}

#else /* NO_SETPRIORITY */

/* for systems that provide setpriority(),
 * set_priority() is just a wrapper
 */
void set_priority(new)
     int new;
{
  if ((setpriority(PRIO_PROCESS, getpid(), new) == -1))
  {
    perror("setpriority");
    exit(-1);
  }
}

#endif /* NO_SETPRIORITY */


void output()
{
  void (*render)();

  if (do_shade)
    render = render_with_shading;
  else
    render = render_no_shading;

  switch (output_mode)
  {
  case ModePPM:
    compute_positions();
    scan_map();
    do_dots();
    ppm_setup(stdout);
    render(ppm_row);
    break;

  case ModeGIF:
    compute_positions();
    scan_map();
    do_dots();
    gif_setup(stdout);
    render(gif_row);
    gif_cleanup();
    break;

  case ModeX:
    if ((!do_fork) || (fork() == 0))
    {
      while (1)
      {
	compute_positions();

	/* should only do this if position has changed ... */
	scan_map();
	do_dots();

        x11_setup();
        render(x11_row);
        x11_cleanup();
        sleep((unsigned) wait_time);
      }
    }
    break;

  default:
    assert(0);
  }
}


void compute_positions()
{
  /* determine "current" time
   */
  if (fixed_time == 0)
  {
    current_time = time(NULL);
    if (start_time == 0)
      start_time = current_time;
    else
      current_time = start_time + (current_time - start_time) * time_warp;
  }
  else
  {
    current_time = fixed_time;
  }

  /* determine position on earth's surface where sun is directly
   * overhead
   */
  if (compute_sun_pos)
    sun_position(current_time, &sun_lat, &sun_lon);

  /* determine viewing position
   */
  if (view_pos_type == ViewPosTypeSun)
  {
    sun_relative_position(&view_lat, &view_lon);
  }
  else if (view_pos_type == ViewPosTypeOrbit)
  {
    simple_orbit(current_time, &view_lat, &view_lon);
  }
}


void sun_relative_position(lat_ret, lon_ret)
     double *lat_ret;		/* (return) latitude  */
     double *lon_ret;		/* (return) longitude */
{
  double lat, lon;

  lat = sun_lat + sun_rel_lat;
  lon = sun_lon + sun_rel_lon;

  /* sanity check */
  assert((lat >= -180) && (lat <= 180));
  assert((lon >= -360) && (lon <= 360));

  if (lat > 90)
  {
    lat  = 180 - lat;
    lon += 180;
  }
  else if (lat < -90)
  {
    lat  = -180 - lat;
    lon += 180;
  }

  if (lon > 180)
  {
    do
      lon -= 360;
    while (lon > 180);
  }
  else if (lon < -180)
  {
    do
      lon += 360;
    while (lon < -180);
  }

  *lat_ret = lat;
  *lon_ret = lon;
}


void simple_orbit(ssue, lat, lon)
     time_t  ssue;		/* seconds since unix epoch */
     double *lat;		/* (return) latitude        */
     double *lon;		/* (return) longitude       */
{
  double x, y, z;
  double a, c, s;
  double t1, t2;

  /* start at 0 N 0 E */
  x = 0;
  y = 0;
  z = 1;

  /* rotate in about y axis (from z towards x) according to the number
   * of orbits we've completed
   */
  a  = ((double) ssue / orbit_period) * (2*M_PI);
  c  = cos(a);
  s  = sin(a);
  t1 = c*z - s*x;
  t2 = s*z + c*x;
  z  = t1;
  x  = t2;

  /* rotate about z axis (from x towards y) according to the
   * inclination of the orbit
   */
  a  = orbit_inclin * (M_PI/180);
  c  = cos(a);
  s  = sin(a);
  t1 = c*x - s*y;
  t2 = s*x + c*y;
  x  = t1;
  y  = t2;

  /* rotate about y axis (from x towards z) according to the number of
   * rotations the earth has made
   */
  a  = ((double) ssue / EarthPeriod) * (2*M_PI);
  c  = cos(a);
  s  = sin(a);
  t1 = c*x - s*z;
  t2 = s*x + c*z;
  x  = t1;
  z  = t2;

  *lat = asin(y) * (180/M_PI);
  *lon = atan2(x, z) * (180/M_PI);
}


/* look through the command line arguments to figure out if we're
 * using X or not (if either of the "-ppm" or "-gif" arguments is
 * found, we're not using X, otherwise we are).
 */
int using_x(argc, argv)
     int   argc;
     char *argv[];
{
  int i;

  /* loop through the args, break if we find "-ppm" or "-gif"
   */
  for (i=1; i<argc; i++)
    if ((strcmp(argv[i], "-ppm") == 0) ||
	(strcmp(argv[i], "-gif") == 0))
      break;

  /* if we made it through the loop without finding "-ppm" or "-gif"
   * (and breaking out), assume we're using X.
   */
  return (i == argc);
}


/* set_defaults() gets called at xearth startup (before command line
 * arguments are handled), regardless of what output mode (x, ppm,
 * gif) is being used.
 */
void set_defaults()
{
  output_mode      = ModeX;
  view_pos_type    = ViewPosTypeSun;
  sun_rel_lat      = 0;
  sun_rel_lon      = 0;
  compute_sun_pos  = 1;
  wdth             = 512;
  hght             = 512;
  shift_x          = 0;
  shift_y          = 0;
  view_mag         = 0.99;
  do_shade         = 1;
  do_label         = 0;
  do_markers       = 1;
  wait_time        = 300;
  time_warp        = 1;
  day              = 100;
  night            = 10;
  use_two_pixmaps  = 1;
  num_colors       = 64;
  do_fork          = 0;
  priority         = 0;
  do_stars         = 1;
  star_freq        = 0.002;
  do_grid          = 0;
  grid_big         = 6;
  grid_small       = 15;
  fixed_time       = 0;
  xgamma           = 1.0;
}


/* procedure for interpreting command line arguments when we're not
 * using X; command_line_x() is used when we are.
 */
void command_line(argc, argv)
     int   argc;
     char *argv[];
{
  int i;

  progname = argv[0];

  for (i=1; i<argc; i++)
  {
    if (strcmp(argv[i], "-pos") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -pos");
      decode_viewing_pos(argv[i]);
    }
    else if (strcmp(argv[i], "-sunpos") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -sunpos");
      decode_sun_pos(argv[i]);
    }
    else if (strcmp(argv[i], "-mag") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -mag");
      sscanf(argv[i], "%lf", &view_mag);
      if (view_mag <= 0)
        fatal("viewing magnification must be positive");
    }
    else if (strcmp(argv[i], "-size") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -size");
      decode_size(argv[i]);
    }
    else if (strcmp(argv[i], "-shift") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -shift");
      decode_shift(argv[i]);
    }
    else if (strcmp(argv[i], "-shade") == 0)
    {
      do_shade = 1;
    }
    else if (strcmp(argv[i], "-noshade") == 0)
    {
      do_shade = 0;
    }
    else if (strcmp(argv[i], "-label") == 0)
    {
      do_label = 1;
      warning("labeling not supported with GIF and PPM output");
    }
    else if (strcmp(argv[i], "-nolabel") == 0)
    {
      do_label = 0;
    }
    else if (strcmp(argv[i], "-markers") == 0)
    {
      do_markers = 1;
      warning("markers not supported with GIF and PPM output");
    }
    else if (strcmp(argv[i], "-nomarkers") == 0)
    {
      do_markers = 0;
    }
    else if (strcmp(argv[i], "-stars") == 0)
    {
      do_stars = 1;
    }
    else if (strcmp(argv[i], "-nostars") == 0)
    {
      do_stars = 0;
    }
    else if (strcmp(argv[i], "-starfreq") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -starfreq");
      sscanf(argv[i], "%lf", &star_freq);
      if ((star_freq < 0) || (star_freq > 1))
	fatal("arg to -starfreq must be between 0 and 1");
    }
    else if (strcmp(argv[i], "-grid") == 0)
    {
      do_grid = 1;
    }
    else if (strcmp(argv[i], "-nogrid") == 0)
    {
      do_grid = 0;
    }
    else if (strcmp(argv[i], "-grid1") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -grid1");
      sscanf(argv[i], "%d", &grid_big);
      if (grid_big <= 0)
	fatal("arg to -grid1 must be positive");
    }
    else if (strcmp(argv[i], "-grid2") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -grid2");
      sscanf(argv[i], "%d", &grid_small);
      if (grid_small <= 0)
	fatal("arg to -grid2 must be positive");
    }
    else if (strcmp(argv[i], "-day") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -day");
      sscanf(argv[i], "%d", &day);
      if ((day > 100) || (day < 0))
        fatal("arg to -day must be between 0 and 100");
    }
    else if (strcmp(argv[i], "-night") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -night");
      sscanf(argv[i], "%d", &night);
      if ((night > 100) || (night < 0))
        fatal("arg to -night must be between 0 and 100");
    }
    else if (strcmp(argv[i], "-gamma") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -gamma");
      sscanf(argv[i], "%lf", &xgamma);
      if (xgamma <= 0)
        fatal("arg to -gamma must be positive");
      warning("gamma correction not supported with GIF and PPM output");
    }
    else if (strcmp(argv[i], "-wait") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -wait");
      sscanf(argv[i], "%d", &wait_time);
      if (wait_time < 0)
        fatal("arg to -wait must be non-negative");
    }
    else if (strcmp(argv[i], "-timewarp") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -timewarp");
      sscanf(argv[i], "%lf", &time_warp);
      if (time_warp <= 0)
	fatal("arg to -timewarp must be positive");
    }
    else if (strcmp(argv[i], "-time") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -time");
      sscanf(argv[i], "%d", &fixed_time);
    }
    else if (strcmp(argv[i], "-onepix") == 0)
    {
      use_two_pixmaps = 0;
      warning("-onepix not relevant for GIF or PPM output");
    }
    else if (strcmp(argv[i], "-twopix") == 0)
    {
      use_two_pixmaps = 1;
      warning("-twopix not relevant for GIF or PPM output");
    }
    else if (strcmp(argv[i], "-mono") == 0)
    {
      warning("monochrome mode not supported with GIF and PPM output");
    }
    else if (strcmp(argv[i], "-nomono") == 0)
    {
      warning("monochrome mode not supported with GIF and PPM output");
    }
    else if (strcmp(argv[i], "-ncolors") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -ncolors");
      sscanf(argv[i], "%d", &num_colors);
      if (num_colors < 3)
        fatal("arg to -ncolors must be >= 3");
    }
    else if (strcmp(argv[i], "-font") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -font");
      warning("-font not relevant for GIF or PPM output");
    }
    else if (strcmp(argv[i], "-fork") == 0)
    {
      do_fork = 1;
    }
    else if (strcmp(argv[i], "-nofork") == 0)
    {
      do_fork = 0;
    }
    else if (strcmp(argv[i], "-nice") == 0)
    {
      i += 1;
      if (i >= argc) usage("missing arg to -nice");
      sscanf(argv[i], "%d", &priority);
    }
    else if (strcmp(argv[i], "-version") == 0)
    {
      version_info();
    }
    else if (strcmp(argv[i], "-ppm") == 0)
    {
      output_mode = ModePPM;
    }
    else if (strcmp(argv[i], "-gif") == 0)
    {
      output_mode = ModeGIF;
    }
    else if (strcmp(argv[i], "-display") == 0)
    {
      fatal("-display not compatible with -ppm or -gif\n");
    }
    else
    {
      usage(NULL);
    }
  }
}


static char **tokenize(s, delim, argc_ret)
     char *s;
     char *delim;
     int  *argc_ret;
{
  int    flag;
  int    lim;
  int    argc;
  char **argv;

  lim  = 4;
  argc = 0;
  argv = (char **) malloc((unsigned) sizeof(char *) * lim);
  assert(argv != NULL);

  flag = 1;

  while (1)
  {
    if (*s == '\0')
    {
      break;
    }
    else if (strchr(delim, *s) != NULL)
    {
      *s = '\0';
      flag = 1;
    }
    else if (flag)
    {
      if (argc == lim)
      {
	lim *= 2;
	argv = (char **) realloc(argv, (unsigned) sizeof(char *) * lim);
	assert(argv != NULL);
      }
      argv[argc++] = s;
      flag = 0;
    }

    s += 1;
  }

  *argc_ret = argc;
  return argv;
}


/* decode viewing position specifier; three possibilities:
 *
 *  fixed lat lon  - viewing position fixed wrt earth at (lat, lon)
 *
 *  sunrel lat lon - viewing position fixed wrt sun at (lat, lon)
 *                   [position interpreted as if sun was at (0, 0)]
 *
 *  orbit per inc  - moving viewing position following simple orbit
 *                   with period per and inclination inc
 *
 * fields can be separated with either spaces, commas, or slashes.
 */
void decode_viewing_pos(s)
     char *s;
{
  int    argc;
  char **argv;
  double arg1;
  double arg2;

  argv = tokenize(s, " ,/", &argc);
  if (argc != 3)
    fatal("wrong number of args in viewing position specifier");

  arg1 = 0;
  arg2 = 0;
  sscanf(argv[1], "%lf", &arg1);
  sscanf(argv[2], "%lf", &arg2);

  if (strcmp(argv[0], "fixed") == 0)
  {
    view_lat      = arg1;
    view_lon      = arg2;
    view_pos_type = ViewPosTypeFixed;

    if ((view_lat > 90) || (view_lat < -90))
      fatal("viewing latitude must be between -90 and 90");
    if ((view_lon > 180) || (view_lon < -180))
      fatal("viewing longitude must be between -180 and 180");
  }
  else if (strcmp(argv[0], "sunrel") == 0)
  {
    sun_rel_lat   = arg1;
    sun_rel_lon   = arg2;
    view_pos_type = ViewPosTypeSun;

    if ((sun_rel_lat > 90) || (sun_rel_lat < -90))
      fatal("latitude relative to sun must be between -90 and 90");
    if ((sun_rel_lon > 180) || (sun_rel_lon < -180))
      fatal("longitude relative to sun must be between -180 and 180");
  }
  else if (strcmp(argv[0], "orbit") == 0)
  {
    orbit_period  = arg1 * 3600;
    orbit_inclin  = arg2;
    view_pos_type = ViewPosTypeOrbit;

    if (orbit_period <= 0)
      fatal("orbital period must be positive");
    if ((orbit_inclin > 90) || (orbit_inclin < -90))
      fatal("orbital inclination must be between -90 and 90");
  }
  else
  {
    fprintf(stderr, "`%s'\n", argv[0]);
    fatal("unrecognized keyword in viewing position specifier");
  }

  free(argv);
}


/* decode sun position specifier:
 *
 *  lat lon - sun position fixed wrt earth at (lat, lon)
 *
 * fields can be separated with either spaces, commas, or slashes.
 */
void decode_sun_pos(s)
     char *s;
{
  int    argc;
  char **argv;

  argv = tokenize(s, " ,/", &argc);
  if (argc != 2)
    fatal("wrong number of args in sun position specifier");

  sscanf(argv[0], "%lf", &sun_lat);
  sscanf(argv[1], "%lf", &sun_lon);

  if ((sun_lat > 90) || (sun_lat < -90))
    fatal("sun latitude must be between -90 and 90");
  if ((sun_lon > 180) || (sun_lon < -180))
    fatal("sun longitude must be between -180 and 180");

  compute_sun_pos = 0;

  free(argv);
}


/* decode size specifier:
 *
 *  width height - width and height of image (in pixels)
 *
 * fields can be separated with either spaces, commas, or slashes.
 */
void decode_size(s)
     char *s;
{
  int    argc;
  char **argv;

  argv = tokenize(s, " ,/", &argc);
  if (argc != 2)
    fatal("wrong number of args in size specifier");

  sscanf(argv[0], "%d", &wdth);
  sscanf(argv[1], "%d", &hght);

  if (wdth <= 0)
    fatal("wdth arg must be positive");
  if (hght <= 0)
    fatal("hght arg must be positive");

  free(argv);
}


/* decode shift specifier:
 *
 *  xofs yofs - offset in x and y dimensions
 *
 * fields can be separated with either spaces, commas, or slashes.
 */
void decode_shift(s)
     char *s;
{
  int    argc;
  char **argv;

  argv = tokenize(s, " ,/", &argc);
  if (argc != 2)
    fatal("wrong number of args in size specifier");

  sscanf(argv[0], "%d", &shift_x);
  sscanf(argv[1], "%d", &shift_y);

  free(argv);
}


void xearth_bzero(buf, len)
     char *buf;
     int   len;
{
  int *tmp;

  /* assume sizeof(int) is a power of two
   */
  assert((sizeof(int) == 4) || (sizeof(int) == 8));

  if (len < sizeof(int))
  {
    /* special case small regions
     */
    while (len > 0)
    {
      *buf = 0;
      buf += 1;
      len -= 1;
    }
  }
  else
  {
    /* zero leading non-word-aligned bytes
     */
    while (((long) buf) & (sizeof(int)-1))
    {
      *buf = 0;
      buf += 1;
      len -= 1;
    }

    /* zero trailing non-word-aligned bytes
     */
    while (len & (sizeof(int)-1))
    {
      len     -= 1;
      buf[len] = 0;
    }

    /* convert remaining len to words
     */
    tmp = (int *) buf;
    if (sizeof(int) == 4)
      len >>= 2;
    else if (sizeof(int) == 8)
      len >>= 3;
    else
      assert(0);

    /* zero trailing non-quadword-aligned words
     */
    while (len & 0x03)
    {
      len     -= 1;
      tmp[len] = 0;
    }

    /* zero remaining data four words at a time
     */
    while (len)
    {
      tmp[0] = 0;
      tmp[1] = 0;
      tmp[2] = 0;
      tmp[3] = 0;
      tmp += 4;
      len -= 4;
    }
  }
}


void version_info()
{
  fflush(stdout);
  fprintf(stderr, "\nThis is xearth version %s.\n\n", VersionString);
  exit(0);
}


void usage(msg)
     char *msg;
{
  fflush(stdout);
  fprintf(stderr, "\n");
  if (msg != NULL)
    fprintf(stderr, "%s\n", msg);
  fprintf(stderr, "usage: %s\n", progname);
  fprintf(stderr, " [-pos pos_spec] [-sunpos sun_pos_spec] [-mag factor]\n");
  fprintf(stderr, " [-size size_spec] [-shift shift_spec] [-shade|-noshade]\n");
  fprintf(stderr, " [-label|-nolabel] [-markers|-nomarkers] [-stars|-nostars]\n");
  fprintf(stderr, " [-starfreq frequency] [-grid|-nogrid] [-grid1 grid1] [-grid2 grid2]\n");
  fprintf(stderr, " [-day pct] [-night pct] [-gamma gamma_value] [-wait secs]\n");
  fprintf(stderr, " [-timewarp timewarp_factor] [-time fixed_time] [-onepix|-twopix]\n");
  fprintf(stderr, " [-mono|-nomono] [-ncolors num_colors] [-font font_name]\n");
  fprintf(stderr, " [-fork|-nofork] [-nice priority] [-gif] [-ppm] [-display dpyname]\n");
  fprintf(stderr, " [-version]\n");
  fprintf(stderr, "\n");
  exit(1);
}


void warning(msg)
     char *msg;
{
  fflush(stdout);
  fprintf(stderr, "\n%s: warning - %s\n", progname, msg);
  fflush(stderr);
}


void fatal(msg)
     char *msg;
{
  fflush(stdout);
  fprintf(stderr, "\n%s: fatal - %s\n", progname, msg);
  fprintf(stderr, "\n");
  exit(1);
}
