#include "icc_writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ── Big-endian write helpers ─────────────────────────────────────── */
static void write_u8 (FILE *f, uint8_t  v) { fwrite(&v, 1, 1, f); }
static void write_u16(FILE *f, uint16_t v) {
    uint8_t b[2] = {v>>8, v&0xff}; fwrite(b, 1, 2, f);
}
static void write_u32(FILE *f, uint32_t v) {
    uint8_t b[4] = {v>>24,(v>>16)&0xff,(v>>8)&0xff,v&0xff}; fwrite(b, 1, 4, f);
}
static void write_4cc(FILE *f, const char s[4]) { fwrite(s, 1, 4, f); }
static void write_zeros(FILE *f, int n) {
    uint8_t z = 0; for (int i=0;i<n;i++) fwrite(&z,1,1,f);
}
/* s15Fixed16 (ICC standard fixed-point) */
static void write_s15f16(FILE *f, double v) {
    write_u32(f, (uint32_t)(int32_t)(v * 65536.0 + 0.5));
}
static void pad4(FILE *f) {
    long p = ftell(f); int r = (int)(p%4); if (r) write_zeros(f, 4-r);
}

/* ── Tag writers — return byte size of tag data written ───────────── */

static uint32_t tag_desc(FILE *f, const char *ascii) {
    long s = ftell(f);
    uint32_t len = (uint32_t)strlen(ascii) + 1;
    write_4cc(f,"desc"); write_zeros(f,4);
    write_u32(f,len);
    fwrite(ascii,1,len,f);
    write_u32(f,0); write_u32(f,0);
    write_u16(f,0); write_u8(f,0); write_zeros(f,67);
    return (uint32_t)(ftell(f)-s);
}

static uint32_t tag_text(FILE *f, const char *text) {
    long s = ftell(f);
    write_4cc(f,"text"); write_zeros(f,4);
    fwrite(text,1,strlen(text)+1,f);
    return (uint32_t)(ftell(f)-s);
}

static uint32_t tag_xyz(FILE *f, double x, double y, double z) {
    long s = ftell(f);
    write_4cc(f,"XYZ "); write_zeros(f,4);
    write_s15f16(f,x); write_s15f16(f,y); write_s15f16(f,z);
    return (uint32_t)(ftell(f)-s);
}

/* Parametric curve: type 0 (simple gamma) */
static uint32_t tag_para_gamma(FILE *f, double gamma) {
    long s = ftell(f);
    write_4cc(f,"para"); write_zeros(f,4);
    write_u16(f,0); write_u16(f,0);        /* type=0, reserved */
    write_s15f16(f, gamma);
    return (uint32_t)(ftell(f)-s);
}

/* ── mft2 (lut16Type) B2A ─────────────────────────────────────────── */
/*
 * B2A0 / B2A1 map PCS XYZ D50 → device RGB codes.
 * Pipeline: identity input tables → CLUT → identity output tables.
 * CLUT: XYZ → inverse-sRGB-matrix → graded-LUT lookup → sRGB encode.
 * Z is the last PCS channel and varies fastest in the CLUT (ICC spec:
 * "the last input channel changes most rapidly"). X varies slowest.
 */

static float clampf01(float v) { return v<0.0f?0.0f:(v>1.0f?1.0f:v); }

static float srgb_enc(float v) {
    if (v <= 0.0031308f) return 12.92f * v;
    return 1.055f * powf(v, 1.0f/2.4f) - 0.055f;
}

/* XYZ D50 → linear sRGB (exact inverse of the forward matrix below) */
static void xyz_d50_to_linear_rgb(float X, float Y, float Z,
                                   float *r, float *g, float *b) {
    *r =  3.1338561f*X - 1.6168667f*Y - 0.4906146f*Z;
    *g = -0.9787684f*X + 1.9161415f*Y + 0.0334540f*Z;
    *b =  0.0719453f*X - 0.2289914f*Y + 1.4052427f*Z;
}

/* Trilinear interpolation in the baked LUT (B-slowest, R-fastest layout). */
static void lut_sample(const float *lut, int gs,
                        float r, float g, float b,
                        float *or_, float *og, float *ob) {
    float rf = clampf01(r) * (float)(gs - 1);
    float gf = clampf01(g) * (float)(gs - 1);
    float bf = clampf01(b) * (float)(gs - 1);
    int ri = (int)rf; if (ri > gs-2) ri = gs-2;
    int gi = (int)gf; if (gi > gs-2) gi = gs-2;
    int bi = (int)bf; if (bi > gs-2) bi = gs-2;
    float rt = rf - ri, gt = gf - gi, bt = bf - bi;
    float c[3];
    for (int ch = 0; ch < 3; ch++) {
#define L(rr,gg,bb) lut[((bb)*gs*gs+(gg)*gs+(rr))*3+(ch)]
        float c00 = L(ri,  gi,   bi  )*(1-rt) + L(ri+1, gi,   bi  )*rt;
        float c10 = L(ri,  gi+1, bi  )*(1-rt) + L(ri+1, gi+1, bi  )*rt;
        float c01 = L(ri,  gi,   bi+1)*(1-rt) + L(ri+1, gi,   bi+1)*rt;
        float c11 = L(ri,  gi+1, bi+1)*(1-rt) + L(ri+1, gi+1, bi+1)*rt;
        c[ch] = (c00*(1-gt)+c10*gt)*(1-bt) + (c01*(1-gt)+c11*gt)*bt;
#undef L
    }
    *or_ = c[0]; *og = c[1]; *ob = c[2];
}

static uint32_t tag_mft2_b2a(FILE *f, const float *lut, int size) {
    long s = ftell(f);
    write_4cc(f,"mft2"); write_zeros(f,4);
    write_u8(f,3); write_u8(f,3);          /* in/out channels */
    write_u8(f,(uint8_t)size);             /* CLUT grid points */
    write_u8(f,0);                          /* padding */

    /* identity e-matrix (s15.16) */
    write_s15f16(f,1); write_s15f16(f,0); write_s15f16(f,0);
    write_s15f16(f,0); write_s15f16(f,1); write_s15f16(f,0);
    write_s15f16(f,0); write_s15f16(f,0); write_s15f16(f,1);

    write_u16(f,256);                       /* input table entries */
    write_u16(f,256);                       /* output table entries */

    /* Input tables: identity — XYZ PCS 0..65535 passes through unchanged */
    for (int c = 0; c < 3; c++)
        for (int i = 0; i < 256; i++)
            write_u16(f, (uint16_t)((i * 65535 + 127) / 255));

    /* CLUT: X varies slowest (first PCS channel), Z varies fastest (last).
     * ICC v2 lut16Type PCS: 65535 = 1+32767/32768 ≈ 2.0, so 1.0 = 32768.
     * Scale each grid coordinate so the grid spans the real XYZ range. */
    const float pcs_scale = 65535.0f / 32768.0f;   /* ≈ 1.99997 */
    for (int xi = 0; xi < size; xi++) {
        for (int yi = 0; yi < size; yi++) {
            for (int zi = 0; zi < size; zi++) {
                float X = ((float)xi / (float)(size - 1)) * pcs_scale;
                float Y = ((float)yi / (float)(size - 1)) * pcs_scale;
                float Z = ((float)zi / (float)(size - 1)) * pcs_scale;

                float r, g, b;
                xyz_d50_to_linear_rgb(X, Y, Z, &r, &g, &b);
                r = clampf01(r); g = clampf01(g); b = clampf01(b);

                float gr, gg, gb;
                lut_sample(lut, size, r, g, b, &gr, &gg, &gb);

                float dr = srgb_enc(clampf01(gr));
                float dg = srgb_enc(clampf01(gg));
                float db = srgb_enc(clampf01(gb));

                write_u16(f, (uint16_t)(clampf01(dr)*65535.0f+0.5f));
                write_u16(f, (uint16_t)(clampf01(dg)*65535.0f+0.5f));
                write_u16(f, (uint16_t)(clampf01(db)*65535.0f+0.5f));
            }
        }
    }

    /* Output tables: identity */
    for (int c = 0; c < 3; c++)
        for (int i = 0; i < 256; i++)
            write_u16(f, (uint16_t)((i * 65535 + 127) / 255));

    return (uint32_t)(ftell(f)-s);
}

/* ── Public API ───────────────────────────────────────────────────── */

int icc_write_devicelink(const float *lut, int size, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    /* 11 tags: desc, cprt, wtpt, B2A0, B2A1(shared), rXYZ, gXYZ, bXYZ, rTRC, gTRC, bTRC */
    const int NT = 11;
    const uint32_t HDR = 128;
    const uint32_t TBL = 4 + NT*12;

    write_zeros(f, HDR);
    write_zeros(f, TBL);

    uint32_t off[11], sz[11];

#define WRITETAG(idx, fn) \
    pad4(f); off[idx]=(uint32_t)ftell(f); sz[idx]=(fn);

    WRITETAG(0, tag_desc(f, "AppGrader Colour Profile"))
    WRITETAG(1, tag_text(f, "AppGrader"))
    /* D50 white point */
    WRITETAG(2, tag_xyz(f, 0.9642, 1.0000, 0.8249))
    /* B2A0: PCS XYZ → graded device RGB (perceptual) */
    WRITETAG(3, tag_mft2_b2a(f, lut, size))
    /* B2A1 (relative colorimetric) shares the same CLUT data as B2A0 */
    off[4] = off[3]; sz[4] = sz[3];
    /* sRGB primaries (XYZ D50) */
    WRITETAG(5, tag_xyz(f, 0.4360747, 0.2225045, 0.0139322))  /* rXYZ */
    WRITETAG(6, tag_xyz(f, 0.3850649, 0.7168786, 0.0971045))  /* gXYZ */
    WRITETAG(7, tag_xyz(f, 0.1430804, 0.0606169, 0.7141733))  /* bXYZ */
    /* TRC gamma 2.2 (actual tone mapping is in the B2A CLUT) */
    WRITETAG(8,  tag_para_gamma(f, 2.2))   /* rTRC */
    WRITETAG(9,  tag_para_gamma(f, 2.2))   /* gTRC */
    WRITETAG(10, tag_para_gamma(f, 2.2))   /* bTRC */
#undef WRITETAG

    uint32_t total = (uint32_t)ftell(f);

    /* ── Write real header ─────────────────────────── */
    fseek(f, 0, SEEK_SET);

    write_u32(f, total);
    write_zeros(f, 4);                  /* CMM type */
    write_u32(f, 0x02100000);           /* ICC v2.1 */
    write_4cc(f, "mntr");               /* profile class: monitor device */
    write_4cc(f, "RGB ");               /* colour space */
    write_4cc(f, "XYZ ");               /* PCS */

    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    write_u16(f,(uint16_t)(t->tm_year+1900));
    write_u16(f,(uint16_t)(t->tm_mon+1));
    write_u16(f,(uint16_t) t->tm_mday);
    write_u16(f,(uint16_t) t->tm_hour);
    write_u16(f,(uint16_t) t->tm_min);
    write_u16(f,(uint16_t) t->tm_sec);

    write_4cc(f, "acsp");
    write_zeros(f, 4);                  /* platform */
    write_u32(f, 0);                    /* flags */
    write_zeros(f, 4);                  /* device manufacturer */
    write_zeros(f, 4);                  /* device model */
    write_zeros(f, 8);                  /* device attributes */
    write_u32(f, 0);                    /* rendering intent: perceptual */

    /* ICC illuminant D50 */
    write_s15f16(f, 0.9642);
    write_s15f16(f, 1.0000);
    write_s15f16(f, 0.8249);

    write_zeros(f, 4);                  /* creator */
    write_zeros(f, 16);                 /* profile ID */
    write_zeros(f, 28);                 /* reserved */

    /* ── Tag table ─────────────────────────────────── */
    static const char *SIGS[11] = {
        "desc","cprt","wtpt","B2A0","B2A1",
        "rXYZ","gXYZ","bXYZ",
        "rTRC","gTRC","bTRC"
    };
    write_u32(f, NT);
    for (int i = 0; i < NT; i++) {
        write_4cc(f, SIGS[i]);
        write_u32(f, off[i]);
        write_u32(f, sz[i]);
    }

    fclose(f);
    return 0;
}
