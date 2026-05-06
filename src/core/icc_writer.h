#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Write an ICC v2 device-link profile (RGB → RGB) containing a 3D LUT.
 * lut:      float array [size^3 * 3], same layout as lut3d
 * size:     edge length of the cube (e.g. 33)
 * path:     output file path
 * Returns 0 on success, -1 on error. */
int icc_write_devicelink(const float *lut, int size, const char *path);

#ifdef __cplusplus
}
#endif
