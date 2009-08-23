#include <stdio.h>
#include <stdlib.h>

/*
 * Based on dewinfont by Simon Tatham
 * http://www.chiark.greenend.org.uk/~sgtatham/fonts/
 */

#define fromword(data,off) ((data)[off] | (data)[(off)+1] << 8)
#define fromdword(data,off) (fromword(data, off) | fromword(data, (off)+2) << 16)

struct Character {
    int width;
    unsigned char *data;
};

struct Character Chars[256];

void writeinc(int height, int maxwidth)
{
    int maxbytes;
    int i, j, k;

    printf("#define Font_height %d\n", height);
    maxbytes = (maxwidth + 7) / 8;
    printf("#define Font_maxbytes %d\n", maxbytes);
    printf("int Font_width[256] = {");
    for (i = 0; i < 256; i++) {
        if (i % 32 == 0) {
            printf("\n    ");
        }
        printf("%d,", Chars[i].width);
    }
    printf("\n};\n");
    printf("unsigned char Font_data[256][%d] = {\n", height * maxbytes);
    for (i = 0; i < 256; i++) {
        int widthbytes;

        printf("    {");
        widthbytes = (Chars[i].width + 7) / 8;
        for (j = 0; j < height; j++) {
            for (k = 0; k < widthbytes; k++) {
                printf("0x%02x,", Chars[i].data[j * widthbytes + k]);
            }
        }
        printf("},\n");
    }
    printf("};\n");
}

void dofnt(unsigned char *fnt, int fntsize)
{
    int version;
    int ftype;
    int off_facename;
    int height;
    int ctstart, ctsize;
    int maxwidth;
    int firstchar, lastchar;
    int i, j, k;

    version = fromword(fnt, 0);
    ftype = fromword(fnt, 0x42);
    if (ftype & 1) {
        fprintf(stderr, "This font is a vector font\n");
        exit(1);
    }
    off_facename = fromdword(fnt, 0x69);
    if (off_facename < 0 || off_facename > fntsize) {
        fprintf(stderr, "Face name not contained within font data\n");
        exit(1);
    }
    fprintf(stderr, "Face name: %s\n", fnt + off_facename);
    fprintf(stderr, "Copyright: %s\n", fnt + 0x6);
    fprintf(stderr, "Point size: %d\n", fromword(fnt, 0x44));
    height = fromword(fnt, 0x58);
    if (version == 0x200) {
        ctstart = 0x76;
        ctsize = 4;
    } else {
        ctstart = 0x94;
        ctsize = 6;
    }
    maxwidth = 0;
    for (i = 0; i < 256; i++) {
        Chars[i].width = 0;
        Chars[i].data = NULL;
    }
    firstchar = fnt[0x5f];
    lastchar = fnt[0x60];
    for (i = firstchar; i <= lastchar; i++) {
        int entry = ctstart + ctsize * (i - firstchar);
        int w = fromword(fnt, entry);
        int off;
        int widthbytes;

        Chars[i].width = w;
        if (w > maxwidth) {
            maxwidth = w;
        }
        if (ctsize == 4) {
            off = fromword(fnt, entry + 2);
        } else {
            off = fromdword(fnt, entry + 2);
        }
        widthbytes = (w + 7) / 8;
        Chars[i].data = malloc(height * widthbytes);
        for (j = 0; j < height; j++) {
            for (k = 0; k < widthbytes; k++) {
                int bytepos = off + k * height + j;
                Chars[i].data[j * widthbytes + k] = fnt[bytepos];
            }
        }
    }
    writeinc(height, maxwidth);
}

void nefon(unsigned char *fon, int fonsize, int neoff)
{
    int rtable;
    int shift;
    int p;

    rtable = neoff + fromword(fon, neoff + 0x24);
    shift = fromword(fon, rtable);
    p = rtable + 2;
    for (;;) {
        int rtype;
        int count;
        int i;

        rtype = fromword(fon, p);
        if (rtype == 0) {
            break;
        }
        count = fromword(fon, p + 2);
        p += 8;
        for (i = 0; i < count; i++) {
            int start = fromword(fon, p) << shift;
            int size = fromword(fon, p + 2) << shift;
            if (start < 0 || size < 0 || start + size > fonsize) {
                fprintf(stderr, "Resource overruns file boundaries\n");
                exit(1);
            }
            if (rtype == 0x8008) {
                fprintf(stderr, "Font at %d size %d\n", start, size);
                dofnt(fon + start, size);
            }
            p += 12;
        }
    }
}

void dofon(unsigned char *fon, int fonsize)
{
    int neoff;

    if (fon[0] != 'M' || fon[1] != 'Z') {
        fprintf(stderr, "MZ signature not found\n");
        exit(1);
    }
    neoff = fromdword(fon, 0x3c);
    if (fon[neoff] == 'N' && fon[neoff+1] == 'E') {
        nefon(fon, fonsize, neoff);
    } else if (fon[neoff] == 'P' && fon[neoff+1] == 'E' && fon[neoff+2] == 0 && fon[neoff+3] == 0) {
        fprintf(stderr, "PE fonts not supported\n");
        exit(1);
    } else {
        fprintf(stderr, "NE or PE signature not found\n");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    FILE *f;
    long size;
    unsigned char *data;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s fontfile.fon\n", argv[0]);
        return 1;
    }
    f = fopen(argv[1], "rb");
    if (f == NULL) {
        perror(argv[1]);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    data = malloc(size);
    fseek(f, 0, SEEK_SET);
    fread(data, 1, size, f);
    dofon(data, size);
    fclose(f);
    return 0;
}
