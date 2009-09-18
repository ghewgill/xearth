/*
 * bmp.c
 * greg hewgill
 * september 2009
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

static void bmp_setup _P((void));
static int  bmp_row _P((u_char *));
static void bmp_cleanup _P((void));

#pragma pack(push, 1)

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef long LONG;
typedef unsigned long DWORD;

#define BI_RGB (0)

struct __attribute__((__packed__)) BITMAPFILEHEADER {
    char bfType[2];
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
};

struct __attribute__((__packed__)) BITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
};

struct __attribute__((__packed__)) RGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
};

#pragma pack(pop)

static u16or32 *dith;


void bmp_output()
{
  compute_positions();
  scan_map();
  do_dots();
  bmp_setup();
  render(bmp_row);
  bmp_cleanup();
}


static void bmp_setup()
{
  int  i;

  if (num_colors > 256)
    fatal("number of colors must be <= 256 with BMP output");

  dither_setup(num_colors);
  dith = (u16or32 *) malloc((unsigned) sizeof(u16or32) * wdth);
  assert(dith != NULL);

  assert(sizeof(struct BITMAPFILEHEADER) == 14);
  assert(sizeof(struct BITMAPINFOHEADER) == 40);

  struct BITMAPFILEHEADER bmf;
  bmf.bfType[0] = 'B';
  bmf.bfType[1] = 'M';
  bmf.bfSize = sizeof(struct BITMAPFILEHEADER)
             + sizeof(struct BITMAPINFOHEADER)
             + num_colors * sizeof(struct RGBQUAD)
             + hght * 4 * ((wdth + 3) / 4);
  bmf.bfReserved1 = 0;
  bmf.bfReserved2 = 0;
  bmf.bfOffBits = sizeof(struct BITMAPFILEHEADER)
                + sizeof(struct BITMAPINFOHEADER)
                + num_colors * sizeof(struct RGBQUAD);
  fwrite(&bmf, 1, sizeof(bmf), stdout);

  struct BITMAPINFOHEADER bmi;
  bmi.biSize = sizeof(bmi);
  bmi.biWidth = wdth;
  bmi.biHeight = -hght;
  bmi.biPlanes = 1;
  bmi.biBitCount = 8;
  bmi.biCompression = BI_RGB;
  bmi.biSizeImage = hght * 4 * ((wdth + 3) / 4);
  bmi.biXPelsPerMeter = 0;
  bmi.biYPelsPerMeter = 0;
  bmi.biClrUsed = num_colors;
  bmi.biClrImportant = num_colors;
  fwrite(&bmi, 1, sizeof(bmi), stdout);

  for (i=0; i<dither_ncolors; i++)
  {
    struct RGBQUAD rgb;
    rgb.rgbBlue = dither_colormap[i*3+2];
    rgb.rgbGreen = dither_colormap[i*3+1];
    rgb.rgbRed = dither_colormap[i*3+0];
    rgb.rgbReserved = 0;
    fwrite(&rgb, 1, sizeof(rgb), stdout);
  }
}


static int bmp_row(row)
     u_char *row;
{
  int      i, i_lim;
  u16or32 *tmp;

  tmp = dith;
  dither_row(row, tmp);

  for (i = 0; i < 4 * ((wdth + 3) / 4); i++)
    fwrite(tmp+i, 1, 1, stdout);

  return 0;
}


static void bmp_cleanup()
{
  dither_cleanup();
  free(dith);
}
