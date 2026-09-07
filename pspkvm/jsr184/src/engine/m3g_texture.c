/*
 * m3g_texture.c — muestreo de texturas y funciones de combinacion.
 *
 * Los texeles llegan como int[] ARGB8888 desde Java (Image2D los
 * convierte una sola vez al construirse), asi que aqui no hay
 * conversion de formatos: solo wrap, filtrado y el texture env.
 *
 * Sin mipmaps: el nivel base es el unico que M3G exige
 * (FILTER_BASE_LEVEL) y los contenidos J2ME son de 64x64/128x128.
 */

#include <stddef.h>

#include "m3g_engine.h"
#include "m3g_internal.h"

static int wrap_coord(int c, int size, int mode) {
    if (mode == M3G_WRAP_REPEAT) {
        c %= size;
        if (c < 0) c += size;
        return c;
    }
    if (c < 0) return 0;
    if (c >= size) return size - 1;
    return c;
}

static m3g_u32 fetch(const m3g_u32 *tex, int w, int h,
                     int x, int y, int wrapS, int wrapT) {
    x = wrap_coord(x, w, wrapS);
    y = wrap_coord(y, h, wrapT);
    return tex[(size_t)y * w + x];
}

m3g_u32 m3g_texel(const M3GRasterState *st, int unit, float u, float v) {
    const m3g_u32 *tex = st->texels[unit];
    int w = st->texW[unit], h = st->texH[unit];
    float fx, fy;

    if (tex == NULL || w <= 0 || h <= 0) return 0xFFFFFFFFu;

    fx = u * (float)w;
    fy = v * (float)h;

    if (st->filter[unit] != M3G_FILTER_LINEAR) {
        int x = (int)(fx < 0.0f ? fx - 1.0f : fx);
        int y = (int)(fy < 0.0f ? fy - 1.0f : fy);
        return fetch(tex, w, h, x, y, st->wrapS[unit], st->wrapT[unit]);
    } else {
        m3g_u32 c00, c10, c01, c11;
        float gx = fx - 0.5f, gy = fy - 0.5f;
        int x0 = (int)(gx < 0.0f ? gx - 1.0f : gx);
        int y0 = (int)(gy < 0.0f ? gy - 1.0f : gy);
        int fu = (int)((gx - (float)x0) * 256.0f);
        int fv = (int)((gy - (float)y0) * 256.0f);
        int iu, iv, sh, r, g, b, a;

        if (fu < 0) fu = 0; else if (fu > 256) fu = 256;
        if (fv < 0) fv = 0; else if (fv > 256) fv = 256;
        iu = 256 - fu;
        iv = 256 - fv;

        c00 = fetch(tex, w, h, x0,     y0,     st->wrapS[unit], st->wrapT[unit]);
        c10 = fetch(tex, w, h, x0 + 1, y0,     st->wrapS[unit], st->wrapT[unit]);
        c01 = fetch(tex, w, h, x0,     y0 + 1, st->wrapS[unit], st->wrapT[unit]);
        c11 = fetch(tex, w, h, x0 + 1, y0 + 1, st->wrapS[unit], st->wrapT[unit]);

        for (sh = 0, r = g = b = a = 0; sh < 32; sh += 8) {
            int v00 = (int)((c00 >> sh) & 0xFF);
            int v10 = (int)((c10 >> sh) & 0xFF);
            int v01 = (int)((c01 >> sh) & 0xFF);
            int v11 = (int)((c11 >> sh) & 0xFF);
            int top = v00 * iu + v10 * fu;
            int bot = v01 * iu + v11 * fu;
            int val = (top * iv + bot * fv) >> 16;
            if (val > 255) val = 255;
            switch (sh) {
                case 0:  b = val; break;
                case 8:  g = val; break;
                case 16: r = val; break;
                default: a = val; break;
            }
        }
        return ((m3g_u32)a << 24) | ((m3g_u32)r << 16)
             | ((m3g_u32)g << 8)  | (m3g_u32)b;
    }
}

/*
 * Texture env de OpenGL ES 1.x, que es el que define JSR-184.
 * (r,g,b,a) entra como color del fragmento y sale combinado con el
 * texel de la unidad indicada. Las unidades se aplican en orden 0,1.
 */
void m3g_tex_combine(const M3GRasterState *st, int unit,
                     m3g_u32 texel, float *r, float *g, float *b, float *a) {
    const float inv = 1.0f / 255.0f;
    float tr = (float)((texel >> 16) & 0xFF) * inv;
    float tg = (float)((texel >> 8)  & 0xFF) * inv;
    float tb = (float)( texel        & 0xFF) * inv;
    float ta = (float)((texel >> 24) & 0xFF) * inv;
    float cr = *r, cg = *g, cb = *b, ca = *a;
    m3g_u32 bc;
    float br, bg, bb;

    switch (st->texBlend[unit]) {
    case M3G_FUNC_REPLACE:
        *r = tr; *g = tg; *b = tb; *a = ta;
        break;
    case M3G_FUNC_DECAL:
        *r = cr * (1.0f - ta) + tr * ta;
        *g = cg * (1.0f - ta) + tg * ta;
        *b = cb * (1.0f - ta) + tb * ta;
        /* alfa del fragmento inalterada */
        break;
    case M3G_FUNC_BLEND:
        bc = st->texBlendColor[unit];
        br = (float)((bc >> 16) & 0xFF) * inv;
        bg = (float)((bc >> 8)  & 0xFF) * inv;
        bb = (float)( bc        & 0xFF) * inv;
        *r = cr * (1.0f - tr) + br * tr;
        *g = cg * (1.0f - tg) + bg * tg;
        *b = cb * (1.0f - tb) + bb * tb;
        *a = ca * ta;
        break;
    case M3G_FUNC_ADD:
        *r = cr + tr; if (*r > 1.0f) *r = 1.0f;
        *g = cg + tg; if (*g > 1.0f) *g = 1.0f;
        *b = cb + tb; if (*b > 1.0f) *b = 1.0f;
        *a = ca * ta;
        break;
    case M3G_FUNC_MODULATE:
    default:
        *r = cr * tr; *g = cg * tg; *b = cb * tb; *a = ca * ta;
        break;
    }
}
