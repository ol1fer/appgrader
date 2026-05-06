#include "lut3d.h"
#include "color_math.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

float *lut3d_alloc_identity(int size) {
    float *lut = (float *)malloc(sizeof(float) * size * size * size * 3);
    if (!lut) return NULL;
    for (int bi = 0; bi < size; bi++) {
        for (int gi = 0; gi < size; gi++) {
            for (int ri = 0; ri < size; ri++) {
                int idx = (bi * size * size + gi * size + ri) * 3;
                lut[idx+0] = (float)ri / (size - 1);
                lut[idx+1] = (float)gi / (size - 1);
                lut[idx+2] = (float)bi / (size - 1);
            }
        }
    }
    return lut;
}

void lut3d_free(float *lut) {
    free(lut);
}

void lut3d_trilinear(const float *lut, int size,
                     float r, float g, float b,
                     float *r_out, float *g_out, float *b_out)
{
    float rf = r * (size - 1);
    float gf = g * (size - 1);
    float bf = b * (size - 1);

    int r0 = (int)rf; if (r0 >= size-1) r0 = size-2;
    int g0 = (int)gf; if (g0 >= size-1) g0 = size-2;
    int b0 = (int)bf; if (b0 >= size-1) b0 = size-2;
    int r1 = r0+1, g1 = g0+1, b1 = b0+1;

    float dr = rf - r0, dg = gf - g0, db = bf - b0;

#define LUT(ri,gi,bi,ch) lut[((bi)*size*size + (gi)*size + (ri))*3 + (ch)]

    for (int ch = 0; ch < 3; ch++) {
        float v000 = LUT(r0,g0,b0,ch), v100 = LUT(r1,g0,b0,ch);
        float v010 = LUT(r0,g1,b0,ch), v110 = LUT(r1,g1,b0,ch);
        float v001 = LUT(r0,g0,b1,ch), v101 = LUT(r1,g0,b1,ch);
        float v011 = LUT(r0,g1,b1,ch), v111 = LUT(r1,g1,b1,ch);
        float v = v000*(1-dr)*(1-dg)*(1-db) + v100*dr*(1-dg)*(1-db)
                + v010*(1-dr)*dg*(1-db)     + v110*dr*dg*(1-db)
                + v001*(1-dr)*(1-dg)*db      + v101*dr*(1-dg)*db
                + v011*(1-dr)*dg*db           + v111*dr*dg*db;
        if (ch == 0) *r_out = v;
        else if (ch == 1) *g_out = v;
        else *b_out = v;
    }
#undef LUT
}

/* ── Display LUT helpers ───────────────────────────────────────────── */

static float disp_decode(float v) {
    if (v <= 0.04045f) return v / 12.92f;
    return powf((v + 0.055f) / 1.055f, 2.4f);
}

static float disp_encode(float v) {
    if (v <= 0.0031308f) return 12.92f * v;
    return 1.055f * powf(v, 1.0f / 2.4f) - 0.055f;
}

static float disp_clamp(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

/* Write to <path>.tmp then rename — atomic on POSIX, and on Windows since
 * MoveFileEx with MOVEFILE_REPLACE_EXISTING. The KWin effect watches `path`,
 * so this guarantees it never reads a half-written file mid-drag. */
static int atomic_write_lut(const char *path,
                             int size,
                             int (*encode_cell)(int r, int g, int b, int sz, void *ctx, float out[3]),
                             void *ctx)
{
    char tmp_path[2048];
    int n = snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
    if (n <= 0 || n >= (int)sizeof(tmp_path)) return -1;

    FILE *f = fopen(tmp_path, "wb");
    if (!f) return -1;

    int32_t s = size;
    if (fwrite(&s, 4, 1, f) != 1) { fclose(f); return -1; }

    for (int b = 0; b < size; b++) {
        for (int g = 0; g < size; g++) {
            for (int r = 0; r < size; r++) {
                float out[3];
                if (encode_cell(r, g, b, size, ctx, out) != 0) { fclose(f); return -1; }
                if (fwrite(out, sizeof(float), 3, f) != 3) { fclose(f); return -1; }
            }
        }
    }

    if (fflush(f) != 0) { fclose(f); return -1; }
    fclose(f);

    if (rename(tmp_path, path) != 0) return -1;
    return 0;
}

static int encode_graded(int r, int g, int b, int size, void *ctx, float out[3]) {
    const float *baked_lut = (const float *)ctx;
    float rs = (float)r / (size - 1);
    float gs = (float)g / (size - 1);
    float bs = (float)b / (size - 1);
    /* Decode sRGB grid point to linear, look up graded value, re-encode. */
    float rl = disp_decode(rs), gl = disp_decode(gs), bl = disp_decode(bs);
    float ro, go, bo;
    lut3d_trilinear(baked_lut, size, rl, gl, bl, &ro, &go, &bo);
    out[0] = disp_encode(disp_clamp(ro));
    out[1] = disp_encode(disp_clamp(go));
    out[2] = disp_encode(disp_clamp(bo));
    return 0;
}

static int encode_identity(int r, int g, int b, int size, void *ctx, float out[3]) {
    (void)ctx;
    out[0] = (float)r / (size - 1);
    out[1] = (float)g / (size - 1);
    out[2] = (float)b / (size - 1);
    return 0;
}

int lut3d_write_display_lut(const float *baked_lut, int size, const char *path) {
    return atomic_write_lut(path, size, encode_graded, (void *)baked_lut);
}

int lut3d_write_display_identity(int size, const char *path) {
    return atomic_write_lut(path, size, encode_identity, NULL);
}

void lut3d_bake(float *lut, int size,
                const ColorParams *params,
                const float *ext_lut, int ext_lut_size)
{
    for (int bi = 0; bi < size; bi++) {
        for (int gi = 0; gi < size; gi++) {
            for (int ri = 0; ri < size; ri++) {
                float r = (float)ri / (size - 1);
                float g = (float)gi / (size - 1);
                float b = (float)bi / (size - 1);
                float ro, go, bo;
                color_params_apply(params, r, g, b,
                                   ext_lut, ext_lut_size,
                                   &ro, &go, &bo);
                int idx = (bi * size * size + gi * size + ri) * 3;
                lut[idx+0] = ro;
                lut[idx+1] = go;
                lut[idx+2] = bo;
            }
        }
    }
}
