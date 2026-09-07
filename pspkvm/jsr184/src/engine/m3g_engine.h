/*
 * m3g_engine.h — Motor M3G (JSR-184) por software para PS Vita.
 *
 * Fase 5 del port vita-j2me-emulator. Este motor NO usa GPU: rasteriza
 * directamente sobre el framebuffer RGB565 de MIDP (gxj_screen_buffer),
 * por tres razones documentadas en jsr184/README_VITA.txt:
 *   1) La Fase 4 (lcd.c) ya es duena del contexto GXM via vita2d, y
 *      sceGxmInitialize() solo admite un contexto por proceso: no se
 *      puede anadir vitaGL encima sin rehacer la Fase 4 ya verificada.
 *   2) M3G compone con el contenido 2D dibujado antes/despues por
 *      Graphics y respeta su clip: en software sale exacto y gratis.
 *   3) El contenido M3G de J2ME es de muy baja densidad geometrica.
 *
 * Toda la memoria de vertices/indices/texeles vive en arrays Java; el
 * motor los lee con SNI_GetRawArrayPointer dentro de un bloque
 * SNI_BEGIN_RAW_POINTERS. El motor no posee heap propio salvo el
 * z-buffer (una sola reserva perezosa).
 */

#ifndef _M3G_ENGINE_H_
#define _M3G_ENGINE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char  m3g_u8;
typedef signed char    m3g_s8;
typedef unsigned short m3g_u16;
typedef signed short   m3g_s16;
typedef unsigned int   m3g_u32;
typedef signed int     m3g_s32;

/* ---------------------------------------------------------------- */
/* Limites                                                           */
/* ---------------------------------------------------------------- */
#define M3G_MAX_LIGHTS        8    /* maxLights de la propiedad JSR-184 */
#define M3G_MAX_VIEWPORT_DIM  1024
#define M3G_MAX_TEXTURE_UNITS 2    /* numTextureUnits */
#define M3G_MAX_CLIP_VERTS    16   /* 3 + 6 planos + margen              */

/* ---------------------------------------------------------------- */
/* Constantes espejo de javax.microedition.m3g (no cambiar valores)   */
/* ---------------------------------------------------------------- */
/* CompositingMode.blending */
#define M3G_BLEND_ALPHA        64
#define M3G_BLEND_ALPHA_ADD    65
#define M3G_BLEND_MODULATE     66
#define M3G_BLEND_MODULATE_X2  67
#define M3G_BLEND_REPLACE      68

/* PolygonMode.culling */
#define M3G_CULL_BACK         160
#define M3G_CULL_FRONT        161
#define M3G_CULL_NONE         162
/* PolygonMode.shading */
#define M3G_SHADE_FLAT        164
#define M3G_SHADE_SMOOTH      165
/* PolygonMode.winding */
#define M3G_WINDING_CCW       168
#define M3G_WINDING_CW        169

/* Texture2D.blending */
#define M3G_FUNC_ADD          224
#define M3G_FUNC_BLEND        225
#define M3G_FUNC_DECAL        226
#define M3G_FUNC_MODULATE     227
#define M3G_FUNC_REPLACE      228
/* Texture2D.wrapping */
#define M3G_WRAP_CLAMP        240
#define M3G_WRAP_REPEAT       241
/* Texture2D.filter */
#define M3G_FILTER_BASE_LEVEL 208
#define M3G_FILTER_LINEAR     209
#define M3G_FILTER_NEAREST    210

/* Light.mode */
#define M3G_LIGHT_AMBIENT      128
#define M3G_LIGHT_DIRECTIONAL  129
#define M3G_LIGHT_OMNI         130
#define M3G_LIGHT_SPOT         131

/* Fog.mode */
#define M3G_FOG_EXPONENTIAL    80
#define M3G_FOG_LINEAR         81

/* Image2D.format */
#define M3G_IMG_ALPHA           96
#define M3G_IMG_LUMINANCE       97
#define M3G_IMG_LUMINANCE_ALPHA 98
#define M3G_IMG_RGB             99
#define M3G_IMG_RGBA           100

/* ---------------------------------------------------------------- */
/* Indices del array int[] de estado (IP_*) que Java empaqueta        */
/* ---------------------------------------------------------------- */
enum {
    M3G_IP_VERTEX_COUNT = 0,   /* vertices en los arrays de atributos  */
    M3G_IP_POS_COMPONENTS,     /* 3 o 4 (w ignorada)                   */
    M3G_IP_POS_TYPE,           /* 1=byte 2=short 4=float               */
    M3G_IP_NORMAL_TYPE,        /* 0=ausente 1=byte 2=short 4=float     */
    M3G_IP_COLOR_COMPONENTS,   /* 0=ausente 3 o 4                      */
    M3G_IP_TC0_TYPE,           /* 0=ausente 1/2/4                      */
    M3G_IP_TC0_COMPONENTS,     /* 2 o 3                                */
    M3G_IP_TC1_TYPE,
    M3G_IP_TC1_COMPONENTS,
    M3G_IP_INDEX_COUNT,        /* total de indices en el IndexBuffer    */
    M3G_IP_STRIP_COUNT,        /* numero de tiras                       */
    M3G_IP_CULLING,
    M3G_IP_WINDING,
    M3G_IP_SHADING,
    M3G_IP_TWO_SIDED,          /* 0/1 */
    M3G_IP_PERSPECTIVE,        /* 0/1 hint de correccion perspectiva    */
    M3G_IP_BLENDING,
    M3G_IP_DEPTH_TEST,         /* 0/1 */
    M3G_IP_DEPTH_WRITE,        /* 0/1 */
    M3G_IP_COLOR_WRITE,        /* 0/1 */
    M3G_IP_ALPHA_WRITE,        /* 0/1 */
    M3G_IP_LIGHTING,           /* 0/1: hay Material                     */
    M3G_IP_VERTEX_COLOR_TRACK, /* 0/1: usar color por vertice en difuso */
    M3G_IP_TEX0_ENABLED,
    M3G_IP_TEX0_WIDTH,
    M3G_IP_TEX0_HEIGHT,
    M3G_IP_TEX0_WRAP_S,
    M3G_IP_TEX0_WRAP_T,
    M3G_IP_TEX0_FILTER,        /* M3G_FILTER_NEAREST | M3G_FILTER_LINEAR */
    M3G_IP_TEX0_BLEND,
    M3G_IP_TEX0_BLEND_COLOR,   /* 0xRRGGBB                              */
    M3G_IP_TEX1_ENABLED,
    M3G_IP_TEX1_WIDTH,
    M3G_IP_TEX1_HEIGHT,
    M3G_IP_TEX1_WRAP_S,
    M3G_IP_TEX1_WRAP_T,
    M3G_IP_TEX1_FILTER,
    M3G_IP_TEX1_BLEND,
    M3G_IP_TEX1_BLEND_COLOR,
    M3G_IP_FOG_MODE,           /* 0=off, M3G_FOG_LINEAR/EXPONENTIAL     */
    M3G_IP_FOG_COLOR,          /* 0xRRGGBB                              */
    M3G_IP_LIGHT_COUNT,
    M3G_IP_DEFAULT_COLOR,      /* 0xAARRGGBB del VertexBuffer            */
    M3G_IP_COUNT               /* tamano del array                      */
};

/* Indices del array float[] de estado (FP_*) */
enum {
    M3G_FP_MODELVIEW = 0,      /* 16 floats, fila-mayor                 */
    M3G_FP_PROJECTION = 16,    /* 16 floats                             */
    M3G_FP_POS_SCALE = 32,     /* 1                                     */
    M3G_FP_POS_BIAS = 33,      /* 3                                     */
    M3G_FP_TC0_SCALE = 36,     /* 1                                     */
    M3G_FP_TC0_BIAS = 37,      /* 3                                     */
    M3G_FP_TC1_SCALE = 40,
    M3G_FP_TC1_BIAS = 41,      /* 3                                     */
    M3G_FP_TEX0_MATRIX = 44,   /* 16                                    */
    M3G_FP_TEX1_MATRIX = 60,   /* 16                                    */
    M3G_FP_MAT_AMBIENT = 76,   /* 4 rgba 0..1                           */
    M3G_FP_MAT_DIFFUSE = 80,   /* 4                                     */
    M3G_FP_MAT_EMISSIVE = 84,  /* 4 (rgb; a ignorada)                   */
    M3G_FP_MAT_SPECULAR = 88,  /* 4                                     */
    M3G_FP_MAT_SHININESS = 92, /* 1                                     */
    M3G_FP_ALPHA_THRESHOLD = 93,
    M3G_FP_DEPTH_OFFSET_FACTOR = 94,
    M3G_FP_DEPTH_OFFSET_UNITS = 95,
    M3G_FP_FOG_DENSITY = 96,
    M3G_FP_FOG_NEAR = 97,
    M3G_FP_FOG_FAR = 98,
    M3G_FP_LIGHTS = 99,        /* M3G_MAX_LIGHTS * 16 floats            */
    M3G_FP_COUNT = 99 + M3G_MAX_LIGHTS * 16
};

/* Layout de cada luz dentro de M3G_FP_LIGHTS (12 floats): */
enum {
    M3G_LP_MODE = 0,          /* M3G_LIGHT_* como float                 */
    M3G_LP_R, M3G_LP_G, M3G_LP_B,   /* color * intensity                */
    M3G_LP_PX, M3G_LP_PY, M3G_LP_PZ,/* posicion en espacio de camara    */
    M3G_LP_DX, M3G_LP_DY, M3G_LP_DZ,/* direccion en espacio de camara   */
    M3G_LP_SPOT_COS,          /* cos(spotAngle)                         */
    M3G_LP_SPOT_EXP,          /* spotExponent                           */
    M3G_LP_ATT_C,             /* atenuacion constante                   */
    M3G_LP_ATT_L,             /* atenuacion lineal                      */
    M3G_LP_ATT_Q,             /* atenuacion cuadratica                  */
    M3G_LP_PAD,
    M3G_LP_STRIDE = 16
};

/* ---------------------------------------------------------------- */
/* Destino de render                                                 */
/* ---------------------------------------------------------------- */
typedef struct {
    m3g_u16 *color;        /* RGB565 (formato gxj: usar GXJ_* macros)   */
    m3g_u8  *alpha;        /* canal alfa del destino, o NULL            */
    int      width, height;/* dimensiones del buffer destino            */
    int      clipX0, clipY0, clipX1, clipY1; /* [x0,x1) [y0,y1)         */
    int      vpX, vpY, vpW, vpH;             /* viewport (coords buffer)*/
    float    depthNear, depthFar;            /* rango de profundidad    */
    int      bound;
} M3GTarget;

/* Datos de un lote de triangulos ya resueltos a punteros nativos. */
typedef struct {
    const void *positions;
    const void *normals;
    const void *colors;      /* siempre byte, RGBA u RGB                */
    const void *texcoords[M3G_MAX_TEXTURE_UNITS];
    const m3g_s32 *indices;  /* NULL => tiras implicitas 0,1,2,...      */
    const m3g_s32 *stripLengths;
    const m3g_u32 *texels[M3G_MAX_TEXTURE_UNITS]; /* ARGB8888           */
    const m3g_s32 *iparams;
    const float   *fparams;
} M3GBatch;

/* ---------------------------------------------------------------- */
/* API del motor (la usa m3g_kni.c)                                  */
/* ---------------------------------------------------------------- */

/** Reserva/redimensiona el z-buffer. Devuelve 0 si no hay memoria. */
int  m3g_target_bind(M3GTarget *t);
void m3g_target_release(void);

/** Viewport y rango de profundidad; se recorta contra el clip activo. */
void m3g_set_viewport(int x, int y, int w, int h);
void m3g_set_depth_range(float near, float far);

/** Borrado: color en 0xAARRGGBB, profundidad en [0,1]. */
void m3g_clear(int argb, int clearColor, int clearDepth, float depthValue);

/** Vuelca un rectangulo ARGB (Background image) ya escalado por Java. */
void m3g_blit_background(const m3g_u32 *argb, int w, int h,
                         int cropX, int cropY, int cropW, int cropH,
                         int wrapX, int wrapY);

/** Dibuja un lote de tiras de triangulos. */
void m3g_render(const M3GBatch *b);

/** Test de interseccion rayo/triangulos en espacio local (pick).
 *  Devuelve 1 si hay impacto y rellena outT/outTri/outUV.            */
int  m3g_pick(const M3GBatch *b,
              const float origin[3], const float dir[3],
              float *outT, int *outTri, float *outU, float *outV,
              int outVerts[3]);

/* Utilidades matriciales compartidas (fila-mayor, m[fila*4+col]). */
void  m3g_mat_identity(float *m);
void  m3g_mat_mul(float *dst, const float *a, const float *b);
void  m3g_mat_transform(const float *m, const float *v4, float *out4);
int   m3g_mat_invert(float *dst, const float *src);
void  m3g_mat_transpose(float *dst, const float *src);

#ifdef __cplusplus
}
#endif
#endif /* _M3G_ENGINE_H_ */
