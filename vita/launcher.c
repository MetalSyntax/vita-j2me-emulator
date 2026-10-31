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
 * Cuadrado ajustes, Triángulo detalles, Círculo salir. (Sesión 46: sin
 * gestor clásico: nada de la UI de PSPKVM llega a pantalla.)
 */
#include "launcher.h"
#include "gamecfg.h"

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/rtc.h>
#include <psp2/power.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/cpu.h>
#include <vita2d.h>
#include <zlib.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define SCR_W 960
#define SCR_H 544
#define MAX_GAMES 256
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
#define C_SYM_SQ     RGBA8(0xF2, 0x8F, 0xD9, 0xFF)

#define CARD_W 200
#define CARD_H 252
#define CARD_GAP 30
#define CARD_Y 132
#define ART_H 168

static vita2d_pgf *font;

/* Estado de la pantalla de carga (ver launcher_loading_start). */
static struct {
    vj_game game;
    vita2d_texture *icon;
    SceUID thread;
    volatile int stop;
    volatile unsigned long midlet_frames;
    SceUInt64 t0;
    int frame;
    int install;    /* pantalla de instalación: nº de juegos del lote */
    int install_base;  /* suites instaladas antes del lote */
    int install_done;
} ld;

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
void fill_rrect(float x, float y, float w, float h, float r,
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

static void glyph_square(float cx, float cy, float s) {
    thick_line(cx - s, cy - s, cx + s, cy - s, 3, C_SYM_SQ);
    thick_line(cx + s, cy - s, cx + s, cy + s, 3, C_SYM_SQ);
    thick_line(cx + s, cy + s, cx - s, cy + s, 3, C_SYM_SQ);
    thick_line(cx - s, cy + s, cx - s, cy - s, 3, C_SYM_SQ);
}

static void glyph_pill(float x, float cy, const char *label) {
    int w = text_w(0.62f, label) + 16;
    fill_rrect(x, cy - 10, w, 20, 10, RGBA8(0x2A, 0x3D, 0x70, 0xFF));
    text(x + 8, cy + 6, 0.62f, C_TEXT_2, label);
}

/* Devuelve el ancho ocupado. kind: 'x','t','s','o' o texto de píldora. */
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
    } else if (kind == 's') {
        glyph_square(x + 10, cy, 6);
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

/* Suites instaladas según la cabecera de _suites.dat (u32 crc, i32 n). */
static int suites_count(void) {
    unsigned char h[8];
    int n = 0;
    FILE *f = fopen(SUITES_DAT, "rb");
    if (!f) return 0;
    if (fread(h, 1, 8, f) == 8) n = rd32(h + 4);
    fclose(f);
    return n < 0 ? 0 : n;
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
        /* HW 039: jarHashLen = -1 (0xFFFFFFFF) en suites sin hash;
         * read_suites_data solo lo usa si es > 0. */
        if (hash_len < 0) hash_len = 0;
        if (hash_len > 256 || pos + hash_len > len) goto bad;
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
        /* El instalador (hack de referencia) no copia el JAR a appdb/: la
         * suite guarda la URL de origen, p.ej. "file:///games/SoD.jar".
         * Las raíces JSR-75 del port cuelgan de VJ_HOME (fileconnection.c). */
        if (strncmp(tmp[5], "file:///", 8) == 0)
            snprintf(e->jar, sizeof(e->jar), VJ_HOME "/%s", tmp[5] + 8);
        else if (tmp[5][0] && !strchr(tmp[5], ':'))
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

/* Directorio central de un zip/JAR abierto (NULL si no es un zip). */
static unsigned char *jar_cd(FILE *f, int *entries, int *cd_size) {
    long size, tail, cd_off;
    unsigned char *t, *cd = NULL;
    int i;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    tail = size < 65557 ? size : 65557;
    t = malloc((size_t)tail);
    if (!t) return NULL;
    fseek(f, size - tail, SEEK_SET);
    if (fread(t, 1, (size_t)tail, f) != (size_t)tail) goto done;
    for (i = (int)tail - 22; i >= 0; i--) {
        if (rd32(t + i) == 0x06054b50) break;
    }
    if (i < 0) goto done;
    *entries = rd16(t + i + 10);
    *cd_size = rd32(t + i + 12);
    cd_off = rd32(t + i + 16);
    if (*cd_size <= 0 || *cd_size > 4 * 1024 * 1024) goto done;
    cd = malloc((size_t)*cd_size);
    if (!cd) goto done;
    fseek(f, cd_off, SEEK_SET);
    if (fread(cd, 1, (size_t)*cd_size, f) != (size_t)*cd_size) {
        free(cd);
        cd = NULL;
    }
done:
    free(t);
    return cd;
}

/* Lee una entrada de un zip/JAR (stored o deflate). */
static unsigned char *jar_entry(const char *jar, const char *entry,
                                int *out_len) {
    FILE *f;
    unsigned char *cd = NULL, *data = NULL, *out = NULL;
    int i, entries = 0, cd_size = 0, p;
    if (!jar[0] || !entry[0]) return NULL;
    f = fopen(jar, "rb");
    if (!f) return NULL;
    cd = jar_cd(f, &entries, &cd_size);
    if (!cd) goto done;
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
    free(cd); free(data);
    fclose(f);
    return out;
}

/* ¿Tiene MANIFEST? Sin él el instalador de MIDP da CORRUPT_JAR (36):
 * p.ej. código de APK de Android pasado a .jar (HW 068-070). */
int jar_has_manifest(const char *jar) {
    int len;
    unsigned char *m = jar_entry(jar, "META-INF/MANIFEST.MF", &len);
    free(m);
    return m != NULL;
}

/* Ancho y alto de un PNG del JAR sin descomprimirlo entero: basta con
 * los primeros 24 bytes (firma + IHDR). */
static int png_size(FILE *f, const unsigned char *ce, int *w, int *h) {
    unsigned char lh[30], in[256], out[24];
    int method = rd16(ce + 10), csize = rd32(ce + 20), n;
    fseek(f, rd32(ce + 42), SEEK_SET);
    if (fread(lh, 1, 30, f) != 30 || rd32(lh) != 0x04034b50) return 0;
    fseek(f, rd32(ce + 42) + 30 + rd16(lh + 26) + rd16(lh + 28), SEEK_SET);
    n = csize < (int)sizeof(in) ? csize : (int)sizeof(in);
    if (n < 24 || fread(in, 1, (size_t)n, f) != (size_t)n) return 0;
    if (method == 0) {
        memcpy(out, in, 24);
    } else if (method == 8) {
        z_stream zs;
        int ok;
        memset(&zs, 0, sizeof(zs));
        zs.next_in = in; zs.avail_in = (uInt)n;
        zs.next_out = out; zs.avail_out = sizeof(out);
        if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) return 0;
        inflate(&zs, Z_SYNC_FLUSH);
        ok = zs.total_out >= 24;
        inflateEnd(&zs);
        if (!ok) return 0;
    } else {
        return 0;
    }
    if (memcmp(out, "\x89PNG", 4) != 0 || memcmp(out + 12, "IHDR", 4) != 0)
        return 0;
    *w = (out[16] << 24) | (out[17] << 16) | (out[18] << 8) | out[19];
    *h = (out[20] << 24) | (out[21] << 16) | (out[22] << 8) | out[23];
    return 1;
}

static int known_res(int w, int h) {
    int i;
    for (i = 0; i < vj_res_n; i++)
        if (vj_res[i][0] == w && vj_res[i][1] == h) return 1;
    return 0;
}

/* Atributo "W,H" o "WxH" del manifiesto. */
static int manifest_size(const char *m, const char *key, int *w, int *h) {
    const char *p = strstr(m, key);
    if (!p) return 0;
    p += strlen(key);
    while (*p == ':' || *p == ' ') p++;
    return sscanf(p, "%d%*[,xX ]%d", w, h) == 2 && *w > 0 && *h > 0 &&
           *w <= 864 && *h <= 864;
}

int jar_detect_screen(const char *jar, int *w, int *h) {
    static const char *const keys[] = {
        "Nokia-MIDlet-Original-Display-Size",
        "Nokia-MIDlet-Target-Display-Size",
        "SEMC-Screen-Size",  /* Sony Ericsson, p.ej. Die Hard: 432,240 */
        "MIDlet-Screen-Size",
    };
    int len = 0, i, p, entries = 0, cd_size = 0, best = 0;
    char *m = (char *)jar_entry(jar, "META-INF/MANIFEST.MF", &len);
    FILE *f;
    unsigned char *cd;
    if (m) {
        for (i = 0; i < (int)(sizeof(keys) / sizeof(keys[0])); i++) {
            if (manifest_size(m, keys[i], w, h)) {
                free(m);
                return 1;
            }
        }
        free(m);
    }
    /* El nombre del fichero: muchos packs lo traen, p.ej.
     * "Alien_Quarantine_480x800(v1.2.6).jar". */
    {
        const char *b = strrchr(jar, '/'), *q;
        for (q = b ? b + 1 : jar; *q; q++) {
            if (*q >= '1' && *q <= '9' && sscanf(q, "%dx%d", w, h) == 2 &&
                known_res(*w, *h)) {
                return 1;
            }
        }
    }
    /* Sin atributos: el PNG más grande con tamaño de pantalla conocido
     * (splash y fondos a pantalla completa, típico de Gameloft y cía). */
    f = fopen(jar, "rb");
    if (!f) return 0;
    cd = jar_cd(f, &entries, &cd_size);
    for (p = 0, i = 0; cd && i < entries && p + 46 <= cd_size; i++) {
        int nlen = rd16(cd + p + 28), pw, ph;
        const char *name = (const char *)cd + p + 46;
        if (rd32(cd + p) != 0x02014b50) break;
        if (nlen > 4 && strncasecmp(name + nlen - 4, ".png", 4) == 0 &&
            png_size(f, cd + p, &pw, &ph) && known_res(pw, ph) &&
            pw * ph > best) {
            best = pw * ph;
            *w = pw;
            *h = ph;
        }
        p += 46 + nlen + rd16(cd + p + 30) + rd16(cd + p + 32);
    }
    if (best) {
        free(cd);
        fclose(f);
        return 1;
    }
    /* Último recurso: constantes de la pantalla en el bytecode. Los juegos
     * de tamaño fijo (Gameloft y cía) usan su ancho/alto como `sipush` por
     * todo el código. Solo valores distintivos (128/240/320 salen en
     * cualquier juego), cada par con su orientación habitual, y gana el
     * par con más apariciones. Validado con 30 JAR reales (sesión 53):
     * Asphalt 6 v1.2.0 -> 640x360, que antes caía en 240x320 cortado. */
    {
        static const int rules[][4] = {  /* w, h, valor a, valor b */
            {480, 854, 854, 480}, {480, 800, 800, 480}, {640, 360, 640, 360},
            {432, 240, 432, 240}, {400, 240, 400, 240}, {320, 480, 480, 320},
            {352, 416, 352, 416},
        };
        static const int vals[] = {854, 800, 640, 480, 432, 416, 400, 360,
                                   352, 320, 240};
        int cnt[11] = {0}, nv = 11, r, score = 0;
        for (p = 0, i = 0; cd && i < entries && p + 46 <= cd_size; i++) {
            int nlen = rd16(cd + p + 28), clen = 0, k, v;
            char cname[256];
            unsigned char *cls;
            if (rd32(cd + p) != 0x02014b50) break;
            if (nlen > 6 && nlen < (int)sizeof(cname) &&
                strncmp((const char *)cd + p + 46 + nlen - 6, ".class", 6) == 0) {
                memcpy(cname, cd + p + 46, (size_t)nlen);
                cname[nlen] = 0;
                cls = jar_entry(jar, cname, &clen);
                for (k = 0; cls && k + 2 < clen; k++) {
                    if (cls[k] != 0x11) continue;  /* sipush */
                    v = (cls[k + 1] << 8) | cls[k + 2];
                    for (r = 0; r < nv; r++)
                        if (vals[r] == v) cnt[r]++;
                }
                free(cls);
            }
            p += 46 + nlen + rd16(cd + p + 30) + rd16(cd + p + 32);
        }
        for (r = 0; r < (int)(sizeof(rules) / sizeof(rules[0])); r++) {
            int a = 0, b = 0, k, s;
            for (k = 0; k < nv; k++) {
                if (vals[k] == rules[r][2]) a = cnt[k];
                if (vals[k] == rules[r][3]) b = cnt[k];
            }
            s = a < b ? a : b;
            if (s >= 8 && s > score) {
                score = s;
                *w = rules[r][0];
                *h = rules[r][1];
            }
        }
        if (score)
            vj_log("cfg: resolución deducida del bytecode: %dx%d (%d usos)",
                   *w, *h, score);
        best = score;
    }
    free(cd);
    fclose(f);
    return best > 0;
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
                if (!g->name[0]) {
                    char *s = line;
                    while (*s == ' ') s++;
                    snprintf(g->name, sizeof(g->name), "%.*s",
                             (int)(f1 - s), s);
                }
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
    int sel;        /* 0..n-1 */
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

static void draw_background(int frame) {
    int i;
    fill_vgrad(0, 0, SCR_W, SCR_H, C_BG_TOP, C_BG_BOT);
    /* burbujas a la deriva (guiño al LiveArea) */
    for (i = 0; i < 9; i++) {
        float t = frame * 0.004f + i * 1.7f;
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
         L("Java ME for PS Vita", "Java ME para PS Vita",
           "Java ME para PS Vita"));
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
    {
        const char *t = L("Your games", "Tus juegos", "Seus jogos");
        text(48, 112, 0.95f, C_WHITE, t);
        snprintf(buf, sizeof(buf), "%d %s", sc->n,
                 sc->n == 1 ? L("game", "juego", "jogo")
                            : L("games", "juegos", "jogos"));
        text(48 + text_w(0.95f, t) + 12, 112, 0.8f, C_TEXT_3, buf);
    }
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
    {
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
        if (g->missing)
            text_fit(x + 14, y + h - 14, 0.68f, C_SYM_CIR,
                     L("JAR file missing", "Falta el archivo JAR",
                       "Arquivo JAR ausente"), (int)w - 28);
        else
            text(x + 14, y + h - 14, 0.68f, C_ACCENT, meta);
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
                 g->midlets > 1 ? L("several MIDlets", "varios MIDlets",
                                    "vários MIDlets") : "MIDP");
        text_fit(48, y + 28, 0.8f, C_TEXT_2, buf, SCR_W - 96);
    }
}

static void draw_bottom_bar(const scene *sc) {
    float x, cy = 522;
    vita2d_draw_rectangle(0, 498, SCR_W, 46, C_BAR);
    vita2d_draw_rectangle(0, 498, SCR_W, 1, RGBA8(0x2B, 0x46, 0x8C, 0xFF));
    x = 48;
    if (sc->info) {
        x += hint(x, cy, 'x', NULL, L("Play", "Jugar", "Jogar")) + 28;
        x += hint(x, cy, 's', NULL, L("Settings", "Ajustes", "Ajustes")) + 28;
        hint(x, cy, 'o', NULL, L("Back", "Volver", "Voltar"));
        return;
    }
    if (sc->sel < sc->n) {
        x += hint(x, cy, 'x', NULL, L("Play", "Jugar", "Jogar")) + 28;
        x += hint(x, cy, 's', NULL, L("Settings", "Ajustes", "Ajustes")) + 28;
        x += hint(x, cy, 't', NULL, L("Details", "Detalles", "Detalhes")) + 28;
    }
    x += hint(x, cy, 0, "SELECT", L("Options", "Opciones", "Opções")) + 28;
    hint(x, cy, 'o', NULL, L("Exit", "Salir", "Sair"));
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
    snprintf(buf, sizeof(buf), "%s:   %d KB",
             L("JAR size", "Tamaño del JAR", "Tamanho do JAR"),
             g->jar_size / 1024);
    text(px + 28, y, 0.8f, C_TEXT_2, buf);
    snprintf(buf, sizeof(buf), "%s:   %s",
             L("MIDlet class", "Clase MIDlet", "Classe MIDlet"), g->cls);
    text_fit(px + 28, y + 30, 0.8f, C_TEXT_2, buf, (int)pw - 56);
    snprintf(buf, sizeof(buf), "Suite:   #%d   ·   %d MIDlet(s)", g->suite_id,
             g->midlets);
    text(px + 28, y + 60, 0.8f, C_TEXT_2, buf);
    snprintf(buf, sizeof(buf), "%s:   %s", L("File", "Archivo", "Arquivo"),
             g->jar);
    text_fit(px + 28, y + 90, 0.8f, C_TEXT_3, buf, (int)pw - 56);
}

static void draw_empty_hint(const scene *sc) {
    if (sc->n) return;
    text(48, CARD_Y + 96, 1.0f, C_WHITE,
         L("No games installed yet", "Todavía no hay juegos instalados",
           "Ainda não há jogos instalados"));
    text(48, CARD_Y + 128, 0.8f, C_TEXT_2,
         L("Copy your .jar files to ux0:data/vitaj2me/games/ and restart "
           "VitaJ2ME:",
           "Copia tus .jar a ux0:data/vitaj2me/games/ y reinicia VitaJ2ME:",
           "Copie seus .jar para ux0:data/vitaj2me/games/ e reinicie o "
           "VitaJ2ME:"));
    text(48, CARD_Y + 152, 0.8f, C_TEXT_2,
         L("they install themselves on startup.",
           "se instalan solos al arrancar.",
           "eles se instalam sozinhos ao iniciar."));
}

static void draw_scene(const scene *sc) {
    int i;
    vita2d_start_drawing();
    vita2d_clear_screen();
    draw_background(sc->frame);
    draw_header(sc);
    /* la seleccionada al final, por encima de sus vecinas */
    for (i = 0; i < sc->n; i++)
        if (i != sc->sel) draw_card(sc, i);
    if (sc->n) draw_card(sc, sc->sel);
    draw_empty_hint(sc);
    draw_selected_info(sc);
    draw_info_overlay(sc);
    draw_bottom_bar(sc);
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

/* ======================================================================
 * Ajustes: por juego (□ en la biblioteca, L+R+START en juego) y globales
 * (SELECT en la biblioteca: idioma, FPS y valores por defecto de todos
 * los juegos). Misma pantalla; cambia la lista de filas.
 * ====================================================================== */

enum {
    ROW_LANG, ROW_FPS,
    ROW_RES, ROW_SCALE, ROW_FILTER, ROW_ROTATE, ROW_MIRROR, ROW_PAD,
    ROW_TOUCH, ROW_SPEED, ROW_MAP0, ROW_RESET = ROW_MAP0 + VJ_BTN_N, ROW_N
};
enum { SET_GAME, SET_INGAME, SET_GLOBAL };

/* Filas visibles según el modo; devuelve cuántas. */
static int settings_rows(int mode, int *rows) {
    int r, n = 0;
    for (r = 0; r < ROW_N; r++) {
        if (mode == SET_GLOBAL) {
            /* resolución y orientación dependen de cada juego */
            if (r == ROW_RES || r == ROW_ROTATE || r == ROW_MIRROR) continue;
        } else if (r == ROW_LANG || r == ROW_FPS) {
            continue;
        }
        rows[n++] = r;
    }
    return n;
}

static const char *lang_names[] = {"English", "Español", "Português"};

static const char *scale_name(int s) {
    switch (s) {
    case VJ_SCALE_INT: return L("Integer (sharp)", "Escalado entero (nítido)",
                                "Inteira (nítida)");
    case VJ_SCALE_STRETCH: return L("Stretch to area", "Estirar a la zona",
                                    "Esticar na área");
    case VJ_SCALE_1X: return L("Original size", "Tamaño original",
                               "Tamanho original");
    default: return L("Fit (keep aspect)", "Ajustar (mantiene proporción)",
                      "Ajustar (mantém proporção)");
    }
}

static const char *mirror_name(int m) {
    switch (m) {
    case 1: return L("Horizontal", "Horizontal", "Horizontal");
    case 2: return L("Vertical", "Vertical", "Vertical");
    case 3: return L("Both", "Horizontal y vertical", "Horizontal e vertical");
    default: return L("None", "Ninguno", "Nenhum");
    }
}

static const char *speed_name(int sp) {
    switch (sp) {
    case 1: return L("Fast (max. 30 FPS)", "Rápida (máx. 30 FPS)",
                     "Rápida (máx. 30 FPS)");
    case 2: return L("Turbo (max. 30 FPS)", "Turbo (máx. 30 FPS)",
                     "Turbo (máx. 30 FPS)");
    default: return L("Normal", "Normal", "Normal");
    }
}

static const char *pad_name(int k) {
    switch (k) {
    case VJ_PAD_OFF: return L("Off", "Desactivado", "Desativado");
    case VJ_PAD_LEFT: return L("Left side", "A la izquierda", "À esquerda");
    case VJ_PAD_RIGHT: return L("Right side", "A la derecha", "À direita");
    case VJ_PAD_BOTH: return L("Both sides", "A ambos lados",
                               "Dos dois lados");
    default: return L("Automatic", "Automático", "Automático");
    }
}

static const char *on_off(int v) {
    return v ? L("On", "Activado", "Ativado") : L("Off", "Desactivado",
                                                  "Desativado");
}

static const char *row_help(int r, int mode) {
    switch (r) {
    case ROW_LANG:
        return L("Language of the VitaJ2ME interface.",
                 "Idioma de la interfaz de VitaJ2ME.",
                 "Idioma da interface do VitaJ2ME.");
    case ROW_FPS:
        return L("Shows the real frames per second in a corner while playing.",
                 "Muestra en una esquina los fotogramas por segundo reales "
                 "mientras juegas.",
                 "Mostra num canto os quadros por segundo reais enquanto "
                 "você joga.");
    case ROW_RES:
        return L("Screen of the phone the game was made for. Automatic reads "
                 "it from the JAR. Restarts the game.",
                 "Pantalla del móvil para el que se hizo el juego. Automática "
                 "la deduce del JAR. Requiere reiniciar el juego.",
                 "Tela do celular para o qual o jogo foi feito. Automática a "
                 "deduz do JAR. Requer reiniciar o jogo.");
    case ROW_SCALE:
        return L("How the game is enlarged on the Vita screen.",
                 "Cómo se agranda el juego en la pantalla de la Vita.",
                 "Como o jogo é ampliado na tela do Vita.");
    case ROW_FILTER:
        return L("Sharp keeps the pixels; smooth blurs them when scaling.",
                 "Nítido conserva los píxeles; suave los difumina al escalar.",
                 "Nítido mantém os pixels; suave os suaviza ao ampliar.");
    case ROW_ROTATE:
        return L("Rotates the game image (clockwise). Touch and D-pad "
                 "rotate with it.",
                 "Gira la imagen del juego (sentido horario). El táctil y la "
                 "cruceta se giran con ella.",
                 "Gira a imagem do jogo (sentido horário). O toque e o "
                 "direcional giram junto.");
    case ROW_MIRROR:
        return L("Mirrors the game image, after rotating it.",
                 "Refleja la imagen del juego, después de girarla.",
                 "Espelha a imagem do jogo, depois de girá-la.");
    case ROW_SPEED:
        return L("Shortens the game's waits (Thread.sleep) so it runs "
                 "smoother, capped at 30 FPS. If a game runs too fast, go "
                 "back to Normal.",
                 "Acorta las esperas del juego (Thread.sleep) para que vaya "
                 "más fluido, con un tope de 30 FPS. Si un juego va acelerado "
                 "de más, vuelve a Normal.",
                 "Encurta as esperas do jogo (Thread.sleep) para rodar mais "
                 "fluido, com limite de 30 FPS. Se um jogo ficar rápido "
                 "demais, volte para Normal.");
    case ROW_PAD:
        return L("Touch phone keypad on the sides, game in the center. "
                 "Automatic: only for portrait games. In game, "
                 "L+R+Triangle shows or hides it.",
                 "Teclado de móvil táctil en los laterales, con el juego en el "
                 "centro. Automático: solo en juegos verticales. En juego, "
                 "L+R+Triángulo lo muestra u oculta.",
                 "Teclado de celular nas laterais, com o jogo no centro. "
                 "Automático: só em jogos verticais. No jogo, "
                 "L+R+Triângulo mostra ou oculta.");
    case ROW_TOUCH:
        return L("Touching the game sends the pointer to the MIDlet (touch "
                 "games). Restarts the game.",
                 "Tocar el juego envía el puntero al MIDlet (juegos "
                 "táctiles). Requiere reiniciar el juego.",
                 "Tocar o jogo envia o ponteiro ao MIDlet (jogos de toque). "
                 "Requer reiniciar o jogo.");
    case ROW_RESET:
        return mode == SET_GLOBAL
            ? L("Back to the factory defaults (language is kept).",
                "Vuelve a los valores de fábrica (el idioma se mantiene).",
                "Volta aos valores de fábrica (o idioma é mantido).")
            : L("Back to the global defaults for this game.",
                "Vuelve a los valores globales por defecto en este juego.",
                "Volta aos valores globais padrão neste jogo.");
    default:
        return L("Phone key sent by this Vita button.",
                 "Tecla del móvil que envía este botón de la Vita.",
                 "Tecla do celular enviada por este botão do Vita.");
    }
}

static void row_text(const vj_cfg *c, int r, char *label, char *val, int cap) {
    int w, h;
    if (r >= ROW_MAP0 && r < ROW_RESET) {
        snprintf(label, cap, "%s", cfg_btn_name(r - ROW_MAP0));
        snprintf(val, cap, "%s", cfg_key_name(c->map[r - ROW_MAP0]));
        return;
    }
    val[0] = 0;
    switch (r) {
    case ROW_LANG:
        snprintf(label, cap, "%s", L("Language", "Idioma", "Idioma"));
        snprintf(val, cap, "%s", lang_names[vj_lang]);
        break;
    case ROW_FPS:
        snprintf(label, cap, "%s", L("FPS counter", "Contador de FPS",
                                     "Contador de FPS"));
        snprintf(val, cap, "%s", on_off(vj_show_fps));
        break;
    case ROW_RES:
        snprintf(label, cap, "%s", L("Resolution", "Resolución", "Resolução"));
        cfg_screen(c, &w, &h);
        if (c->res) snprintf(val, cap, "%d x %d", w, h);
        else snprintf(val, cap, "%s (%d x %d)",
                      L("Automatic", "Automática", "Automática"), w, h);
        break;
    case ROW_SCALE:
        snprintf(label, cap, "%s", L("Scaling", "Escala", "Escala"));
        snprintf(val, cap, "%s", scale_name(c->scale));
        break;
    case ROW_FILTER:
        snprintf(label, cap, "%s", L("Filter", "Filtro", "Filtro"));
        snprintf(val, cap, "%s", c->filter ? L("Smooth", "Suave", "Suave")
                                           : L("Sharp", "Nítido", "Nítido"));
        break;
    case ROW_PAD:
        snprintf(label, cap, "%s", L("Virtual keypad", "Teclado virtual",
                                     "Teclado virtual"));
        snprintf(val, cap, "%s", pad_name(c->keypad));
        break;
    case ROW_TOUCH:
        snprintf(label, cap, "%s", L("Touch screen", "Pantalla táctil",
                                     "Tela de toque"));
        snprintf(val, cap, "%s", on_off(c->touch));
        break;
    case ROW_ROTATE:
        snprintf(label, cap, "%s", L("Rotation", "Rotación", "Rotação"));
        snprintf(val, cap, "%d°", c->rotate * 90);
        break;
    case ROW_MIRROR:
        snprintf(label, cap, "%s", L("Mirror", "Espejo", "Espelho"));
        snprintf(val, cap, "%s", mirror_name(c->mirror));
        break;
    case ROW_SPEED:
        snprintf(label, cap, "%s", L("Speed", "Velocidad", "Velocidade"));
        snprintf(val, cap, "%s", speed_name(c->speed));
        break;
    default:
        snprintf(label, cap, "%s", L("Restore defaults",
                                     "Restaurar valores por defecto",
                                     "Restaurar padrões"));
        break;
    }
}

static int wrap(int v, int d, int n) {
    v += d;
    if (v < 0) v = n - 1;
    if (v >= n) v = 0;
    return v;
}

static void row_change(vj_cfg *c, int r, int d) {
    int i;
    if (r >= ROW_MAP0 && r < ROW_RESET) {
        int *m = &c->map[r - ROW_MAP0];
        for (i = 0; i < vj_keys_n; i++)
            if (vj_key_codes[i] == *m) break;
        *m = vj_key_codes[wrap(i < vj_keys_n ? i : 0, d, vj_keys_n)];
        return;
    }
    switch (r) {
    case ROW_LANG: vj_lang = wrap(vj_lang, d, 3); break;
    case ROW_FPS: vj_show_fps = !vj_show_fps; break;
    case ROW_RES: c->res = wrap(c->res, d, vj_res_n + 1); break;
    case ROW_SCALE: c->scale = wrap(c->scale - 1, d, 4) + 1; break;
    case ROW_FILTER: c->filter = !c->filter; break;
    case ROW_PAD: c->keypad = wrap(c->keypad, d, 5); break;
    case ROW_TOUCH: c->touch = !c->touch; break;
    case ROW_ROTATE: c->rotate = wrap(c->rotate, d, 4); break;
    case ROW_MIRROR: c->mirror = wrap(c->mirror, d, 4); break;
    case ROW_SPEED: c->speed = wrap(c->speed, d, 3); break;
    }
}

/* Miniatura de la pantalla de la Vita con el layout real. */
static void draw_preview(const vj_cfg *c, float px, float py, float pw) {
    vj_layout lo;
    float k = pw / SCR_W, ph = SCR_H * k;
    char buf[32];
    int i, w, h;
    cfg_layout(c, &lo);
    cfg_screen(c, &w, &h);
    fill_rrect(px - 6, py - 6, pw + 12, ph + 12, 12, RGBA8(0x00, 0x00, 0x00, 0xFF));
    for (i = 0; i < 2; i++)
        if (lo.panel[i][2] > 0)
            vita2d_draw_rectangle(px + lo.panel[i][0] * k, py + lo.panel[i][1] * k,
                                  lo.panel[i][2] * k, lo.panel[i][3] * k,
                                  RGBA8(0x07, 0x10, 0x2C, 0xFF));
    for (i = 0; i < lo.nkeys; i++)
        fill_rrect(px + lo.keys[i].x * k, py + lo.keys[i].y * k,
                   lo.keys[i].w * k, lo.keys[i].h * k, 3,
                   RGBA8(0x18, 0x2A, 0x5A, 0xFF));
    fill_vgrad(px + lo.dest[0] * k, py + lo.dest[1] * k, lo.dest[2] * k,
               lo.dest[3] * k, art_top[0], art_bot[0]);
    snprintf(buf, sizeof(buf), "%d x %d", w, h);
    text(px + (lo.dest[0] + lo.dest[2] / 2) * k - text_w(0.8f, buf) / 2.0f,
         py + (lo.dest[1] + lo.dest[3] / 2) * k + 6, 0.8f, C_WHITE, buf);
}

/* Texto en varias líneas, partiendo por palabras. Devuelve la y final. */
static float text_wrap(float x, float y, float scale, unsigned int c,
                       const char *s, int max_w) {
    char line[200] = "", trial[200];
    while (*s) {
        int n = (int)strcspn(s, " ");
        snprintf(trial, sizeof(trial), "%s%s%.*s", line, line[0] ? " " : "",
                 n, s);
        if (line[0] && text_w(scale, trial) > max_w) {
            text(x, y, scale, c, line);
            y += 24;
            snprintf(line, sizeof(line), "%.*s", n, s);
        } else {
            memcpy(line, trial, sizeof(line));
        }
        s += n;
        while (*s == ' ') s++;
    }
    if (line[0]) text(x, y, scale, c, line);
    return y + 24;
}

static void draw_settings(const scene *sc, const char *subtitle,
                          const vj_cfg *c, const int *rows, int nrows,
                          int sel, int top, int mode) {
    const int vis = 10, rh = 34;
    char label[80], val[80];
    float y, x0 = 48, x1 = 560;
    int i;
    const char *title =
        mode == SET_INGAME ? L("Game paused", "Juego en pausa", "Jogo pausado")
        : mode == SET_GLOBAL ? L("Options", "Opciones", "Opções")
                             : L("Game settings", "Ajustes del juego",
                                 "Ajustes do jogo");
    vita2d_start_drawing();
    vita2d_clear_screen();
    draw_background(sc->frame);
    text(48, 54, 1.3f, C_WHITE, title);
    text_fit(48 + text_w(1.3f, title) + 14, 54, 0.9f, C_TEXT_2, subtitle,
             SCR_W - 420);

    y = 92;
    for (i = top; i < top + vis && i < nrows; i++, y += rh) {
        int on = i == sel;
        row_text(c, rows[i], label, val, sizeof(label));
        if (on) fill_rrect(x0 - 12, y, x1 - x0 + 24, rh - 4, 10, C_ACCENT);
        text_fit(x0, y + 22, 0.82f, on ? C_WHITE : C_TEXT_2, label, 230);
        if (val[0]) {
            float vw = text_w(0.82f, val);
            if (vw > 250) vw = 250;
            if (on) {
                glyph_pill(x1 - vw - 46, y + 13, "<");
                text_fit(x1 - vw - 16, y + 22, 0.82f, C_WHITE, val, 250);
                glyph_pill(x1 - 6, y + 13, ">");
            } else {
                text_fit(x1 - vw, y + 22, 0.82f, C_WHITE, val, 250);
            }
        }
    }
    if (nrows > vis) {
        float bh = (float)(vis * rh) * vis / nrows;
        float by = 92 + (float)(vis * rh - bh) * top / (nrows - vis);
        vita2d_draw_rectangle(x1 + 22, 92, 3, vis * rh, C_PANEL_LINE);
        vita2d_draw_rectangle(x1 + 21, by, 5, bh, C_GLOW);
    }

    /* derecha: vista previa (por juego) + ayuda */
    y = 104;
    if (mode != SET_GLOBAL) {
        text(612, 104, 0.8f, C_TEXT_2, L("Preview", "Vista previa",
                                         "Pré-visualização"));
        draw_preview(c, 612, 120, 300);
        y = 330;
    }
    y = text_wrap(612, y, 0.78f, C_TEXT_2, row_help(rows[sel], mode), 300);
    if (mode == SET_INGAME)
        text_wrap(612, y + 8, 0.72f, C_TEXT_3,
                  L("In game: L+R+START this menu · L+R+X quit · "
                    "L+R+Triangle virtual keypad",
                    "En juego: L+R+START este menú · L+R+X salir · "
                    "L+R+Triángulo teclado virtual",
                    "No jogo: L+R+START este menu · L+R+X sair · "
                    "L+R+Triângulo teclado virtual"), 300);

    vita2d_draw_rectangle(0, 498, SCR_W, 46, C_BAR);
    vita2d_draw_rectangle(0, 498, SCR_W, 1, C_PANEL_LINE);
    x0 = 48;
    if (rows[sel] == ROW_RESET)
        x0 += hint(x0, 522, 'x', NULL, L("Restore", "Restaurar",
                                         "Restaurar")) + 28;
    else
        x0 += hint(x0, 522, 0, "< >", L("Change", "Cambiar", "Mudar")) + 28;
    if (mode == SET_INGAME) {
        x0 += hint(x0, 522, 'o', NULL, L("Back to game", "Volver al juego",
                                         "Voltar ao jogo")) + 28;
        hint(x0, 522, 't', NULL, L("Quit to library",
                                   "Salir a la biblioteca",
                                   "Sair para a biblioteca"));
    } else {
        hint(x0, 522, 'o', NULL, L("Save and go back", "Guardar y volver",
                                   "Salvar e voltar"));
    }
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

/* g = NULL -> opciones globales. Devuelve 1 si se guardaron cambios; en
 * el juego, 2 = salir del juego. *out queda con los ajustes vigentes. */
static int settings_run(scene *sc, const vj_game *g, int mode, vj_cfg *out) {
    static vj_cfg c, orig;
    static int rows[ROW_N];
    SceCtrlData pad;
    unsigned int old = ~0u;
    int sel = 0, top = 0, hold = 0, quit = 0, nrows, changed;
    int lang0 = vj_lang, fps0 = vj_show_fps;
    const char *subtitle = g ? g->name : L("All games", "Todos los juegos",
                                           "Todos os jogos");
    nrows = settings_rows(mode, rows);
    if (mode == SET_GLOBAL) cfg_global_get(&c);
    else cfg_load(g, &c);
    orig = c;
    for (;;) {
        unsigned int now, down;
        int r = rows[sel];
        sceCtrlPeekBufferPositive(0, &pad, 1);
        now = pad.buttons;
        if (pad.ly < 50) now |= SCE_CTRL_UP;
        if (pad.ly > 205) now |= SCE_CTRL_DOWN;
        if (pad.lx < 50) now |= SCE_CTRL_LEFT;
        if (pad.lx > 205) now |= SCE_CTRL_RIGHT;
        down = now & ~old;
        if (now & (SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_LEFT | SCE_CTRL_RIGHT)) {
            if (++hold > 22 && hold % 5 == 0)
                down |= now & (SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_LEFT |
                               SCE_CTRL_RIGHT);
        } else {
            hold = 0;
        }
        old = now;

        if (down & SCE_CTRL_UP) sel = wrap(sel, -1, nrows);
        if (down & SCE_CTRL_DOWN) sel = wrap(sel, 1, nrows);
        if (down & (SCE_CTRL_LEFT | SCE_CTRL_LTRIGGER)) row_change(&c, r, -1);
        if (down & (SCE_CTRL_RIGHT | SCE_CTRL_RTRIGGER | SCE_CTRL_CROSS)) {
            if (r == ROW_RESET) {
                if (down & SCE_CTRL_CROSS) {
                    int aw = c.auto_w, ah = c.auto_h;
                    if (mode == SET_GLOBAL) {
                        cfg_factory(&c);
                        vj_show_fps = 0;
                    } else {
                        cfg_defaults(&c);
                    }
                    c.auto_w = aw;
                    c.auto_h = ah;
                }
            } else {
                row_change(&c, r, 1);
            }
        }
        if (down & SCE_CTRL_CIRCLE) break;
        if (mode == SET_INGAME && (down & SCE_CTRL_TRIANGLE)) {
            quit = 1;
            break;
        }
        if (sel < top) top = sel;
        if (sel >= top + 10) top = sel - 9;

        sc->frame++;
        draw_settings(sc, subtitle, &c, rows, nrows, sel, top, mode);
    }
    if (out) *out = c;
    changed = memcmp(&c, &orig, sizeof(c)) != 0 ||
              lang0 != vj_lang || fps0 != vj_show_fps;
    if (changed) {
        if (mode == SET_GLOBAL) cfg_global_save(&c);
        else cfg_save(g, &c);
        vj_log("launcher: ajustes de '%s' guardados", subtitle);
    }
    return quit ? 2 : changed;
}

/* Menú del juego: lo llama lcd.c desde el hilo de la VM (juego en pausa).
 * Devuelve 0 seguir, 1 salir a la biblioteca, 2 reiniciar el juego (la
 * resolución o el táctil solo se pueden fijar al arrancar la VM). */
extern int suspend_key_input;
extern volatile int vita_media_paused;

int launcher_game_menu(const vj_game *g) {
    static scene sc;
    vj_cfg c, before;
    SceCtrlData pad;
    int r, w0, h0, w1, h1;
    suspend_key_input = 1;
    vita_media_paused = 1;
    if (!font) font = vita2d_load_default_pgf();
    if (!font) {
        suspend_key_input = 0;
        vita_media_paused = 0;
        return 0;
    }
    vj_log("menú del juego abierto");
    vita2d_set_vblank_wait(1);
    cfg_load(g, &before);
    r = settings_run(&sc, g, SET_INGAME, &c);
    /* no devolver el control con un botón aún pulsado (llegaría al juego) */
    do {
        static int rows[ROW_N];
        int n = settings_rows(SET_INGAME, rows);
        sceCtrlPeekBufferPositive(0, &pad, 1);
        sc.frame++;
        draw_settings(&sc, g->name, &c, rows, n, 0, 0, SET_INGAME);
    } while (pad.buttons & (SCE_CTRL_CIRCLE | SCE_CTRL_TRIANGLE |
                            SCE_CTRL_CROSS | SCE_CTRL_START));
    vita2d_wait_rendering_done();
    vita2d_set_vblank_wait(0);  /* lcd.c presenta sin vblank (bug #24) */
    cfg_apply(&c, 1);
    vita_media_paused = 0;
    suspend_key_input = 0;
    if (r == 2) {
        vj_log("menú del juego cerrado (salir)");
        return 1;
    }
    cfg_screen(&before, &w0, &h0);
    cfg_screen(&c, &w1, &h1);
    if (w0 != w1 || h0 != h1 || before.touch != c.touch) {
        vj_log("menú del juego: %dx%d -> %dx%d / táctil %d -> %d: "
               "reiniciar el juego", w0, h0, w1, h1, before.touch, c.touch);
        return 2;
    }
    vj_log("menú del juego cerrado (seguir)");
    return 0;
}

/* ======================================================================
 * Bucle
 * ====================================================================== */

static float target_scroll(const scene *sc) {
    float t = 48 + sc->sel * (CARD_W + CARD_GAP) + CARD_W / 2.0f - SCR_W / 2;
    float maxs = 48 + sc->n * (CARD_W + CARD_GAP) - CARD_GAP + 48 - SCR_W;
    if (t > maxs) t = maxs;
    if (t < 0) t = 0;
    return t;
}

int launcher_list(vj_game *g, int max) {
    int i, n = load_suites(g, max);
    for (i = 0; i < n; i++) {
        FILE *f = fopen(g[i].jar, "rb");
        g[i].missing = !f;
        if (f) fclose(f);
        else vj_log("launcher: '%s' sin JAR (%s)", g[i].name, g[i].jar);
        fill_from_manifest(&g[i]);
    }
    return n;
}

int launcher_run(vj_game *out, int preselect) {
    static scene sc;
    SceCtrlData pad;
    SceTouchData touch;
    unsigned int old = 0;
    int was_touch = 0, hold = 0, result = LAUNCHER_EXIT, i;

    memset(&sc, 0, sizeof(sc));
    font = vita2d_load_default_pgf();
    if (!font) {
        vj_log("launcher: sin fuente PGF -> salir");
        return LAUNCHER_EXIT;
    }
    sc.n = launcher_list(sc.games, MAX_GAMES);
    for (i = 0; i < sc.n; i++) {
        sc.icons[i] = load_icon(&sc.games[i]);
        if (sc.games[i].suite_id == preselect) sc.sel = i;
    }
    sc.scroll = target_scroll(&sc);
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
            for (i = 0; i < sc.n; i++) {
                float x, y, w, h;
                card_rect(&sc, i, &x, &y, &w, &h);
                if (tx >= x && tx < x + w && ty >= y && ty < y + h) {
                    if (i == sc.sel) act_play = 1;
                    else sc.sel = i;
                }
            }
        }
        was_touch = touch.reportNum > 0;

        if (down & SCE_CTRL_SELECT) {
            settings_run(&sc, NULL, SET_GLOBAL, NULL);
            old = ~0u;
            continue;
        }
        if ((down & SCE_CTRL_SQUARE) && sc.sel < sc.n) {
            settings_run(&sc, &sc.games[sc.sel], SET_GAME, NULL);
            old = ~0u;  /* que el ○ de salir de ajustes no cierre esto */
            continue;
        }
        if (sc.info) {
            if (down & SCE_CTRL_CROSS) act_play = 1;
            if (down & (SCE_CTRL_CIRCLE | SCE_CTRL_TRIANGLE)) sc.info = 0;
        } else {
            if (down & (SCE_CTRL_LEFT | SCE_CTRL_LTRIGGER) && sc.sel > 0)
                sc.sel--;
            if (down & (SCE_CTRL_RIGHT | SCE_CTRL_RTRIGGER) &&
                sc.sel < sc.n - 1)
                sc.sel++;
            if (down & SCE_CTRL_CROSS) act_play = 1;
            if ((down & SCE_CTRL_TRIANGLE) && sc.sel < sc.n) sc.info = 1;
            if (down & SCE_CTRL_CIRCLE) act_back = 1;
        }

        /* El proceso termina enseguida: nada que liberar (HW 075: la
         * salida se colgó sin traza; así no espera a la GPU). */
        if (act_back) {
            vj_log("launcher: círculo pulsado -> salir");
            return LAUNCHER_EXIT;
        }
        if (act_play && sc.sel < sc.n) {
            if (sc.games[sc.sel].missing) continue;  /* sin JAR no arranca */
            if (!sc.games[sc.sel].cls[0]) {
                vj_log("launcher: '%s' sin clase de MIDlet, no se puede "
                       "lanzar", sc.games[sc.sel].name);
                continue;
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

    vita2d_wait_rendering_done(); /* la GPU suelta iconos y atlas PGF */
    for (i = 0; i < sc.n; i++) {
        if (!sc.icons[i]) continue;
        /* el icono del juego elegido lo usa la pantalla de carga */
        if (result == LAUNCHER_PLAY && i == sc.sel) {
            ld.icon = sc.icons[i];
            continue;
        }
        vita2d_free_texture(sc.icons[i]);
    }
    if (result == LAUNCHER_PLAY) {
        ld.game = *out;
        ld.frame = sc.frame;
        vj_log("launcher: lanzar juego directo");
        return result;  /* fuente e icono siguen vivos: launcher_loading_start */
    }
    vita2d_free_pgf(font);
    font = NULL;
    /* lcd.c presenta sin esperar vblank (bug #24): restaurar. */
    vita2d_set_vblank_wait(0);
    return result;
}

/* ======================================================================
 * Pantalla de carga dinámica (mientras arranca el MIDlet)
 *
 * Hilo propio en el núcleo de usuario 2 (la JVM vive en otro): anima de
 * verdad mientras la JVM trabaja. lcd.c no presenta los frames de Java
 * mientras sean un color liso (vita_lcd_loading_hook) y, al primer frame
 * con contenido, llama al hook: se para este hilo, se libera todo y la
 * JVM toma la pantalla. Nunca dibujan los dos a la vez.
 * ====================================================================== */

extern int (*vita_lcd_loading_hook)(int has_content);
extern volatile unsigned long vita_jvmlog_bytes;

#define LOADING_MAX_US (120ull * 1000 * 1000)  /* luego: pantalla real */

static void draw_loading(float fade) {
    float t = (float)((sceKernelGetProcessTimeWide() - ld.t0) / 1000000.0);
    float cx = SCR_W / 2.0f, tile = 132, ty = 124;
    int hue = name_hash(ld.game.name), i;
    char buf[96];
    const char *status;

    /* OJO (HW 042): nada de `scene` en la pila aquí — son ~44 KB (64
     * vj_game) y este hilo tiene 32 KB: desbordaba en el primer frame. */
    vita2d_start_drawing();
    vita2d_clear_screen();
    draw_background(ld.frame);
    /* icono del juego con halo que respira */
    {
        float glow = 10 + sinf(ld.frame * 0.06f) * 4;
        fill_rrect(cx - tile / 2 - glow, ty - glow, tile + glow * 2,
                   tile + glow * 2, 30 + glow,
                   with_alpha(C_GLOW, 40));
    }
    fill_rrect(cx - tile / 2, ty, tile, tile, 28, art_top[hue]);
    if (ld.icon) {
        float iw = vita2d_texture_get_width(ld.icon);
        float ih = vita2d_texture_get_height(ld.icon);
        float sc = (tile - 36) / (iw > ih ? iw : ih);
        fill_rrect(cx - tile / 2 + 12, ty + 12, tile - 24, tile - 24, 20,
                   C_WHITE);
        vita2d_draw_texture_scale(ld.icon, cx - iw * sc / 2,
                                  ty + (tile - ih * sc) / 2, sc, sc);
    } else {
        draw_initials(ld.game.name, cx, ty + tile / 2 - 8, 2.0f);
    }
    text_fit(cx - text_w(1.3f, ld.game.name) / 2.0f, ty + tile + 52, 1.3f,
             C_WHITE, ld.game.name, SCR_W - 96);
    if (ld.game.vendor[0])
        text(cx - text_w(0.85f, ld.game.vendor) / 2.0f, ty + tile + 80,
             0.85f, C_TEXT_2, ld.game.vendor);

    /* spinner: arco que gira y cambia de longitud */
    {
        float a0 = ld.frame * 0.11f;
        float len = 1.2f + sinf(ld.frame * 0.045f) * 0.9f;
        float r = 15, sy = ty + tile + 124;
        int n = 28;
        for (i = 0; i < n; i++) {
            float a = 6.2831853f * i / n;
            vita2d_draw_fill_circle(cx + cosf(a) * r, sy + sinf(a) * r, 1.6f,
                                    with_alpha(C_TEXT_3, 90));
        }
        for (i = 0; i <= 24; i++) {
            float a = a0 + len * i / 24;
            vita2d_draw_fill_circle(cx + cosf(a) * r, sy + sinf(a) * r, 2.4f,
                                    C_GLOW);
        }
    }

    /* estado real: hasta que el MIDlet empieza a pintar, arranque de
     * la VM; después, el juego ya corre y carga sus recursos */
    if (ld.install) {
        /* progreso real: suites nuevas en appdb/_suites.dat */
        if (ld.frame % 30 == 0) {
            /* 0 = no se pudo leer (la VM lo está reescribiendo): seguir */
            int c = suites_count(), n = c - ld.install_base;
            if (c > 0)
                ld.install_done = n < 0 ? 0 : n > ld.install ? ld.install : n;
        }
        snprintf(buf, sizeof(buf), "%s  %d / %d",
                 L("Installing games", "Instalando juegos", "Instalando jogos"),
                 ld.install_done, ld.install);
        status = buf;
        {
            float bw = 360, bx = cx - bw / 2, by = ty + tile + 148;
            fill_rrect(bx, by, bw, 8, 4, with_alpha(C_TEXT_3, 90));
            if (ld.install_done)
                fill_rrect(bx, by, bw * ld.install_done / ld.install, 8, 4,
                           C_GLOW);
        }
    } else
    status = ld.midlet_frames ? L("Loading game resources",
                                    "Cargando recursos del juego",
                                    "Carregando recursos do jogo")
                                : L("Starting the Java virtual machine",
                                    "Arrancando la máquina virtual Java",
                                    "Iniciando a máquina virtual Java");
    text(cx - text_w(0.85f, status) / 2.0f, ty + tile + 176, 0.85f,
         C_TEXT_2, status);
    snprintf(buf, sizeof(buf), "%d s", (int)t);
    text(cx - text_w(0.7f, buf) / 2.0f, ty + tile + 202, 0.7f, C_TEXT_3,
         buf);
    if (!ld.install) {
        const char *keys =
            L("In game:  L+R+START menu   ·   L+R+X quit   ·   "
              "L+R+Triangle virtual keypad",
              "En juego:  L+R+START menú   ·   L+R+X salir   ·   "
              "L+R+Triángulo teclado virtual",
              "No jogo:  L+R+START menu   ·   L+R+X sair   ·   "
              "L+R+Triângulo teclado virtual");
        text(cx - text_w(0.7f, keys) / 2.0f, SCR_H - 22, 0.7f, C_TEXT_3,
             keys);
    }

    if (fade < 1.0f)
        vita2d_draw_rectangle(0, 0, SCR_W, SCR_H,
                              with_alpha(C_BG_BOT, (int)(255 * (1 - fade))));
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

static int loading_thread(SceSize args, void *argp) {
    int i;
    (void)args; (void)argp;
    vita2d_set_vblank_wait(1);
    for (i = 1; i <= 12 && !ld.stop; i++) {
        ld.frame++;
        draw_loading(i / 12.0f);
    }
    while (!ld.stop) {
        ld.frame++;
        draw_loading(1.0f);
    }
    vita2d_wait_rendering_done();
    if (ld.icon) vita2d_free_texture(ld.icon);
    ld.icon = NULL;
    vita2d_free_pgf(font);
    font = NULL;
    vita2d_set_vblank_wait(0);  /* lcd.c presenta sin vblank (bug #24) */
    return 0;
}

/* Lo llama lcd.c (hilo de la JVM) en cada flush mientras carga. */
/* Para el hilo de la pantalla de carga/instalación si sigue vivo. */
static void loading_stop(void) {
    if (ld.thread <= 0) return;
    ld.stop = 1;
    sceKernelWaitThreadEnd(ld.thread, NULL, NULL);
    sceKernelDeleteThread(ld.thread);
    ld.thread = 0;
}

/* has_content: 0 frame vacío, 1 con contenido, 2 vita2d se va (lcd.c). */
static int loading_hook(int has_content) {
    int timeout = (sceKernelGetProcessTimeWide() - ld.t0) > LOADING_MAX_US;
    if (!has_content && !timeout) {
        ld.midlet_frames++;
        return 1;
    }
    loading_stop();
    vj_log("carga: %s tras %d s (%lu frames vacíos del MIDlet)",
           has_content ? "primer frame con contenido" : "tiempo agotado",
           (int)((sceKernelGetProcessTimeWide() - ld.t0) / 1000000),
           ld.midlet_frames);
    return 0;
}

/* Precarga: la VM arranca detrás de la biblioteca; ninguno de sus
 * frames se presenta hasta launcher_loading_start(). */
static int hold_hook(int has_content) {
    if (has_content == 2) loading_stop();  /* instalación: la VM termina */
    return 1;
}

void launcher_preload_hold(void) {
    vita_lcd_loading_hook = hold_hook;
}

/* Instalador por lotes (CommandLineInstaller "B"): la VM corre detrás de
 * una pantalla nativa con el progreso y NINGUNO de sus frames (UI vieja,
 * avisos) se ve. */
void launcher_install_start(int total) {
    static vj_game g;
    memset(&g, 0, sizeof(g));
    snprintf(g.name, sizeof(g.name), "VitaJ2ME");
    snprintf(g.vendor, sizeof(g.vendor), "%s",
             L("Adding your games to the library",
               "Añadiendo tus juegos a la biblioteca",
               "Adicionando seus jogos à biblioteca"));
    ld.install = total;
    ld.install_base = suites_count();
    ld.install_done = 0;
    launcher_loading_start(&g);
    vita_lcd_loading_hook = hold_hook;
}

void launcher_loading_start(const vj_game *g) {
    if (!font) font = vita2d_load_default_pgf();
    if (!font) {
        vita_lcd_loading_hook = 0;
        return;
    }
    ld.game = *g;
    if (!ld.icon) ld.icon = load_icon(g);  /* la biblioteca deja el suyo */
    ld.stop = 0;
    ld.midlet_frames = 0;
    ld.t0 = sceKernelGetProcessTimeWide();
    ld.thread = sceKernelCreateThread("vj_loading", loading_thread,
                                      0x10000100, 0x8000, 0,
                                      SCE_KERNEL_CPU_MASK_USER_2, NULL);
    if (ld.thread < 0) {
        vj_log("carga: no se pudo crear el hilo (0x%08x)", ld.thread);
        vita2d_free_pgf(font);
        font = NULL;
        vita2d_set_vblank_wait(0);
        vita_lcd_loading_hook = 0;
        return;
    }
    vita_lcd_loading_hook = loading_hook;
    sceKernelStartThread(ld.thread, 0, NULL);
}
