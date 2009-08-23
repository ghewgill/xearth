#include "xearth.h"

#include "font.inc"

void font_extent(s, height, width)
    const char *s;
    int *height;
    int *width;
{
    *height = Font_height;
    *width = 0;
    while (*s) {
        *width += Font_width[(unsigned char) *s];
        s++;
    }
}


void font_draw(x, y, s, dots)
    int x;
    int y;
    const char *s;
    ExtArr dots;
{
    int cx, cy, w;
    unsigned char c;
    ScanDot *new;

    while (*s) {
        c = *s++;
        w = (Font_width[c] + 7) / 8;
        for (cy = 0; cy < Font_height; cy++) {
            for (cx = 0; cx < Font_width[c]; cx++) {
                if (Font_data[c][cy * w + (cx / 8)] & (0x80 >> (cx % 8))) {
                    new = (ScanDot *) extarr_next(dots);
                    new->x    = x + cx;
                    new->y    = y + cy;
                    new->type = DotTypeStar;
                }
            }
        }
        x += Font_width[c];
    }
}
