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
} vj_game;

enum {
    LAUNCHER_EXIT = -1,     /* salir de la app */
    LAUNCHER_CLASSIC = 0,   /* abrir el gestor clásico (AMS Java) */
    LAUNCHER_PLAY = 1       /* lanzar *out */
};

/* Requiere vita2d ya inicializado (javacall_lcd_init). Bloquea hasta que
 * el usuario elige; deja en pantalla un frame "Iniciando…". */
int launcher_run(vj_game *out);

/* Lo implementa main.c (escribe en bringup-NNN.log). */
void vj_log(const char *fmt, ...);

#endif
