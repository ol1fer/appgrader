#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char   title[256];
    int    size;     /* edge length of the 3D cube (e.g. 33) */
    float  domain_min[3];
    float  domain_max[3];
    float *data;     /* size^3 * 3 floats, same layout as lut3d */
} CubeLut;

/* Parse a .cube file. Returns 0 on success, -1 on error.
 * On success, lut->data is heap-allocated; caller must free with cube_lut_free(). */
int  cube_lut_load(const char *path, CubeLut *lut);
void cube_lut_free(CubeLut *lut);

#ifdef __cplusplus
}
#endif
