#include "xearth.h"

#include <gd.h>

#define ImageUnknown (0)
#define ImageGif     (1)
#define ImagePng     (2)
#define ImageJpeg    (3)

static int image_type _P((FILE *));

static gdImagePtr overlay;

void overlay_init()
{
    FILE *f;

    if (overlayfile != NULL) {
        f = fopen(overlayfile, "rb");
        if (f != NULL) {
            switch (image_type(f)) {
            case ImageGif:
                overlay = gdImageCreateFromGif(f);
                break;
            case ImagePng:
                overlay = gdImageCreateFromPng(f);
                break;
            case ImageJpeg:
                overlay = gdImageCreateFromJpeg(f);
                break;
            default:
                fprintf(stderr, "xearth: warning: unknown image file format: %s\n", overlayfile);
                overlay = NULL;
                break;
            }
            fclose(f);
        } else {
            fprintf(stderr, "xearth: warning: file not found: %s\n", overlayfile);
        }
    }
}

int overlay_pixel(double lat, double lon)
{
    int x, y;
    double r;
    int c;

    if (overlay == NULL) {
        return -1;
    }
    x = (int) ((lon + M_PI) * gdImageSX(overlay) / (2*M_PI));
    r = 1.0; /* gdImageSX(overlay) / (gdImageSY(overlay) * 2.0); */
    y = (int) (-lat * gdImageSY(overlay) / M_PI * r + gdImageSY(overlay)/2);
    /* handle minor rounding errors */
    if (x == -1) x++;
    if (x == gdImageSX(overlay)) x--;
    if (y == -1) y++;
    if (y == gdImageSY(overlay)) y--;
    if (x < 0 || x >= gdImageSX(overlay) || y < 0 || y >= gdImageSY(overlay)) {
        return -1;
    }
    c = gdImageGetPixel(overlay, x, y);
    return PixRGB(gdImageRed(overlay, c), gdImageGreen(overlay, c), gdImageBlue(overlay, c));
}

void overlay_close()
{
    if (overlay != NULL) {
        gdImageDestroy(overlay);
    }
    overlay = NULL;
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
