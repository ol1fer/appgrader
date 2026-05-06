#include "spatial_params.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void spatial_params_init(SpatialParams *p) {
    memset(p, 0, sizeof(*p));
    p->version = 1;
    p->vignette_distance = 0.5f;
    p->noise_static = 1;
    p->bloom_threshold = 0.7f;
    p->bloom_radius = 0.5f;
}

int spatial_params_is_identity(const SpatialParams *p) {
    return fabsf(p->vignette_amount)  < 1e-4f
        && fabsf(p->noise_amount)     < 1e-4f
        && fabsf(p->sharpen_amount)   < 1e-4f
        && fabsf(p->clarity_amount)   < 1e-4f
        && fabsf(p->bloom_amount)     < 1e-4f;
}

int spatial_params_write(const SpatialParams *p, const char *path) {
    char tmp_path[2048];
    int n = snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
    if (n <= 0 || n >= (int)sizeof(tmp_path)) return -1;

    FILE *f = fopen(tmp_path, "wb");
    if (!f) return -1;

    /* SpatialParams is plain-old-data with explicit-width fields, so a raw
     * write is portable enough for our same-host producer/consumer pair. */
    if (fwrite(p, sizeof(*p), 1, f) != 1) { fclose(f); return -1; }
    if (fflush(f) != 0) { fclose(f); return -1; }
    fclose(f);

    if (rename(tmp_path, path) != 0) return -1;
    return 0;
}
