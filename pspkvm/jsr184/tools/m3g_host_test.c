/* Test de host del motor M3G: renderiza a un buffer RGB565 y vuelca PPM. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "m3g_engine.h"
#include "m3g_pixel.h"

#define W 200
#define H 150

static m3g_u16 fb[W*H];
static m3g_u8  fba[W*H];

static void dump(const char *name) {
    FILE *f = fopen(name, "wb");
    int i;
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (i = 0; i < W*H; ++i) {
        unsigned char px[3];
        px[0] = M3G_R8(fb[i]); px[1] = M3G_G8(fb[i]); px[2] = M3G_B8(fb[i]);
        fwrite(px, 1, 3, f);
    }
    fclose(f);
}

static void perspective(float *m, float fovy, float aspect, float n, float f) {
    float t = 1.0f / tanf(fovy * 0.5f * 3.14159265f / 180.0f);
    memset(m, 0, 16*sizeof(float));
    m[0]  = t / aspect;
    m[5]  = t;
    m[10] = (f + n) / (n - f);
    m[11] = 2.0f * f * n / (n - f);
    m[14] = -1.0f;
}

int main(void) {
    M3GTarget t;
    M3GBatch  b;
    m3g_s32 ip[M3G_IP_COUNT];
    float   fp[M3G_FP_COUNT];
    /* cubo: 8 vertices, tiras */
    static const short pos[] = {
        -100,-100,-100,   100,-100,-100,  -100, 100,-100,   100, 100,-100,
        -100,-100, 100,   100,-100, 100,  -100, 100, 100,   100, 100, 100
    };
    static const signed char nrm[] = {
        -73,-73,-73,  73,-73,-73, -73, 73,-73,  73, 73,-73,
        -73,-73, 73,  73,-73, 73, -73, 73, 73,  73, 73, 73
    };
    static const unsigned char col[] = {
        255,0,0,255,   0,255,0,255,   0,0,255,255,   255,255,0,255,
        255,0,255,255, 0,255,255,255, 255,255,255,255, 128,128,128,255
    };
    static const short tc[] = {
        0,0, 1,0, 0,1, 1,1, 0,0, 1,0, 0,1, 1,1
    };
    /* 6 caras como 6 tiras de 4 indices */
    static const m3g_s32 idx[] = {
        0,1,2,3,   5,4,7,6,   4,5,0,1,   2,3,6,7,   4,0,6,2,   1,5,3,7
    };
    static const m3g_s32 strips[] = { 4,4,4,4,4,4 };
    static m3g_u32 tex[16*16];
    float mv[16], proj[16], rot[16], trans[16];
    int i, x, y;

    for (y = 0; y < 16; ++y)
        for (x = 0; x < 16; ++x)
            tex[y*16+x] = (((x>>2)+(y>>2)) & 1) ? 0xFFFFFFFFu : 0xFF404080u;

    for (i = 0; i < W*H; ++i) { fb[i] = 0; fba[i] = 255; }

    memset(&t, 0, sizeof(t));
    t.color = fb; t.alpha = fba;
    t.width = W; t.height = H;
    t.clipX0 = 0; t.clipY0 = 0; t.clipX1 = W; t.clipY1 = H;
    if (!m3g_target_bind(&t)) { printf("bind FAIL\n"); return 1; }
    m3g_set_viewport(0, 0, W, H);
    m3g_clear(0xFF102030, 1, 1, 1.0f);

    memset(ip, 0, sizeof(ip));
    memset(fp, 0, sizeof(fp));
    ip[M3G_IP_VERTEX_COUNT] = 8;
    ip[M3G_IP_POS_COMPONENTS] = 3;
    ip[M3G_IP_POS_TYPE] = 2;
    ip[M3G_IP_NORMAL_TYPE] = 1;
    ip[M3G_IP_COLOR_COMPONENTS] = 4;
    ip[M3G_IP_TC0_TYPE] = 2;
    ip[M3G_IP_TC0_COMPONENTS] = 2;
    ip[M3G_IP_INDEX_COUNT] = 24;
    ip[M3G_IP_STRIP_COUNT] = 6;
    ip[M3G_IP_CULLING] = M3G_CULL_BACK;
    ip[M3G_IP_WINDING] = M3G_WINDING_CCW;
    ip[M3G_IP_SHADING] = M3G_SHADE_SMOOTH;
    ip[M3G_IP_PERSPECTIVE] = 1;
    ip[M3G_IP_BLENDING] = M3G_BLEND_REPLACE;
    ip[M3G_IP_DEPTH_TEST] = 1;
    ip[M3G_IP_DEPTH_WRITE] = 1;
    ip[M3G_IP_COLOR_WRITE] = 1;
    ip[M3G_IP_ALPHA_WRITE] = 1;
    ip[M3G_IP_LIGHTING] = 1;
    ip[M3G_IP_VERTEX_COLOR_TRACK] = 1;
    ip[M3G_IP_TEX0_ENABLED] = 1;
    ip[M3G_IP_TEX0_WIDTH] = 16;
    ip[M3G_IP_TEX0_HEIGHT] = 16;
    ip[M3G_IP_TEX0_WRAP_S] = M3G_WRAP_REPEAT;
    ip[M3G_IP_TEX0_WRAP_T] = M3G_WRAP_REPEAT;
    ip[M3G_IP_TEX0_FILTER] = M3G_FILTER_LINEAR;
    ip[M3G_IP_TEX0_BLEND] = M3G_FUNC_MODULATE;
    ip[M3G_IP_LIGHT_COUNT] = 2;
    ip[M3G_IP_DEFAULT_COLOR] = 0xFFFFFFFF;

    fp[M3G_FP_POS_SCALE] = 1.0f/100.0f;
    fp[M3G_FP_TC0_SCALE] = 1.0f;
    m3g_mat_identity(fp + M3G_FP_TEX0_MATRIX);
    m3g_mat_identity(fp + M3G_FP_TEX1_MATRIX);
    fp[M3G_FP_MAT_AMBIENT+0]=0.2f; fp[M3G_FP_MAT_AMBIENT+1]=0.2f; fp[M3G_FP_MAT_AMBIENT+2]=0.2f; fp[M3G_FP_MAT_AMBIENT+3]=1;
    fp[M3G_FP_MAT_DIFFUSE+0]=1; fp[M3G_FP_MAT_DIFFUSE+1]=1; fp[M3G_FP_MAT_DIFFUSE+2]=1; fp[M3G_FP_MAT_DIFFUSE+3]=1;
    fp[M3G_FP_MAT_SPECULAR+0]=1; fp[M3G_FP_MAT_SPECULAR+1]=1; fp[M3G_FP_MAT_SPECULAR+2]=1;
    fp[M3G_FP_MAT_SHININESS]=32.0f;
    /* luz 0: ambiente */
    fp[M3G_FP_LIGHTS + 0*M3G_LP_STRIDE + M3G_LP_MODE] = (float)M3G_LIGHT_AMBIENT;
    fp[M3G_FP_LIGHTS + 0*M3G_LP_STRIDE + M3G_LP_R] = 0.5f;
    fp[M3G_FP_LIGHTS + 0*M3G_LP_STRIDE + M3G_LP_G] = 0.5f;
    fp[M3G_FP_LIGHTS + 0*M3G_LP_STRIDE + M3G_LP_B] = 0.5f;
    /* luz 1: direccional desde (0,0,-1) */
    fp[M3G_FP_LIGHTS + 1*M3G_LP_STRIDE + M3G_LP_MODE] = (float)M3G_LIGHT_DIRECTIONAL;
    fp[M3G_FP_LIGHTS + 1*M3G_LP_STRIDE + M3G_LP_R] = 1.0f;
    fp[M3G_FP_LIGHTS + 1*M3G_LP_STRIDE + M3G_LP_G] = 1.0f;
    fp[M3G_FP_LIGHTS + 1*M3G_LP_STRIDE + M3G_LP_B] = 1.0f;
    fp[M3G_FP_LIGHTS + 1*M3G_LP_STRIDE + M3G_LP_DZ] = -1.0f;
    fp[M3G_FP_LIGHTS + 1*M3G_LP_STRIDE + M3G_LP_ATT_C] = 1.0f;

    /* modelview: rotacion Y 35 + X 25, traslacion z -4 */
    {
        float a = 35.0f * 3.14159265f/180.0f, c = cosf(a), s = sinf(a);
        float bx = 25.0f * 3.14159265f/180.0f, cb = cosf(bx), sb = sinf(bx);
        float ry[16], rx[16];
        m3g_mat_identity(ry);
        ry[0]=c; ry[2]=s; ry[8]=-s; ry[10]=c;
        m3g_mat_identity(rx);
        rx[5]=cb; rx[6]=-sb; rx[9]=sb; rx[10]=cb;
        m3g_mat_mul(rot, rx, ry);
        m3g_mat_identity(trans);
        trans[11] = -4.0f;
        m3g_mat_mul(mv, trans, rot);
    }
    perspective(proj, 45.0f, (float)W/(float)H, 0.5f, 50.0f);
    memcpy(fp + M3G_FP_MODELVIEW, mv, 16*sizeof(float));
    memcpy(fp + M3G_FP_PROJECTION, proj, 16*sizeof(float));

    memset(&b, 0, sizeof(b));
    b.positions = pos;
    b.normals = nrm;
    b.colors = col;
    b.texcoords[0] = tc;
    b.indices = idx;
    b.stripLengths = strips;
    b.texels[0] = tex;
    b.iparams = ip;
    b.fparams = fp;

    m3g_render(&b);
    dump("out.ppm");

    /* Estadisticas basicas de validacion */
    {
        int painted = 0, bg = 0;
        m3g_u16 bgpx = M3G_PACK565(0x10, 0x20, 0x30);
        for (i = 0; i < W*H; ++i) { if (fb[i] == bgpx) bg++; else painted++; }
        printf("pixeles pintados por el cubo: %d / %d (fondo %d)\n", painted, W*H, bg);
        if (painted < 1000) { printf("FALLO: el cubo casi no se dibujo\n"); return 1; }
    }

    /* Picking: rayo por el centro hacia -Z debe golpear */
    {
        float o[3] = {0,0,10}, d[3] = {0,0,-1};
        float tt, uu, vv; int tri;
        int pv[3];
        if (m3g_pick(&b, o, d, &tt, &tri, &uu, &vv, pv)) {
            printf("pick OK: t=%.3f tri=%d u=%.3f v=%.3f verts=%d,%d,%d\n", tt, tri, uu, vv, pv[0], pv[1], pv[2]);
        } else {
            printf("FALLO: pick no encontro interseccion\n");
            return 1;
        }
    }
    printf("OK\n");
    return 0;
}
