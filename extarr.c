/*
 * extarr.c
 * kirk johnson
 * july 1993
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


ExtArr extarr_alloc(eltsize)
     unsigned eltsize;
{
  ExtArr rslt;

  rslt = (ExtArr) malloc((unsigned) sizeof(struct extarr));
  assert(rslt != NULL);

  rslt->eltsize = eltsize;
  rslt->limit   = 1;
  rslt->count   = 0;

  rslt->body = (void *) malloc((unsigned) eltsize*rslt->limit);
  assert(rslt->body != NULL);

  return rslt;
}


void extarr_free(x)
     ExtArr x;
{
  free(x->body);
  free(x);
}


void *extarr_next(x)
     ExtArr x;
{
  unsigned eltsize;
  unsigned limit;
  unsigned count;
  void    *body;
  void    *rslt;

  eltsize = x->eltsize;
  limit   = x->limit;
  count   = x->count;
  body    = x->body;

  if (count == limit)
  {
    limit *= 2;
    body   = (void *) realloc(body, (unsigned) eltsize*limit);
    assert(body != NULL);

    x->limit = limit;
    x->body  = body;
  }

  rslt = (void *) ((char *) body + (count * eltsize));
  x->count = count + 1;

  return rslt;
}
