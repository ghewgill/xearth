/*
 * giflib/gifint.h
 * kirk johnson
 * november 1989
 *
 * RCS $Id: gifint.h,v 1.4 1994/05/20 01:37:40 tuna Exp $
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

#ifndef _GIFINT_H_
#define _GIFINT_H_

#include "giflib.h"
#include "kljcpyrt.h"

#define GIF_SIG      "GIF87a"
#define GIF_SIG_LEN  ((unsigned) 6) /* GIF signature length */
#define GIF_SD_SIZE  ((unsigned) 7) /* GIF screen descriptor size */
#define GIF_ID_SIZE  ((unsigned) 9) /* GIF image descriptor size */

#define GIF_SEPARATOR   ','     /* GIF image separator */
#define GIF_EXTENSION   '!'     /* GIF extension block marker */
#define GIF_TERMINATOR  ';'     /* GIF terminator */

#define STAB_SIZE  4096         /* string table size */
#define PSTK_SIZE  4096         /* pixel stack size */

#define NULL_CODE  -1           /* string table null code */

#endif
