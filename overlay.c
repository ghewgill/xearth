#include <gd.h>

#include "xearth.h"

static gdImagePtr overlay;

void overlay_init()
{
    FILE *f = fopen("TRKWLDUS.GIF", "rb");
    overlay = gdImageCreateFromGif(f);
    fclose(f);

    {
        /*int lat, lon;
        char buf[20];
        int font = gdFontGetSmall();
        int c = gdImageColorAllocate(overlay, 255, 255, 255);
        for (lat = -90; lat <= 90; lat += 15) {
            for (lon = -180; lon < 180; lon += 30) {
                fprintf(stderr, "lat=%d, lon=%d\n", lat, lon);
                double rlat = lat * (M_PI/180);
                double rlon = lon * (M_PI/180);
                int x = (int) ((rlon + M_PI) * gdImageSX(overlay) / (2*M_PI));
                int y = (int) (-rlat * gdImageSY(overlay) / M_PI * 1.4 + gdImageSY(overlay)/2);
                fprintf(stderr, "x=%d, y=%d\n", x, y);
                gdImageSetPixel(overlay, x, y, c);
                sprintf(buf, "%d,%d", lat, lon);
                gdImageString(overlay, font, x, y, buf, c);
            }
        }*/
        f = fopen("world.gif", "wb");
        gdImageGif(overlay, f);
        fclose(f);
    }
}

int overlay_pixel(double lat, double lon)
{
    int x = (int) ((lon + M_PI) * gdImageSX(overlay) / (2*M_PI));
    int y = (int) (-lat * gdImageSY(overlay) / M_PI * 1.4 + gdImageSY(overlay)/2);
    return gdImageGetPixel(overlay, x, y);
}

void overlay_close()
{
    gdImageDestroy(overlay);
}
