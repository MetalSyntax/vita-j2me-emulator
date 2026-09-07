/*
 * m3g_target.c — destino de render, z-buffer, clear y Background.
 *
 * El z-buffer es una unica reserva perezosa del tamano del buffer
 * destino (480x272x2 = 255 KB en la Vita) que se reutiliza entre
 * frames; solo se realoja si cambian las dimensiones.
 */

#include <stdlib.h>
#include <string.h>

#include "m3g_engine.h"
#include "m3g_pixel.h"
#include "m3g_internal.h"

M3GTarget  m3g_target;
m3g_u16   *m3g_depth;
int        m3g_depth_w;
int        m3g_depth_h;

int m3g_target_bind(M3GTarget *t) {
    int n;

    if (t == NULL || t->color == NULL || t->width <= 0 || t->height <= 0) {
        return 0;
    }

    m3g_target = *t;

    /* Clip saneado contra el buffer. */
    if (m3g_target.clipX0 < 0) m3g_target.clipX0 = 0;
    if (m3g_target.clipY0 < 0) m3g_target.clipY0 = 0;
    if (m3g_target.clipX1 > m3g_target.width)  m3g_target.clipX1 = m3g_target.width;
    if (m3g_target.clipY1 > m3g_target.height) m3g_target.clipY1 = m3g_target.height;

    n = m3g_target.width * m3g_target.height;
    if (m3g_depth == NULL || m3g_depth_w != m3g_target.width
                          || m3g_depth_h != m3g_target.height) {
        free(m3g_depth);
        m3g_depth = (m3g_u16 *)malloc((size_t)n * sizeof(m3g_u16));
        if (m3g_depth == NULL) {
            m3g_depth_w = m3g_depth_h = 0;
            m3g_target.bound = 0;
            return 0;
        }
        m3g_depth_w = m3g_target.width;
        m3g_depth_h = m3g_target.height;
    }

    /* Viewport por defecto = destino completo. */
    m3g_target.vpX = 0;
    m3g_target.vpY = 0;
    m3g_target.vpW = m3g_target.width;
    m3g_target.vpH = m3g_target.height;
    m3g_target.depthNear = 0.0f;
    m3g_target.depthFar  = 1.0f;
    m3g_target.bound = 1;
    return 1;
}

void m3g_target_release(void) {
    m3g_target.bound = 0;
    m3g_target.color = NULL;
    m3g_target.alpha = NULL;
    /* El z-buffer se conserva a proposito: evita un malloc por frame. */
}

void m3g_set_viewport(int x, int y, int w, int h) {
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    if (w > M3G_MAX_VIEWPORT_DIM) w = M3G_MAX_VIEWPORT_DIM;
    if (h > M3G_MAX_VIEWPORT_DIM) h = M3G_MAX_VIEWPORT_DIM;
    m3g_target.vpX = x;
    m3g_target.vpY = y;
    m3g_target.vpW = w;
    m3g_target.vpH = h;
}

void m3g_set_depth_range(float near, float far) {
    if (near < 0.0f) near = 0.0f;
    if (near > 1.0f) near = 1.0f;
    if (far  < 0.0f) far  = 0.0f;
    if (far  > 1.0f) far  = 1.0f;
    m3g_target.depthNear = near;
    m3g_target.depthFar  = far;
}

/* Region efectiva de escritura: clip ∩ viewport ∩ buffer. */
void m3g_scissor(int *x0, int *y0, int *x1, int *y1) {
    int a, b, c, d;
    a = m3g_target.clipX0;
    b = m3g_target.clipY0;
    c = m3g_target.clipX1;
    d = m3g_target.clipY1;
    if (m3g_target.vpX > a) a = m3g_target.vpX;
    if (m3g_target.vpY > b) b = m3g_target.vpY;
    if (m3g_target.vpX + m3g_target.vpW < c) c = m3g_target.vpX + m3g_target.vpW;
    if (m3g_target.vpY + m3g_target.vpH < d) d = m3g_target.vpY + m3g_target.vpH;
    if (a < 0) a = 0;
    if (b < 0) b = 0;
    if (c > m3g_target.width)  c = m3g_target.width;
    if (d > m3g_target.height) d = m3g_target.height;
    *x0 = a; *y0 = b; *x1 = c; *y1 = d;
}

void m3g_clear(int argb, int clearColor, int clearDepth, float depthValue) {
    int x0, y0, x1, y1, x, y;

    if (!m3g_target.bound) return;
    m3g_scissor(&x0, &y0, &x1, &y1);
    if (x0 >= x1 || y0 >= y1) return;

    if (clearColor) {
        m3g_u16 px = M3G_PACK565((argb >> 16) & 0xFF,
                                 (argb >> 8)  & 0xFF,
                                 (argb)       & 0xFF);
        m3g_u8 a = (m3g_u8)((argb >> 24) & 0xFF);
        for (y = y0; y < y1; ++y) {
            m3g_u16 *row = m3g_target.color + (size_t)y * m3g_target.width;
            for (x = x0; x < x1; ++x) row[x] = px;
            if (m3g_target.alpha != NULL) {
                m3g_u8 *arow = m3g_target.alpha + (size_t)y * m3g_target.width;
                for (x = x0; x < x1; ++x) arow[x] = a;
            }
        }
    }

    if (clearDepth && m3g_depth != NULL) {
        m3g_u16 dv;
        if (depthValue < 0.0f) depthValue = 0.0f;
        if (depthValue > 1.0f) depthValue = 1.0f;
        dv = (m3g_u16)(depthValue * 65535.0f + 0.5f);
        for (y = y0; y < y1; ++y) {
            m3g_u16 *row = m3g_depth + (size_t)y * m3g_depth_w;
            for (x = x0; x < x1; ++x) row[x] = dv;
        }
    }
}

/*
 * Background image: Java entrega el bitmap ARGB del Image2D tal cual y
 * los parametros de crop; aqui se muestrea punto a punto sobre el
 * viewport (M3G escala el rectangulo de crop al viewport completo).
 */
void m3g_blit_background(const m3g_u32 *argb, int w, int h,
                         int cropX, int cropY, int cropW, int cropH,
                         int wrapX, int wrapY) {
    int x0, y0, x1, y1, x, y;
    float su, sv;

    if (!m3g_target.bound || argb == NULL || w <= 0 || h <= 0) return;
    if (cropW == 0 || cropH == 0) return;
    m3g_scissor(&x0, &y0, &x1, &y1);
    if (x0 >= x1 || y0 >= y1) return;

    su = (float)cropW / (float)m3g_target.vpW;
    sv = (float)cropH / (float)m3g_target.vpH;

    for (y = y0; y < y1; ++y) {
        m3g_u16 *row  = m3g_target.color + (size_t)y * m3g_target.width;
        m3g_u8  *arow = (m3g_target.alpha != NULL)
                      ? m3g_target.alpha + (size_t)y * m3g_target.width : NULL;
        int sy = cropY + (int)(((float)(y - m3g_target.vpY) + 0.5f) * sv);
        if (wrapY) {
            sy %= h; if (sy < 0) sy += h;
        } else {
            if (sy < 0) sy = 0; else if (sy >= h) sy = h - 1;
        }
        for (x = x0; x < x1; ++x) {
            m3g_u32 c;
            int sx = cropX + (int)(((float)(x - m3g_target.vpX) + 0.5f) * su);
            if (wrapX) {
                sx %= w; if (sx < 0) sx += w;
            } else {
                if (sx < 0) sx = 0; else if (sx >= w) sx = w - 1;
            }
            c = argb[(size_t)sy * w + sx];
            row[x] = M3G_PACK565((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
            if (arow != NULL) arow[x] = (m3g_u8)((c >> 24) & 0xFF);
        }
    }
}
