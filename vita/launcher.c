/* launcher.c — biblioteca nativa de VitaJ2ME (estilo tienda/LiveArea).
 *
 * Todo se dibuja con primitivas de vita2d y la fuente del sistema (PGF):
 * ninguna imagen del tema. Lo único que se carga es el icono del propio
 * juego, extraído de su JAR (zip + zlib); si no tiene, se dibujan sus
 * iniciales.
 *
 * Datos: appdb/_suites.dat, escrito por el suitestore de MIDP
 * (suitestore_intern.c: write_suites_data/read_suites_data):
 *   u32 crc | i32 n | n x { 39 bytes fijos de MidletSuiteData
 *   (suiteId@0, isEnabled@8, numberOfMidlets@12, jarSize@24,
 *   jarHashLen@28) | hash | 7 x (alinear a 4, i32 len, len x UTF-16) }
 *   cadenas: clase, nombre, icono, vendor, nombre suite, jar, ajustes.
 * MIDLET_SUITE_DATA_SIZE = sizeof(struct) - sizeof(var) - 1 - 4 - 4 = 39
 * en ARM EABI, independiente del tamaño de pcsl_string.
 *
 * Controles: cruceta/stick/L/R o táctil para elegir, X jugar,
 * Triángulo detalles, START gestor clásico, Círculo salir.
 */
#include "launcher.h"

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/rtc.h>
#include <psp2/power.h>
#include <psp2/kernel/threadmgr.h>
#include <vita2d.h>
#include <zlib.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCR_W 960
#define SCR_H 544
#define MAX_GAMES 64
#define SUITES_DAT "ux0:data/vitaj2me/appdb/_suites.dat"
#define VJ_HOME "ux0:data/vitaj2me"

/* ---- paleta (misma marca que el LiveArea) ---------------------------- */
#define C_BG_TOP     RGBA8(0x0B, 0x22, 0x5E, 0xFF)
#define C_BG_BOT     RGBA8(0x03, 0x0B, 0x26, 0xFF)
#define C_BUBBLE     RGBA8(0x4A, 0x8C, 0xFF, 0x16)
#define C_WHITE      RGBA8(0xFF, 0xFF, 0xFF, 0xFF)
#define C_TEXT_2     RGBA8(0xA9, 0xBE, 0xEA, 0xFF)
#define C_TEXT_3     RGBA8(0x6E, 0x86, 0xBE, 0xFF)
#define C_CARD       RGBA8(0xFF, 0xFF, 0xFF, 0xFF)
#define C_CARD_TEXT  RGBA8(0x1A, 0x22, 0x33, 0xFF)
#define C_CARD_TEXT2 RGBA8(0x5D, 0x67, 0x79, 0xFF)
#define C_ACCENT     RGBA8(0x31, 0x77, 0xE2, 0xFF)
#define C_GLOW       RGBA8(0x7F, 0xB0, 0xFF, 0xFF)
#define C_SHADOW     RGBA8(0x00, 0x05, 0x18, 0x60)
#define C_BAR        RGBA8(0x02, 0x08, 0x1E, 0xC8)
#define C_DIM        RGBA8(0x01, 0x05, 0x14, 0xB0)
#define C_PANEL      RGBA8(0x11, 0x23, 0x55, 0xFF)
#define C_PANEL_LINE RGBA8(0x2B, 0x46, 0x8C, 0xFF)
/* Colores de los símbolos de PlayStation */
#define C_SYM_X      RGBA8(0x8C, 0xB4, 0xFF, 0xFF)
#define C_SYM_TRI    RGBA8(0x44, 0xD6, 0xA8, 0xFF)
#define C_SYM_CIR    RGBA8(0xFF, 0x70, 0x82, 0xFF)

#define CARD_W 200
#define CARD_H 252
#define CARD_GAP 30
#define CARD_Y 132
#define ART_H 168

static vita2d_pgf *font;

/* ======================================================================
 * Primitivas
 * ====================================================================== */

static unsigned int lerp_col(unsigned int a, unsigned int b, float t) {
    int i;
    unsigned int out = 0;
    for (i = 0; i < 32; i += 8) {
        int ca = (a >> i) & 0xFF, cb = (b >> i) & 0xFF;
        out |= (unsigned int)((int)(ca + (cb - ca) * t) & 0xFF) << i;
    }
    return out;
}

static unsigned int with_alpha(unsigned int c, int a) {
    return (c & 0x00FFFFFF) | ((unsigned int)(a & 0xFF) << 24);
}

/* Rectángulo redondeado OPACO (los círculos de esquina se solapan con
 * los rects; con alfa se notarían las juntas). */
static void fill_rrect(float x, float y, float w, float h, float r,
                       unsigned int c) {
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    vita2d_draw_rectangle(x + r, y, w - 2 * r, h, c);
    vita2d_draw_rectangle(x, y + r, r, h - 2 * r, c);
    vita2d_draw_rectangle(x + w - r, y + r, r, h - 2 * r, c);
    vita2d_draw_fill_circle(x + r, y + r, r, c);
    vita2d_draw_fill_circle(x + w - r, y + r, r, c);
    vita2d_draw_fill_circle(x + r, y + h - r, r, c);
    vita2d_draw_fill_circle(x + w - r, y + h - r, r, c);
}

/* Degradado vertical por franjas. */
static void fill_vgrad(float x, float y, float w, float h, unsigned int top,
                       unsigned int bot) {
    int i, steps = (int)(h / 4) + 1;
    for (i = 0; i < steps; i++) {
        float yy = y + i * 4;
        float hh = (yy + 4 > y + h) ? (y + h - yy) : 4;
        if (hh <= 0) break;
        vita2d_draw_rectangle(x, yy, w, hh,
                              lerp_col(top, bot, (float)i / steps));
    }
}

/* Línea gruesa: círculos estampados a lo largo del trazo. */
static void thick_line(float x0, float y0, float x1, float y1, float t,
                       unsigned int c) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int i, n = (int)(len * 1.5f) + 1;
    for (i = 0; i <= n; i++) {
        float k = (float)i / n;
        vita2d_draw_fill_circle(x0 + dx * k, y0 + dy * k, t / 2, c);
    }
}

static void ring(float cx, float cy, float r, float t, unsigned int c) {
    int i, n = (int)(r * 6) + 12;
    for (i = 0; i < n; i++) {
        float a = (float)i / n * 6.2831853f;
        vita2d_draw_fill_circle(cx + cosf(a) * r, cy + sinf(a) * r, t / 2, c);
    }
}

static int text_w(float scale, const char *s) {
    return vita2d_pgf_text_width(font, scale, s);
}

/* y = línea base. */
static void text(float x, float y, float scale, unsigned int c,
                 const char *s) {
    vita2d_pgf_draw_text(font, (int)x, (int)y, c, scale, s);
}

static void text_fit(float x, float y, float scale, unsigned int c,
                     const char *s, int max_w) {
    char buf[160];
    int n;
    if (text_w(scale, s) <= max_w) {
        text(x, y, scale, c, s);
        return;
    }
    snprintf(buf, sizeof(buf), "%s", s);
    n = (int)strlen(buf);
    while (n > 1) {
        /* recortar sin partir una secuencia UTF-8 */
        do { n--; } while (n > 0 && (buf[n] & 0xC0) == 0x80);
        strcpy(buf + n, "...");
        if (text_w(scale, buf) <= max_w) break;
    }
    text(x, y, scale, c, buf);
}

/* ---- glifos de botones (dibujados, no imágenes) ---------------------- */

static void glyph_cross(float cx, float cy, float s) {
    thick_line(cx - s, cy - s, cx + s, cy + s, 3, C_SYM_X);
    thick_line(cx + s, cy - s, cx - s, cy + s, 3, C_SYM_X);
}

static void glyph_triangle(float cx, float cy, float s) {
    float h = s * 1.1f;
    thick_line(cx, cy - h, cx + s * 1.15f, cy + h * 0.75f, 3, C_SYM_TRI);
    thick_line(cx + s * 1.15f, cy + h * 0.75f, cx - s * 1.15f, cy + h * 0.75f,
               3, C_SYM_TRI);
    thick_line(cx - s * 1.15f, cy + h * 0.75f, cx, cy - h, 3, C_SYM_TRI);
}

static void glyph_circle(float cx, float cy, float s) {
    ring(cx, cy, s * 1.05f, 3, C_SYM_CIR);
}

static void glyph_pill(float x, float cy, const char *label) {
    int w = text_w(0.62f, label) + 16;
    fill_rrect(x, cy - 10, w, 20, 10, RGBA8(0x2A, 0x3D, 0x70, 0xFF));
    text(x + 8, cy + 6, 0.62f, C_TEXT_2, label);
}

/* Devuelve el ancho ocupado. kind: 'x','t','o' o texto de píldora. */
static int hint(float x, float cy, int kind, const char *pill,
                const char *label) {
    int gw = 22;
    if (pill) {
        glyph_pill(x, cy, pill);
        gw = text_w(0.62f, pill) + 22;
    } else if (kind == 'x') {
        glyph_cross(x + 10, cy, 7);
    } else if (kind == 't') {
        glyph_triangle(x + 10, cy + 1, 7);
    } else {
        glyph_circle(x + 10, cy, 7);
    }
    text(x + gw + 6, cy + 7, 0.85f, C_WHITE, label);
    return gw + 6 + text_w(0.85f, label);
}

/* ======================================================================
 * Datos: _suites.dat + JAR
 * ====================================================================== */

static unsigned char *read_all(const char *path, int *len) {
    FILE *f = fopen(path, "rb");
    unsigned char *buf;
    long n;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > 4 * 1024 * 1024) { fclose(f); return NULL; }
    buf = malloc((size_t)n + 1);
    if (buf && fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        buf = NULL;
    }
    fclose(f);
    if (buf) { buf[n] = 0; *len = (int)n; }
    return buf;
}

static int rd32(const unsigned char *p) {
    return (int)(p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24));
}

static int rd16(const unsigned char *p) {
    return p[0] | (p[1] << 8);
}

/* UTF-16LE (BMP) -> UTF-8 */
static void u16_to_utf8(const unsigned char *s, int n, char *out, int cap) {
    int i, o = 0;
    for (i = 0; i < n && o < cap - 4; i++) {
        unsigned c = (unsigned)rd16(s + i * 2);
        if (c < 0x80) {
            out[o++] = (char)c;
        } else if (c < 0x800) {
            out[o++] = (char)(0xC0 | (c >> 6));
            out[o++] = (char)(0x80 | (c & 0x3F));
        } else {
            out[o++] = (char)(0xE0 | (c >> 12));
            out[o++] = (char)(0x80 | ((c >> 6) & 0x3F));
            out[o++] = (char)(0x80 | (c & 0x3F));
        }
    }
    out[o] = 0;
}

static int load_suites(vj_game *g, int max) {
    int len = 0, pos, n, i, k, count = 0;
    unsigned char *b = read_all(SUITES_DAT, &len);
    if (!b) {
        vj_log("launcher: sin %s (nada instalado)", SUITES_DAT);
        return 0;
    }
    if (len < 8) { free(b); return 0; }
    n = rd32(b + 4);
    pos = 8;
    vj_log("launcher: _suites.dat %d bytes, %d suite(s)", len, n);
    for (i = 0; i < n && count < max; i++) {
        vj_game *e = &g[count];
        char tmp[7][256];
        int hash_len;
        if (pos + 39 > len) break;
        memset(e, 0, sizeof(*e));
        e->suite_id = rd32(b + pos);
        e->midlets = rd32(b + pos + 12);
        e->jar_size = rd32(b + pos + 24);
        hash_len = rd32(b + pos + 28);
        pos += 39;
        if (hash_len < 0 || hash_len > 256 || pos + hash_len > len) break;
        pos += hash_len;
        for (k = 0; k < 7; k++) {
            int sl;
            pos = (pos + 3) & ~3;
            if (pos + 4 > len) goto bad;
            sl = rd32(b + pos);
            pos += 4;
            tmp[k][0] = 0;
            if (sl > 0) {
                if (sl > 4096 || pos + sl * 2 > len) goto bad;
                u16_to_utf8(b + pos, sl, tmp[k], sizeof(tmp[k]));
                pos += sl * 2;
            }
        }
        snprintf(e->cls, sizeof(e->cls), "%s", tmp[0]);
        snprintf(e->name, sizeof(e->name), "%s",
                 tmp[1][0] ? tmp[1] : tmp[4]);
        snprintf(e->icon, sizeof(e->icon), "%s",
                 tmp[2][0] == '/' ? tmp[2] + 1 : tmp[2]);
        snprintf(e->vendor, sizeof(e->vendor), "%s", tmp[3]);
        if (tmp[5][0] && !strchr(tmp[5], ':'))
            snprintf(e->jar, sizeof(e->jar), VJ_HOME "/%s", tmp[5]);
        else
            snprintf(e->jar, sizeof(e->jar), "%s", tmp[5]);
        vj_log("launcher: suite id=%d midlets=%d jar=%d '%s' / '%s' "
               "clase='%s' icono='%s' jar='%s'", e->suite_id, e->midlets,
               e->jar_size, e->name, e->vendor, e->cls, e->icon, e->jar);
        if (e->suite_id <= 0 || e->suite_id > 0x7FFFFF) goto bad;
        count++;
    }
    free(b);
    return count;
bad:
    vj_log("launcher: _suites.dat con formato inesperado en pos %d", pos);
    free(b);
    return count;
}

/* Lee una entrada de un zip/JAR (stored o deflate). */
static unsigned char *jar_entry(const char *jar, const char *entry,
                                int *out_len) {
    FILE *f;
    long size, tail, cd_off;
    unsigned char *t = NULL, *cd = NULL, *data = NULL, *out = NULL;
    int i, entries, cd_size, p;
    if (!jar[0] || !entry[0]) return NULL;
    f = fopen(jar, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    tail = size < 65557 ? size : 65557;
    t = malloc((size_t)tail);
    if (!t) goto done;
    fseek(f, size - tail, SEEK_SET);
    if (fread(t, 1, (size_t)tail, f) != (size_t)tail) goto done;
    for (i = (int)tail - 22; i >= 0; i--) {
        if (rd32(t + i) == 0x06054b50) break;
    }
    if (i < 0) goto done;
    entries = rd16(t + i + 10);
    cd_size = rd32(t + i + 12);
    cd_off = rd32(t + i + 16);
    if (cd_size <= 0 || cd_size > 4 * 1024 * 1024) goto done;
    cd = malloc((size_t)cd_size);
    if (!cd) goto done;
    fseek(f, cd_off, SEEK_SET);
    if (fread(cd, 1, (size_t)cd_size, f) != (size_t)cd_size) goto done;
    for (p = 0, i = 0; i < entries && p + 46 <= cd_size; i++) {
        int method = rd16(cd + p + 10);
        int csize = rd32(cd + p + 20), usize = rd32(cd + p + 24);
        int nlen = rd16(cd + p + 28), xlen = rd16(cd + p + 30);
        int clen = rd16(cd + p + 32);
        long lho = rd32(cd + p + 42);
        if (rd32(cd + p) != 0x02014b50) break;
        if ((int)strlen(entry) == nlen &&
            memcmp(cd + p + 46, entry, (size_t)nlen) == 0) {
            unsigned char lh[30];
            if (usize <= 0 || usize > 4 * 1024 * 1024) goto done;
            fseek(f, lho, SEEK_SET);
            if (fread(lh, 1, 30, f) != 30 || rd32(lh) != 0x04034b50)
                goto done;
            fseek(f, lho + 30 + rd16(lh + 26) + rd16(lh + 28), SEEK_SET);
            data = malloc((size_t)csize + 1);
            out = malloc((size_t)usize + 1);
            if (!data || !out ||
                fread(data, 1, (size_t)csize, f) != (size_t)csize) {
                free(out); out = NULL; goto done;
            }
            if (method == 0) {
                memcpy(out, data, (size_t)usize);
            } else if (method == 8) {
                z_stream zs;
                memset(&zs, 0, sizeof(zs));
                zs.next_in = data; zs.avail_in = (uInt)csize;
                zs.next_out = out; zs.avail_out = (uInt)usize;
                if (inflateInit2(&zs, -MAX_WBITS) != Z_OK ||
                    inflate(&zs, Z_FINISH) != Z_STREAM_END) {
                    inflateEnd(&zs);
                    free(out); out = NULL; goto done;
                }
                inflateEnd(&zs);
            } else {
                free(out); out = NULL; goto done;
            }
            out[usize] = 0;
            *out_len = usize;
            goto done;
        }
        p += 46 + nlen + xlen + clen;
    }
done:
    free(t); free(cd); free(data);
    fclose(f);
    return out;
}

/* Completa clase/icono desde MIDlet-1 del manifiesto si faltan. */
static void fill_from_manifest(vj_game *g) {
    int len = 0;
    char *m, *p, line[512];
    int o = 0;
    if (g->cls[0] && g->icon[0]) return;
    m = (char *)jar_entry(g->jar, "META-INF/MANIFEST.MF", &len);
    if (!m) return;
    p = strstr(m, "MIDlet-1:");
    if (p) {
        p += 9;
        /* une líneas de continuación (empiezan con espacio) */
        while (*p && o < (int)sizeof(line) - 1) {
            if (*p == '\r') { p++; continue; }
            if (*p == '\n') {
                if (p[1] == ' ') { p += 2; continue; }
                break;
            }
            line[o++] = *p++;
        }
        line[o] = 0;
        {
            char *f1 = strchr(line, ',');
            char *f2 = f1 ? strchr(f1 + 1, ',') : NULL;
            if (f1 && f2) {
                char icon[128], cls[128];
                *f2 = 0;
                snprintf(icon, sizeof(icon), "%s", f1 + 1);
                snprintf(cls, sizeof(cls), "%s", f2 + 1);
                /* trim */
                {
                    char *s = icon; while (*s == ' ') s++;
                    char *e = s + strlen(s); while (e > s && e[-1] == ' ') *--e = 0;
                    if (!g->icon[0])
                        snprintf(g->icon, sizeof(g->icon), "%s",
                                 *s == '/' ? s + 1 : s);
                    s = cls; while (*s == ' ') s++;
                    e = s + strlen(s); while (e > s && e[-1] == ' ') *--e = 0;
                    if (!g->cls[0]) snprintf(g->cls, sizeof(g->cls), "%s", s);
                }
            }
        }
    }
    free(m);
}

static vita2d_texture *load_icon(const vj_game *g) {
    int len = 0;
    vita2d_texture *t = NULL;
    unsigned char *png = jar_entry(g->jar, g->icon, &len);
    if (png) {
        t = vita2d_load_PNG_buffer(png);
        free(png);
    }
    if (t) vita2d_texture_set_filters(t, SCE_GXM_TEXTURE_FILTER_POINT,
                                      SCE_GXM_TEXTURE_FILTER_LINEAR);
    vj_log("launcher: icono '%s' de %s -> %s", g->icon, g->jar,
           t ? "OK" : "no (se dibujan iniciales)");
    return t;
}

/* ======================================================================
 * Escena
 * ====================================================================== */

typedef struct {
    vj_game games[MAX_GAMES];
    vita2d_texture *icons[MAX_GAMES];
    int n;          /* juegos */
    int sel;        /* 0..n (n = tarjeta "Gestor clásico") */
    float scroll;
    float pulse;
    int frame;
    int info;       /* overlay de detalles abierto */
} scene;

static const unsigned int art_top[] = {
    RGBA8(0x3B, 0x86, 0xF0, 0xFF), RGBA8(0x8A, 0x5C, 0xF0, 0xFF),
    RGBA8(0xF0, 0x6A, 0x8C, 0xFF), RGBA8(0x22, 0xB8, 0xA8, 0xFF),
    RGBA8(0xF5, 0x9A, 0x36, 0xFF),
};
static const unsigned int art_bot[] = {
    RGBA8(0x12, 0x3A, 0x9C, 0xFF), RGBA8(0x35, 0x1A, 0x8A, 0xFF),
    RGBA8(0x8A, 0x1E, 0x46, 0xFF), RGBA8(0x0B, 0x56, 0x66, 0xFF),
    RGBA8(0x9A, 0x44, 0x0E, 0xFF),
};

static int name_hash(const char *s) {
    unsigned h = 5381;
    while (*s) h = h * 33 + (unsigned char)*s++;
    return (int)(h % 5);
}

static void draw_background(const scene *sc) {
    int i;
    fill_vgrad(0, 0, SCR_W, SCR_H, C_BG_TOP, C_BG_BOT);
    /* burbujas a la deriva (guiño al LiveArea) */
    for (i = 0; i < 9; i++) {
        float t = sc->frame * 0.004f + i * 1.7f;
        float x = 80 + (i * 113) % 860 + sinf(t) * 40;
        float y = 120 + (i * 197) % 380 + cosf(t * 0.8f) * 30;
        float r = 50 + (i * 37) % 110;
        vita2d_draw_fill_circle(x, y, r, C_BUBBLE);
    }
}

static void draw_header(const scene *sc) {
    char buf[48];
    SceDateTime now;
    int bat;
    float x;
    text(48, 54, 1.45f, C_WHITE, "VitaJ2ME");
    text(48 + text_w(1.45f, "VitaJ2ME") + 14, 54, 0.9f, C_TEXT_2,
         "Biblioteca Java ME");
    /* reloj + batería, como la barra de estado del sistema */
    memset(&now, 0, sizeof(now));
    sceRtcGetCurrentClockLocalTime(&now);
    snprintf(buf, sizeof(buf), "%02d:%02d", now.hour, now.minute);
    x = SCR_W - 48 - text_w(1.1f, buf);
    text(x, 52, 1.1f, C_WHITE, buf);
    bat = scePowerGetBatteryLifePercent();
    if (bat >= 0 && bat <= 100) {
        float bx = x - 62, by = 36;
        unsigned int bc = bat <= 15 ? C_SYM_CIR : C_WHITE;
        vita2d_draw_rectangle(bx, by, 36, 18, C_TEXT_2);
        vita2d_draw_rectangle(bx + 2, by + 2, 32, 14, C_BG_TOP);
        vita2d_draw_rectangle(bx + 36, by + 5, 3, 8, C_TEXT_2);
        vita2d_draw_rectangle(bx + 3, by + 3, 30.0f * bat / 100, 12, bc);
    }
    /* encabezado de sección */
    text(48, 112, 0.95f, C_WHITE, "Tus juegos");
    snprintf(buf, sizeof(buf), sc->n == 1 ? "%d juego" : "%d juegos", sc->n);
    text(48 + text_w(0.95f, "Tus juegos") + 12, 112, 0.8f, C_TEXT_3, buf);
}

static void draw_initials(const char *name, float cx, float cy, float sc) {
    char ini[4] = {0};
    const char *p = name;
    int n = 0;
    while (*p && n < 2) {
        if ((p == name || p[-1] == ' ') && *p != ' ' &&
            (unsigned char)*p < 0x80) {
            ini[n++] = *p;
        }
        p++;
    }
    if (!n) ini[0] = '?';
    text(cx - text_w(sc, ini) / 2.0f, cy + 14 * sc, sc, C_WHITE, ini);
}

static void draw_gear(float cx, float cy, float r, unsigned int c,
                      unsigned int hole) {
    int i;
    for (i = 0; i < 8; i++) {
        float a = i * 0.7853982f;
        vita2d_draw_fill_circle(cx + cosf(a) * r, cy + sinf(a) * r,
                                r * 0.3f, c);
    }
    vita2d_draw_fill_circle(cx, cy, r, c);
    vita2d_draw_fill_circle(cx, cy, r * 0.42f, hole);
}

static void card_rect(const scene *sc, int i, float *x, float *y, float *w,
                      float *h) {
    float grow = (i == sc->sel) ? 10 + sinf(sc->pulse) * 1.5f : 0;
    *x = 48 + i * (CARD_W + CARD_GAP) - sc->scroll - grow;
    *y = CARD_Y - grow;
    *w = CARD_W + grow * 2;
    *h = CARD_H + grow * 2;
}

static void draw_card(const scene *sc, int i) {
    float x, y, w, h, art;
    int selected = (i == sc->sel);
    card_rect(sc, i, &x, &y, &w, &h);
    if (x > SCR_W || x + w < 0) return;
    art = ART_H * (h / CARD_H);

    vita2d_draw_rectangle(x + 6, y + h - 2, w - 12, 10, C_SHADOW);
    if (selected) {
        fill_rrect(x - 4, y - 4, w + 8, h + 8, 18, C_GLOW);
    }
    if (i < sc->n) {
        const vj_game *g = &sc->games[i];
        int hue = name_hash(g->name);
        vita2d_texture *ic = sc->icons[i];
        char meta[64];
        fill_rrect(x, y, w, h, 14, C_CARD);
        /* arte: degradado de color propio del juego */
        fill_rrect(x, y, w, 28, 14, art_top[hue]);
        fill_vgrad(x, y + 14, w, art - 14, art_top[hue], art_bot[hue]);
        {
            float tile = 96 * (w / CARD_W);
            float tx = x + (w - tile) / 2, ty = y + (art - tile) / 2;
            fill_rrect(tx, ty, tile, tile, 20, RGBA8(0xFF, 0xFF, 0xFF, 0xFF));
            if (ic) {
                float iw = vita2d_texture_get_width(ic);
                float ih = vita2d_texture_get_height(ic);
                float s = (tile - 20) / (iw > ih ? iw : ih);
                vita2d_draw_texture_scale(ic, tx + (tile - iw * s) / 2,
                                          ty + (tile - ih * s) / 2, s, s);
            } else {
                fill_rrect(tx + 8, ty + 8, tile - 16, tile - 16, 14,
                           art_bot[hue]);
                draw_initials(g->name, tx + tile / 2, ty + tile / 2 - 8,
                              1.6f);
            }
        }
        text_fit(x + 14, y + art + 30, 0.95f, C_CARD_TEXT, g->name,
                 (int)w - 28);
        text_fit(x + 14, y + art + 54, 0.75f, C_CARD_TEXT2,
                 g->vendor[0] ? g->vendor : "Java ME", (int)w - 28);
        snprintf(meta, sizeof(meta), "%d KB", g->jar_size / 1024);
        text(x + 14, y + h - 14, 0.68f, C_ACCENT, meta);
    } else {
        /* tarjeta del gestor clásico */
        fill_rrect(x, y, w, h, 14, C_PANEL);
        draw_gear(x + w / 2, y + art / 2, 26 * (w / CARD_W), C_TEXT_2,
                  C_PANEL);
        text_fit(x + 14, y + art + 30, 0.95f, C_WHITE, "Gestor clásico",
                 (int)w - 28);
        text_fit(x + 14, y + art + 54, 0.72f, C_TEXT_2,
                 "VM, red, teclas, instalar", (int)w - 28);
        text(x + 14, y + h - 14, 0.68f, C_GLOW, "AMS de PSPKVM");
    }
}

static void draw_selected_info(const scene *sc) {
    char buf[200];
    float y = 440;
    if (sc->sel < sc->n) {
        const vj_game *g = &sc->games[sc->sel];
        text_fit(48, y, 1.15f, C_WHITE, g->name, SCR_W - 96);
        snprintf(buf, sizeof(buf), "%s   ·   %d KB   ·   %s",
                 g->vendor[0] ? g->vendor : "Java ME", g->jar_size / 1024,
                 g->midlets > 1 ? "varios MIDlets" : "MIDP");
        text_fit(48, y + 28, 0.8f, C_TEXT_2, buf, SCR_W - 96);
    } else {
        text(48, y, 1.15f, C_WHITE, "Gestor clásico");
        text(48, y + 28, 0.8f, C_TEXT_2,
             "Abre el administrador original de PSPKVM (ajustes de la VM, "
             "red, mapeo de teclas).");
    }
}

static void draw_bottom_bar(const scene *sc) {
    float x, cy = 522;
    vita2d_draw_rectangle(0, 498, SCR_W, 46, C_BAR);
    vita2d_draw_rectangle(0, 498, SCR_W, 1, RGBA8(0x2B, 0x46, 0x8C, 0xFF));
    x = 48;
    if (sc->info) {
        x += hint(x, cy, 'x', NULL, sc->sel < sc->n ? "Jugar" : "Abrir") + 28;
        hint(x, cy, 'o', NULL, "Volver");
        return;
    }
    x += hint(x, cy, 'x', NULL, sc->sel < sc->n ? "Jugar" : "Abrir") + 28;
    if (sc->sel < sc->n) x += hint(x, cy, 't', NULL, "Detalles") + 28;
    x += hint(x, cy, 0, "START", "Gestor clásico") + 28;
    hint(x, cy, 'o', NULL, "Salir");
}

static void draw_info_overlay(const scene *sc) {
    const vj_game *g;
    float px = 150, py = 92, pw = 660, ph = 360;
    char buf[300];
    float y;
    int hue;
    if (!sc->info || sc->sel >= sc->n) return;
    g = &sc->games[sc->sel];
    hue = name_hash(g->name);
    vita2d_draw_rectangle(0, 0, SCR_W, SCR_H, C_DIM);
    fill_rrect(px - 1, py - 1, pw + 2, ph + 2, 19, C_PANEL_LINE);
    fill_rrect(px, py, pw, ph, 18, C_PANEL);
    fill_rrect(px + 28, py + 28, 140, 140, 24, art_top[hue]);
    if (sc->icons[sc->sel]) {
        vita2d_texture *ic = sc->icons[sc->sel];
        float iw = vita2d_texture_get_width(ic);
        float ih = vita2d_texture_get_height(ic);
        float s = 100 / (iw > ih ? iw : ih);
        vita2d_draw_texture_scale(ic, px + 28 + (140 - iw * s) / 2,
                                  py + 28 + (140 - ih * s) / 2, s, s);
    } else {
        draw_initials(g->name, px + 98, py + 90, 2.0f);
    }
    text_fit(px + 196, py + 62, 1.3f, C_WHITE, g->name, (int)pw - 220);
    text_fit(px + 196, py + 94, 0.9f, C_TEXT_2,
             g->vendor[0] ? g->vendor : "Java ME", (int)pw - 220);
    y = py + 206;
    snprintf(buf, sizeof(buf), "Tamaño del JAR:   %d KB", g->jar_size / 1024);
    text(px + 28, y, 0.8f, C_TEXT_2, buf);
    snprintf(buf, sizeof(buf), "Clase MIDlet:   %s", g->cls);
    text_fit(px + 28, y + 30, 0.8f, C_TEXT_2, buf, (int)pw - 56);
    snprintf(buf, sizeof(buf), "Suite:   #%d   ·   %d MIDlet(s)", g->suite_id,
             g->midlets);
    text(px + 28, y + 60, 0.8f, C_TEXT_2, buf);
    snprintf(buf, sizeof(buf), "Archivo:   %s", g->jar);
    text_fit(px + 28, y + 90, 0.8f, C_TEXT_3, buf, (int)pw - 56);
}

static void draw_empty_hint(const scene *sc) {
    if (sc->n) return;
    text(48 + CARD_W + CARD_GAP + 24, CARD_Y + 96, 1.0f, C_WHITE,
         "Todavía no hay juegos instalados");
    text(48 + CARD_W + CARD_GAP + 24, CARD_Y + 128, 0.8f, C_TEXT_2,
         "Copia tus .jar a ux0:data/vitaj2me/games/ y reinicia VitaJ2ME:");
    text(48 + CARD_W + CARD_GAP + 24, CARD_Y + 152, 0.8f, C_TEXT_2,
         "se instalan solos al arrancar.");
}

static void draw_scene(const scene *sc) {
    int i;
    vita2d_start_drawing();
    vita2d_clear_screen();
    draw_background(sc);
    draw_header(sc);
    /* la seleccionada al final, por encima de sus vecinas */
    for (i = 0; i <= sc->n; i++)
        if (i != sc->sel) draw_card(sc, i);
    draw_card(sc, sc->sel);
    draw_empty_hint(sc);
    draw_selected_info(sc);
    draw_info_overlay(sc);
    draw_bottom_bar(sc);
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

/* Frame final mientras arranca la JVM: queda en pantalla hasta que el
 * MIDlet pinta su primer frame. */
static void draw_launching(const scene *sc, const char *name, float fade) {
    int i;
    vita2d_start_drawing();
    vita2d_clear_screen();
    draw_background(sc);
    vita2d_draw_rectangle(0, 0, SCR_W, SCR_H,
                          with_alpha(C_BG_BOT, (int)(255 * fade)));
    text((SCR_W - text_w(1.3f, name)) / 2.0f, 300, 1.3f, C_WHITE, name);
    text((SCR_W - text_w(0.85f, "Iniciando…")) / 2.0f, 336, 0.85f,
         C_TEXT_2, "Iniciando…");
    for (i = 0; i < 8; i++) {
        float a = i * 0.7853982f + sc->frame * 0.12f;
        vita2d_draw_fill_circle(SCR_W / 2 + cosf(a) * 22,
                                232 + sinf(a) * 22, 2.5f + i * 0.45f,
                                with_alpha(C_GLOW, 60 + i * 24));
    }
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

/* ======================================================================
 * Bucle
 * ====================================================================== */

static float target_scroll(const scene *sc) {
    float t = 48 + sc->sel * (CARD_W + CARD_GAP) + CARD_W / 2.0f - SCR_W / 2;
    float maxs = 48 + sc->n * (CARD_W + CARD_GAP) + CARD_W + 48 - SCR_W;
    if (t > maxs) t = maxs;
    if (t < 0) t = 0;
    return t;
}

int launcher_run(vj_game *out) {
    static scene sc;
    SceCtrlData pad;
    SceTouchData touch;
    unsigned int old = 0;
    int was_touch = 0, hold = 0, result = LAUNCHER_EXIT, i;

    memset(&sc, 0, sizeof(sc));
    font = vita2d_load_default_pgf();
    if (!font) {
        vj_log("launcher: sin fuente PGF -> gestor clásico");
        return LAUNCHER_CLASSIC;
    }
    sc.n = load_suites(sc.games, MAX_GAMES);
    for (i = 0; i < sc.n; i++) {
        fill_from_manifest(&sc.games[i]);
        sc.icons[i] = load_icon(&sc.games[i]);
    }
    vj_log("launcher: %d juego(s) listados", sc.n);

    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,
                             SCE_TOUCH_SAMPLING_STATE_START);
    vita2d_set_vblank_wait(1);  /* animación suave a 60 Hz */

    for (;;) {
        unsigned int now, down;
        int act_play = 0, act_back = 0;
        sceCtrlPeekBufferPositive(0, &pad, 1);
        now = pad.buttons;
        if (pad.lx < 50) now |= SCE_CTRL_LEFT;
        if (pad.lx > 205) now |= SCE_CTRL_RIGHT;
        down = now & ~old;
        /* repetición al mantener izquierda/derecha */
        if (now & (SCE_CTRL_LEFT | SCE_CTRL_RIGHT)) {
            hold++;
            if (hold > 24 && hold % 7 == 0)
                down |= now & (SCE_CTRL_LEFT | SCE_CTRL_RIGHT);
        } else {
            hold = 0;
        }
        old = now;

        memset(&touch, 0, sizeof(touch));
        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
        if (touch.reportNum > 0 && !was_touch && !sc.info) {
            float tx = touch.report[0].x / 2.0f, ty = touch.report[0].y / 2.0f;
            for (i = 0; i <= sc.n; i++) {
                float x, y, w, h;
                card_rect(&sc, i, &x, &y, &w, &h);
                if (tx >= x && tx < x + w && ty >= y && ty < y + h) {
                    if (i == sc.sel) act_play = 1;
                    else sc.sel = i;
                }
            }
        }
        was_touch = touch.reportNum > 0;

        if (sc.info) {
            if (down & SCE_CTRL_CROSS) act_play = 1;
            if (down & (SCE_CTRL_CIRCLE | SCE_CTRL_TRIANGLE)) sc.info = 0;
        } else {
            if (down & (SCE_CTRL_LEFT | SCE_CTRL_LTRIGGER) && sc.sel > 0)
                sc.sel--;
            if (down & (SCE_CTRL_RIGHT | SCE_CTRL_RTRIGGER) && sc.sel < sc.n)
                sc.sel++;
            if (down & SCE_CTRL_CROSS) act_play = 1;
            if ((down & SCE_CTRL_TRIANGLE) && sc.sel < sc.n) sc.info = 1;
            if (down & SCE_CTRL_START) {
                sc.sel = sc.n;
                act_play = 1;
            }
            if (down & SCE_CTRL_CIRCLE) act_back = 1;
        }

        if (act_back) { result = LAUNCHER_EXIT; break; }
        if (act_play) {
            if (sc.sel >= sc.n) { result = LAUNCHER_CLASSIC; break; }
            if (!sc.games[sc.sel].cls[0]) {
                vj_log("launcher: '%s' sin clase de MIDlet -> gestor "
                       "clásico", sc.games[sc.sel].name);
                result = LAUNCHER_CLASSIC;
                break;
            }
            *out = sc.games[sc.sel];
            result = LAUNCHER_PLAY;
            break;
        }

        sc.scroll += (target_scroll(&sc) - sc.scroll) * 0.2f;
        sc.pulse += 0.08f;
        sc.frame++;
        draw_scene(&sc);
    }

    /* transición + frame de espera */
    if (result != LAUNCHER_EXIT) {
        const char *nm = result == LAUNCHER_PLAY ? out->name
                                                 : "Gestor clásico";
        for (i = 1; i <= 14; i++) {
            sc.frame++;
            draw_launching(&sc, nm, i / 14.0f);
        }
        vj_log("launcher: %s", result == LAUNCHER_PLAY ?
               "lanzar juego directo" : "abrir gestor clásico");
    }
    vita2d_wait_rendering_done(); /* la GPU suelta iconos y atlas PGF */
    for (i = 0; i < sc.n; i++)
        if (sc.icons[i]) vita2d_free_texture(sc.icons[i]);
    vita2d_free_pgf(font);
    font = NULL;
    /* lcd.c presenta sin esperar vblank (bug #24): restaurar. */
    vita2d_set_vblank_wait(0);
    return result;
}
