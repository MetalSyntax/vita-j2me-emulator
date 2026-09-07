/*
 * m3g_math.c — algebra 4x4 fila-mayor para el motor M3G.
 * Fila-mayor = el mismo orden que javax.microedition.m3g.Transform.get(float[]).
 */

#include "m3g_engine.h"

void m3g_mat_identity(float *m) {
    int i;
    for (i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void m3g_mat_mul(float *dst, const float *a, const float *b) {
    float t[16];
    int r, c;
    for (r = 0; r < 4; ++r) {
        for (c = 0; c < 4; ++c) {
            t[r * 4 + c] = a[r * 4 + 0] * b[0 * 4 + c]
                         + a[r * 4 + 1] * b[1 * 4 + c]
                         + a[r * 4 + 2] * b[2 * 4 + c]
                         + a[r * 4 + 3] * b[3 * 4 + c];
        }
    }
    for (r = 0; r < 16; ++r) dst[r] = t[r];
}

void m3g_mat_transform(const float *m, const float *v4, float *out4) {
    float x = v4[0], y = v4[1], z = v4[2], w = v4[3];
    out4[0] = m[0] * x + m[1] * y + m[2] * z + m[3] * w;
    out4[1] = m[4] * x + m[5] * y + m[6] * z + m[7] * w;
    out4[2] = m[8] * x + m[9] * y + m[10] * z + m[11] * w;
    out4[3] = m[12] * x + m[13] * y + m[14] * z + m[15] * w;
}

void m3g_mat_transpose(float *dst, const float *src) {
    float t[16];
    int r, c;
    for (r = 0; r < 4; ++r)
        for (c = 0; c < 4; ++c)
            t[c * 4 + r] = src[r * 4 + c];
    for (r = 0; r < 16; ++r) dst[r] = t[r];
}

/* Inversa general por cofactores. Devuelve 0 si es singular. */
int m3g_mat_invert(float *dst, const float *m) {
    float inv[16], det;
    int i;

    inv[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
             + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
             - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8]  =  m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
             + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
             - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
             - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
             + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9]  = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
             - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
             + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2]  =  m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
             + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
    inv[6]  = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
             - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
             + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
             - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
    inv[3]  = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
             - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
    inv[7]  =  m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
             + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
             - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
             + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

    det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    if (det == 0.0f) return 0;
    det = 1.0f / det;
    for (i = 0; i < 16; ++i) dst[i] = inv[i] * det;
    return 1;
}
