#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Effects that depend on pixel position or neighbours, so they cannot be
 * baked into a 3D LUT. The KWin shader reads these via a separate small
 * file (~/.config/appgrader/spatial.params) and applies them per-fragment. */
typedef struct {
    int32_t version;          /* always 1 for now */
    float vignette_amount;    /* -1..1 (neg = brighten edges, pos = darken) */
    float vignette_distance;  /*  0..1 (radius / falloff width) */
    float noise_amount;       /*  0..1 */
    float noise_color;        /*  0..1 (0 = mono luma noise, 1 = RGB noise) */
    float noise_scale;        /*  0..1 (0 = 1px grain, 1 ≈ 10px) */
    float sharpen_amount;     /*  0..1 */
    float clarity_amount;     /* -1..1 (neg = hazy/glow, pos = micro-contrast) */
    /* Carved out of the original _pad[8]. Old files (with these zero-filled)
     * still round-trip cleanly: noise_static=0 → animated, noise_seed=0 → no
     * offset, both matching the previous default. */
    int32_t noise_static;     /* 0 = animated (default), 1 = freeze frame */
    float noise_seed;         /*  0..1 — shifts the per-cell hash */
    /* Bloom: bright-pass single-pass approximation in the KWin shader.
     * Carved out of the original _pad. Old files (zero-filled here)
     * deserialize as amount=0 → no bloom regardless of threshold/radius. */
    float bloom_amount;       /*  0..1 */
    float bloom_threshold;    /*  0..1 — luma threshold above which highlights bloom */
    float bloom_radius;       /*  0..1 — mapped to ~2..16 px in the shader */
    float _pad[3];            /* remaining room to grow without breaking format */
} SpatialParams;

void spatial_params_init(SpatialParams *p);

/* Returns nonzero if every field is at its default (identity) value. */
int spatial_params_is_identity(const SpatialParams *p);

/* Atomic write to <path>: writes <path>.tmp then rename(). */
int spatial_params_write(const SpatialParams *p, const char *path);

#ifdef __cplusplus
}
#endif
