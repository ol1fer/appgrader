#include "color_math.h"
#include "lut3d.h"
#include <math.h>
#include <string.h>

void color_params_init(ColorParams *p) {
    memset(p, 0, sizeof(*p));
    p->lut_intensity = 1.0f;
    for (int c = 0; c < 4; c++) {
        p->curve_num_points[c] = 2;
        p->curve_x[c][0] = 0.0f; p->curve_y[c][0] = 0.0f;
        p->curve_x[c][1] = 1.0f; p->curve_y[c][1] = 1.0f;
    }
}

/* --- helpers --- */

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

/* Monotone cubic spline interpolation through (x[n], y[n]) points.
 * Evaluates at position t in [0,1]. Points must be sorted by x. */
static float eval_curve(const float *cx, const float *cy, int n, float t) {
    if (n <= 1) return t;
    if (t <= cx[0]) return cy[0];
    if (t >= cx[n-1]) return cy[n-1];
    int i = 0;
    while (i < n - 2 && t > cx[i+1]) i++;
    float dx = cx[i+1] - cx[i];
    if (dx < 1e-6f) return cy[i];
    float u = (t - cx[i]) / dx;
    /* Catmull-Rom tangents clamped for monotonicity */
    float m0, m1;
    if (i == 0)
        m0 = (cy[i+1] - cy[i]) / dx;
    else
        m0 = 0.5f * ((cy[i+1] - cy[i]) / dx + (cy[i] - cy[i-1]) / (cx[i] - cx[i-1]));
    if (i == n - 2)
        m1 = (cy[i+1] - cy[i]) / dx;
    else
        m1 = 0.5f * ((cy[i+2] - cy[i+1]) / (cx[i+2] - cx[i+1]) + (cy[i+1] - cy[i]) / dx);
    float u2 = u * u, u3 = u2 * u;
    return (2*u3 - 3*u2 + 1)*cy[i] + (u3 - 2*u2 + u)*dx*m0
         + (-2*u3 + 3*u2)*cy[i+1] + (u3 - u2)*dx*m1;
}

/* RGB <-> HSL */
static void rgb_to_hsl(float r, float g, float b, float *h, float *s, float *l) {
    float mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
    float mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
    *l = (mx + mn) * 0.5f;
    float d = mx - mn;
    if (d < 1e-6f) { *h = 0; *s = 0; return; }
    *s = d / (1.0f - fabsf(2.0f * (*l) - 1.0f));
    if (mx == r)      *h = (g - b) / d + (g < b ? 6.0f : 0.0f);
    else if (mx == g) *h = (b - r) / d + 2.0f;
    else              *h = (r - g) / d + 4.0f;
    *h /= 6.0f;
}

static float hue_to_rgb(float p, float q, float t) {
    if (t < 0) t += 1; if (t > 1) t -= 1;
    if (t < 1.0f/6) return p + (q-p)*6*t;
    if (t < 1.0f/2) return q;
    if (t < 2.0f/3) return p + (q-p)*(2.0f/3 - t)*6;
    return p;
}

static void hsl_to_rgb(float h, float s, float l, float *r, float *g, float *b) {
    if (s < 1e-6f) { *r = *g = *b = l; return; }
    float q = l < 0.5f ? l*(1+s) : l+s-l*s;
    float p = 2*l - q;
    *r = hue_to_rgb(p, q, h + 1.0f/3);
    *g = hue_to_rgb(p, q, h);
    *b = hue_to_rgb(p, q, h - 1.0f/3);
}

/* Per-colour HSL adjustment.
 * Hue range centres (0..1): R=0/1, O=0.083, Y=0.167, G=0.333, C=0.5, B=0.667, P=0.833, M=0.917 */
static const float HSL_RANGE_CENTERS[8] = {
    0.0f, 0.083f, 0.167f, 0.333f, 0.500f, 0.667f, 0.833f, 0.917f
};

static void apply_hsl_ranges(const ColorParams *p, float *r, float *g, float *b) {
    float h, s, l;
    rgb_to_hsl(*r, *g, *b, &h, &s, &l);

    float total_hue = 0, total_sat = 0, total_lum = 0;
    int any_active = 0;
    for (int i = 0; i < 8; i++) {
        if (fabsf(p->hsl_hue[i]) < 1e-4f &&
            fabsf(p->hsl_sat[i]) < 1e-4f &&
            fabsf(p->hsl_lum[i]) < 1e-4f) continue;
        any_active = 1;

        float d = h - HSL_RANGE_CENTERS[i];
        /* Wrap distance */
        if (d > 0.5f) d -= 1.0f;
        if (d < -0.5f) d += 1.0f;
        float w = expf(-d * d / (2.0f * 0.03f)); /* sigma ~0.17 range */
        total_hue += p->hsl_hue[i] * w;
        total_sat += p->hsl_sat[i] * w;
        total_lum += p->hsl_lum[i] * w;
    }
    if (!any_active) return;

    h = fmodf(h + total_hue * 0.5f + 1.0f, 1.0f);
    s = clampf(s + total_sat, 0, 1);
    l = clampf(l + total_lum * 0.5f, 0, 1);
    hsl_to_rgb(h, s, l, r, g, b);
}

/* 3×3 matrix multiply (row-major) */
static void mat3_mul_rgb(const float m[9], float r, float g, float b,
                         float *ro, float *go, float *bo) {
    *ro = m[0]*r + m[1]*g + m[2]*b;
    *go = m[3]*r + m[4]*g + m[5]*b;
    *bo = m[6]*r + m[7]*g + m[8]*b;
}

/* Build saturation matrix (ITU-R BT.709 luminance coefficients) */
static void build_sat_matrix(float sat_amount, float m[9]) {
    float wr = 0.2126f, wg = 0.7152f, wb = 0.0722f;
    float s = sat_amount + 1.0f; /* sat_amount in [-1,1] → s in [0,2] */
    m[0] = (1-s)*wr + s;  m[1] = (1-s)*wg;      m[2] = (1-s)*wb;
    m[3] = (1-s)*wr;      m[4] = (1-s)*wg + s;  m[5] = (1-s)*wb;
    m[6] = (1-s)*wr;      m[7] = (1-s)*wg;      m[8] = (1-s)*wb + s;
}

/* Build hue rotation matrix in linear RGB space */
static void build_hue_matrix(float angle_deg, float m[9]) {
    float a = angle_deg * 3.14159265f / 180.0f;
    float c = cosf(a), s = sinf(a);
    float wr = 0.2126f, wg = 0.7152f, wb = 0.0722f;
    /* Rodrigues rotation around luminance axis (wr,wg,wb)/|(wr,wg,wb)| */
    float len = sqrtf(wr*wr + wg*wg + wb*wb);
    float ux = wr/len, uy = wg/len, uz = wb/len;
    m[0] = c + ux*ux*(1-c);      m[1] = ux*uy*(1-c) - uz*s; m[2] = ux*uz*(1-c) + uy*s;
    m[3] = uy*ux*(1-c) + uz*s;   m[4] = c + uy*uy*(1-c);    m[5] = uy*uz*(1-c) - ux*s;
    m[6] = uz*ux*(1-c) - uy*s;   m[7] = uz*uy*(1-c) + ux*s; m[8] = c + uz*uz*(1-c);
}

/* Highlights/shadows tone adjustment.
 * pivot: 0=shadow end ~0.25, 1=highlight start ~0.75 */
static float tone_range_adjust(float v, float amount, float pivot, float width) {
    float d = v - pivot;
    float influence = expf(-d * d / (2.0f * width * width));
    return clampf(v + amount * influence, 0, 1);
}

/* Project a wheel (x, y) onto per-channel R/G/B amounts.
 * Wheel layout: R at 0°, G at 120°, B at 240° (standard colour-wheel).
 * Returns signed contribution per channel. */
static void wheel_to_rgb(float x, float y, float *r, float *g, float *b) {
    /* cos(0)=1, sin(0)=0; cos(120)=-0.5, sin(120)=0.866; cos(240)=-0.5, sin(240)=-0.866 */
    *r = x;
    *g = x * -0.5f + y *  0.8660254f;
    *b = x * -0.5f + y * -0.8660254f;
}

/* ASC CDL wheels: out = pow(max(0, in*slope + offset), 1/power). */
static void apply_cdl_wheels(const ColorParams *p, float *r, float *g, float *b) {
    /* Skip if everything is identity. */
    float sumsq =
        p->lift_x*p->lift_x   + p->lift_y*p->lift_y   + p->lift_lum*p->lift_lum +
        p->gamma_x*p->gamma_x + p->gamma_y*p->gamma_y + p->gamma_lum*p->gamma_lum +
        p->gain_x*p->gain_x   + p->gain_y*p->gain_y   + p->gain_lum*p->gain_lum;
    if (sumsq < 1e-6f) return;

    float lr, lg, lb, mr, mg, mb, gr, gg, gb;
    wheel_to_rgb(p->lift_x,  p->lift_y,  &lr, &lg, &lb);
    wheel_to_rgb(p->gamma_x, p->gamma_y, &mr, &mg, &mb);
    wheel_to_rgb(p->gain_x,  p->gain_y,  &gr, &gg, &gb);

    /* Magnitude scaling tuned so 100% wheel offset is a strong-but-usable shift. */
    const float OFF_K   = 0.15f;  /* lift  → offset */
    const float SLOPE_K = 0.5f;   /* gain  → slope  */
    const float GAMMA_K = 0.5f;   /* gamma → power  */

    float off_r = (lr + p->lift_lum) * OFF_K;
    float off_g = (lg + p->lift_lum) * OFF_K;
    float off_b = (lb + p->lift_lum) * OFF_K;

    float slope_r = 1.0f + (gr + p->gain_lum) * SLOPE_K;
    float slope_g = 1.0f + (gg + p->gain_lum) * SLOPE_K;
    float slope_b = 1.0f + (gb + p->gain_lum) * SLOPE_K;

    float pow_r = 1.0f / fmaxf(1.0f + (mr + p->gamma_lum) * GAMMA_K, 0.05f);
    float pow_g = 1.0f / fmaxf(1.0f + (mg + p->gamma_lum) * GAMMA_K, 0.05f);
    float pow_b = 1.0f / fmaxf(1.0f + (mb + p->gamma_lum) * GAMMA_K, 0.05f);

    float vr = *r * slope_r + off_r;
    float vg = *g * slope_g + off_g;
    float vb = *b * slope_b + off_b;
    *r = vr <= 0 ? 0 : powf(vr, pow_r);
    *g = vg <= 0 ? 0 : powf(vg, pow_g);
    *b = vb <= 0 ? 0 : powf(vb, pow_b);
}

/* Vibrance: boost saturation less on already-saturated colours */
static void apply_vibrance(float amount, float *r, float *g, float *b) {
    if (fabsf(amount) < 1e-4f) return;
    float h, s, l;
    rgb_to_hsl(*r, *g, *b, &h, &s, &l);
    float boost = amount * (1.0f - s);
    s = clampf(s + boost, 0, 1);
    hsl_to_rgb(h, s, l, r, g, b);
}

/* ------------------------------------------------------------------ */

void color_params_apply(
    const ColorParams *p,
    float r, float g, float b,
    const float *ext_lut3d, int lut_size,
    float *r_out, float *g_out, float *b_out)
{
    /* 1. Exposure */
    float ev = powf(2.0f, p->exposure);
    r *= ev; g *= ev; b *= ev;

    /* 2. Temperature (warm = boost R, reduce B; cool = opposite) */
    float temp = p->temperature * 0.25f;
    r = clampf(r * (1.0f + temp), 0, 4);
    b = clampf(b * (1.0f - temp), 0, 4);

    /* 3. Tint (pos = magenta = boost R+B, reduce G) */
    float tnt = p->tint * 0.20f;
    g = clampf(g * (1.0f - tnt), 0, 4);

    /* 4. Saturation matrix */
    if (fabsf(p->saturation) > 1e-4f) {
        float m[9]; build_sat_matrix(p->saturation, m);
        mat3_mul_rgb(m, r, g, b, &r, &g, &b);
        r = clampf(r, 0, 4); g = clampf(g, 0, 4); b = clampf(b, 0, 4);
    }

    /* 5. Hue rotation */
    if (fabsf(p->hue) > 1e-4f) {
        float m[9]; build_hue_matrix(p->hue * 180.0f, m);
        mat3_mul_rgb(m, r, g, b, &r, &g, &b);
        r = clampf(r, 0, 4); g = clampf(g, 0, 4); b = clampf(b, 0, 4);
    }

    /* 6. Vibrance */
    apply_vibrance(p->vibrance, &r, &g, &b);

    /* 6b. Lift / Gamma / Gain colour wheels */
    apply_cdl_wheels(p, &r, &g, &b);
    r = clampf(r, 0, 4); g = clampf(g, 0, 4); b = clampf(b, 0, 4);

    /* 7. Per-colour HSL adjustments */
    apply_hsl_ranges(p, &r, &g, &b);

    /* 8. Contrast (S-curve around 0.5) */
    if (fabsf(p->contrast) > 1e-4f) {
        float k = p->contrast + 1.0f;
        r = clampf(0.5f + (r - 0.5f) * k, 0, 1);
        g = clampf(0.5f + (g - 0.5f) * k, 0, 1);
        b = clampf(0.5f + (b - 0.5f) * k, 0, 1);
    }

    /* 9. Brightness */
    if (fabsf(p->brightness) > 1e-4f) {
        r = clampf(r + p->brightness * 0.5f, 0, 1);
        g = clampf(g + p->brightness * 0.5f, 0, 1);
        b = clampf(b + p->brightness * 0.5f, 0, 1);
    }

    /* 10. Highlights (tone range ~0.7-1.0) */
    if (fabsf(p->highlights) > 1e-4f) {
        float a = p->highlights * 0.5f;
        r = tone_range_adjust(r, a, 0.75f, 0.2f);
        g = tone_range_adjust(g, a, 0.75f, 0.2f);
        b = tone_range_adjust(b, a, 0.75f, 0.2f);
    }

    /* 11. Shadows (tone range ~0.0-0.3) */
    if (fabsf(p->shadows) > 1e-4f) {
        float a = p->shadows * 0.5f;
        r = tone_range_adjust(r, a, 0.25f, 0.2f);
        g = tone_range_adjust(g, a, 0.25f, 0.2f);
        b = tone_range_adjust(b, a, 0.25f, 0.2f);
    }

    /* 12. Whites clip point */
    if (fabsf(p->whites) > 1e-4f) {
        float clip = 1.0f + p->whites * 0.5f;
        r = clampf(r / clip, 0, 1);
        g = clampf(g / clip, 0, 1);
        b = clampf(b / clip, 0, 1);
    }

    /* 13. Blacks lift point */
    if (fabsf(p->blacks) > 1e-4f) {
        float lift = p->blacks * 0.15f;
        r = clampf(r + lift, 0, 1);
        g = clampf(g + lift, 0, 1);
        b = clampf(b + lift, 0, 1);
    }

    /* 14. Per-channel tone curves (master applied first, then RGB) */
    /* Master curve */
    r = eval_curve(p->curve_x[0], p->curve_y[0], p->curve_num_points[0], r);
    g = eval_curve(p->curve_x[0], p->curve_y[0], p->curve_num_points[0], g);
    b = eval_curve(p->curve_x[0], p->curve_y[0], p->curve_num_points[0], b);
    /* Per-channel curves */
    r = eval_curve(p->curve_x[1], p->curve_y[1], p->curve_num_points[1], r);
    g = eval_curve(p->curve_x[2], p->curve_y[2], p->curve_num_points[2], g);
    b = eval_curve(p->curve_x[3], p->curve_y[3], p->curve_num_points[3], b);

    /* 15. External LUT (trilinear interpolation) */
    if (ext_lut3d && lut_size > 1 && p->lut_intensity > 1e-4f) {
        float lr, lg, lb;
        lut3d_trilinear(ext_lut3d, lut_size, r, g, b, &lr, &lg, &lb);
        r = lerpf(r, lr, p->lut_intensity);
        g = lerpf(g, lg, p->lut_intensity);
        b = lerpf(b, lb, p->lut_intensity);
    }

    *r_out = clampf(r, 0, 1);
    *g_out = clampf(g, 0, 1);
    *b_out = clampf(b, 0, 1);
}
