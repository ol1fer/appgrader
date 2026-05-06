#pragma once
#include <stdint.h>
#include "color_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Allocate and return a 3D LUT filled with the identity transform.
 * Size is the number of grid points per edge (e.g. 33).
 * Layout: lut[b_idx * size*size + g_idx * size + r_idx] = {r,g,b} interleaved.
 * Caller owns the memory (free with lut3d_free). */
float *lut3d_alloc_identity(int size);
void   lut3d_free(float *lut);

/* Trilinear interpolation lookup into a 3D LUT.
 * r, g, b in [0,1]. Outputs also in [0,1]. */
void lut3d_trilinear(const float *lut, int size,
                     float r, float g, float b,
                     float *r_out, float *g_out, float *b_out);

/* Bake all color_params into a 3D LUT.
 * lut must be allocated by lut3d_alloc_identity (or lut3d_alloc).
 * ext_lut / ext_lut_size: optional external LUT to chain (NULL to skip).
 * This is the main "compile" step — call when any param changes. */
void lut3d_bake(float *lut, int size,
                const ColorParams *params,
                const float *ext_lut, int ext_lut_size);

/* Write a display LUT binary file for the KWin colour-grade effect.
 * baked_lut: linear→linear LUT produced by lut3d_bake.
 * The output encodes sRGB→graded-sRGB so the KWin shader needs no gamma logic.
 * File layout: [int32_t size][float3 data], B-slowest R-fastest, size^3 entries.
 * Returns 0 on success, -1 on failure. */
int lut3d_write_display_lut(const float *baked_lut, int size, const char *path);

/* Write an identity (pass-through) display LUT — used when bypassed. */
int lut3d_write_display_identity(int size, const char *path);

#ifdef __cplusplus
}
#endif
