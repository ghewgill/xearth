/*
 * markers.c
 * kirk johnson
 * august 1995
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

#define BuiltInName "built-in"
#define MaxLineLen  (1024)

static void        builtin_marker_info _P((void));
static void        user_marker_info _P((char *));
static void        process_line _P((char *, const char *, int));
static const char *decode_marker_info _P((int, char **));
static const char *decode_extra_marker_info _P((int, char **, MarkerInfo *));
static char       *get_line _P((FILE *, char *, int, const char **));

MarkerInfo   *marker_info;
static ExtArr info = NULL;

/* builtin_marker_data[] contains the "built-in" marker data that is
 * compiled into xearth. (My apologies for misspellings, omissions of
 * your favorite location, or geographic inaccuracies. This is
 * primarily just a pile of data that I had handy instead of an
 * attempt to provide a sample that is "globally correct" in some
 * sense.)
 */

static const char *builtin_marker_data[] =
{
  "  61.17 -150.00 \"Anchorage\"           # Alaska, USA           ",
  "  38.00   23.73 \"Athens\"              # Greece                ",
  "  33.4    44.4  \"Baghdad\"             # Iraq                  ",
  "  13.73  100.50 \"Bangkok\"             # Thailand              ",
  "  39.92  116.43 \"Beijing\"             # China                 ",
  "  52.53   13.42 \"Berlin\"              # Germany               ",
  "  32.3   -64.7  \"Bermuda\"             # Bermuda               ",
  "  42.33  -71.08 \"Boston\"              # Massachusetts, USA    ",
  " -15.8   -47.9  \"Brasilia\"            # Brazil                ",
  "  -4.2    15.3  \"Brazzaville\"         # Congo                 ",
  " -34.67  -58.50 \"Buenos Aires\"        # Argentina             ",
  "  31.05   31.25 \"Cairo\"               # Egypt                 ",
  "  22.5    88.3  \"Calcutta\"            # India                 ",
  " -33.93   18.47 \"Cape Town\"           # South Africa          ",
  "  33.6    -7.6  \"Casablanca\"          # Morocco (Rabat?)      ",
  "  41.83  -87.75 \"Chicago\"             # Illinois, USA         ",
  "  32.78  -96.80 \"Dallas\"              # Texas, USA            ",
  "  28.63   77.20 \"New Delhi\"           # India                 ",
  "  39.75 -105.00 \"Denver\"              # Colorado, USA         ",
  "  24.23   55.28 \"Dubai\"               # UAE (Abu Dhabi?)      ",
  " -27.1  -109.4  \"Easter Island\"       # Easter Island         ",
  " -18.0   178.1  \"Fiji\"                # Fiji                  ",
  "  13.5   144.8  \"Guam\"                # Guam                  ",
  "  60.13   25.00 \"Helsinki\"            # Finland               ",
  "  22.2   114.1  \"Hong Kong\"           # Hong Kong             ",
  "  21.32 -157.83 \"Honolulu\"            # Hawaii, USA           ",
  "  52.2   104.3  \"Irkutsk\"             # Irkutsk, Russia       ",
  "  41.0    29.0  \"Istanbul\"            # Turkey (Ankara?)      ",
  "  -6.13  106.75 \"Jakarta\"             # Indonesia             ",
  "  31.8    35.2  \"Jerusalem\"           # Israel                ",
  "  34.5    69.2  \"Kabul\"               # Afghanistan           ",
  "  27.7    85.3  \"Kathmandu\"           # Nepal                 ",
  "  50.4    30.5  \"Kiev\"                # Ukraine               ",
  "   3.13  101.70 \"Kuala Lumpur\"        # Malaysia              ",
  "   6.45    3.47 \"Lagos\"               # Nigeria               ",
  " -12.10  -77.05 \"Lima\"                # Peru                  ",
  "  51.50   -0.17 \"London\"              # United Kingdom        ",
  "  40.42   -3.72 \"Madrid\"              # Spain                 ",
  "  14.6   121.0  \"Manila\"              # The Phillipines       ",
  "  21.5    39.8  \"Mecca\"               # Saudi Arabia          ",
  "  19.4   -99.1  \"Mexico City\"         # Mexico                ",
  "  25.8   -80.2  \"Miami\"               # Florida, USA          ",
  "   6.2   -10.8  \"Monrovia\"            # Liberia               ",
  "  45.5   -73.5  \"Montreal\"            # Quebec, Canada        ",
  "  55.75   37.70 \"Moscow\"              # Russia                ",
  "  -1.28   36.83 \"Nairobi\"             # Kenya                 ",
  "  59.93   10.75 \"Oslo\"                # Norway                ",
  "  48.87    2.33 \"Paris\"               # France                ",
  " -32.0   115.9  \"Perth\"               # Australia             ",
  "  45.5  -122.5  \"Portland\"            # Oregon, USA           ",
  "  -0.2   -78.5  \"Quito\"               # Ecuador               ",
  "  64.15  -21.97 \"Reykjavik\"           # Iceland               ",
  " -22.88  -43.28 \"Rio de Janeiro\"      # Brazil                ",
  "  41.88   12.50 \"Rome\"                # Italy                 ",
  "  11.0   106.7  \"Ho Chi Minh City\"    # Vietnam (Hanoi?)      ",
  "  37.75 -122.45 \"San Francisco\"       # California, USA       ",
  "   9.98  -84.07 \"San Jose\"            # Costa Rica            ",
  "  18.5   -66.1  \"San Juan\"            # Puerto Rico           ",
  " -33.5   -70.7  \"Santiago\"            # Chile                 ",
  "   1.2   103.9  \"Singapore\"           # Singapore             ",
  "  42.67   23.30 \"Sofia\"               # Bulgaria              ",
  "  59.33   18.08 \"Stockholm\"           # Sweden                ",
  " -33.92  151.17 \"Sydney\"              # Australia             ",
  " -17.6  -149.5  \"Tahiti\"              # Tahiti                ",
  "  16.8    -3.0  \"Timbuktu\"            # Mali (Bamako?)        ",
  "  35.67  139.75 \"Tokyo\"               # Japan                 ",
  "  43.70  -79.42 \"Toronto\"             # Ontario, Canada       ",
  "  32.9    13.2  \"Tripoli\"             # Libya                 ",
  "  47.9   106.9  \"Ulan Bator\"          # Mongolia              ",
  "  49.22 -123.10 \"Vancouver\"           # B.C., Canada          ",
  "  48.22   16.37 \"Vienna\"              # Austria               ",
  "  38.9   -77.0  \"Washington\"          # United States         ",
  " -41.28  174.78 \"Wellington\"          # New Zealand           ",
  "  62.5  -114.3  \"Yellowknife\"         # N.T., Canada          ",
  "  90.00    0.00 \"North Pole\"          # North Pole            ",
  " -90.00    0.00 \"South Pole\"          # South Pole            ",
  NULL
};


void load_marker_info(fname)
     char *fname;
{
  int         i;
  MarkerInfo *new;

  if (info == NULL)
  {
    /* first time through, allocate a new extarr
     */
    info = extarr_alloc(sizeof(MarkerInfo));
  }
  else
  {
    /* on subsequent passes, just clean it up for reuse
     */
    for (i=0; i<(info->count-1); i++)
      free(((MarkerInfo *) info->body)[i].label);
    info->count = 0;
  }

  if (strcmp(fname, BuiltInName) == 0)
    builtin_marker_info();
  else
    user_marker_info(fname);

  new = (MarkerInfo *) extarr_next(info);
  new->lat   = 0;
  new->lon   = 0;
  new->label = NULL;

  marker_info = (MarkerInfo *) info->body;
}


static void builtin_marker_info()
{
  int   idx;
  char *buf;

  buf = (char *) malloc(MaxLineLen);
  assert(buf != NULL);

  idx = 0;
  while (builtin_marker_data[idx] != NULL)
  {
    strcpy(buf, builtin_marker_data[idx]);
    idx += 1;
    process_line(buf, BuiltInName, idx);
  }

  free(buf);
}


static void user_marker_info(fname)
     char *fname;
{
  int         idx;
  char       *buf;
  const char *msg;
  FILE       *ins;

  ins = fopen(fname, "r");
  if (ins == NULL)
  {
    warning("unable to open marker info file");
    return;
  }

  idx = 1;
  buf = (char *) malloc(MaxLineLen);
  assert(buf != NULL);

  while (get_line(ins, buf, MaxLineLen, &msg) != NULL)
  {
    if (msg != NULL)
    {
      fflush(stdout);
      fprintf(stderr, "%s, line %d: %s\n", fname, idx, msg);
      fprintf(stderr, " (length = %d)\n", (int) strlen(buf));
      fflush(stderr);
    }

    process_line(buf, fname, idx);
    idx += 1;
  }

  free(buf);
  fclose(ins);
}


static void process_line(buf, fname, idx)
     char       *buf;
     const char *fname;
     int         idx;
{
  int         tokc;
  char      **tokv;
  const char *msg;

  tokv = tokenize(buf, &tokc, &msg);
  if (msg != NULL)
  {
    fflush(stdout);
    fprintf(stderr, "%s, line %d: %s\n", fname, idx, msg);
    fflush(stderr);
  }

  msg = decode_marker_info(tokc, tokv);
  if (msg != NULL)
  {
    fflush(stdout);
    fprintf(stderr, "%s, line %d: %s\n", fname, idx, msg);
    fflush(stderr);
  }

  free(tokv);
}



static const char *decode_marker_info(tokc, tokv)
     int    tokc;
     char **tokv;
{
  double      lat;
  double      lon;
  char       *label;
  MarkerInfo *new;
  const char *rslt;

  if (tokc == 0)
  {
    /* blank or comment line, no problem
     */
    rslt = NULL;
  }
  else if (tokc < 3)
  {
    /* less than three tokens is a problem
     */
    rslt = "syntax error in required info";
  }
  else
  {
    /* decode first three tokens
     */
    sscanf(tokv[0], "%lf", &lat);
    sscanf(tokv[1], "%lf", &lon);
    label = (char *) malloc(strlen(tokv[2]) + 1);
    assert(label != NULL);
    strcpy(label, tokv[2]);

    if ((lat > 90) || (lat < -90))
    {
      rslt = "latitude must be between -90 and 90";
    }
    else if ((lon > 180) || (lon < -180))
    {
      rslt = "longitude must be between -180 and 180";
    }
    else
    {
      new = (MarkerInfo *) extarr_next(info);
      new->lat   = lat;
      new->lon   = lon;
      new->label = label;

      /* decode any extra tokens
       */
      rslt = decode_extra_marker_info(tokc-3, tokv+3, new);
    }
  }

  return rslt;
}


static const char *decode_extra_marker_info(tokc, tokv, inf)
     int        tokc;
     char     **tokv;
     MarkerInfo *inf;
{
  int   i;
  char *key;
  char *val;

  /* set defaults
   */
  inf->align = MarkerAlignDefault;

  for (i=0; i<tokc; i++)
  {
    /* look for '=' separator; quit gracefully if none found, else
     * change it to a NUL and set key and val appropriately
     */
    val = tokv[i];
    while ((*val != '=') && (*val != '\0'))
      val += 1;

    if (*val == '\0')
      return "syntax error in optional info";

    key  = tokv[i];
    *val = '\0';
    val += 1;

    /* big decoding switch
     */
    if (strcmp(key, "align") == 0)
    {
      if (strcmp(val, "left") == 0)
        inf->align = MarkerAlignLeft;
      else if (strcmp(val, "right") == 0)
        inf->align = MarkerAlignRight;
      else if (strcmp(val, "above") == 0)
        inf->align = MarkerAlignAbove;
      else if (strcmp(val, "below") == 0)
        inf->align = MarkerAlignBelow;
      else
        return "unrecognized alignment value";
    }
    else
    {
      return "unrecognized key in optional info";
    }
  }

  return NULL;
}


static char *get_line(ins, buf, len, msg)
     FILE        *ins;
     char        *buf;
     int          len;
     const char **msg;
{
  int   ch;
  char *rslt;

  *msg = NULL;
  rslt = buf;

  while (1)
  {
    ch = getc(ins);

    if (ch == EOF)
    {
      if (buf == rslt)
      {
        /* if EOF is the first thing we read on a line,
         * there aren't any more lines to read
         */
        rslt = NULL;
        break;
      }
      else
      {
        /* line terminated by EOF
         */
        *buf = '\0';
        break;
      }
    }
    else if (ch == '\n')
    {
      /* line terminated by newline
       */
      *buf = '\0';
      break;
    }
    else
    {
      *buf = ch;
      buf += 1;
      len -= 1;

      if (len == 0)
      {
        /* line terminated by exceeding maximum length
         */
        *msg = "line exceeds maximum length";
        *(buf-1) = '\0';
        do
          ch = getc(ins);
        while ((ch != EOF) && (ch != '\n'));
        break;
      }
    }
  }

  return rslt;
}


void show_marker_info(fname)
     char *fname;
{
  MarkerInfo *minfo;

  printf("# xearth marker data from \"%s\":\n", fname);

  load_marker_info(fname);

  minfo = marker_info;
  while (minfo->label != NULL)
  {
    printf("%7.2f %8.2f \"%s\"", minfo->lat, minfo->lon, minfo->label);

    if (minfo->align == MarkerAlignLeft)
      printf(" align=left");
    else if (minfo->align == MarkerAlignRight)
      printf(" align=right");
    else if (minfo->align == MarkerAlignAbove)
      printf(" align=above");
    else if (minfo->align == MarkerAlignBelow)
      printf(" align=below");
    else
      assert(minfo->align == MarkerAlignDefault);

    printf("\n");
    minfo += 1;
  }

  exit(0);
}
