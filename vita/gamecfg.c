/* gamecfg.c — ajustes por juego y su efecto en la VM.
 *
 * - Pantalla: resolución virtual (automática desde el JAR o manual), modo
 *   de escala y filtro -> globals de lcd.c (vita_lcd_*).
 * - Teclado virtual: paneles laterales con soft keys, cruceta y teclado
 *   numérico; el juego queda en el centro. Se dibuja dentro del frame de
 *   la VM (vita_lcd_overlay_hook) y se pulsa con el táctil.
 * - Táctil: fuera de las teclas, la pantalla es el puntero del MIDlet
 *   (javanotify_pen_event en coordenadas del juego).
 * - Botones: tabla propia para keypress.c (vita_keymap_custom).
 */
#include "gamecfg.h"

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/threadmgr.h>
#include <vita2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "javacall_keymap.h"
#include "javacall_keypress.h"
#include "javacall_penevent.h"

#define CFG_DIR "ux0:data/vitaj2me/config"
#define SCR_W 960
#define SCR_H 544

/* lcd.c / keymap.c / main.c (libjavacall y frontend) */
extern int vita_lcd_scale;
extern int vita_lcd_area[4];
extern int vita_lcd_filter;
extern volatile float vita_lcd_dest[4];
extern void (*vita_lcd_overlay_hook)(void);
extern int (*vita_lcd_loading_hook)(int has_content);
extern void vita_lcd_set_virtual_size(int w, int h);
extern int javacall_lcd_get_screen_width(void);
extern int javacall_lcd_get_screen_height(void);
extern javacall_keymap vita_keymap_custom[32];
extern int vita_keymap_custom_n;
extern int suspend_key_input;

/* Resoluciones típicas de móviles Java (máximo 640: tamaño del offscreen). */
const int vj_res[][2] = {
    {128, 128}, {128, 160}, {132, 176}, {176, 204}, {176, 208},
    {176, 220}, {208, 208}, {240, 320}, {320, 240}, {240, 400},
    {400, 240}, {352, 416}, {360, 640}, {640, 360}, {480, 272},
};
const int vj_res_n = sizeof(vj_res) / sizeof(vj_res[0]);

/* Orden fijo: es el del fichero .cfg (map=...). */
const char *const vj_btn_name[VJ_BTN_N] = {
    "Cruceta arriba", "Cruceta abajo", "Cruceta izquierda", "Cruceta derecha",
    "Cruz", "Círculo", "Cuadrado", "Triángulo", "L", "R", "SELECT", "START",
    "Stick arriba", "Stick abajo", "Stick izquierda", "Stick derecha",
};
/* Bits de keypress.c para el stick (VITA_CTRL_ANALOG_*). */
static const unsigned int btn_mask[VJ_BTN_N] = {
    SCE_CTRL_UP, SCE_CTRL_DOWN, SCE_CTRL_LEFT, SCE_CTRL_RIGHT,
    SCE_CTRL_CROSS, SCE_CTRL_CIRCLE, SCE_CTRL_SQUARE, SCE_CTRL_TRIANGLE,
    SCE_CTRL_LTRIGGER, SCE_CTRL_RTRIGGER, SCE_CTRL_SELECT, SCE_CTRL_START,
    0x40000000, 0x80000000, 0x10000000, 0x20000000,
};
static const int btn_default[VJ_BTN_N] = {
    JAVACALL_KEY_UP, JAVACALL_KEY_DOWN, JAVACALL_KEY_LEFT, JAVACALL_KEY_RIGHT,
    JAVACALL_KEY_SELECT, '0', '1', '3', '*', '#',
    JAVACALL_KEY_SOFT1, JAVACALL_KEY_SOFT2,
    JAVACALL_KEY_UP, JAVACALL_KEY_DOWN, JAVACALL_KEY_LEFT, JAVACALL_KEY_RIGHT,
};

const int vj_key_codes[] = {
    0, JAVACALL_KEY_UP, JAVACALL_KEY_DOWN, JAVACALL_KEY_LEFT,
    JAVACALL_KEY_RIGHT, JAVACALL_KEY_SELECT, JAVACALL_KEY_SOFT1,
    JAVACALL_KEY_SOFT2, JAVACALL_KEY_CLEAR,
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '#',
};
const char *const vj_key_names[] = {
    "Nada", "Arriba", "Abajo", "Izquierda", "Derecha", "OK / Disparo",
    "Soft izquierda", "Soft derecha", "Borrar",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "#",
};
const int vj_keys_n = sizeof(vj_key_codes) / sizeof(vj_key_codes[0]);

const char *cfg_key_name(int code) {
    int i;
    for (i = 0; i < vj_keys_n; i++)
        if (vj_key_codes[i] == code) return vj_key_names[i];
    return "?";
}

void cfg_defaults(vj_cfg *c) {
    memset(c, 0, sizeof(*c));
    c->scale = VJ_SCALE_FIT;
    c->keypad = VJ_PAD_AUTO;
    c->touch = 1;
    memcpy(c->map, btn_default, sizeof(c->map));
}

static void cfg_path(const vj_game *g, char *out, int cap) {
    snprintf(out, cap, CFG_DIR "/%d.cfg", g->suite_id);
}

void cfg_load(const vj_game *g, vj_cfg *c) {
    char path[96], line[256];
    FILE *f;
    cfg_defaults(c);
    if (jar_detect_screen(g->jar, &c->auto_w, &c->auto_h)) {
        vj_log("cfg: '%s' pantalla detectada en el JAR: %dx%d", g->name,
               c->auto_w, c->auto_h);
    } else {
        c->auto_w = 240;
        c->auto_h = 320;
        vj_log("cfg: '%s' sin pistas de resolución en el JAR -> 240x320",
               g->name);
    }
    cfg_path(g, path, sizeof(path));
    f = fopen(path, "r");
    if (!f) return;
    while (fgets(line, sizeof(line), f)) {
        char *v = strchr(line, '=');
        int w, h, i;
        if (!v) continue;
        *v++ = 0;
        if (!strcmp(line, "res")) {
            c->res = 0;
            if (sscanf(v, "%dx%d", &w, &h) == 2)
                for (i = 0; i < vj_res_n; i++)
                    if (vj_res[i][0] == w && vj_res[i][1] == h) c->res = i + 1;
        } else if (!strcmp(line, "scale")) {
            c->scale = atoi(v);
            if (c->scale < VJ_SCALE_FIT || c->scale > VJ_SCALE_1X)
                c->scale = VJ_SCALE_FIT;
        } else if (!strcmp(line, "filter")) {
            c->filter = atoi(v) ? 1 : 0;
        } else if (!strcmp(line, "keypad")) {
            c->keypad = atoi(v);
            if (c->keypad < VJ_PAD_AUTO || c->keypad > VJ_PAD_BOTH)
                c->keypad = VJ_PAD_AUTO;
        } else if (!strcmp(line, "touch")) {
            c->touch = atoi(v) ? 1 : 0;
        } else if (!strcmp(line, "map")) {
            char *p = v;
            for (i = 0; i < VJ_BTN_N && *p; i++) {
                c->map[i] = (int)strtol(p, &p, 10);
                if (*p == ',') p++;
            }
        }
    }
    fclose(f);
}

void cfg_save(const vj_game *g, const vj_cfg *c) {
    char path[96];
    FILE *f;
    int i;
    sceIoMkdir(CFG_DIR, 0777);
    cfg_path(g, path, sizeof(path));
    f = fopen(path, "w");
    if (!f) {
        vj_log("cfg: no se pudo escribir %s", path);
        return;
    }
    if (c->res > 0)
        fprintf(f, "res=%dx%d\n", vj_res[c->res - 1][0], vj_res[c->res - 1][1]);
    else
        fprintf(f, "res=auto\n");
    fprintf(f, "scale=%d\nfilter=%d\nkeypad=%d\ntouch=%d\nmap=", c->scale,
            c->filter, c->keypad, c->touch);
    for (i = 0; i < VJ_BTN_N; i++)
        fprintf(f, i ? ",%d" : "%d", c->map[i]);
    fprintf(f, "\n");
    fclose(f);
}

void cfg_screen(const vj_cfg *c, int *w, int *h) {
    if (c->res > 0 && c->res <= vj_res_n) {
        *w = vj_res[c->res - 1][0];
        *h = vj_res[c->res - 1][1];
    } else {
        *w = c->auto_w;
        *h = c->auto_h;
    }
}

int cfg_last_suite(void) {
    FILE *f = fopen(CFG_DIR "/last", "r");
    int id = -1;
    if (f) {
        if (fscanf(f, "%d", &id) != 1) id = -1;
        fclose(f);
    }
    return id;
}

void cfg_set_last_suite(int suite_id) {
    FILE *f;
    sceIoMkdir(CFG_DIR, 0777);
    f = fopen(CFG_DIR "/last", "w");
    if (f) {
        fprintf(f, "%d\n", suite_id);
        fclose(f);
    }
}

/* ======================================================================
 * Layout: zona del juego + teclas del teclado virtual
 * ====================================================================== */

static const char *const num_label[12] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#",
};
static const int num_code[12] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '0', '#',
};

static void add_key(vj_layout *lo, float x, float y, float w, float h,
                    int code, const char *label) {
    vj_padkey *k;
    if (lo->nkeys >= VJ_PAD_MAX) return;
    k = &lo->keys[lo->nkeys++];
    k->x = x + 3; k->y = y + 3; k->w = w - 6; k->h = h - 6;
    k->code = code;
    k->label = label;
}

static void add_dpad(vj_layout *lo, float cx, float cy, float cell) {
    add_key(lo, cx - cell / 2, cy - cell * 1.5f, cell, cell, JAVACALL_KEY_UP, NULL);
    add_key(lo, cx - cell * 1.5f, cy - cell / 2, cell, cell, JAVACALL_KEY_LEFT, NULL);
    add_key(lo, cx - cell / 2, cy - cell / 2, cell, cell, JAVACALL_KEY_SELECT, "OK");
    add_key(lo, cx + cell / 2, cy - cell / 2, cell, cell, JAVACALL_KEY_RIGHT, NULL);
    add_key(lo, cx - cell / 2, cy + cell / 2, cell, cell, JAVACALL_KEY_DOWN, NULL);
}

static void add_numpad(vj_layout *lo, float x, float y, float w, float h) {
    int i;
    for (i = 0; i < 12; i++)
        add_key(lo, x + (i % 3) * w / 3, y + (i / 3) * h / 4, w / 3, h / 4,
                num_code[i], num_label[i]);
}

static void set4(float *d, float a, float b, float c, float e) {
    d[0] = a; d[1] = b; d[2] = c; d[3] = e;
}

/* Misma cuenta que vita_dest_rect() de lcd.c para vita_lcd_scale > 0. */
static void dest_in(const vj_cfg *c, int gw, int gh, const float *a,
                    float *d) {
    float sx = a[2] / gw, sy = a[3] / gh;
    float s = sx < sy ? sx : sy;
    if (c->scale == VJ_SCALE_INT) {
        if ((float)(int)s >= 1.0f) s = (float)(int)s;
    } else if (c->scale == VJ_SCALE_1X) {
        s = 1.0f;
    }
    if (c->scale == VJ_SCALE_STRETCH) {
        d[2] = a[2]; d[3] = a[3];
    } else {
        d[2] = gw * s; d[3] = gh * s;
    }
    d[0] = a[0] + (a[2] - d[2]) / 2;
    d[1] = a[1] + (a[3] - d[3]) / 2;
}

void cfg_layout(const vj_cfg *c, vj_layout *lo) {
    int gw, gh, mode = c->keypad;
    memset(lo, 0, sizeof(*lo));
    cfg_screen(c, &gw, &gh);
    /* Auto: teclado a los lados solo si el juego es vertical o cuadrado
     * (ahí sobra pantalla); en horizontal el juego ocupa todo. */
    if (mode == VJ_PAD_AUTO)
        mode = gw * 100 < gh * 115 ? VJ_PAD_BOTH : VJ_PAD_OFF;

    if (mode == VJ_PAD_BOTH) {
        float side = (SCR_W - gw * (float)SCR_H / gh) / 2;
        float r;
        if (side < 200) side = 200;
        if (side > 300) side = 300;
        r = SCR_W - side;
        set4(lo->area, side, 0, SCR_W - 2 * side, SCR_H);
        set4(lo->panel[0], 0, 0, side, SCR_H);
        set4(lo->panel[1], r, 0, side, SCR_H);
        add_key(lo, 14, 18, side - 28, 60, JAVACALL_KEY_SOFT1, "Soft 1");
        add_dpad(lo, side / 2, 316, (side - 20) / 3 > 92 ? 92 : (side - 20) / 3);
        add_key(lo, r + 14, 18, side - 28, 60, JAVACALL_KEY_SOFT2, "Soft 2");
        add_numpad(lo, r + 14, 120, side - 28, 4 * 84);
    } else if (mode == VJ_PAD_LEFT || mode == VJ_PAD_RIGHT) {
        float p = 300, x0 = mode == VJ_PAD_LEFT ? 0 : SCR_W - p;
        set4(lo->area, mode == VJ_PAD_LEFT ? p : 0, 0, SCR_W - p, SCR_H);
        set4(lo->panel[0], x0, 0, p, SCR_H);
        add_key(lo, x0 + 12, 12, 138, 54, JAVACALL_KEY_SOFT1, "Soft 1");
        add_key(lo, x0 + 150, 12, 138, 54, JAVACALL_KEY_SOFT2, "Soft 2");
        add_dpad(lo, x0 + p / 2, 172, 68);
        add_numpad(lo, x0 + 12, 290, p - 24, 242);
    } else {
        set4(lo->area, 0, 0, SCR_W, SCR_H);
    }
    dest_in(c, gw, gh, lo->area, lo->dest);
}

/* ======================================================================
 * En juego: overlay + táctil
 * ====================================================================== */

static vj_layout cur;
static int cur_touch;
static volatile unsigned char key_down[VJ_PAD_MAX];
static vita2d_pgf *pad_font;
static SceUID touch_tid = -1;

static void draw_arrow(float cx, float cy, float s, int code,
                       unsigned int col) {
    /* triángulo relleno con franjas (vita2d no tiene polígonos) */
    int i, n = (int)(s * 2);
    for (i = 0; i <= n; i++) {
        float t = (float)i / n, half = s * t, d = -s + 2 * s * t;
        switch (code) {
        case JAVACALL_KEY_UP:
            vita2d_draw_rectangle(cx - half, cy + d * 0.8f, half * 2, 2, col);
            break;
        case JAVACALL_KEY_DOWN:
            vita2d_draw_rectangle(cx - half, cy - d * 0.8f, half * 2, 2, col);
            break;
        case JAVACALL_KEY_LEFT:
            vita2d_draw_rectangle(cx + d * 0.8f, cy - half, 2, half * 2, col);
            break;
        default:
            vita2d_draw_rectangle(cx - d * 0.8f, cy - half, 2, half * 2, col);
            break;
        }
    }
}

/* Hilo de la VM, dentro del frame de lcd.c. */
static void pad_overlay(void) {
    int i;
    for (i = 0; i < 2; i++)
        if (cur.panel[i][2] > 0)
            vita2d_draw_rectangle(cur.panel[i][0], cur.panel[i][1],
                                  cur.panel[i][2], cur.panel[i][3],
                                  RGBA8(0x07, 0x10, 0x2C, 0xFF));
    for (i = 0; i < cur.nkeys; i++) {
        const vj_padkey *k = &cur.keys[i];
        unsigned int bg = key_down[i] ? RGBA8(0x31, 0x77, 0xE2, 0xFF)
                                      : RGBA8(0x18, 0x2A, 0x5A, 0xFF);
        fill_rrect(k->x, k->y, k->w, k->h, 12, bg);
        if (!k->label) {
            draw_arrow(k->x + k->w / 2, k->y + k->h / 2, k->h * 0.22f,
                       k->code, RGBA8(0xE6, 0xEE, 0xFF, 0xFF));
        } else if (pad_font) {
            float sc = k->h > 70 ? 1.3f : 1.0f;
            int tw = vita2d_pgf_text_width(pad_font, sc, k->label);
            vita2d_pgf_draw_text(pad_font, (int)(k->x + (k->w - tw) / 2),
                                 (int)(k->y + k->h / 2 + 9 * sc),
                                 RGBA8(0xE6, 0xEE, 0xFF, 0xFF), sc, k->label);
        }
    }
}

static int inside(float x, float y, const float *r) {
    return x >= r[0] && x < r[0] + r[2] && y >= r[1] && y < r[1] + r[3];
}

static int touch_thread(SceSize args, void *argp) {
    SceTouchData t;
    int pen_id = -1, pen_x = 0, pen_y = 0, i, k;
    (void)args; (void)argp;
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,
                             SCE_TOUCH_SAMPLING_STATE_START);
    for (;;) {
        sceKernelDelayThread(16 * 1000);
        /* Biblioteca o pantalla de carga todavía delante: nada al MIDlet. */
        if (vita_lcd_loading_hook || suspend_key_input) continue;
        memset(&t, 0, sizeof(t));
        if (sceTouchPeek(SCE_TOUCH_PORT_FRONT, &t, 1) < 0) continue;

        for (k = 0; k < cur.nkeys; k++) {
            float r[4];
            int on = 0;
            set4(r, cur.keys[k].x, cur.keys[k].y, cur.keys[k].w, cur.keys[k].h);
            for (i = 0; i < t.reportNum; i++)
                if (inside(t.report[i].x / 2.0f, t.report[i].y / 2.0f, r)) on = 1;
            if (on != key_down[k]) {
                key_down[k] = (unsigned char)on;
                javanotify_key_event(cur.keys[k].code,
                                     on ? JAVACALL_KEYPRESSED
                                        : JAVACALL_KEYRELEASED);
            }
        }

        if (!cur_touch) continue;
        {
            float d[4];
            int found = -1, gw = javacall_lcd_get_screen_width();
            int gh = javacall_lcd_get_screen_height();
            set4(d, vita_lcd_dest[0], vita_lcd_dest[1], vita_lcd_dest[2],
                 vita_lcd_dest[3]);
            if (d[2] < 1 || d[3] < 1) continue;
            for (i = 0; i < t.reportNum; i++) {
                float x = t.report[i].x / 2.0f, y = t.report[i].y / 2.0f;
                if (pen_id >= 0 ? t.report[i].id == pen_id : inside(x, y, d)) {
                    found = i;
                    break;
                }
            }
            if (found >= 0) {
                int x = (int)((t.report[found].x / 2.0f - d[0]) * gw / d[2]);
                int y = (int)((t.report[found].y / 2.0f - d[1]) * gh / d[3]);
                if (x < 0) x = 0;
                if (x >= gw) x = gw - 1;
                if (y < 0) y = 0;
                if (y >= gh) y = gh - 1;
                if (pen_id < 0) {
                    pen_id = t.report[found].id;
                    javanotify_pen_event(x, y, JAVACALL_PENPRESSED);
                } else if (x != pen_x || y != pen_y) {
                    javanotify_pen_event(x, y, JAVACALL_PENDRAGGED);
                }
                pen_x = x;
                pen_y = y;
            } else if (pen_id >= 0) {
                javanotify_pen_event(pen_x, pen_y, JAVACALL_PENRELEASED);
                pen_id = -1;
            }
        }
    }
    return 0;
}

void cfg_apply(const vj_cfg *c) {
    int gw, gh, i, n = 0;
    cfg_screen(c, &gw, &gh);
    vita_lcd_set_virtual_size(gw, gh);

    cfg_layout(c, &cur);
    vita_lcd_area[0] = (int)cur.area[0];
    vita_lcd_area[1] = (int)cur.area[1];
    vita_lcd_area[2] = (int)cur.area[2];
    vita_lcd_area[3] = (int)cur.area[3];
    vita_lcd_scale = c->scale;
    vita_lcd_filter = c->filter;
    cur_touch = c->touch;

    for (i = 0; i < VJ_BTN_N; i++) {
        if (!c->map[i]) continue;
        vita_keymap_custom[n].nativeKey = btn_mask[i];
        vita_keymap_custom[n].javaKey = (javacall_key)c->map[i];
        vita_keymap_custom[n].shift = 0;
        n++;
    }
    /* sin ningún botón mapeado se usaría el de PSPKVM: dejar una
     * entrada inerte para que siga siendo "propio" */
    if (!n) {
        vita_keymap_custom[0].nativeKey = 0;
        vita_keymap_custom[0].javaKey = JAVACALL_KEY_INVALID;
        vita_keymap_custom[0].shift = 0;
        n = 1;
    }
    vita_keymap_custom_n = n;

    if (cur.nkeys && !pad_font) pad_font = vita2d_load_default_pgf();
    vita_lcd_overlay_hook = cur.nkeys ? pad_overlay : 0;
    if (touch_tid < 0 && (cur.nkeys || c->touch)) {
        touch_tid = sceKernelCreateThread("vj_touch", touch_thread, 0x10000100,
                                          0x4000, 0, 0, NULL);
        if (touch_tid >= 0) sceKernelStartThread(touch_tid, 0, NULL);
    }
    vj_log("cfg: pantalla %dx%d escala=%d filtro=%d teclado=%d tecla(s) "
           "táctil=%d juego en %.0f,%.0f %.0fx%.0f", gw, gh, c->scale,
           c->filter, cur.nkeys, c->touch, cur.dest[0], cur.dest[1],
           cur.dest[2], cur.dest[3]);
}
