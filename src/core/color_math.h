#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* All parameters that the UI exposes. Defaults = identity (no change). */
typedef struct {
    /* Basic */
    float exposure;       /* EV stops: -3.0 to +3.0, default 0 */
    float contrast;       /* -1.0 to +1.0, default 0 */
    float brightness;     /* -1.0 to +1.0, default 0 */
    float highlights;     /* -1.0 to +1.0, default 0 */
    float shadows;        /* -1.0 to +1.0, default 0 */
    float whites;         /* -1.0 to +1.0, default 0 */
    float blacks;         /* -1.0 to +1.0, default 0 */

    /* Colour */
    float temperature;    /* -1.0 to +1.0, default 0 (neg=cool, pos=warm) */
    float tint;           /* -1.0 to +1.0, default 0 (neg=green, pos=magenta) */
    float saturation;     /* -1.0 to +1.0, default 0 */
    float vibrance;       /* -1.0 to +1.0, default 0 */
    float hue;            /* -1.0 to +1.0 (maps to -180..+180 deg), default 0 */

    /* HSL per colour range (8 ranges: R O Y G C B P M) */
    float hsl_hue[8];        /* -1.0 to +1.0 */
    float hsl_sat[8];        /* -1.0 to +1.0 */
    float hsl_lum[8];        /* -1.0 to +1.0 */

    /* Lift / Gamma / Gain colour wheels (ASC CDL style).
     * Each wheel: dot offset (x, y) in unit disk + luminance scalar.
     * Lift  = offset (affects shadows visibly).
     * Gamma = power  (affects midtones).
     * Gain  = slope  (affects highlights). */
    float lift_x,  lift_y,  lift_lum;
    float gamma_x, gamma_y, gamma_lum;
    float gain_x,  gain_y,  gain_lum;

    /* Per-channel tone curves: 5 control points each (x,y in [0,1]).
     * Index 0 = master/luma, 1=R, 2=G, 3=B.
     * num_points[c] is the actual count (2..16). */
    int   curve_num_points[4];
    float curve_x[4][16];
    float curve_y[4][16];

    /* LUT */
    float lut_intensity;  /* 0.0 to 1.0 */
} ColorParams;

/* Initialise params to identity defaults. */
void color_params_init(ColorParams *p);

/* Apply all params to a single linear-light RGB triple.
 * ext_lut3d: optional pre-loaded 3D LUT (NULL to skip).
 * lut_size:  size of one edge of the 3D LUT cube (e.g. 33).
 * r_out, g_out, b_out are clamped to [0,1]. */
void color_params_apply(
    const ColorParams *p,
    float r_in, float g_in, float b_in,
    const float *ext_lut3d, int lut_size,
    float *r_out, float *g_out, float *b_out
);

#ifdef __cplusplus
}
#endif
