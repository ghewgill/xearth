/*
 * giflib/giflib.h
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

#ifndef _GIFLIB_H_
#define _GIFLIB_H_

#include "kljcpyrt.h"

/*
 * giflib return codes
 */

#define GIFLIB_SUCCESS       0  /* success */
#define GIFLIB_DONE          1  /* no more images */

#define GIFLIB_ERR_BAD_SD   -1  /* bad screen descriptor */
#define GIFLIB_ERR_BAD_SEP  -2  /* bad image separator */
#define GIFLIB_ERR_BAD_SIG  -3  /* bad signature */
#define GIFLIB_ERR_EOD      -4  /* unexpected end of raster data */
#define GIFLIB_ERR_EOF      -5  /* unexpected end of input stream */
#define GIFLIB_ERR_OUT      -6  /* i/o output error */
#define GIFLIB_ERR_FAO      -7  /* file already open */
#define GIFLIB_ERR_IAO      -8  /* image already open */
#define GIFLIB_ERR_ISO      -9  /* image still open */
#define GIFLIB_ERR_NFO     -10  /* no file open */
#define GIFLIB_ERR_NIO     -11  /* no image open */

/*
 * colormap indices
 */

#define GIFLIB_RED  0
#define GIFLIB_GRN  1
#define GIFLIB_BLU  2

/*
 * typedef BYTE for convenience
 */

typedef unsigned char BYTE;

#ifdef USING_GIF_IN

/*
 * procedures exported by gifin.c
 */

extern int gifin_open_file();
extern int gifin_open_image();
extern int gifin_get_pixel();
extern int gifin_get_row();
extern int gifin_close_image();
extern int gifin_close_file();

/*
 * variables exported by gifin.c
 */

extern int  gifin_rast_width;       /* raster width */
extern int  gifin_rast_height;      /* raster height */
extern BYTE gifin_g_cmap_flag;      /* global colormap flag */
extern int  gifin_g_pixel_bits;     /* bits per pixel, global colormap */
extern int  gifin_g_ncolors;        /* number of colors, global colormap */
extern BYTE gifin_g_cmap[3][256];   /* global colormap */
extern int  gifin_bg_color;         /* background color index */
extern int  gifin_color_bits;       /* bits of color resolution */

extern int  gifin_img_left;         /* image position on raster */
extern int  gifin_img_top;          /* image position on raster */
extern int  gifin_img_width;        /* image width */
extern int  gifin_img_height;       /* image height */
extern BYTE gifin_l_cmap_flag;      /* local colormap flag */
extern int  gifin_l_pixel_bits;     /* bits per pixel, local colormap */
extern int  gifin_l_ncolors;        /* number of colors, local colormap */
extern BYTE gifin_l_cmap[3][256];   /* local colormap */
extern BYTE gifin_interlace_flag;   /* interlace image format flag */

#endif /* USING_GIF_IN */

/*
 * procedures exported by gifout.c
 */

extern int  gifout_open_file _P((FILE *, int, int, int, BYTE [3][256], int));
extern int  gifout_open_image _P((int, int, int, int));
extern void gifout_put_pixel _P((int));
extern void gifout_put_row _P((int *));
extern int  gifout_close_image _P((void));
extern int  gifout_close_file _P((void));

#endif
