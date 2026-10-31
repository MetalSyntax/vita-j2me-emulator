/* launcher.h — biblioteca nativa de juegos (vita2d, 960x544).
 *
 * Sustituye a la lista del AMS Java como pantalla de inicio: lee las
 * suites instaladas de appdb/_suites.dat y lanza la elegida DIRECTAMENTE
 * (runMidlet <suiteId> <clase>) en el isolate principal, sin pasar por
 * MVMManager ni por la creación de un isolate nuevo. */
#ifndef VITAJ2ME_LAUNCHER_H
#define VITAJ2ME_LAUNCHER_H

typedef struct {
    int suite_id;
    char cls[128];       /* clase del MIDlet a lanzar */
    char name[96];       /* nombre visible */
    char vendor[64];
    char jar[256];       /* ruta del JAR instalado (appdb/) */
    char icon[128];      /* entrada PNG dentro del JAR, sin '/' inicial */
    int jar_size;
    int midlets;
    int missing;         /* el JAR de la suite ya no está en su sitio */
} vj_game;

enum {
    LAUNCHER_EXIT = -1,     /* salir de la app */
    LAUNCHER_PLAY = 1       /* lanzar *out */
};

/* Requiere vita2d ya inicializado (javacall_lcd_init). Bloquea hasta que
 * el usuario elige; empieza con el juego `preselect` (suiteId) marcado. */
int launcher_run(vj_game *out, int preselect);

/* Juegos instalados (sin iconos). */
int launcher_list(vj_game *g, int max);

/* Pantalla de carga animada en su propio hilo hasta que el MIDlet pinta
 * contenido. */
void launcher_loading_start(const vj_game *g);

/* Menú del juego (L+R+START): 1 = salir a la biblioteca, 2 = reiniciar
 * el juego (cambió la resolución o el táctil). */
int launcher_game_menu(const vj_game *g);

/* Instalación por lotes de `total` juegos con pantalla nativa y progreso
 * (sin UI de PSPKVM). */
void launcher_install_start(int total);

/* Precarga: la VM corre detrás de la biblioteca sin presentar nada. */
void launcher_preload_hold(void);

/* Resolución de pantalla deducida del JAR (1 = encontrada). */
int jar_detect_screen(const char *jar, int *w, int *h);

/* 1 si el JAR tiene META-INF/MANIFEST.MF (si no, MIDP no lo instala). */
int jar_has_manifest(const char *jar);

/* Primitiva compartida con el teclado virtual (gamecfg.c). */
void fill_rrect(float x, float y, float w, float h, float r, unsigned int c);

/* Lo implementa main.c (escribe en bringup-NNN.log). */
void vj_log(const char *fmt, ...);

#endif
