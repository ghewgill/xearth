/*
 * ppm.c
 * kirk johnson
 * july 1993
 *
 * RCS $Id: ppm.c,v 1.4 1994/05/20 01:37:40 tuna Exp $
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

static FILE    *outs;
static unsigned bytes_per_row;


void ppm_setup(s)
     FILE *s;
{
  outs          = s;
  bytes_per_row = wdth * 3;

  fprintf(outs, "P6\n%d %d\n255\n", wdth, hght);
}


void ppm_row(row)
     u_char *row;
{
  assert(fwrite(row, 1, bytes_per_row, outs) == bytes_per_row);
}
