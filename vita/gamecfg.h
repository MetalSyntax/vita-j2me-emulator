/* gamecfg.h — ajustes por juego (pantalla, teclado virtual, táctil,
 * mapeo de botones) y su aplicación en la VM.
 *
 * Fichero: ux0:data/vitaj2me/config/<suiteId>.cfg (clave=valor). */
#ifndef VITAJ2ME_GAMECFG_H
#define VITAJ2ME_GAMECFG_H

#include "launcher.h"

enum { VJ_SCALE_FIT = 1, VJ_SCALE_INT, VJ_SCALE_STRETCH, VJ_SCALE_1X };
enum { VJ_PAD_AUTO = 0, VJ_PAD_OFF, VJ_PAD_LEFT, VJ_PAD_RIGHT, VJ_PAD_BOTH };

#define VJ_BTN_N 16

typedef struct {
    int res;            /* 0 = automática; si no, índice+1 en vj_res */
    int scale;          /* VJ_SCALE_* */
    int filter;         /* 0 nítido, 1 suave */
    int keypad;         /* VJ_PAD_* */
    int touch;          /* 1 = la pantalla táctil es el puntero del juego */
    int map[VJ_BTN_N];  /* tecla Java por botón (JAVACALL_KEY_*, 0 = nada) */
    int auto_w, auto_h; /* detectada en el JAR (0 = sin detectar aún) */
} vj_cfg;

typedef struct {
    float x, y, w, h;
    int code;           /* JAVACALL_KEY_* */
    const char *label;
} vj_padkey;

#define VJ_PAD_MAX 32

typedef struct {
    float area[4];      /* zona de 960x544 donde va el juego */
    float dest[4];      /* rectángulo del juego escalado */
    int nkeys;
    vj_padkey keys[VJ_PAD_MAX];
    float panel[2][4];  /* fondos de los paneles laterales (w=0: no hay) */
} vj_layout;

extern const int vj_res[][2];
extern const int vj_res_n;
extern const char *const vj_btn_name[VJ_BTN_N];
extern const int vj_key_codes[];
extern const char *const vj_key_names[];
extern const int vj_keys_n;

void cfg_defaults(vj_cfg *c);
/* Lee el .cfg (o los valores por defecto) y detecta la resolución. */
void cfg_load(const vj_game *g, vj_cfg *c);
void cfg_save(const vj_game *g, const vj_cfg *c);
void cfg_screen(const vj_cfg *c, int *w, int *h);
void cfg_layout(const vj_cfg *c, vj_layout *lo);
const char *cfg_key_name(int code);
/* Antes de JavaTask (live=0) o en caliente con el juego corriendo
 * (live=1: no toca la resolución, que la VM ya fijó). */
void cfg_apply(const vj_cfg *c, int live);

int cfg_last_suite(void);
void cfg_set_last_suite(int suite_id);

#endif
