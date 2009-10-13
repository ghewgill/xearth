#include "xearth.h"

#include <gd.h>

#define ImageUnknown (0)
#define ImageGif     (1)
#define ImagePng     (2)
#define ImageJpeg    (3)

static int image_type _P((FILE *));

static gdImagePtr overlay;
static gdImagePtr clouds;
static int clouds_alpha;

void overlay_init()
{
    FILE *f;
    int x;

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
    if (cloudfile != NULL) {
        f = fopen(cloudfile, "rb");
        if (f != NULL) {
            switch (image_type(f)) {
            case ImageGif:
                clouds = gdImageCreateFromGif(f);
                break;
            case ImagePng:
                clouds = gdImageCreateFromPng(f);
                break;
            case ImageJpeg:
                clouds = gdImageCreateFromJpeg(f);
                break;
            default:
                fprintf(stderr, "xearth: warning: unknown image file format: %s\n", cloudfile);
                break;
            }
            fclose(f);
            for (x = 0; x < gdImageSX(clouds); x++) {
                if (gdImageAlpha(clouds, gdImageGetPixel(clouds, x, gdImageSY(clouds) / 2)) != 0) {
                    clouds_alpha = 1;
                    break;
                }
            }
        } else {
            fprintf(stderr, "xearth: warning: file not found: %s\n", cloudfile);
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

int cloud_pixel(double lat, double lon, int p)
{
    int x, y;

    if (clouds == NULL) {
        return p;
    }
    x = (int) ((lon + M_PI) * gdImageSX(clouds) / (2*M_PI));
    y = (int) (-lat * gdImageSY(clouds) / M_PI + gdImageSY(clouds)/2);
    /* handle minor rounding errors */
    if (x == -1) x++;
    if (x == gdImageSX(clouds)) x--;
    if (y == -1) y++;
    if (y == gdImageSY(clouds)) y--;
    if (x >= 0 && x < gdImageSX(clouds) && y >= 0 && y < gdImageSY(clouds)) {
        int c = gdImageGetPixel(clouds, x, y);
        int r = PixRed(p);
        int g = PixGreen(p);
        int b = PixBlue(p);
        if (clouds_alpha) {
            int a = gdImageAlpha(clouds, c);
            return PixRGB(
                a * r / 127 + (127 - a) * gdImageRed(clouds, c) / 127,
                a * g / 127 + (127 - a) * gdImageGreen(clouds, c) / 127,
                a * b / 127 + (127 - a) * gdImageBlue(clouds, c) / 127
            );
        } else {
            return PixRGB(
                r + gdImageRed(clouds, c) * (255 - r) / 255,
                g + gdImageGreen(clouds, c) * (255 - g) / 255,
                b + gdImageBlue(clouds, c) * (255 - b) / 255
            );
        }
    }
    return p;
}

void overlay_close()
{
    if (overlay != NULL) {
        gdImageDestroy(overlay);
    }
    overlay = NULL;
    if (clouds != NULL) {
        gdImageDestroy(clouds);
    }
    clouds = NULL;
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
