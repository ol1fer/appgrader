#include "cube_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void cube_lut_free(CubeLut *lut) {
    if (lut && lut->data) { free(lut->data); lut->data = NULL; }
}

static void trim_newline(char *s) {
    int n = (int)strlen(s);
    while (n > 0 && (s[n-1] == '\r' || s[n-1] == '\n' || s[n-1] == ' ')) s[--n] = '\0';
}

int cube_lut_load(const char *path, CubeLut *lut) {
    memset(lut, 0, sizeof(*lut));
    lut->domain_min[0] = lut->domain_min[1] = lut->domain_min[2] = 0.0f;
    lut->domain_max[0] = lut->domain_max[1] = lut->domain_max[2] = 1.0f;

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[512];
    int data_index = 0;

    while (fgets(line, sizeof(line), f)) {
        trim_newline(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        if (strncmp(line, "TITLE", 5) == 0) {
            /* TITLE "name" */
            char *q1 = strchr(line, '"');
            if (q1) {
                char *q2 = strchr(q1+1, '"');
                if (q2) { int n = (int)(q2-q1-1); if (n > 255) n = 255; memcpy(lut->title, q1+1, n); }
            }
        } else if (strncmp(line, "LUT_3D_SIZE", 11) == 0) {
            lut->size = atoi(line + 11);
            if (lut->size < 2 || lut->size > 256) { fclose(f); return -1; }
            int total = lut->size * lut->size * lut->size * 3;
            lut->data = (float *)malloc(sizeof(float) * total);
            if (!lut->data) { fclose(f); return -1; }
        } else if (strncmp(line, "DOMAIN_MIN", 10) == 0) {
            sscanf(line + 10, "%f %f %f", &lut->domain_min[0], &lut->domain_min[1], &lut->domain_min[2]);
        } else if (strncmp(line, "DOMAIN_MAX", 10) == 0) {
            sscanf(line + 10, "%f %f %f", &lut->domain_max[0], &lut->domain_max[1], &lut->domain_max[2]);
        } else if (lut->data && line[0] >= '0' && line[0] <= '9' ||
                   (lut->data && line[0] == '-')) {
            /* Data line */
            float r, g, b;
            if (sscanf(line, "%f %f %f", &r, &g, &b) == 3) {
                int total = lut->size * lut->size * lut->size * 3;
                if (data_index + 2 < total) {
                    lut->data[data_index++] = r;
                    lut->data[data_index++] = g;
                    lut->data[data_index++] = b;
                }
            }
        }
    }
    fclose(f);

    if (!lut->data || data_index < lut->size * lut->size * lut->size * 3)
        return -1;

    return 0;
}
