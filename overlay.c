#include "xearth.h"

#include <gd.h>

#define ImageUnknown (0)
#define ImageGif     (1)
#define ImagePng     (2)
#define ImageJpeg    (3)

static int image_type _P((FILE *));

static gdImagePtr map;
static gdImagePtr overlay[MAX_OVERLAY];
static int overlay_alpha[MAX_OVERLAY];

void overlay_init()
{
    FILE *f;
    int i;
    int x;

    if (mapfile != NULL) {
        f = fopen(mapfile, "rb");
        if (f != NULL) {
            switch (image_type(f)) {
            case ImageGif:
                map = gdImageCreateFromGif(f);
                break;
            case ImagePng:
                map = gdImageCreateFromPng(f);
                break;
            case ImageJpeg:
                map = gdImageCreateFromJpeg(f);
                break;
            default:
                fprintf(stderr, "xearth: warning: unknown image file format: %s\n", mapfile);
                map = NULL;
                break;
            }
            fclose(f);
        } else {
            fprintf(stderr, "xearth: warning: file not found: %s\n", mapfile);
        }
    }
    for (i = 0; i < overlay_count; i++) {
        f = fopen(overlayfile[i], "rb");
        if (f != NULL) {
            switch (image_type(f)) {
            case ImageGif:
                overlay[i] = gdImageCreateFromGif(f);
                break;
            case ImagePng:
                overlay[i] = gdImageCreateFromPng(f);
                break;
            case ImageJpeg:
                overlay[i] = gdImageCreateFromJpeg(f);
                break;
            default:
                fprintf(stderr, "xearth: warning: unknown image file format: %s\n", overlayfile[i]);
                break;
            }
            fclose(f);
            overlay_alpha[i] = 0;
            for (x = 0; x < gdImageSX(overlay[i]); x++) {
                if (gdImageAlpha(overlay[i], gdImageGetPixel(overlay[i], x, gdImageSY(overlay[i]) / 2)) != 0) {
                    overlay_alpha[i] = 1;
                    break;
                }
            }
        } else {
            fprintf(stderr, "xearth: warning: file not found: %s\n", overlayfile[i]);
        }
    }
}

int map_pixel(double lat, double lon)
{
    int x, y;
    double r;
    int c;

    if (map == NULL) {
        return -1;
    }
    x = (int) ((lon + M_PI) * gdImageSX(map) / (2*M_PI));
    r = 1.0; /* gdImageSX(map) / (gdImageSY(map) * 2.0); */
    y = (int) (-lat * gdImageSY(map) / M_PI * r + gdImageSY(map)/2);
    /* handle minor rounding errors */
    if (x == -1) x++;
    if (x == gdImageSX(map)) x--;
    if (y == -1) y++;
    if (y == gdImageSY(map)) y--;
    if (x < 0 || x >= gdImageSX(map) || y < 0 || y >= gdImageSY(map)) {
        return -1;
    }
    c = gdImageGetPixel(map, x, y);
    return PixRGB(gdImageRed(map, c), gdImageGreen(map, c), gdImageBlue(map, c));
}

int overlay_pixel(double lat, double lon, int p)
{
    int i, x, y;
    gdImagePtr ov;

    for (i = 0; i < overlay_count; i++) {
        ov = overlay[i];
        if (ov == NULL) {
            continue;
        }
        x = (int) ((lon + M_PI) * gdImageSX(ov) / (2*M_PI));
        y = (int) (-lat * gdImageSY(ov) / M_PI + gdImageSY(ov)/2);
        /* handle minor rounding errors */
        if (x == -1) x++;
        if (x == gdImageSX(ov)) x--;
        if (y == -1) y++;
        if (y == gdImageSY(ov)) y--;
        if (x >= 0 && x < gdImageSX(ov) && y >= 0 && y < gdImageSY(ov)) {
            int c = gdImageGetPixel(ov, x, y);
            int r = PixRed(p);
            int g = PixGreen(p);
            int b = PixBlue(p);
            if (overlay_alpha[i]) {
                int a = gdImageAlpha(ov, c);
                p = PixRGB(
                    a * r / 127 + (127 - a) * gdImageRed(ov, c) / 127,
                    a * g / 127 + (127 - a) * gdImageGreen(ov, c) / 127,
                    a * b / 127 + (127 - a) * gdImageBlue(ov, c) / 127
                );
            } else {
                p = PixRGB(
                    r + gdImageRed(ov, c) * (255 - r) / 255,
                    g + gdImageGreen(ov, c) * (255 - g) / 255,
                    b + gdImageBlue(ov, c) * (255 - b) / 255
                );
            }
        }
    }
    return p;
}

void overlay_close()
{
    int i;

    if (map != NULL) {
        gdImageDestroy(map);
    }
    map = NULL;
    for (i = 0; i < overlay_count; i++) {
        if (overlay[i] != NULL) {
            gdImageDestroy(overlay[i]);
        }
        overlay[i] = NULL;
    }
}

static int image_type(f)
    FILE *f;
{
  int r = ImageUnknown;
  u_char buf[8];

  fread(buf, 1, sizeof(buf), f);
  if (memcmp(buf, "GIF8", 4) == 0)
  {
    r = ImageGif;
  }
  else if (memcmp(buf, "\x89PNG", 4) == 0)
  {
    r = ImagePng;
  }
  else if (memcmp(buf, "\xff\xd8", 2) == 0)
  {
    r = ImageJpeg;
  }
  fseek(f, 0, SEEK_SET);

  return r;
}
