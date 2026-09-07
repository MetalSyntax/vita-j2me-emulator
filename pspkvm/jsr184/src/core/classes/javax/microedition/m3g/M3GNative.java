/*
 * M3GNative.java — puente KNI con el motor por software (jsr184/src/engine).
 *
 * Todos los datos masivos (vertices, indices, texeles) se pasan como
 * arrays Java; el lado nativo los lee en sitio con SNI_GetRawArrayPointer,
 * de modo que no hay memoria nativa asociada a objetos Java ni, por
 * tanto, nada que liberar cuando el GC los recoge (CLDC 1.1 no tiene
 * finalize()).
 */
package javax.microedition.m3g;

final class M3GNative {

    private M3GNative() { }

    /* --- indices del array int[] de estado (espejo de m3g_engine.h) --- */
    static final int IP_VERTEX_COUNT       = 0;
    static final int IP_POS_COMPONENTS     = 1;
    static final int IP_POS_TYPE           = 2;
    static final int IP_NORMAL_TYPE        = 3;
    static final int IP_COLOR_COMPONENTS   = 4;
    static final int IP_TC0_TYPE           = 5;
    static final int IP_TC0_COMPONENTS     = 6;
    static final int IP_TC1_TYPE           = 7;
    static final int IP_TC1_COMPONENTS     = 8;
    static final int IP_INDEX_COUNT        = 9;
    static final int IP_STRIP_COUNT        = 10;
    static final int IP_CULLING            = 11;
    static final int IP_WINDING            = 12;
    static final int IP_SHADING            = 13;
    static final int IP_TWO_SIDED          = 14;
    static final int IP_PERSPECTIVE        = 15;
    static final int IP_BLENDING           = 16;
    static final int IP_DEPTH_TEST         = 17;
    static final int IP_DEPTH_WRITE        = 18;
    static final int IP_COLOR_WRITE        = 19;
    static final int IP_ALPHA_WRITE        = 20;
    static final int IP_LIGHTING           = 21;
    static final int IP_VERTEX_COLOR_TRACK = 22;
    static final int IP_TEX0_ENABLED       = 23;
    static final int IP_TEX0_WIDTH         = 24;
    static final int IP_TEX0_HEIGHT        = 25;
    static final int IP_TEX0_WRAP_S        = 26;
    static final int IP_TEX0_WRAP_T        = 27;
    static final int IP_TEX0_FILTER        = 28;
    static final int IP_TEX0_BLEND         = 29;
    static final int IP_TEX0_BLEND_COLOR   = 30;
    static final int IP_TEX1_ENABLED       = 31;
    static final int IP_TEX1_WIDTH         = 32;
    static final int IP_TEX1_HEIGHT        = 33;
    static final int IP_TEX1_WRAP_S        = 34;
    static final int IP_TEX1_WRAP_T        = 35;
    static final int IP_TEX1_FILTER        = 36;
    static final int IP_TEX1_BLEND         = 37;
    static final int IP_TEX1_BLEND_COLOR   = 38;
    static final int IP_FOG_MODE           = 39;
    static final int IP_FOG_COLOR          = 40;
    static final int IP_LIGHT_COUNT        = 41;
    static final int IP_DEFAULT_COLOR      = 42;
    static final int IP_COUNT              = 43;

    /* --- indices del array float[] de estado --- */
    static final int MAX_LIGHTS            = 8;
    static final int FP_MODELVIEW          = 0;
    static final int FP_PROJECTION         = 16;
    static final int FP_POS_SCALE          = 32;
    static final int FP_POS_BIAS           = 33;
    static final int FP_TC0_SCALE          = 36;
    static final int FP_TC0_BIAS           = 37;
    static final int FP_TC1_SCALE          = 40;
    static final int FP_TC1_BIAS           = 41;
    static final int FP_TEX0_MATRIX        = 44;
    static final int FP_TEX1_MATRIX        = 60;
    static final int FP_MAT_AMBIENT        = 76;
    static final int FP_MAT_DIFFUSE        = 80;
    static final int FP_MAT_EMISSIVE       = 84;
    static final int FP_MAT_SPECULAR       = 88;
    static final int FP_MAT_SHININESS      = 92;
    static final int FP_ALPHA_THRESHOLD    = 93;
    static final int FP_DEPTH_OFFSET_FACTOR = 94;
    static final int FP_DEPTH_OFFSET_UNITS  = 95;
    static final int FP_FOG_DENSITY        = 96;
    static final int FP_FOG_NEAR           = 97;
    static final int FP_FOG_FAR            = 98;
    static final int FP_LIGHTS             = 99;
    static final int FP_COUNT              = 99 + MAX_LIGHTS * 16;

    /* --- layout de cada luz --- */
    static final int LP_MODE     = 0;
    static final int LP_R        = 1;
    static final int LP_G        = 2;
    static final int LP_B        = 3;
    static final int LP_PX       = 4;
    static final int LP_PY       = 5;
    static final int LP_PZ       = 6;
    static final int LP_DX       = 7;
    static final int LP_DY       = 8;
    static final int LP_DZ       = 9;
    static final int LP_SPOT_COS = 10;
    static final int LP_SPOT_EXP = 11;
    static final int LP_ATT_C    = 12;
    static final int LP_ATT_L    = 13;
    static final int LP_ATT_Q    = 14;
    static final int LP_STRIDE   = 16;

    /* --- indices del array de objetos que recibe nRender --- */
    static final int ARR_POSITIONS = 0;
    static final int ARR_NORMALS   = 1;
    static final int ARR_COLORS    = 2;
    static final int ARR_TC0       = 3;
    static final int ARR_TC1       = 4;
    static final int ARR_INDICES   = 5;
    static final int ARR_STRIPS    = 6;
    static final int ARR_TEX0      = 7;
    static final int ARR_TEX1      = 8;
    static final int ARR_COUNT     = 9;

    /** Fija como destino el buffer de un Graphics de MIDP. */
    static native boolean nBindGraphics(Object graphics);

    /** Fija como destino los texeles de un Image2D (int[] ARGB). */
    static native boolean nBindImage(int[] argb, int width, int height);

    /** Vuelca el destino Image2D al int[] Java y libera sus buffers. */
    static native void nFlushImage(int[] argb);

    /** Libera el destino ligado. */
    static native void nReleaseTarget();

    static native void nSetViewport(int x, int y, int width, int height);

    static native void nSetDepthRange(float near, float far);

    static native void nClear(int argb, boolean clearColor,
                              boolean clearDepth, float depthValue);

    static native void nBlitBackground(int[] argb, int width, int height,
                                       int cropX, int cropY,
                                       int cropW, int cropH,
                                       boolean wrapX, boolean wrapY);

    static native void nRender(Object[] arrays, int[] iparams, float[] fparams);

    /**
     * Interseccion rayo/malla en coordenadas locales.
     * ray = {ox,oy,oz,dx,dy,dz}; outIdx recibe {i0,i1,i2,triIndex} y
     * outHit recibe {t,u,v}.
     */
    static native boolean nPick(Object positions, int[] iparams,
                                float[] fparams, Object indices,
                                int[] stripLengths, float[] ray,
                                int[] outIdx, float[] outHit);

    /**
     * Descomprime un flujo zlib (RFC1950) reutilizando el inflater de
     * MIDP: CLDC 1.1 no incluye java.util.zip.
     */
    static native boolean nInflate(byte[] src, int srcOff, int srcLen,
                                   byte[] dst, int dstLen);

    /** Ancho/alto del destino ligado (para los viewports por defecto). */
    static native int nGetTargetWidth();

    static native int nGetTargetHeight();
}
