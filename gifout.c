/*
 * giflib/gifout.c
 * kirk johnson
 * may 1990
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

#include <stdio.h>
#include "port.h"
#include "gifint.h"
#include "kljcpyrt.h"


/****
 **
 ** local #defines
 **
 ****/

#define HASHSZ     (2048)
#define HASH(p, e) (((p)&(HASHSZ-1))^(e))

#define PUT_CODE(val)                       \
{                                           \
  work_data |= ((long) (val) << work_bits); \
  work_bits += code_size;                   \
  while (work_bits >= 8)                    \
  {                                         \
    PUT_BYTE(work_data & 0xFF);             \
    work_data >>= 8;                        \
    work_bits  -= 8;                        \
  }                                         \
}

#define PUT_BYTE(val)                 \
{                                     \
  buf[buf_idx++] = (val);             \
  if (buf_idx == 255)                 \
  {                                   \
    write_data_block(255, buf, outs); \
    buf_idx = 0;                      \
  }                                   \
}


/****
 **
 ** local variables
 **
 ****/

static int  cmap_bits _P((int));
static int  root_bits _P((int));
static void put_clr_code _P((void));
static void write_data_block _P((int, BYTE *, FILE *));
static void reset_string_out _P((void));
static void add_string_out _P((int, int));
static int  find_string_out _P((int, int));
static void gifout_fatal _P((const char *)) _noreturn;


static BYTE file_open  = 0;     /* status flags */
static BYTE image_open = 0;

static int rast_width;          /* raster width */
static int rast_height;         /* raster height */
static int ncolors;             /* number of colors */
static int img_width;           /* image width */
static int img_height;          /* image height */

static FILE *outs;              /* output file */

static int root_size;           /* root code size */
static int clr_code;            /* clear code */
static int eoi_code;            /* end of info code */
static int code_size;           /* current code size */
static int code_mask;           /* current code mask */
static int old_code;            /* previous code */

static long work_data;          /* working bits */
static int  work_bits;          /* working bit count */

static BYTE buf[256];           /* byte buffer */
static int  buf_idx;            /* buffer index */

static int table_size;          /* string table size */
static int htable[HASHSZ];
static int pref_extn[STAB_SIZE]; /* (prefix << 16) | extension */
static int next[STAB_SIZE];


/****
 **
 ** exported procedures
 **
 ****/

/*
 * open a GIF file for writing on stream s
 */
int gifout_open_file(s, w, h, sz, cmap, bg)
     FILE *s;
     int   w;                   /* raster width (in pixels) */
     int   h;                   /* raster height (in pixels) */
     int   sz;                  /* number of colors */
     BYTE  cmap[3][256];        /* global colormap */
     int   bg;                  /* background color index */
{
  int i;
  int pixel_bits;

  /* make sure there isn't already a file open */
  if (file_open)
    return GIFLIB_ERR_FAO;

  /* remember that we've got this file open */
  file_open   = 1;
  outs        = s;
  rast_width  = w;
  rast_height = h;

  /* write GIF signature */
  if (fwrite(GIF_SIG, sizeof(char), GIF_SIG_LEN, outs) != GIF_SIG_LEN)
    return GIFLIB_ERR_OUT;

  /* write screen descriptor */
  pixel_bits = cmap_bits(sz);
  ncolors    = 1 << pixel_bits;

  buf[0] = (w & 0x00FF);
  buf[1] = (w & 0xFF00) >> 8;
  buf[2] = (h & 0x00FF);
  buf[3] = (h & 0xFF00) >> 8;
  buf[4] = (pixel_bits - 1) | 0x80;
  buf[5] = bg;
  buf[6] = 0;

  if (fwrite(buf, sizeof(char), GIF_SD_SIZE, outs) != GIF_SD_SIZE)
    return GIFLIB_ERR_OUT;

  /* write (global) color map */
  for (i=0; i<sz; i++)
  {
    buf[GIFLIB_RED] = cmap[GIFLIB_RED][i];
    buf[GIFLIB_GRN] = cmap[GIFLIB_GRN][i];
    buf[GIFLIB_BLU] = cmap[GIFLIB_BLU][i];

    if (fwrite(buf, sizeof(BYTE), (unsigned) 3, outs) != 3)
      return GIFLIB_ERR_OUT;
  }

  for (i=sz; i<ncolors; i++)
  {
    buf[GIFLIB_RED] = 0;
    buf[GIFLIB_GRN] = 0;
    buf[GIFLIB_BLU] = 0;

    if (fwrite(buf, sizeof(BYTE), (unsigned) 3, outs) != 3)
      return GIFLIB_ERR_OUT;
  }

  /* done! */
  return GIFLIB_SUCCESS;
}


/*
 * open a new GIF image for writing in the current GIF file
 */
int gifout_open_image(left, top, w, h)
     int left;                  /* column index for left edge */
     int top;                   /* row index for top edge */
     int w;                     /* image width (in pixels) */
     int h;                     /* image height (in pixels) */
{
  /* make sure there's a file open */
  if (!file_open)
    return GIFLIB_ERR_NFO;

  /* make sure there isn't already an image open */
  if (image_open)
    return GIFLIB_ERR_IAO;

  /* remember that we've got this image open */
  image_open = 1;
  img_width  = w;
  img_height = h;

  /* write image separator */
  putc(GIF_SEPARATOR, outs);

  /* write image descriptor */
  buf[0] = (left & 0x00FF);
  buf[1] = (left & 0xFF00) >> 8;
  buf[2] = (top  & 0x00FF);
  buf[3] = (top  & 0xFF00) >> 8;
  buf[4] = (w & 0x00FF);
  buf[5] = (w & 0xFF00) >> 8;
  buf[6] = (h & 0x00FF);
  buf[7] = (h & 0xFF00) >> 8;
  buf[8] = 0;

  if (fwrite(buf, sizeof(BYTE), GIF_ID_SIZE, outs) != GIF_ID_SIZE)
    return GIFLIB_ERR_OUT;

  /* initialize raster data stream */
  root_size = root_bits(ncolors);
  putc(root_size, outs);

  clr_code  = 1 << root_size;
  eoi_code  = clr_code + 1;
  code_size = root_size + 1;
  code_mask = (1 << code_size) - 1;
  old_code  = NULL_CODE;

  work_bits = 0;
  work_data = 0;

  buf_idx = 0;

  /* initialize string table */
  reset_string_out();

  /* output initial clear code */
  put_clr_code();

  /* done! */
  return GIFLIB_SUCCESS;
}


/*
 * write a pixel into the current image
 */
void gifout_put_pixel(val)
     int val;                   /* pixel color index */
{
  int idx;

  /* see if string is in table already */
  idx = find_string_out(old_code, val);

  if (idx != NULL_CODE)
  {
    /* found a match */
    old_code = idx;
  }
  else
  {
    /* no match */
    PUT_CODE(old_code);
    add_string_out(old_code, val);
    old_code = val;

    /* check for full string table */
    if (table_size == STAB_SIZE)
    {
      /* output remaining code */
      PUT_CODE(old_code);

      /* reset encoder */
      put_clr_code();
    }
  }
}


/*
 * write a row of pixels into the current image
 */
void gifout_put_row(row)
     int *row;                  /* array of size img_width */
{
  int col;
  int idx;

  for (col=0; col<img_width; col++)
  {
    /* see if string is in table already */
    idx = find_string_out(old_code, row[col]);

    if (idx != NULL_CODE)
    {
      /* found a match */
      old_code = idx;
    }
    else
    {
      /* no match */
      PUT_CODE(old_code);
      add_string_out(old_code, row[col]);
      old_code = row[col];

      /* check for full string table */
      if (table_size == STAB_SIZE)
      {
        /* output remaining code */
        PUT_CODE(old_code);

        /* reset encoder */
        put_clr_code();
      }
    }
  }
}


/*
 * close an open GIF image
 */
int gifout_close_image()
{
  /* make sure there's an image open */
  if (!image_open)
    return GIFLIB_ERR_NIO;

  /* flush any remaining code */
  if (old_code != NULL_CODE)
    PUT_CODE(old_code);

  /* output end of info code */
  PUT_CODE(eoi_code);

  /* flush any extra bits */
  while (work_bits > 0)
  {
    PUT_BYTE(work_data & 0xFF);
    work_data >>= 8;
    work_bits  -= 8;
  }

  /* flush any extra bytes */
  if (buf_idx > 0)
    write_data_block(buf_idx, buf, outs);

  /* trailing zero byte */
  putc(0, outs);

  /* mark image as closed */
  image_open = 0;

  /* done! */
  return GIFLIB_SUCCESS;
}


/*
 * close an open GIF file
 */
int gifout_close_file()
{
  /* make sure there's a file open */
  if (!file_open)
    return GIFLIB_ERR_NFO;

  /* make sure there isn't an image open */
  if (image_open)
    return GIFLIB_ERR_ISO;

  /* write gif terminator */
  putc(GIF_TERMINATOR, outs);

  /* mark file as closed */
  file_open = 0;

  /* done! */
  return GIFLIB_SUCCESS;
}


/****
 **
 ** internal procedures
 **
 ****/

static int cmap_bits(n)
     int n;
{
  int nbits;

  if (n < 2)
    gifout_fatal("cmap_bits(): argument out of range");

  n    -= 1;
  nbits = 0;

  while (n != 0)
  {
    n    >>= 1;
    nbits += 1;
  }

  return nbits;
}


static int root_bits(n)
     int n;
{
  int rslt;

  rslt = cmap_bits(n);
  if (rslt < 2)
    rslt = 2;

  return rslt;
}


static void put_clr_code()
{
  /* output clear code */
  PUT_CODE(clr_code);

  /* reset raster data stream */
  code_size = root_size + 1;
  code_mask = (1 << code_size) - 1;
  old_code  = NULL_CODE;

  /* clear the string table */
  reset_string_out();
}


static void write_data_block(cnt, outbuf, s)
     int   cnt;
     BYTE *outbuf;
     FILE *s;
{
  putc(cnt, s);

  if (fwrite(outbuf, sizeof(BYTE), (unsigned) cnt, s) != cnt)
    gifout_fatal("write_data_block(): problems writing data block");
}


static void reset_string_out()
{
  int i;

  for (i=0; i<HASHSZ; i++)
    htable[i] = NULL_CODE;

  table_size = eoi_code + 1;
}


static void add_string_out(p, e)
     int p;
     int e;
{
  int idx;

  idx = HASH(p, e);

  pref_extn[table_size] = (p << 16) | e;
  next[table_size]      = htable[idx];
  htable[idx]           = table_size;

  if ((table_size > code_mask) && (code_size < 12))
  {
    code_size += 1;
    code_mask  = (1 << code_size) - 1;
  }

  table_size += 1;
}


static int find_string_out(p, e)
     int p;
     int e;
{
  int idx;
  int tmp;
  int rslt;

  if (p == NULL_CODE)
  {
    /* a lone symbol is always in table */
    rslt = e;
  }
  else
  {
    rslt = NULL_CODE;

    /* search the hash table */
    idx = htable[HASH(p, e)];
    tmp = (p << 16) | e;
    while (idx != NULL_CODE)
    {
      if (pref_extn[idx] == tmp)
      {
        rslt = idx;
        break;
      }
      else
      {
        idx = next[idx];
      }
    }
  }

  return rslt;
}


/*
 * semi-graceful fatal error mechanism
 */
static void gifout_fatal(msg)
     const char *msg;
{
  fprintf(stderr, "\n");
  fprintf(stderr, "gifout.c: fatal error\n");
  fprintf(stderr, "    %s\n", msg);
  exit(1);
}
