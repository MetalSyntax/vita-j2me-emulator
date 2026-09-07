/*
 * m3g_pipeline.c — etapa de vertices: transformacion, iluminacion,
 * niebla, recortado por los 6 planos del frustum, proyeccion, culling
 * y emision de triangulos al rasterizador.
 *
 * Todo el trabajo por vertice vive aqui (en C) a proposito: la CVM del
 * port usa el interprete en C (arch=c), asi que cualquier bucle por
 * vertice en Java costaria un orden de magnitud mas.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "m3g_engine.h"
#include "m3g_pixel.h"
#include "m3g_internal.h"

/* ---------------------------------------------------------------- */
/* Scratch de vertices transformados (se reutiliza entre lotes)       */
/* ---------------------------------------------------------------- */
static M3GClipVert *s_verts;
static int          s_vertsCap;

static M3GClipVert *ensure_verts(int n) {
    if (n > s_vertsCap) {
        M3GClipVert *p = (M3GClipVert *)realloc(s_verts,
                            (size_t)n * sizeof(M3GClipVert));
        if (p == NULL) return NULL;
        s_verts = p;
        s_vertsCap = n;
    }
    return s_verts;
}

/* ---------------------------------------------------------------- */
/* Lectura de atributos                                              */
/* ---------------------------------------------------------------- */
static float attr_get(const void *base, int type, int index) {
    switch (type) {
    case 1: return (float)((const m3g_s8 *)base)[index];
    case 2: return (float)((const m3g_s16 *)base)[index];
    case 4: return ((const float *)base)[index];
    default: return 0.0f;
    }
}

/* Normales: el rango entero se mapea a [-1,1] como exige JSR-184. */
static float normal_get(const void *base, int type, int index) {
    switch (type) {
    case 1: return (float)((const m3g_s8 *)base)[index] * (1.0f / 127.0f);
    case 2: return (float)((const m3g_s16 *)base)[index] * (1.0f / 32767.0f);
    case 4: return ((const float *)base)[index];
    default: return 0.0f;
    }
}

/* ---------------------------------------------------------------- */
/* Iluminacion por vertice (modelo GL ES 1.x, que es el de JSR-184)   */
/* ---------------------------------------------------------------- */
typedef struct {
    int   enabled;
    const float *lights;
    int   lightCount;
    const float *matAmbient, *matDiffuse, *matEmissive, *matSpecular;
    float shininess;
    int   twoSided;
    int   trackVertexColor;
} M3GLightCtx;

static void shade_vertex(const M3GLightCtx *lc,
                         const float eye[3], const float nrm[3],
                         const float vcol[4], float flip,
                         float out[4]) {
    float amb[3], dif[3], spc[3];
    float r, g, b;
    int i;
    float n[3];
    float len;

    n[0] = nrm[0] * flip; n[1] = nrm[1] * flip; n[2] = nrm[2] * flip;
    len = n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
    if (len > 0.0f) {
        len = 1.0f / (float)sqrt(len);
        n[0] *= len; n[1] *= len; n[2] *= len;
    }

    /* Colores del material; el color por vertice sustituye a
     * ambiente+difuso (unico modo de color material en GL ES 1.x). */
    if (lc->trackVertexColor) {
        amb[0] = dif[0] = vcol[0];
        amb[1] = dif[1] = vcol[1];
        amb[2] = dif[2] = vcol[2];
    } else {
        amb[0] = lc->matAmbient[0]; amb[1] = lc->matAmbient[1]; amb[2] = lc->matAmbient[2];
        dif[0] = lc->matDiffuse[0]; dif[1] = lc->matDiffuse[1]; dif[2] = lc->matDiffuse[2];
    }
    spc[0] = lc->matSpecular[0]; spc[1] = lc->matSpecular[1]; spc[2] = lc->matSpecular[2];

    r = lc->matEmissive[0];
    g = lc->matEmissive[1];
    b = lc->matEmissive[2];

    for (i = 0; i < lc->lightCount; ++i) {
        const float *L = lc->lights + i * M3G_LP_STRIDE;
        int mode = (int)L[M3G_LP_MODE];
        float lr = L[M3G_LP_R], lg = L[M3G_LP_G], lb = L[M3G_LP_B];
        float dir[3], dist2, dist, att = 1.0f, ndotl;

        if (mode == M3G_LIGHT_AMBIENT) {
            r += amb[0] * lr; g += amb[1] * lg; b += amb[2] * lb;
            continue;
        }

        if (mode == M3G_LIGHT_DIRECTIONAL) {
            /* La direccion apunta hacia donde viaja la luz: L = -dir. */
            dir[0] = -L[M3G_LP_DX];
            dir[1] = -L[M3G_LP_DY];
            dir[2] = -L[M3G_LP_DZ];
            dist2 = dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2];
            if (dist2 > 0.0f) {
                float inv = 1.0f / (float)sqrt(dist2);
                dir[0] *= inv; dir[1] *= inv; dir[2] *= inv;
            }
        } else {
            dir[0] = L[M3G_LP_PX] - eye[0];
            dir[1] = L[M3G_LP_PY] - eye[1];
            dir[2] = L[M3G_LP_PZ] - eye[2];
            dist2 = dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2];
            dist = (float)sqrt(dist2);
            if (dist > 0.0f) {
                float inv = 1.0f / dist;
                dir[0] *= inv; dir[1] *= inv; dir[2] *= inv;
            }
            att = L[M3G_LP_ATT_C] + L[M3G_LP_ATT_L] * dist
                + L[M3G_LP_ATT_Q] * dist2;
            att = (att > 0.0f) ? 1.0f / att : 1.0f;

            if (mode == M3G_LIGHT_SPOT) {
                float cosA = -(dir[0]*L[M3G_LP_DX] + dir[1]*L[M3G_LP_DY]
                             + dir[2]*L[M3G_LP_DZ]);
                if (cosA < L[M3G_LP_SPOT_COS] || cosA <= 0.0f) {
                    continue;
                }
                if (L[M3G_LP_SPOT_EXP] != 0.0f) {
                    att *= (float)pow(cosA, L[M3G_LP_SPOT_EXP]);
                }
            }
        }

        ndotl = n[0]*dir[0] + n[1]*dir[1] + n[2]*dir[2];
        if (ndotl > 0.0f) {
            float k = att * ndotl;
            r += dif[0] * lr * k;
            g += dif[1] * lg * k;
            b += dif[2] * lb * k;

            if (lc->shininess > 0.0f
                && (spc[0] != 0.0f || spc[1] != 0.0f || spc[2] != 0.0f)) {
                /* Camara local desactivada: el observador esta en
                 * (0,0,1) infinito, igual que el modo por defecto de
                 * GL ES; el semivector se simplifica. */
                float h[3], hl, ndoth, sp;
                h[0] = dir[0]; h[1] = dir[1]; h[2] = dir[2] + 1.0f;
                hl = h[0]*h[0] + h[1]*h[1] + h[2]*h[2];
                if (hl > 0.0f) {
                    hl = 1.0f / (float)sqrt(hl);
                    h[0] *= hl; h[1] *= hl; h[2] *= hl;
                }
                ndoth = n[0]*h[0] + n[1]*h[1] + n[2]*h[2];
                if (ndoth > 0.0f) {
                    sp = (float)pow(ndoth, lc->shininess) * att;
                    r += spc[0] * lr * sp;
                    g += spc[1] * lg * sp;
                    b += spc[2] * lb * sp;
                }
            }
        }
    }

    out[0] = r > 1.0f ? 1.0f : (r < 0.0f ? 0.0f : r);
    out[1] = g > 1.0f ? 1.0f : (g < 0.0f ? 0.0f : g);
    out[2] = b > 1.0f ? 1.0f : (b < 0.0f ? 0.0f : b);
    out[3] = lc->trackVertexColor ? vcol[3] : lc->matDiffuse[3];
}

/* ---------------------------------------------------------------- */
/* Recortado por los 6 planos del frustum (Sutherland-Hodgman)        */
/* ---------------------------------------------------------------- */
static float plane_dist(const M3GClipVert *v, int plane) {
    switch (plane) {
    case 0: return v->clip[3] + v->clip[0]; /* x >= -w */
    case 1: return v->clip[3] - v->clip[0]; /* x <=  w */
    case 2: return v->clip[3] + v->clip[1];
    case 3: return v->clip[3] - v->clip[1];
    case 4: return v->clip[3] + v->clip[2];
    default: return v->clip[3] - v->clip[2];
    }
}

static void lerp_vert(M3GClipVert *o, const M3GClipVert *a,
                      const M3GClipVert *b, float t) {
    int i, u;
    for (i = 0; i < 4; ++i) o->clip[i] = a->clip[i] + (b->clip[i] - a->clip[i]) * t;
    for (i = 0; i < 8; ++i) o->color[i] = a->color[i] + (b->color[i] - a->color[i]) * t;
    for (u = 0; u < M3G_MAX_TEXTURE_UNITS; ++u) {
        o->tc[u][0] = a->tc[u][0] + (b->tc[u][0] - a->tc[u][0]) * t;
        o->tc[u][1] = a->tc[u][1] + (b->tc[u][1] - a->tc[u][1]) * t;
    }
    o->fog = a->fog + (b->fog - a->fog) * t;
}

/* Devuelve el numero de vertices del poligono recortado (0 si se
 * descarta por completo). */
static int clip_polygon(M3GClipVert *poly, int count) {
    M3GClipVert tmp[M3G_MAX_CLIP_VERTS];
    int plane;

    for (plane = 0; plane < 6; ++plane) {
        int out = 0, i;
        for (i = 0; i < count; ++i) {
            const M3GClipVert *cur = &poly[i];
            const M3GClipVert *nxt = &poly[(i + 1) % count];
            float dc = plane_dist(cur, plane);
            float dn = plane_dist(nxt, plane);
            if (dc >= 0.0f) {
                if (out < M3G_MAX_CLIP_VERTS) tmp[out++] = *cur;
            }
            if ((dc >= 0.0f) != (dn >= 0.0f)) {
                float denom = dc - dn;
                if (denom != 0.0f && out < M3G_MAX_CLIP_VERTS) {
                    lerp_vert(&tmp[out++], cur, nxt, dc / denom);
                }
            }
        }
        count = out;
        if (count < 3) return 0;
        memcpy(poly, tmp, (size_t)count * sizeof(M3GClipVert));
    }
    return count;
}

/* ---------------------------------------------------------------- */
/* Proyeccion a pantalla                                             */
/* ---------------------------------------------------------------- */
static void project(const M3GClipVert *in, M3GVert *out) {
    float w = in->clip[3];
    float iw = (w != 0.0f) ? 1.0f / w : 0.0f;
    float ndcX = in->clip[0] * iw;
    float ndcY = in->clip[1] * iw;
    float ndcZ = in->clip[2] * iw;
    int i, u;

    out->x = (float)m3g_target.vpX + (ndcX * 0.5f + 0.5f) * (float)m3g_target.vpW;
    out->y = (float)m3g_target.vpY + (0.5f - ndcY * 0.5f) * (float)m3g_target.vpH;
    out->z = m3g_target.depthNear
           + (ndcZ * 0.5f + 0.5f) * (m3g_target.depthFar - m3g_target.depthNear);
    out->iw = iw;
    for (i = 0; i < 8; ++i) out->color[i] = in->color[i];
    for (u = 0; u < M3G_MAX_TEXTURE_UNITS; ++u) {
        out->tc[u][0] = in->tc[u][0];
        out->tc[u][1] = in->tc[u][1];
    }
    out->fog = in->fog;
}

/* ---------------------------------------------------------------- */
/* Emision de un triangulo (recorte + culling + rasterizado)          */
/* ---------------------------------------------------------------- */
typedef struct {
    const M3GRasterState *st;
    int culling;
    int windingCCW;
    int twoSided;
    int flatShading;
} M3GEmitCtx;

static void emit_triangle(const M3GEmitCtx *ec, const M3GClipVert *a,
                          const M3GClipVert *b, const M3GClipVert *c) {
    M3GClipVert poly[M3G_MAX_CLIP_VERTS];
    M3GVert sv[M3G_MAX_CLIP_VERTS];
    int n, i;
    float area;
    int front, colorOfs;

    poly[0] = *a; poly[1] = *b; poly[2] = *c;
    n = clip_polygon(poly, 3);
    if (n < 3) return;

    for (i = 0; i < n; ++i) project(&poly[i], &sv[i]);

    /* Orientacion en coordenadas de pantalla (eje Y hacia abajo): un
     * triangulo CCW en el sistema de GL sale con area negativa aqui. */
    area = (sv[1].x - sv[0].x) * (sv[2].y - sv[0].y)
         - (sv[2].x - sv[0].x) * (sv[1].y - sv[0].y);
    if (area == 0.0f) return;
    front = ec->windingCCW ? (area < 0.0f) : (area > 0.0f);

    if (ec->culling == M3G_CULL_BACK && !front) return;
    if (ec->culling == M3G_CULL_FRONT && front) return;

    colorOfs = (ec->twoSided && !front) ? 4 : 0;

    if (ec->flatShading) {
        /* Vertice proveedor = el primero del triangulo. */
        int j;
        for (i = 1; i < n; ++i)
            for (j = 0; j < 8; ++j) sv[i].color[j] = sv[0].color[j];
    }

    for (i = 1; i + 1 < n; ++i) {
        m3g_raster_triangle(ec->st, &sv[0], &sv[i], &sv[i + 1], colorOfs);
    }
}

/* ---------------------------------------------------------------- */
/* Entrada principal                                                  */
/* ---------------------------------------------------------------- */
void m3g_render(const M3GBatch *b) {
    const m3g_s32 *ip;
    const float   *fp;
    M3GRasterState st;
    M3GEmitCtx     ec;
    M3GLightCtx    lc;
    M3GClipVert   *vs;
    float mvp[16];
    int vertexCount, posType, posComps, normType, colComps;
    int tcType[M3G_MAX_TEXTURE_UNITS], tcComps[M3G_MAX_TEXTURE_UNITS];
    int i, u, strip, idxPos;
    int hasNormals, lighting;
    float posScale, posBias[3];
    float tcScale[M3G_MAX_TEXTURE_UNITS], tcBias[M3G_MAX_TEXTURE_UNITS][3];
    const float *mv;

    if (!m3g_target.bound || m3g_depth == NULL) return;
    if (b == NULL || b->iparams == NULL || b->fparams == NULL) return;
    if (b->positions == NULL || b->stripLengths == NULL) return;

    ip = b->iparams;
    fp = b->fparams;
    vertexCount = ip[M3G_IP_VERTEX_COUNT];
    if (vertexCount <= 0) return;

    vs = ensure_verts(vertexCount);
    if (vs == NULL) return;

    /* --- estado de rasterizacion --- */
    memset(&st, 0, sizeof(st));
    st.blending      = ip[M3G_IP_BLENDING];
    st.depthTest     = ip[M3G_IP_DEPTH_TEST];
    st.depthWrite    = ip[M3G_IP_DEPTH_WRITE];
    st.colorWrite    = ip[M3G_IP_COLOR_WRITE];
    st.alphaWrite    = ip[M3G_IP_ALPHA_WRITE];
    st.shading       = ip[M3G_IP_SHADING];
    st.perspective   = ip[M3G_IP_PERSPECTIVE];
    st.alphaThreshold     = fp[M3G_FP_ALPHA_THRESHOLD];
    st.depthOffsetFactor  = fp[M3G_FP_DEPTH_OFFSET_FACTOR];
    st.depthOffsetUnits   = fp[M3G_FP_DEPTH_OFFSET_UNITS];
    st.fogMode       = ip[M3G_IP_FOG_MODE];
    st.fogColor      = (m3g_u32)ip[M3G_IP_FOG_COLOR];

    st.texEnabled[0] = ip[M3G_IP_TEX0_ENABLED] && b->texels[0] != NULL;
    st.texW[0]       = ip[M3G_IP_TEX0_WIDTH];
    st.texH[0]       = ip[M3G_IP_TEX0_HEIGHT];
    st.wrapS[0]      = ip[M3G_IP_TEX0_WRAP_S];
    st.wrapT[0]      = ip[M3G_IP_TEX0_WRAP_T];
    st.filter[0]     = ip[M3G_IP_TEX0_FILTER];
    st.texBlend[0]   = ip[M3G_IP_TEX0_BLEND];
    st.texBlendColor[0] = (m3g_u32)ip[M3G_IP_TEX0_BLEND_COLOR];
    st.texels[0]     = b->texels[0];

    st.texEnabled[1] = ip[M3G_IP_TEX1_ENABLED] && b->texels[1] != NULL;
    st.texW[1]       = ip[M3G_IP_TEX1_WIDTH];
    st.texH[1]       = ip[M3G_IP_TEX1_HEIGHT];
    st.wrapS[1]      = ip[M3G_IP_TEX1_WRAP_S];
    st.wrapT[1]      = ip[M3G_IP_TEX1_WRAP_T];
    st.filter[1]     = ip[M3G_IP_TEX1_FILTER];
    st.texBlend[1]   = ip[M3G_IP_TEX1_BLEND];
    st.texBlendColor[1] = (m3g_u32)ip[M3G_IP_TEX1_BLEND_COLOR];
    st.texels[1]     = b->texels[1];

    m3g_scissor(&st.sx0, &st.sy0, &st.sx1, &st.sy1);
    if (st.sx0 >= st.sx1 || st.sy0 >= st.sy1) return;

    /* --- matrices --- */
    mv = fp + M3G_FP_MODELVIEW;
    m3g_mat_mul(mvp, fp + M3G_FP_PROJECTION, mv);

    /* --- formatos de atributos --- */
    posComps = ip[M3G_IP_POS_COMPONENTS];
    posType  = ip[M3G_IP_POS_TYPE];
    normType = ip[M3G_IP_NORMAL_TYPE];
    colComps = ip[M3G_IP_COLOR_COMPONENTS];
    tcType[0]  = ip[M3G_IP_TC0_TYPE];  tcComps[0] = ip[M3G_IP_TC0_COMPONENTS];
    tcType[1]  = ip[M3G_IP_TC1_TYPE];  tcComps[1] = ip[M3G_IP_TC1_COMPONENTS];
    hasNormals = (normType != 0 && b->normals != NULL);
    lighting   = ip[M3G_IP_LIGHTING] && hasNormals;

    posScale   = fp[M3G_FP_POS_SCALE];
    posBias[0] = fp[M3G_FP_POS_BIAS + 0];
    posBias[1] = fp[M3G_FP_POS_BIAS + 1];
    posBias[2] = fp[M3G_FP_POS_BIAS + 2];
    tcScale[0] = fp[M3G_FP_TC0_SCALE];
    tcBias[0][0] = fp[M3G_FP_TC0_BIAS + 0];
    tcBias[0][1] = fp[M3G_FP_TC0_BIAS + 1];
    tcBias[0][2] = fp[M3G_FP_TC0_BIAS + 2];
    tcScale[1] = fp[M3G_FP_TC1_SCALE];
    tcBias[1][0] = fp[M3G_FP_TC1_BIAS + 0];
    tcBias[1][1] = fp[M3G_FP_TC1_BIAS + 1];
    tcBias[1][2] = fp[M3G_FP_TC1_BIAS + 2];

    /* --- contexto de iluminacion --- */
    lc.enabled     = lighting;
    lc.lights      = fp + M3G_FP_LIGHTS;
    lc.lightCount  = ip[M3G_IP_LIGHT_COUNT];
    if (lc.lightCount > M3G_MAX_LIGHTS) lc.lightCount = M3G_MAX_LIGHTS;
    lc.matAmbient  = fp + M3G_FP_MAT_AMBIENT;
    lc.matDiffuse  = fp + M3G_FP_MAT_DIFFUSE;
    lc.matEmissive = fp + M3G_FP_MAT_EMISSIVE;
    lc.matSpecular = fp + M3G_FP_MAT_SPECULAR;
    lc.shininess   = fp[M3G_FP_MAT_SHININESS];
    lc.twoSided    = ip[M3G_IP_TWO_SIDED];
    /* El seguimiento de color material usa el color por vertice, o el
     * color por defecto del VertexBuffer si no hay array. */
    lc.trackVertexColor = ip[M3G_IP_VERTEX_COLOR_TRACK] != 0;

    /* --- transformacion de todos los vertices --- */
    for (i = 0; i < vertexCount; ++i) {
        float obj[4], eye[4], nrm[3], vcol[4];
        M3GClipVert *v = &vs[i];

        obj[0] = attr_get(b->positions, posType, i * posComps + 0) * posScale + posBias[0];
        obj[1] = attr_get(b->positions, posType, i * posComps + 1) * posScale + posBias[1];
        obj[2] = (posComps >= 3)
               ? attr_get(b->positions, posType, i * posComps + 2) * posScale + posBias[2]
               : 0.0f;
        obj[3] = 1.0f;

        m3g_mat_transform(mvp, obj, v->clip);

        {   /* Sin array de colores se usa el color por defecto del
             * VertexBuffer, igual que glColor en GL ES. */
            m3g_u32 dc = (m3g_u32)ip[M3G_IP_DEFAULT_COLOR];
            vcol[0] = (float)((dc >> 16) & 0xFF) * (1.0f / 255.0f);
            vcol[1] = (float)((dc >> 8)  & 0xFF) * (1.0f / 255.0f);
            vcol[2] = (float)( dc        & 0xFF) * (1.0f / 255.0f);
            vcol[3] = (float)((dc >> 24) & 0xFF) * (1.0f / 255.0f);
        }
        if (colComps != 0 && b->colors != NULL) {
            const m3g_u8 *cp = (const m3g_u8 *)b->colors;
            vcol[0] = (float)cp[i * colComps + 0] * (1.0f / 255.0f);
            vcol[1] = (float)cp[i * colComps + 1] * (1.0f / 255.0f);
            vcol[2] = (float)cp[i * colComps + 2] * (1.0f / 255.0f);
            vcol[3] = (colComps == 4)
                    ? (float)cp[i * colComps + 3] * (1.0f / 255.0f) : 1.0f;
        }

        if (lighting) {
            m3g_mat_transform(mv, obj, eye);
            nrm[0] = normal_get(b->normals, normType, i * 3 + 0);
            nrm[1] = normal_get(b->normals, normType, i * 3 + 1);
            nrm[2] = normal_get(b->normals, normType, i * 3 + 2);
            {   /* normal a espacio de camara: 3x3 superior de mv
                 * (aproximacion valida salvo escalado no uniforme, que
                 * se compensa renormalizando en shade_vertex) */
                float n0 = mv[0]*nrm[0] + mv[1]*nrm[1] + mv[2]*nrm[2];
                float n1 = mv[4]*nrm[0] + mv[5]*nrm[1] + mv[6]*nrm[2];
                float n2 = mv[8]*nrm[0] + mv[9]*nrm[1] + mv[10]*nrm[2];
                nrm[0] = n0; nrm[1] = n1; nrm[2] = n2;
            }
            shade_vertex(&lc, eye, nrm, vcol, 1.0f, &v->color[0]);
            if (lc.twoSided) {
                shade_vertex(&lc, eye, nrm, vcol, -1.0f, &v->color[4]);
            } else {
                int k;
                for (k = 0; k < 4; ++k) v->color[4 + k] = v->color[k];
            }
        } else {
            int k;
            for (k = 0; k < 4; ++k) {
                v->color[k] = vcol[k];
                v->color[4 + k] = vcol[k];
            }
            if (ip[M3G_IP_LIGHTING] && !hasNormals) {
                /* Material sin normales: solo emisivo + ambiente. */
                v->color[0] = v->color[4] = fp[M3G_FP_MAT_EMISSIVE + 0];
                v->color[1] = v->color[5] = fp[M3G_FP_MAT_EMISSIVE + 1];
                v->color[2] = v->color[6] = fp[M3G_FP_MAT_EMISSIVE + 2];
                v->color[3] = v->color[7] = fp[M3G_FP_MAT_DIFFUSE + 3];
            }
        }

        /* niebla: distancia en espacio de camara (-z) */
        if (st.fogMode != 0) {
            float ez, f;
            if (!lighting) {
                m3g_mat_transform(mv, obj, eye);
            }
            ez = -eye[2];
            if (st.fogMode == M3G_FOG_LINEAR) {
                float nf = fp[M3G_FP_FOG_NEAR], ff = fp[M3G_FP_FOG_FAR];
                f = (ff != nf) ? (ff - ez) / (ff - nf) : 1.0f;
            } else {
                f = (float)exp(-fp[M3G_FP_FOG_DENSITY] * ez);
            }
            v->fog = f < 0.0f ? 0.0f : (f > 1.0f ? 1.0f : f);
        } else {
            v->fog = 1.0f;
        }

        for (u = 0; u < M3G_MAX_TEXTURE_UNITS; ++u) {
            float s, t, tv[4], tr[4];
            const float *tm;
            if (!st.texEnabled[u] || tcType[u] == 0 || b->texcoords[u] == NULL) {
                v->tc[u][0] = v->tc[u][1] = 0.0f;
                continue;
            }
            s = attr_get(b->texcoords[u], tcType[u], i * tcComps[u] + 0)
                * tcScale[u] + tcBias[u][0];
            t = attr_get(b->texcoords[u], tcType[u], i * tcComps[u] + 1)
                * tcScale[u] + tcBias[u][1];
            tv[0] = s; tv[1] = t; tv[2] = 0.0f; tv[3] = 1.0f;
            tm = fp + (u == 0 ? M3G_FP_TEX0_MATRIX : M3G_FP_TEX1_MATRIX);
            m3g_mat_transform(tm, tv, tr);
            if (tr[3] != 0.0f && tr[3] != 1.0f) {
                tr[0] /= tr[3]; tr[1] /= tr[3];
            }
            v->tc[u][0] = tr[0];
            v->tc[u][1] = tr[1];
        }
    }

    /* --- emision de tiras --- */
    ec.st = &st;
    ec.culling    = ip[M3G_IP_CULLING];
    ec.windingCCW = (ip[M3G_IP_WINDING] != M3G_WINDING_CW);
    ec.twoSided   = lc.twoSided && lighting;
    ec.flatShading = (ip[M3G_IP_SHADING] == M3G_SHADE_FLAT);

    idxPos = 0;
    for (strip = 0; strip < ip[M3G_IP_STRIP_COUNT]; ++strip) {
        int len = b->stripLengths[strip];
        int k;
        for (k = 0; k + 2 < len; ++k) {
            int i0, i1, i2;
            if (b->indices != NULL) {
                i0 = b->indices[idxPos + k];
                i1 = b->indices[idxPos + k + 1];
                i2 = b->indices[idxPos + k + 2];
            } else {
                i0 = idxPos + k;
                i1 = idxPos + k + 1;
                i2 = idxPos + k + 2;
            }
            if (i0 < 0 || i1 < 0 || i2 < 0 ||
                i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount) {
                continue;
            }
            /* Las tiras alternan el sentido en los triangulos impares. */
            if ((k & 1) == 0) {
                emit_triangle(&ec, &vs[i0], &vs[i1], &vs[i2]);
            } else {
                emit_triangle(&ec, &vs[i1], &vs[i0], &vs[i2]);
            }
        }
        idxPos += len;
    }
}

/* ---------------------------------------------------------------- */
/* Picking: interseccion rayo/triangulo en coordenadas locales        */
/* ---------------------------------------------------------------- */
/*
 * Java transforma el rayo al espacio local de la malla y llama aqui.
 * Moller-Trumbore, sin culling: M3G devuelve el impacto mas cercano
 * independientemente de la orientacion de la cara.
 */
int m3g_pick(const M3GBatch *b,
             const float origin[3], const float dir[3],
             float *outT, int *outTri, float *outU, float *outV,
             int outVerts[3]) {
    const m3g_s32 *ip;
    const float *fp;
    int vertexCount, posComps, posType, strip, idxPos, triIndex = 0;
    float posScale, posBias[3];
    float best = 0.0f;
    int found = 0;

    if (b == NULL || b->iparams == NULL || b->fparams == NULL) return 0;
    if (b->positions == NULL || b->stripLengths == NULL) return 0;

    ip = b->iparams;
    fp = b->fparams;
    vertexCount = ip[M3G_IP_VERTEX_COUNT];
    posComps = ip[M3G_IP_POS_COMPONENTS];
    posType  = ip[M3G_IP_POS_TYPE];
    posScale = fp[M3G_FP_POS_SCALE];
    posBias[0] = fp[M3G_FP_POS_BIAS + 0];
    posBias[1] = fp[M3G_FP_POS_BIAS + 1];
    posBias[2] = fp[M3G_FP_POS_BIAS + 2];

    idxPos = 0;
    for (strip = 0; strip < ip[M3G_IP_STRIP_COUNT]; ++strip) {
        int len = b->stripLengths[strip];
        int k;
        for (k = 0; k + 2 < len; ++k, ++triIndex) {
            int ix[3], j;
            float v[3][3], e1[3], e2[3], p[3], q[3], s[3];
            float det, invDet, u, vv, t;

            if (b->indices != NULL) {
                ix[0] = b->indices[idxPos + k];
                ix[1] = b->indices[idxPos + k + 1];
                ix[2] = b->indices[idxPos + k + 2];
            } else {
                ix[0] = idxPos + k; ix[1] = idxPos + k + 1; ix[2] = idxPos + k + 2;
            }
            if ((k & 1) != 0) { int t2 = ix[0]; ix[0] = ix[1]; ix[1] = t2; }
            if (ix[0] < 0 || ix[1] < 0 || ix[2] < 0 ||
                ix[0] >= vertexCount || ix[1] >= vertexCount ||
                ix[2] >= vertexCount) {
                continue;
            }

            for (j = 0; j < 3; ++j) {
                v[j][0] = attr_get(b->positions, posType, ix[j] * posComps + 0)
                          * posScale + posBias[0];
                v[j][1] = attr_get(b->positions, posType, ix[j] * posComps + 1)
                          * posScale + posBias[1];
                v[j][2] = (posComps >= 3)
                        ? attr_get(b->positions, posType, ix[j] * posComps + 2)
                          * posScale + posBias[2]
                        : 0.0f;
            }

            e1[0] = v[1][0] - v[0][0]; e1[1] = v[1][1] - v[0][1]; e1[2] = v[1][2] - v[0][2];
            e2[0] = v[2][0] - v[0][0]; e2[1] = v[2][1] - v[0][1]; e2[2] = v[2][2] - v[0][2];

            p[0] = dir[1]*e2[2] - dir[2]*e2[1];
            p[1] = dir[2]*e2[0] - dir[0]*e2[2];
            p[2] = dir[0]*e2[1] - dir[1]*e2[0];
            det = e1[0]*p[0] + e1[1]*p[1] + e1[2]*p[2];
            if (det > -1e-9f && det < 1e-9f) continue;
            invDet = 1.0f / det;

            s[0] = origin[0] - v[0][0];
            s[1] = origin[1] - v[0][1];
            s[2] = origin[2] - v[0][2];
            u = (s[0]*p[0] + s[1]*p[1] + s[2]*p[2]) * invDet;
            if (u < 0.0f || u > 1.0f) continue;

            q[0] = s[1]*e1[2] - s[2]*e1[1];
            q[1] = s[2]*e1[0] - s[0]*e1[2];
            q[2] = s[0]*e1[1] - s[1]*e1[0];
            vv = (dir[0]*q[0] + dir[1]*q[1] + dir[2]*q[2]) * invDet;
            if (vv < 0.0f || u + vv > 1.0f) continue;

            t = (e2[0]*q[0] + e2[1]*q[1] + e2[2]*q[2]) * invDet;
            if (t <= 0.0f) continue;

            if (!found || t < best) {
                found = 1;
                best = t;
                if (outT)   *outT = t;
                if (outTri) *outTri = triIndex;
                if (outU)   *outU = u;
                if (outV)   *outV = vv;
                if (outVerts) {
                    outVerts[0] = ix[0];
                    outVerts[1] = ix[1];
                    outVerts[2] = ix[2];
                }
            }
        }
        idxPos += len;
    }
    return found;
}
