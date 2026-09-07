/*
 * m3g_kni.c — puente KNI entre javax.microedition.m3g.M3GNative y el
 * motor por software de jsr184/src/engine.
 *
 * Todos los datos masivos siguen viviendo en arrays Java: aqui solo se
 * toman punteros crudos dentro de un bloque SNI_BEGIN_RAW_POINTERS, que
 * garantiza que el GC no mueva nada mientras el motor los recorre.
 */

#include <stdlib.h>
#include <string.h>

#include <kni.h>
#include <sni.h>
#include <commonKNIMacros.h>
#include <ROMStructs.h>

#include <gxapi_graphics.h>
#include <gxj_putpixel.h>

#include "m3g_engine.h"
#include "m3g_pixel.h"

/* ---------------------------------------------------------------- */
/* Estado del destino ligado                                         */
/* ---------------------------------------------------------------- */

/* Traslacion del Graphics activo: el viewport de M3G se da en el
 * sistema de coordenadas del Graphics, no en el del framebuffer. */
static int      s_transX;
static int      s_transY;
static int      s_targetW;
static int      s_targetH;

/* Destino Image2D: buffers propios que se convierten al liberar. */
static m3g_u16 *s_imgColor;
static m3g_u8  *s_imgAlpha;
static int      s_imgW;
static int      s_imgH;

static void free_image_target(void) {
    free(s_imgColor);
    free(s_imgAlpha);
    s_imgColor = NULL;
    s_imgAlpha = NULL;
    s_imgW = 0;
    s_imgH = 0;
}

/**
 * boolean nBindGraphics(Object graphics)
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_m3g_M3GNative_nBindGraphics) {
    jboolean ok = KNI_FALSE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(gh);
    KNI_GetParameterAsObject(1, gh);

    if (!KNI_IsNullHandle(gh)) {
        java_graphics *g = GXAPI_GET_GRAPHICS_PTR(gh);
        const java_imagedata *imgdata =
            (g->img != NULL) ? g->img->imageData : NULL;
        gxj_screen_buffer  sbuf;
        gxj_screen_buffer *sb =
            gxj_get_image_screen_buffer_impl(imgdata, &sbuf, NULL);
        if (sb == NULL) {
            sb = &gxj_system_screen_buffer;
        }
        if (sb->pixelData != NULL && sb->width > 0 && sb->height > 0) {
            M3GTarget t;
            memset(&t, 0, sizeof(t));
            t.color  = sb->pixelData;
            t.alpha  = sb->alphaData;
            t.width  = sb->width;
            t.height = sb->height;
            t.clipX0 = g->clipX1;
            t.clipY0 = g->clipY1;
            t.clipX1 = g->clipX2;
            t.clipY1 = g->clipY2;
            if (m3g_target_bind(&t)) {
                free_image_target();
                s_transX = g->transX;
                s_transY = g->transY;
                s_targetW = sb->width;
                s_targetH = sb->height;
                ok = KNI_TRUE;
            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(ok);
}

/**
 * boolean nBindImage(int[] argb, int width, int height)
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_m3g_M3GNative_nBindImage) {
    int height = KNI_GetParameterAsInt(3);
    int width  = KNI_GetParameterAsInt(2);
    jboolean ok = KNI_FALSE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(ah);
    KNI_GetParameterAsObject(1, ah);

    if (!KNI_IsNullHandle(ah) && width > 0 && height > 0) {
        int n = width * height;
        m3g_u16 *color = (m3g_u16 *)malloc((size_t)n * sizeof(m3g_u16));
        m3g_u8  *alpha = (m3g_u8 *)malloc((size_t)n);
        if (color != NULL && alpha != NULL) {
            M3GTarget t;
            int i;
            SNI_BEGIN_RAW_POINTERS;
            {
                const jint *src = (const jint *)SNI_GetRawArrayPointer(ah);
                for (i = 0; i < n; ++i) {
                    m3g_u32 c = (m3g_u32)src[i];
                    color[i] = M3G_PACK565((c >> 16) & 0xFF,
                                           (c >> 8) & 0xFF, c & 0xFF);
                    alpha[i] = (m3g_u8)((c >> 24) & 0xFF);
                }
            }
            SNI_END_RAW_POINTERS;

            memset(&t, 0, sizeof(t));
            t.color  = color;
            t.alpha  = alpha;
            t.width  = width;
            t.height = height;
            t.clipX0 = 0;
            t.clipY0 = 0;
            t.clipX1 = width;
            t.clipY1 = height;
            if (m3g_target_bind(&t)) {
                free_image_target();
                s_imgColor = color;
                s_imgAlpha = alpha;
                s_imgW = width;
                s_imgH = height;
                s_transX = 0;
                s_transY = 0;
                s_targetW = width;
                s_targetH = height;
                ok = KNI_TRUE;
            }
        }
        if (!ok) {
            free(color);
            free(alpha);
        }
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(ok);
}

/**
 * void nReleaseTarget()
 * Con destino Image2D hay que devolver los pixeles al int[] Java; el
 * array se pasa de nuevo desde Java para no retener referencias aqui.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_m3g_M3GNative_nReleaseTarget) {
    m3g_target_release();
    /* Red de seguridad: si Java no llego a volcar el Image2D (por una
     * excepcion, por ejemplo), aqui se libera igualmente. */
    free_image_target();
    KNI_ReturnVoid();
}

/**
 * void nFlushImage(int[] argb)
 * Vuelca el destino Image2D al array Java y lo libera.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_m3g_M3GNative_nFlushImage) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(ah);
    KNI_GetParameterAsObject(1, ah);

    if (!KNI_IsNullHandle(ah) && s_imgColor != NULL) {
        int n = s_imgW * s_imgH;
        int i;
        SNI_BEGIN_RAW_POINTERS;
        {
            jint *dst = (jint *)SNI_GetRawArrayPointer(ah);
            for (i = 0; i < n; ++i) {
                m3g_u16 p = s_imgColor[i];
                dst[i] = (jint)(((m3g_u32)s_imgAlpha[i] << 24)
                              | ((m3g_u32)M3G_R8(p) << 16)
                              | ((m3g_u32)M3G_G8(p) << 8)
                              |  (m3g_u32)M3G_B8(p));
            }
        }
        SNI_END_RAW_POINTERS;
    }
    free_image_target();

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_m3g_M3GNative_nGetTargetWidth) {
    KNI_ReturnInt(s_targetW);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_m3g_M3GNative_nGetTargetHeight) {
    KNI_ReturnInt(s_targetH);
}

/**
 * void nSetViewport(int x, int y, int width, int height)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_m3g_M3GNative_nSetViewport) {
    int h = KNI_GetParameterAsInt(4);
    int w = KNI_GetParameterAsInt(3);
    int y = KNI_GetParameterAsInt(2);
    int x = KNI_GetParameterAsInt(1);
    m3g_set_viewport(x + s_transX, y + s_transY, w, h);
    KNI_ReturnVoid();
}

/**
 * void nSetDepthRange(float near, float far)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_m3g_M3GNative_nSetDepthRange) {
    jfloat far  = KNI_GetParameterAsFloat(2);
    jfloat near = KNI_GetParameterAsFloat(1);
    m3g_set_depth_range((float)near, (float)far);
    KNI_ReturnVoid();
}

/**
 * void nClear(int argb, boolean color, boolean depth, float depthValue)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_m3g_M3GNative_nClear) {
    jfloat depthValue = KNI_GetParameterAsFloat(4);
    jboolean cd = KNI_GetParameterAsBoolean(3);
    jboolean cc = KNI_GetParameterAsBoolean(2);
    jint argb = KNI_GetParameterAsInt(1);
    m3g_clear((int)argb, cc == KNI_TRUE, cd == KNI_TRUE, (float)depthValue);
    KNI_ReturnVoid();
}

/**
 * void nBlitBackground(int[] argb, int w, int h, int cx, int cy,
 *                      int cw, int ch, boolean wrapX, boolean wrapY)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_m3g_M3GNative_nBlitBackground) {
    jboolean wrapY = KNI_GetParameterAsBoolean(9);
    jboolean wrapX = KNI_GetParameterAsBoolean(8);
    int ch = KNI_GetParameterAsInt(7);
    int cw = KNI_GetParameterAsInt(6);
    int cy = KNI_GetParameterAsInt(5);
    int cx = KNI_GetParameterAsInt(4);
    int h  = KNI_GetParameterAsInt(3);
    int w  = KNI_GetParameterAsInt(2);

    KNI_StartHandles(1);
    KNI_DeclareHandle(ah);
    KNI_GetParameterAsObject(1, ah);

    if (!KNI_IsNullHandle(ah)) {
        SNI_BEGIN_RAW_POINTERS;
        m3g_blit_background((const m3g_u32 *)SNI_GetRawArrayPointer(ah),
                            w, h, cx, cy, cw, ch,
                            wrapX == KNI_TRUE, wrapY == KNI_TRUE);
        SNI_END_RAW_POINTERS;
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/* Indices del Object[] que envia Java (espejo de M3GNative.ARR_*). */
#define ARR_POSITIONS 0
#define ARR_NORMALS   1
#define ARR_COLORS    2
#define ARR_TC0       3
#define ARR_TC1       4
#define ARR_INDICES   5
#define ARR_STRIPS    6
#define ARR_TEX0      7
#define ARR_TEX1      8

/**
 * void nRender(Object[] arrays, int[] iparams, float[] fparams)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_m3g_M3GNative_nRender) {
    KNI_StartHandles(12);
    KNI_DeclareHandle(arraysH);
    KNI_DeclareHandle(ipH);
    KNI_DeclareHandle(fpH);
    KNI_DeclareHandle(hPos);
    KNI_DeclareHandle(hNrm);
    KNI_DeclareHandle(hCol);
    KNI_DeclareHandle(hTc0);
    KNI_DeclareHandle(hTc1);
    KNI_DeclareHandle(hIdx);
    KNI_DeclareHandle(hStr);
    KNI_DeclareHandle(hTex0);
    KNI_DeclareHandle(hTex1);

    KNI_GetParameterAsObject(1, arraysH);
    KNI_GetParameterAsObject(2, ipH);
    KNI_GetParameterAsObject(3, fpH);

    if (!KNI_IsNullHandle(arraysH) && !KNI_IsNullHandle(ipH)
        && !KNI_IsNullHandle(fpH)) {
        M3GBatch b;
        KNI_GetObjectArrayElement(arraysH, ARR_POSITIONS, hPos);
        KNI_GetObjectArrayElement(arraysH, ARR_NORMALS,   hNrm);
        KNI_GetObjectArrayElement(arraysH, ARR_COLORS,    hCol);
        KNI_GetObjectArrayElement(arraysH, ARR_TC0,       hTc0);
        KNI_GetObjectArrayElement(arraysH, ARR_TC1,       hTc1);
        KNI_GetObjectArrayElement(arraysH, ARR_INDICES,   hIdx);
        KNI_GetObjectArrayElement(arraysH, ARR_STRIPS,    hStr);
        KNI_GetObjectArrayElement(arraysH, ARR_TEX0,      hTex0);
        KNI_GetObjectArrayElement(arraysH, ARR_TEX1,      hTex1);

        memset(&b, 0, sizeof(b));
        SNI_BEGIN_RAW_POINTERS;
        b.positions    = KNI_IsNullHandle(hPos) ? NULL : SNI_GetRawArrayPointer(hPos);
        b.normals      = KNI_IsNullHandle(hNrm) ? NULL : SNI_GetRawArrayPointer(hNrm);
        b.colors       = KNI_IsNullHandle(hCol) ? NULL : SNI_GetRawArrayPointer(hCol);
        b.texcoords[0] = KNI_IsNullHandle(hTc0) ? NULL : SNI_GetRawArrayPointer(hTc0);
        b.texcoords[1] = KNI_IsNullHandle(hTc1) ? NULL : SNI_GetRawArrayPointer(hTc1);
        b.indices      = KNI_IsNullHandle(hIdx) ? NULL
                       : (const m3g_s32 *)SNI_GetRawArrayPointer(hIdx);
        b.stripLengths = KNI_IsNullHandle(hStr) ? NULL
                       : (const m3g_s32 *)SNI_GetRawArrayPointer(hStr);
        b.texels[0]    = KNI_IsNullHandle(hTex0) ? NULL
                       : (const m3g_u32 *)SNI_GetRawArrayPointer(hTex0);
        b.texels[1]    = KNI_IsNullHandle(hTex1) ? NULL
                       : (const m3g_u32 *)SNI_GetRawArrayPointer(hTex1);
        b.iparams      = (const m3g_s32 *)SNI_GetRawArrayPointer(ipH);
        b.fparams      = (const float *)SNI_GetRawArrayPointer(fpH);
        m3g_render(&b);
        SNI_END_RAW_POINTERS;
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * boolean nPick(Object positions, int[] iparams, float[] fparams,
 *               Object indices, int[] stripLengths, float[] ray,
 *               int[] outIdx, float[] outHit)
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_m3g_M3GNative_nPick) {
    jboolean hit = KNI_FALSE;

    KNI_StartHandles(8);
    KNI_DeclareHandle(hPos);
    KNI_DeclareHandle(hIp);
    KNI_DeclareHandle(hFp);
    KNI_DeclareHandle(hIdx);
    KNI_DeclareHandle(hStr);
    KNI_DeclareHandle(hRay);
    KNI_DeclareHandle(hOutI);
    KNI_DeclareHandle(hOutF);

    KNI_GetParameterAsObject(1, hPos);
    KNI_GetParameterAsObject(2, hIp);
    KNI_GetParameterAsObject(3, hFp);
    KNI_GetParameterAsObject(4, hIdx);
    KNI_GetParameterAsObject(5, hStr);
    KNI_GetParameterAsObject(6, hRay);
    KNI_GetParameterAsObject(7, hOutI);
    KNI_GetParameterAsObject(8, hOutF);

    if (!KNI_IsNullHandle(hPos) && !KNI_IsNullHandle(hIp)
        && !KNI_IsNullHandle(hFp) && !KNI_IsNullHandle(hStr)
        && !KNI_IsNullHandle(hRay) && !KNI_IsNullHandle(hOutI)
        && !KNI_IsNullHandle(hOutF)) {
        M3GBatch b;
        float t = 0.0f, u = 0.0f, v = 0.0f;
        int tri = 0, verts[3];
        int found;

        memset(&b, 0, sizeof(b));
        SNI_BEGIN_RAW_POINTERS;
        {
            const float *ray = (const float *)SNI_GetRawArrayPointer(hRay);
            float origin[3], dir[3];
            origin[0] = ray[0]; origin[1] = ray[1]; origin[2] = ray[2];
            dir[0] = ray[3]; dir[1] = ray[4]; dir[2] = ray[5];

            b.positions    = SNI_GetRawArrayPointer(hPos);
            b.indices      = KNI_IsNullHandle(hIdx) ? NULL
                           : (const m3g_s32 *)SNI_GetRawArrayPointer(hIdx);
            b.stripLengths = (const m3g_s32 *)SNI_GetRawArrayPointer(hStr);
            b.iparams      = (const m3g_s32 *)SNI_GetRawArrayPointer(hIp);
            b.fparams      = (const float *)SNI_GetRawArrayPointer(hFp);

            found = m3g_pick(&b, origin, dir, &t, &tri, &u, &v, verts);
            if (found) {
                jint  *oi = (jint *)SNI_GetRawArrayPointer(hOutI);
                float *of = (float *)SNI_GetRawArrayPointer(hOutF);
                oi[0] = verts[0];
                oi[1] = verts[1];
                oi[2] = verts[2];
                oi[3] = tri;
                of[0] = t;
                of[1] = u;
                of[2] = v;
                hit = KNI_TRUE;
            }
        }
        SNI_END_RAW_POINTERS;
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(hit);
}

/* ---------------------------------------------------------------- */
/* Inflate para secciones .m3g comprimidas                           */
/* ---------------------------------------------------------------- */
/*
 * El formato M3G comprime las secciones con zlib (RFC1950). Aqui se
 * reutiliza inflateData() del subsistema jarutil de MIDP -- el mismo
 * inflater con el que la VM abre los JAR -- en vez de anadir una
 * dependencia nueva o escribir un inflate en Java: CLDC 1.1 no trae
 * java.util.zip.
 */

#include <jar.h>

typedef struct {
    const unsigned char *data;
    long len;
    long pos;
} M3GMemFile;

static long m3g_mem_size(void *state) {
    return ((M3GMemFile *)state)->len;
}

static long m3g_mem_read(void *state, unsigned char *buffer, long n) {
    M3GMemFile *f = (M3GMemFile *)state;
    long avail = f->len - f->pos;
    if (avail <= 0) {
        return 0;
    }
    if (n > avail) {
        n = avail;
    }
    memcpy(buffer, f->data + f->pos, (size_t)n);
    f->pos += n;
    return n;
}

static int m3g_mem_seek(void *state, long offset, int whence) {
    M3GMemFile *f = (M3GMemFile *)state;
    long p;
    switch (whence) {
    case 0:  p = offset; break;              /* SEEK_SET */
    case 1:  p = f->pos + offset; break;     /* SEEK_CUR */
    default: p = f->len + offset; break;     /* SEEK_END */
    }
    if (p < 0 || p > f->len) {
        return -1;
    }
    f->pos = p;
    return 0;
}

static int m3g_mem_readChar(void *state) {
    M3GMemFile *f = (M3GMemFile *)state;
    if (f->pos >= f->len) {
        return -1;
    }
    return (int)f->data[f->pos++];
}

static void *m3g_heap_alloc(void *state, int n) {
    (void)state;
    return malloc((size_t)n);
}

static void m3g_heap_free(void *state, void *handle) {
    (void)state;
    free(handle);
}

static void *m3g_heap_addr(void *state, void *handle) {
    (void)state;
    return handle;
}

/**
 * boolean nInflate(byte[] src, int srcOff, int srcLen,
 *                  byte[] dst, int dstLen)
 * srcOff/srcLen delimitan el flujo zlib COMPLETO (con cabecera de 2
 * bytes); aqui se salta la cabecera y se inflan los datos deflate.
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_m3g_M3GNative_nInflate) {
    int dstLen = KNI_GetParameterAsInt(5);
    int srcLen = KNI_GetParameterAsInt(3);
    int srcOff = KNI_GetParameterAsInt(2);
    jboolean ok = KNI_FALSE;

    KNI_StartHandles(2);
    KNI_DeclareHandle(srcH);
    KNI_DeclareHandle(dstH);
    KNI_GetParameterAsObject(1, srcH);
    KNI_GetParameterAsObject(4, dstH);

    if (!KNI_IsNullHandle(srcH) && !KNI_IsNullHandle(dstH)
        && srcLen > 2 && dstLen > 0) {
        /* El inflater puede leer unos bytes mas alla del final del flujo,
         * asi que se copia a un buffer con holgura. */
        int padded = srcLen + 64;
        unsigned char *comp = (unsigned char *)malloc((size_t)padded);
        unsigned char *out = (unsigned char *)malloc((size_t)dstLen);
        if (comp != NULL && out != NULL) {
            M3GMemFile mf;
            FileObj fo;
            HeapManObj hm;

            memset(comp, 0, (size_t)padded);
            SNI_BEGIN_RAW_POINTERS;
            memcpy(comp,
                   (const unsigned char *)SNI_GetRawArrayPointer(srcH) + srcOff,
                   (size_t)srcLen);
            SNI_END_RAW_POINTERS;

            /* Se salta la cabecera zlib de 2 bytes (CMF + FLG). */
            mf.data = comp + 2;
            mf.len  = srcLen - 2;
            mf.pos  = 0;

            fo.state = &mf;
            fo.size = m3g_mem_size;
            fo.read = m3g_mem_read;
            fo.seek = m3g_mem_seek;
            fo.readChar = m3g_mem_readChar;

            hm.state = NULL;
            hm.alloc = m3g_heap_alloc;
            hm.free = m3g_heap_free;
            hm.addrFromHandle = m3g_heap_addr;

            if (inflateData(&fo, &hm, (int)mf.len, out, dstLen, 0)) {
                SNI_BEGIN_RAW_POINTERS;
                memcpy(SNI_GetRawArrayPointer(dstH), out, (size_t)dstLen);
                SNI_END_RAW_POINTERS;
                ok = KNI_TRUE;
            }
        }
        free(comp);
        free(out);
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(ok);
}
