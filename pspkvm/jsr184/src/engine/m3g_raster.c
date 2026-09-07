/*
 * m3g_raster.c — rasterizador de triangulos por software.
 *
 * Estrategia: ecuaciones de arista para obtener el tramo exacto de cada
 * scanline (sin desperdiciar pixeles del bounding box) e interpolacion
 * por plano (df/dx, df/dy constantes) para todos los atributos.
 *
 * - z e 1/w: lineales en pantalla (exacto).
 * - texcoords: perspectiva correcta (u/w, v/w interpolados y divididos
 *   por pixel). La division por pixel cuesta ~15 ciclos de VFP en el
 *   Cortex-A9; la subdivision afin de 8 pixeles queda como optimizacion
 *   futura si algun contenido lo pide.
 * - color y niebla: lineales en pantalla. GL ES los interpola con
 *   perspectiva, pero la diferencia es invisible con los triangulos
 *   pequenos del contenido J2ME.
 */

#include <stddef.h>

#include "m3g_engine.h"
#include "m3g_pixel.h"
#include "m3g_internal.h"

/* Planos de interpolacion: valor en el vertice 0 mas gradientes. */
typedef struct {
    float f0, dfdx, dfdy;
} M3GPlane;

static void plane_setup(M3GPlane *p, float f0, float f1, float f2,
                        float dx1, float dy1, float dx2, float dy2,
                        float invArea) {
    p->f0   = f0;
    p->dfdx = ((f1 - f0) * dy2 - (f2 - f0) * dy1) * invArea;
    p->dfdy = ((f2 - f0) * dx1 - (f1 - f0) * dx2) * invArea;
}

#define PLANE_AT(p, ddx, ddy) ((p).f0 + (p).dfdx * (ddx) + (p).dfdy * (ddy))

/* Mezcla el fragmento (r,g,b,a en 0..1) con el destino. */
static void blend_write(const M3GRasterState *st, m3g_u16 *dst, m3g_u8 *dsta,
                        float r, float g, float b, float a) {
    int sr = (int)(r * 255.0f + 0.5f);
    int sg = (int)(g * 255.0f + 0.5f);
    int sb = (int)(b * 255.0f + 0.5f);
    int sa = (int)(a * 255.0f + 0.5f);
    int dr, dg, db, da, nr, ng, nb, na;

    sr = M3G_CLAMP255(sr); sg = M3G_CLAMP255(sg);
    sb = M3G_CLAMP255(sb); sa = M3G_CLAMP255(sa);

    if (st->blending == M3G_BLEND_REPLACE) {
        nr = sr; ng = sg; nb = sb; na = sa;
    } else {
        m3g_u16 d = *dst;
        dr = M3G_R8(d); dg = M3G_G8(d); db = M3G_B8(d);
        da = (dsta != NULL) ? (int)(*dsta) : 255;
        switch (st->blending) {
        case M3G_BLEND_ALPHA_ADD:
            nr = dr + sr * sa / 255;
            ng = dg + sg * sa / 255;
            nb = db + sb * sa / 255;
            na = da + sa * sa / 255;
            break;
        case M3G_BLEND_MODULATE:
            nr = dr * sr / 255; ng = dg * sg / 255;
            nb = db * sb / 255; na = da * sa / 255;
            break;
        case M3G_BLEND_MODULATE_X2:
            nr = 2 * dr * sr / 255; ng = 2 * dg * sg / 255;
            nb = 2 * db * sb / 255; na = 2 * da * sa / 255;
            break;
        case M3G_BLEND_ALPHA:
        default: {
            int ia = 255 - sa;
            nr = (sr * sa + dr * ia) / 255;
            ng = (sg * sa + dg * ia) / 255;
            nb = (sb * sa + db * ia) / 255;
            na = (sa * sa + da * ia) / 255;
            break;
        }
        }
        nr = M3G_CLAMP255(nr); ng = M3G_CLAMP255(ng);
        nb = M3G_CLAMP255(nb); na = M3G_CLAMP255(na);
    }

    if (st->colorWrite) {
        *dst = M3G_PACK565(nr, ng, nb);
    }
    if (st->alphaWrite && dsta != NULL) {
        *dsta = (m3g_u8)na;
    }
}

void m3g_raster_triangle(const M3GRasterState *st,
                         const M3GVert *v0, const M3GVert *v1,
                         const M3GVert *v2, int colorOfs) {
    float x0, y0, x1, y1, x2, y2, dx1, dy1, dx2, dy2, area, invArea;
    float ea, eb, ec;        /* terminos constantes de las 3 aristas   */
    float ax[3], ay[3], ac[3];
    M3GPlane pz, piw, pr, pg, pb, pa, pfog;
    M3GPlane pu[M3G_MAX_TEXTURE_UNITS], pv[M3G_MAX_TEXTURE_UNITS];
    int textured = (st->texEnabled[0] || st->texEnabled[1]);
    int y, ymin, ymax, u;
    float depthBias = 0.0f;

    x0 = v0->x; y0 = v0->y;
    x1 = v1->x; y1 = v1->y;
    x2 = v2->x; y2 = v2->y;

    dx1 = x1 - x0; dy1 = y1 - y0;
    dx2 = x2 - x0; dy2 = y2 - y0;
    area = dx1 * dy2 - dx2 * dy1;
    if (area == 0.0f) return;

    /* Trabajamos siempre con area positiva: si es negativa se
     * intercambian v1 y v2 (el orden de recorrido no afecta al color
     * plano, que ya se resolvio en la etapa de pipeline). */
    if (area < 0.0f) {
        const M3GVert *t = v1; v1 = v2; v2 = t;
        x1 = v1->x; y1 = v1->y;
        x2 = v2->x; y2 = v2->y;
        dx1 = x1 - x0; dy1 = y1 - y0;
        dx2 = x2 - x0; dy2 = y2 - y0;
        area = dx1 * dy2 - dx2 * dy1;
        if (area <= 0.0f) return;
    }
    invArea = 1.0f / area;

    /* Aristas: e_i(x,y) = ax[i]*x + ay[i]*y + ac[i] >= 0 dentro. */
    ax[0] = -(y1 - y0); ay[0] =  (x1 - x0);
    ax[1] = -(y2 - y1); ay[1] =  (x2 - x1);
    ax[2] = -(y0 - y2); ay[2] =  (x0 - x2);
    ac[0] = -(ax[0] * x0 + ay[0] * y0);
    ac[1] = -(ax[1] * x1 + ay[1] * y1);
    ac[2] = -(ax[2] * x2 + ay[2] * y2);

    /* Regla arriba-izquierda: las aristas que no lo son se hacen
     * exclusivas con un sesgo negativo minusculo. */
    ea = ((ay[0] < 0.0f) || (ay[0] == 0.0f && ax[0] < 0.0f)) ? 0.0f : -0.0001f;
    eb = ((ay[1] < 0.0f) || (ay[1] == 0.0f && ax[1] < 0.0f)) ? 0.0f : -0.0001f;
    ec = ((ay[2] < 0.0f) || (ay[2] == 0.0f && ax[2] < 0.0f)) ? 0.0f : -0.0001f;
    ac[0] += ea; ac[1] += eb; ac[2] += ec;

    /* Planos de interpolacion. */
    plane_setup(&pz,  v0->z,  v1->z,  v2->z,  dx1, dy1, dx2, dy2, invArea);
    plane_setup(&piw, v0->iw, v1->iw, v2->iw, dx1, dy1, dx2, dy2, invArea);
    plane_setup(&pr, v0->color[colorOfs + 0], v1->color[colorOfs + 0],
                     v2->color[colorOfs + 0], dx1, dy1, dx2, dy2, invArea);
    plane_setup(&pg, v0->color[colorOfs + 1], v1->color[colorOfs + 1],
                     v2->color[colorOfs + 1], dx1, dy1, dx2, dy2, invArea);
    plane_setup(&pb, v0->color[colorOfs + 2], v1->color[colorOfs + 2],
                     v2->color[colorOfs + 2], dx1, dy1, dx2, dy2, invArea);
    plane_setup(&pa, v0->color[colorOfs + 3], v1->color[colorOfs + 3],
                     v2->color[colorOfs + 3], dx1, dy1, dx2, dy2, invArea);
    plane_setup(&pfog, v0->fog, v1->fog, v2->fog, dx1, dy1, dx2, dy2, invArea);

    for (u = 0; u < M3G_MAX_TEXTURE_UNITS; ++u) {
        if (!st->texEnabled[u]) continue;
        if (st->perspective) {
            plane_setup(&pu[u], v0->tc[u][0] * v0->iw, v1->tc[u][0] * v1->iw,
                                v2->tc[u][0] * v2->iw, dx1, dy1, dx2, dy2, invArea);
            plane_setup(&pv[u], v0->tc[u][1] * v0->iw, v1->tc[u][1] * v1->iw,
                                v2->tc[u][1] * v2->iw, dx1, dy1, dx2, dy2, invArea);
        } else {
            plane_setup(&pu[u], v0->tc[u][0], v1->tc[u][0], v2->tc[u][0],
                        dx1, dy1, dx2, dy2, invArea);
            plane_setup(&pv[u], v0->tc[u][1], v1->tc[u][1], v2->tc[u][1],
                        dx1, dy1, dx2, dy2, invArea);
        }
    }

    /* depthOffset: unidades absolutas mas pendiente maxima. */
    if (st->depthOffsetUnits != 0.0f || st->depthOffsetFactor != 0.0f) {
        float sx = pz.dfdx < 0.0f ? -pz.dfdx : pz.dfdx;
        float sy = pz.dfdy < 0.0f ? -pz.dfdy : pz.dfdy;
        float slope = sx > sy ? sx : sy;
        depthBias = st->depthOffsetFactor * slope
                  + st->depthOffsetUnits * (1.0f / 65535.0f);
    }

    /* Rango vertical. */
    {
        float fmin = y0, fmax = y0;
        if (y1 < fmin) fmin = y1;
        if (y1 > fmax) fmax = y1;
        if (y2 < fmin) fmin = y2;
        if (y2 > fmax) fmax = y2;
        /* Acotar antes de convertir a int: tras el recortado del
         * frustum las coordenadas caben, pero un triangulo degenerado
         * no debe poder desbordar la conversion. */
        if (fmin < (float)st->sy0) fmin = (float)st->sy0;
        if (fmax > (float)st->sy1) fmax = (float)st->sy1;
        ymin = (int)(fmin - 0.5f + 0.9999f); /* ceil(fmin - 0.5) */
        ymax = (int)(fmax - 0.5f);
        if (ymin < st->sy0) ymin = st->sy0;
        if (ymax >= st->sy1) ymax = st->sy1 - 1;
    }

    for (y = ymin; y <= ymax; ++y) {
        float yc = (float)y + 0.5f;
        float lo = (float)st->sx0, hi = (float)(st->sx1 - 1);
        int i, xs, xe, x;
        int empty = 0;
        m3g_u16 *crow;
        m3g_u8  *arow;
        m3g_u16 *zrow;
        float ddy, zv, iwv, rv, gv, bv, av, fv;
        float uv[M3G_MAX_TEXTURE_UNITS], vv[M3G_MAX_TEXTURE_UNITS];

        for (i = 0; i < 3; ++i) {
            float k = ay[i] * yc + ac[i];
            if (ax[i] > 0.0f) {
                float bound = -k / ax[i];
                if (bound > lo) lo = bound;
            } else if (ax[i] < 0.0f) {
                float bound = -k / ax[i];
                if (bound < hi) hi = bound;
            } else if (k < 0.0f) {
                empty = 1;
                break;
            }
        }
        if (empty) continue;

        if (lo < (float)st->sx0) lo = (float)st->sx0;
        if (hi > (float)(st->sx1 - 1)) hi = (float)(st->sx1 - 1);
        if (lo > hi) continue;
        xs = (int)(lo - 0.5f + 0.9999f);   /* primer centro >= lo */
        xe = (int)(hi - 0.5f);             /* ultimo centro <= hi */
        if (xs < st->sx0) xs = st->sx0;
        if (xe >= st->sx1) xe = st->sx1 - 1;
        if (xs > xe) continue;

        crow = m3g_target.color + (size_t)y * m3g_target.width;
        arow = (m3g_target.alpha != NULL)
             ? m3g_target.alpha + (size_t)y * m3g_target.width : NULL;
        zrow = m3g_depth + (size_t)y * m3g_depth_w;

        ddy = yc - y0;
        {
            float ddx = (float)xs + 0.5f - x0;
            zv  = PLANE_AT(pz,  ddx, ddy) + depthBias;
            iwv = PLANE_AT(piw, ddx, ddy);
            rv  = PLANE_AT(pr,  ddx, ddy);
            gv  = PLANE_AT(pg,  ddx, ddy);
            bv  = PLANE_AT(pb,  ddx, ddy);
            av  = PLANE_AT(pa,  ddx, ddy);
            fv  = PLANE_AT(pfog, ddx, ddy);
            for (u = 0; u < M3G_MAX_TEXTURE_UNITS; ++u) {
                if (!st->texEnabled[u]) continue;
                uv[u] = PLANE_AT(pu[u], ddx, ddy);
                vv[u] = PLANE_AT(pv[u], ddx, ddy);
            }
        }

        for (x = xs; x <= xe; ++x) {
            float r, g, b, a;
            int zi;

            if (zv < 0.0f || zv > 1.0f) goto next_pixel; /* fuera del rango */
            zi = (int)(zv * 65535.0f + 0.5f);

            if (st->depthTest && zi > (int)zrow[x]) goto next_pixel;

            r = rv; g = gv; b = bv; a = av;

            if (textured) {
                float w = (st->perspective && iwv != 0.0f) ? 1.0f / iwv : 1.0f;
                for (u = 0; u < M3G_MAX_TEXTURE_UNITS; ++u) {
                    m3g_u32 texel;
                    if (!st->texEnabled[u]) continue;
                    texel = m3g_texel(st, u,
                                      st->perspective ? uv[u] * w : uv[u],
                                      st->perspective ? vv[u] * w : vv[u]);
                    m3g_tex_combine(st, u, texel, &r, &g, &b, &a);
                }
            }

            if (a < st->alphaThreshold) goto next_pixel;

            if (st->fogMode != 0) {
                float f = fv;
                float fr, fg, fb;
                if (f < 0.0f) f = 0.0f; else if (f > 1.0f) f = 1.0f;
                fr = (float)((st->fogColor >> 16) & 0xFF) * (1.0f / 255.0f);
                fg = (float)((st->fogColor >> 8)  & 0xFF) * (1.0f / 255.0f);
                fb = (float)( st->fogColor        & 0xFF) * (1.0f / 255.0f);
                r = r * f + fr * (1.0f - f);
                g = g * f + fg * (1.0f - f);
                b = b * f + fb * (1.0f - f);
            }

            if (st->colorWrite || st->alphaWrite) {
                blend_write(st, crow + x, (arow != NULL) ? arow + x : NULL,
                            r, g, b, a);
            }
            if (st->depthWrite) {
                zrow[x] = (m3g_u16)zi;
            }

        next_pixel:
            zv  += pz.dfdx;
            iwv += piw.dfdx;
            rv  += pr.dfdx;
            gv  += pg.dfdx;
            bv  += pb.dfdx;
            av  += pa.dfdx;
            fv  += pfog.dfdx;
            for (u = 0; u < M3G_MAX_TEXTURE_UNITS; ++u) {
                if (!st->texEnabled[u]) continue;
                uv[u] += pu[u].dfdx;
                vv[u] += pv[u].dfdx;
            }
        }
    }
}
