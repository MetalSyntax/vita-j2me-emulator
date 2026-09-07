/*
 * m3g_internal.h — estado compartido entre las etapas del motor.
 * No forma parte de la API publica (solo lo usan m3g_*.c).
 */
#ifndef _M3G_INTERNAL_H_
#define _M3G_INTERNAL_H_

#include "m3g_engine.h"

/* --- destino (m3g_target.c) --- */
extern M3GTarget m3g_target;
extern m3g_u16  *m3g_depth;
extern int       m3g_depth_w;
extern int       m3g_depth_h;

void m3g_scissor(int *x0, int *y0, int *x1, int *y1);

/* --- vertices --- */

/** Vertice en espacio de recorte (antes de la division perspectiva). */
typedef struct {
    float clip[4];        /* x, y, z, w */
    /* color[0..3] = cara frontal, color[4..7] = cara trasera
     * (twoSidedLighting; iguales cuando esta desactivado). */
    float color[8];
    float tc[M3G_MAX_TEXTURE_UNITS][2];
    float fog;            /* 1 = sin niebla, 0 = niebla total */
} M3GClipVert;

/** Vertice ya proyectado a pantalla. */
typedef struct {
    float x, y;           /* pixeles (centro de pixel en +0.5)          */
    float z;              /* profundidad en [0,1]                       */
    float iw;             /* 1/w para correccion perspectiva            */
    float color[8];
    float tc[M3G_MAX_TEXTURE_UNITS][2];
    float fog;
} M3GVert;

/** Estado de rasterizacion derivado del Appearance. */
typedef struct {
    int     blending;
    int     depthTest, depthWrite, colorWrite, alphaWrite;
    float   alphaThreshold;
    float   depthOffsetFactor, depthOffsetUnits;
    int     shading;
    int     perspective;
    int     texEnabled[M3G_MAX_TEXTURE_UNITS];
    const m3g_u32 *texels[M3G_MAX_TEXTURE_UNITS];
    int     texW[M3G_MAX_TEXTURE_UNITS], texH[M3G_MAX_TEXTURE_UNITS];
    int     wrapS[M3G_MAX_TEXTURE_UNITS], wrapT[M3G_MAX_TEXTURE_UNITS];
    int     filter[M3G_MAX_TEXTURE_UNITS], texBlend[M3G_MAX_TEXTURE_UNITS];
    m3g_u32 texBlendColor[M3G_MAX_TEXTURE_UNITS];
    int     fogMode;
    m3g_u32 fogColor;
    /* Region efectiva de escritura, precalculada por lote. */
    int     sx0, sy0, sx1, sy1;
} M3GRasterState;

/* --- m3g_raster.c --- */
/** colorOfs = 0 (cara frontal) o 4 (cara trasera, twoSidedLighting). */
void m3g_raster_triangle(const M3GRasterState *st,
                         const M3GVert *a, const M3GVert *b, const M3GVert *c,
                         int colorOfs);

/* --- m3g_texture.c --- */
m3g_u32 m3g_texel(const M3GRasterState *st, int unit, float u, float v);
void    m3g_tex_combine(const M3GRasterState *st, int unit, m3g_u32 texel,
                        float *r, float *g, float *b, float *a);

#endif /* _M3G_INTERNAL_H_ */
