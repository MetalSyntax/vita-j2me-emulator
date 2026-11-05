/* prof.c — perfilador ligero de nativos de MIDP (v1.1, plan Fase F).
 *
 * La VM llama a los nativos KNI por la tabla de nativos (otro objeto), así
 * que `--wrap` los intercepta sin recompilar la VM. Cada envoltorio mide
 * el tiempo del nativo real; el HB (main.c) imprime cada 10 s los que más
 * tiempo se llevan y el resto del hilo de la VM (= intérprete + GC + lo no
 * medido).
 *
 * Solo mide si existe ux0:data/vitaj2me/config/profile (si no, el coste es
 * un `if` por llamada). Los nativos KNI no tienen argumentos C (los leen de
 * la pila Java) y devuelven en r0/r1: el envoltorio devuelve 64 bits para
 * conservar los dos registros.
 */
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <string.h>

void vj_log(const char *fmt, ...);

#define PROF_LIST(X) \
    X(Java_javax_microedition_lcdui_Graphics_render) \
    X(Java_javax_microedition_lcdui_Graphics_renderRegion) \
    X(Java_javax_microedition_lcdui_Graphics_drawRGB) \
    X(Java_javax_microedition_lcdui_Graphics_fillRect) \
    X(Java_javax_microedition_lcdui_Graphics_drawRect) \
    X(Java_javax_microedition_lcdui_Graphics_drawLine) \
    X(Java_javax_microedition_lcdui_Graphics_fillTriangle) \
    X(Java_javax_microedition_lcdui_Graphics_fillArc) \
    X(Java_javax_microedition_lcdui_Graphics_fillRoundRect) \
    X(Java_javax_microedition_lcdui_Graphics_drawString) \
    X(Java_javax_microedition_lcdui_Graphics_drawSubstring) \
    X(Java_javax_microedition_lcdui_Graphics_drawChars) \
    X(Java_javax_microedition_lcdui_Graphics_drawChar) \
    X(Java_javax_microedition_lcdui_Display_refresh0) \
    X(Java_javax_microedition_lcdui_Display_directFlush) \
    X(Java_javax_microedition_lcdui_ImageDataFactory_loadPNG) \
    X(Java_javax_microedition_lcdui_ImageDataFactory_loadRGB) \
    X(Java_javax_microedition_lcdui_ImageDataFactory_loadRegion) \
    X(Java_javax_microedition_lcdui_ImageData_getRGB) \
    X(Java_com_nokia_mid_ui_DirectGraphicsImpl_drawPixels___3SZIIIIIIII) \
    X(Java_com_nokia_mid_ui_DirectGraphicsImpl_drawPixels___3IZIIIIIIII) \
    X(Java_com_nokia_mid_ui_DirectGraphicsImpl_fillPolygon) \
    X(Java_javax_microedition_m3g_M3GNative_nRender)

enum {
#define X(f) P_##f,
    PROF_LIST(X)
#undef X
    P_N
};

static const char *const prof_name[P_N] = {
#define X(f) #f,
    PROF_LIST(X)
#undef X
};

int vj_prof_on = 0;
static unsigned long long prof_us[P_N];
static unsigned long prof_calls[P_N];

#define X(f)                                                          \
    long long __real_##f(void);                                       \
    long long __wrap_##f(void) {                                      \
        SceUInt32 t0;                                                 \
        long long r;                                                  \
        if (!vj_prof_on) return __real_##f();                         \
        t0 = sceKernelGetProcessTimeLow();                            \
        r = __real_##f();                                             \
        prof_us[P_##f] += sceKernelGetProcessTimeLow() - t0;          \
        prof_calls[P_##f]++;                                          \
        return r;                                                     \
    }
PROF_LIST(X)
#undef X

void vj_prof_init(void) {
    SceIoStat st;
    vj_prof_on = sceIoGetstat("ux0:data/vitaj2me/config/profile", &st) >= 0;
    if (vj_prof_on)
        vj_log("perfil: ACTIVO (config/profile): nativos de dibujo medidos");
}

/* Nombre corto: lo que va tras el último '_' de la clase. */
static const char *short_name(const char *n) {
    const char *p = strstr(n, "Graphics_");
    if (p) return p + 9;
    p = strstr(n, "Display_");
    if (p) return p + 8;
    p = strstr(n, "ImageDataFactory_");
    if (p) return p + 17;
    p = strstr(n, "DirectGraphicsImpl_");
    if (p) return p + 19;
    p = strstr(n, "ImageData_");
    if (p) return p + 10;
    p = strstr(n, "M3GNative_");
    return p ? p + 10 : n;
}

/* Desde el HB cada 10 s: vm_us = tiempo de CPU del hilo de la VM. */
void vj_prof_report(unsigned long long vm_us) {
    char line[512];
    unsigned long long tot = 0;
    int order[P_N], i, j, n = 0, len;
    if (!vj_prof_on) return;
    for (i = 0; i < P_N; i++) {
        tot += prof_us[i];
        order[i] = i;
    }
    for (i = 0; i < P_N; i++)
        for (j = i + 1; j < P_N; j++)
            if (prof_us[order[j]] > prof_us[order[i]]) {
                int t = order[i];
                order[i] = order[j];
                order[j] = t;
            }
    len = snprintf(line, sizeof(line),
                   "PERF 10s: VM %llu ms = nativos %llu ms + resto %llu ms |",
                   vm_us / 1000, tot / 1000,
                   vm_us > tot ? (vm_us - tot) / 1000 : 0);
    for (i = 0; i < P_N && n < 8; i++) {
        int k = order[i];
        if (!prof_calls[k]) break;
        len += snprintf(line + len, sizeof(line) - len, " %s %llums/%lu",
                        short_name(prof_name[k]), prof_us[k] / 1000,
                        prof_calls[k]);
        if (len >= (int)sizeof(line)) break;
        n++;
    }
    vj_log("%s", line);
    memset(prof_us, 0, sizeof(prof_us));
    memset(prof_calls, 0, sizeof(prof_calls));
}
