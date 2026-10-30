// vita-j2me-emulator/vita/main.c — Frontend VitaJ2ME (Fase 2b).
//
// Flujo (igual que psp/pspkvm.c, sin pspsdk):
//   1. Diagnóstico bringup (log): ctrl, audio, apputil/locale, heap.
//   2. chdir(ux0:data/vitaj2me): midpHome="." -> classes.zip, lib/,
//      appdb/, jwc_properties.ini (auto) y suitestore quedan en un
//      único raíz escribible por FTP.
//   3. javanotify_start()  -> evento runMidlet -1 MVMManager (AMS).
//   4. JavaTask()          -> bucle de eventos + VM (no retorna hasta
//      el shutdown; corre en el hilo principal).
//   5. finalize + exit.
//
// Salir del juego: L+R+START (DEBUG_TRACE, log), L+R+X apaga el MIDlet,
// Sesión 46: sin AMS clásico; en juego L+R+START abre el menú nativo.
// NOTA: JavaTaskImpl/runMidlet/md vienen de libmidp.a; JavaTask crea
// el hilo VM internamente (midpInitializeMemory incluido).

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/audioout.h>
#include <psp2/apputil.h>
#include <psp2/system_param.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <psp2/appmgr.h>
#include <psp2/power.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "javacall_logging.h"
#include "javacall_events.h"
#include "launcher.h"
#include "gamecfg.h"

/* Entrypoints de libmidp.a (jams_port/javacall + javacall_common). */
void JavaTask(void);
void javanotify_start_suite(char *suiteId);
/* LCD javacall (libjavacall.a): init temprano + splash para no dejar la
 * pantalla en negro durante el bootstrap (el CommandLineInstaller no
 * pinta nada: parecía que "no corre"). Idempotente con el lcd_init que
 * la JVM llama después (lcd.c re-aplica filtro/fit en cada init). */
int javacall_lcd_init(void);
void javacall_lcd_show_splash(void);
javacall_result javacall_events_init(void);
void javacall_logging_channel(int channel);

/* Requerido por keypress.c (hilo de teclas): 1 = input suspendido.
 * En v1 siempre 0 (sin diálogos nativos que lo necesiten). */
int suspend_key_input = 0;

/* Sesión de logs (la lee logging.c para el jvm-*.log). Se fija en main. */
int vitaj2me_session_id = 0;

/* Contadores de javacall (lcd.c/keypress.c) para el watchdog. */
extern volatile unsigned long vita_flush_count;
extern volatile unsigned long vita_key_count;

/* Hilo principal (corre la JVM dentro de JavaTask): se muestrea para
 * distinguir spin (clocks avanzan) de bloqueo (WAITING + quieto). */
static SceUID vita_main_tid = -1;

static void log_line(const char *fmt, ...);

/* Watchdog: cada 10 s deja en el bringup una línea con frames
 * presentados y teclas vistas desde la anterior. Permite distinguir
 * "colgado" (contadores quietos) de "lento" (avanzan a 1 FPS), y mide
 * FPS reales sin depender del stdout del juego. Hilo aparte: sobrevive
 * aunque la JVM esté atascada; sceKernelExitProcess lo mata al salir. */
extern volatile unsigned long vita_jvmlog_writes;
extern volatile unsigned long vita_jvmlog_fails;
extern volatile int vita_jvmlog_lasterr;
extern volatile unsigned long vita_jvmlog_empty;
extern volatile unsigned long vita_jvmlog_bytes;
extern volatile unsigned long long vita_jvmlog_io_us;
int vita_event_cond_uid(void);
void vita_jvmlog_flush(void);
void javanotify_key_event(int key, int type);
#define VJ_KEY_DEBUG_TRACE (-22) /* JAVACALL_KEY_DEBUG_TRACE */
#define VJ_KEYPRESSED 1

/* Volcado automático (sesión 38): en HW 037 el Launch dejó el hilo de la
 * JVM dormido en la cola de eventos (waitType 0x100 = cond) sin pintar
 * nada. Eso es "ningún hilo Java ejecutable": deadlock o espera en Java.
 * Tras 30 s así se inyecta JAVACALL_KEY_DEBUG_TRACE: checkForSystemSignal
 * llama a pss() y la pila de TODOS los hilos Java va al jvm.log. Máximo 3
 * por sesión; se rearma cuando vuelve a haber frames. */
#define HB_IDLE_DUMP_SECS 30
#define HB_MAX_DUMPS 3

static int hb_thread(SceSize args, void *argp) {
    unsigned long last_flush = 0, last_keys = 0;
    unsigned long long last_clocks = 0;
    unsigned int ticks = 0;
    unsigned int idle_secs = 0;
    int dumps = 0, armed = 1;
    (void)args; (void)argp;

    for (;;) {
        unsigned long f, k;
        SceKernelThreadInfo ti;
        unsigned long long clocks = 0;
        unsigned int st = 0, wt = 0;
        int wid = 0;

        sceKernelDelayThread(10 * 1000 * 1000);
        ticks += 10;
        vita_jvmlog_flush();  /* el jvm log se vuelca por tiempo: no dejarlo colgado */
        f = vita_flush_count;
        k = vita_key_count;
        memset(&ti, 0, sizeof(ti));
        ti.size = sizeof(ti);
        if (vita_main_tid >= 0 &&
            sceKernelGetThreadInfo(vita_main_tid, &ti) == 0) {
            st = ti.status;
            wt = ti.waitType;
            wid = (int)ti.waitId;
            clocks = (unsigned long long)ti.runClocks;
        }
        {
            int in_queue = (wid != 0 && wid == vita_event_cond_uid());
            log_line("HB +%us: flush=%lu (+%lu ~%lu FPS) keys=%lu (+%lu) "
                     "main: st=%u wait=0x%x/0x%08x%s clocks=%llu (+%llu) "
                     "jvmlog=%lu (vacias %lu, %lu B, io %llu ms) "
                     "fail=%lu err=0x%08x",
                     ticks, f, f - last_flush, (f - last_flush) / 10,
                     k, k - last_keys,
                     st, wt, (unsigned)wid,
                     in_queue ? "(cola eventos)" : "",
                     clocks, clocks - last_clocks,
                     vita_jvmlog_writes, vita_jvmlog_empty,
                     vita_jvmlog_bytes, vita_jvmlog_io_us / 1000,
                     vita_jvmlog_fails,
                     (unsigned)vita_jvmlog_lasterr);
            /* HW 041: el juego corría a ~1 FPS fuera de la cola. Lento
             * (<20 frames en 10 s) durante 30 s también pide volcado:
             * la pila de los hilos Java dice en qué se va el tiempo. */
            if (f != last_flush && (f - last_flush) >= 20) {
                idle_secs = 0;
                armed = 1;
            } else if (f > 1 && (in_queue || f != last_flush)) {
                idle_secs += 10;
            }
            if (armed && dumps < HB_MAX_DUMPS &&
                idle_secs >= HB_IDLE_DUMP_SECS) {
                log_line("HB: VM ocioso o lento (<2 FPS) %us -> "
                         "volcado pss automatico #%d (ver jvm-%03d.log)",
                         idle_secs, dumps + 1, vitaj2me_session_id);
                javanotify_key_event(VJ_KEY_DEBUG_TRACE, VJ_KEYPRESSED);
                dumps++;
                armed = 0;
            }
        }
        last_flush = f;
        last_keys = k;
        last_clocks = clocks;
    }
    return 0;
}

static void start_watchdog(void) {
    SceUID th = sceKernelCreateThread("vitaj2me_hb", hb_thread,
                                      0x10000100, 0x2000, 0, 0, 0);
    if (th >= 0) {
        sceKernelStartThread(th, 0, 0);
        log_line("watchdog HB cada 10s (FPS + teclas)");
    } else {
        log_line("WARN sin watchdog (0x%08x)", (unsigned)th);
    }
}

#define LOG_DIR "ux0:data/vitaj2me/logs"
#define VITA_HOME "ux0:data/vitaj2me"
#define FB_W 960
#define FB_H 544

/* Ruta del log de esta sesión; se rellena en boot_session_log(). */
static char session_log_path[128] = "";

static void log_line(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printf("[vitaj2me] %s\n", buf);

    SceUID fd = sceIoOpen(session_log_path,
        SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd >= 0) {
        char line[560];
        int n = snprintf(line, sizeof(line), "[vitaj2me] %s\n", buf);
        sceIoWrite(fd, line, (SceSize)n);
        sceIoClose(fd);
    }
}

/* Logs incrementales: cada arranque suma 1 (logs/session.txt) y escribe
 * logs/bringup-NNN.log + logs/jvm-NNN.log (este último vía logging.c,
 * que lee vitaj2me_session_id). Nada se borra: registro de pruebas. */
static void boot_session_log(void) {
    char numbuf[16];
    int last = 0;
    SceUID fd;

    sceIoMkdir(LOG_DIR, 0777);

    fd = sceIoOpen(LOG_DIR "/session.txt", SCE_O_RDONLY, 0777);
    if (fd >= 0) {
        int n = sceIoRead(fd, numbuf, sizeof(numbuf) - 1);
        sceIoClose(fd);
        if (n > 0) {
            numbuf[n] = '\0';
            last = atoi(numbuf);
            if (last < 0 || last > 99998) {
                last = 0;
            }
        }
    }

    vitaj2me_session_id = last + 1;
    snprintf(session_log_path, sizeof(session_log_path),
             LOG_DIR "/bringup-%03d.log", vitaj2me_session_id);

    fd = sceIoOpen(LOG_DIR "/session.txt",
                   SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd >= 0) {
        int n = snprintf(numbuf, sizeof(numbuf), "%d\n", vitaj2me_session_id);
        sceIoWrite(fd, numbuf, (SceSize)n);
        sceIoClose(fd);
    }
}

static void make_dirs(void) {
    sceIoMkdir(VITA_HOME, 0777);
    sceIoMkdir("ux0:data/vitaj2me/root", 0777);
    sceIoMkdir("ux0:data/vitaj2me/private", 0777);
    sceIoMkdir("ux0:data/vitaj2me/music", 0777);
    sceIoMkdir("ux0:data/vitaj2me/photo", 0777);
    sceIoMkdir("ux0:data/vitaj2me/video", 0777);
    sceIoMkdir("ux0:data/vitaj2me/recordings", 0777);
    sceIoMkdir("ux0:data/vitaj2me/tones", 0777);
    sceIoMkdir("ux0:data/vitaj2me/graphics", 0777);
    sceIoMkdir("ux0:data/vitaj2me/fonts", 0777);
    sceIoMkdir("ux0:data/vitaj2me/games", 0777);
    sceIoMkdir("ux0:data/vitaj2me/lib", 0777);
    sceIoMkdir("ux0:data/vitaj2me/appdb", 0777);
}

/* Inventario del home J2ME.
 *
 * OJO (Bug real #16): las sesiones 19/20 loguearon "classes.zip NO EXISTE"
 * teniendo el archivo en la consola. El chequeo hacia
 * sceIoGetstat("classes.zip", ...) — RELATIVO — dando por sentado que el
 * chdir(VITA_HOME) de arriba lo afectaba. No lo afecta: chdir() solo mueve
 * el cwd interno de newlib (el que usan fopen/open/stat), mientras que las
 * sceIo* son la syscall cruda de SceIofilemgr y no conocen ningun cwd —
 * siempre necesitan la ruta completa con dispositivo. El resultado era un
 * falso negativo que mando a perseguir el problema equivocado dos sesiones
 * seguidas. Desde aca todas las sceIo* van con ruta absoluta.
 */
static int stat_entry(const char *name, const char *label) {
    char path[256];
    SceIoStat st;

    snprintf(path, sizeof(path), "%s/%s", VITA_HOME, name);
    memset(&st, 0, sizeof(st));
    if (sceIoGetstat(path, &st) < 0) {
        /* Segunda opinion por newlib antes de dar algo por ausente: este
         * chequeo ya dio un falso negativo una vez (ver nota de arriba) y
         * abortar el arranque por error sale mas caro que insistir. */
        FILE *f = fopen(path, "rb");
        if (f) {
            fclose(f);
            log_line("  [ok    ] %-16s %s (sceIoGetstat fallo, fopen no)",
                     name, label);
            return 1;
        }
        log_line("  [FALTA] %-16s %s", name, label);
        return 0;
    }
    if (SCE_S_ISDIR(st.st_mode)) {
        log_line("  [dir  ] %-16s %s", name, label);
    } else {
        log_line("  [%6lld] %-16s %s", (long long)st.st_size, name, label);
    }
    return 1;
}

/* Lista un subdirectorio del home (para ver si el .jar esta donde va). */
static void list_subdir(const char *name, int max_entries) {
    char path[256];
    SceIoDirent ent;
    SceUID dfd;
    int n = 0;

    snprintf(path, sizeof(path), "%s/%s", VITA_HOME, name);
    dfd = sceIoDopen(path);
    if (dfd < 0) {
        log_line("  %s/: no se puede abrir (0x%08x)", name, (unsigned)dfd);
        return;
    }
    memset(&ent, 0, sizeof(ent));
    while (sceIoDread(dfd, &ent) > 0) {
        if (ent.d_name[0] == '.') {
            memset(&ent, 0, sizeof(ent));
            continue;
        }
        if (n < max_entries) {
            log_line("  %s/%-28s %lld bytes", name, ent.d_name,
                     (long long)ent.d_stat.st_size);
        }
        n++;
        memset(&ent, 0, sizeof(ent));
    }
    sceIoDclose(dfd);
    if (n == 0) {
        log_line("  %s/: VACIO", name);
    } else if (n > max_entries) {
        log_line("  %s/: ... y %d mas", name, n - max_entries);
    }
}

/* Devuelve 0 si falta algo imprescindible para que arranque el AMS. */
static int check_j2me_home(void) {
    int ok;

    log_line("inventario de %s:", VITA_HOME);
    ok = stat_entry("classes.zip", "(clases MIDP; IMPRESCINDIBLE)");
    stat_entry("lib", "(jwc_properties.ini, fuentes)");
    stat_entry("appdb", "(suitestore: MIDlets instalados)");
    stat_entry("games", "(.jar/.jad a instalar)");
    list_subdir("games", 12);
    list_subdir("lib", 12);
    list_subdir("fonts", 16);
    list_subdir("appdb", 32);

    return ok;
}

/* Sufijo insensible a mayúsculas (".JAR"/".Jar" valen; el FS Vita sí
 * distingue mayúsculas y muchos MIDlets traen extensiones en mayúsculas). */
static int has_suffix_ci(const char *name, const char *suf) {
    size_t nl = strlen(name), sl = strlen(suf);
    size_t i;
    if (nl < sl) {
        return 0;
    }
    for (i = 0; i < sl; i++) {
        char a = name[nl - sl + i], b = suf[i];
        if (a >= 'A' && a <= 'Z') {
            a += 'a' - 'A';
        }
        if (a != b) {
            return 0;
        }
    }
    return 1;
}

/* Tamaño de un fichero por ruta absoluta, -1 si no existe. */
static long long file_size_abs(const char *path) {
    SceIoStat st;
    memset(&st, 0, sizeof(st));
    if (sceIoGetstat(path, &st) < 0) {
        return -1;
    }
    return (long long)st.st_size;
}

/* Auto-instalación de games/*.jad|jar: MODO INSTALADOR vs MODO AMS.
 *
 * Por qué existe: el DiscoveryApp del AMS ("Install from memory stick")
 * está hardcodeado a "ms0:/" (DiscoveryApp.java:172,183-186) y su
 * listado sale vacío en Vita — instalar a mano desde el menú no puede
 * funcionar sin recompilar MIDP (bloqueado por el SIGSEGV de romgen,
 * bug #14). En cambio el CommandLineInstaller ("I <file://url>",
 * silencioso) sí funciona si la URL cuelga de una raíz JSR-75 válida:
 * fileconnection.c expone games/ como 9na raíz.
 *
 * Restricción arquitectónica (bug #20, jvm-026.log): JavaTask() procesa
 * UN SOLO evento START por proceso (JavaTaskIsGoOn=FALSE tras el
 * primero, javaTask.c:128) y el pump interno de la JVM
 * (checkForSystemSignal, midp_msgQueue_md.c) NO tiene caso para
 * START_ARBITRARY_ARG: lo consume y lo tira en silencio. Encolar el
 * install DETRÁS del arranque del AMS (diseño de la sesión 24) no corre
 * nunca: ni stdout, ni error, ni crash — nada. Comprobado en HW.
 *
 * Diseño actual: arranques de propósito único.
 *  - Si hay un .jad/.jar pendiente → MODO INSTALADOR: el ÚNICO evento
 *    del proceso es el install (CommandLineInstaller). Al terminar, la
 *    JVM sale sola y el frontend pide reiniciar. Un .jar por arranque;
 *    con varios se encadenan solos (uno por boot).
 *  - Si no hay nada pendiente → MODO AMS: arranque normal al menú, el
 *    juego ya está en "Installed midlets" (L+R+Triángulo vuelve al AMS).
 *
 * Idempotencia: appdb/.autoinst_<fich>.mark guarda "<tamaño>\n<intentos>".
 * Se considera instalado si el tamaño coincide Y appdb tiene suites. Si
 * appdb está vacío tras 3 intentos, se da por imposible (log) y se pasa
 * al AMS para no atrapar al usuario en un bucle (borrar el .mark
 * reintenta). Cambiar el .jar por otro tamaño reinstala.
 */
/* ¿Hay al menos una suite instalada? (un subdirectorio en appdb). */
static int appdb_has_suites(void) {
    static const char *ADIR = VITA_HOME "/appdb";
    SceUID dfd;
    SceIoDirent ent;
    int found = 0;

    dfd = sceIoDopen(ADIR);
    if (dfd < 0) {
        return 0;
    }
    memset(&ent, 0, sizeof(ent));
    while (sceIoDread(dfd, &ent) > 0) {
        if (ent.d_name[0] == '.') {
            memset(&ent, 0, sizeof(ent));
            continue;
        }
        /* Las suites son subdirectorios... pero el instalador con el
         * hack PSPKVM (referencia al .jar, sin copia) a veces solo deja
         * FICHEROS: 0000000N.ii/.ap (registros) y _suites.dat (índice).
         * En consola se confirmó 00000002.ii/.ap + _suites.dat de 248 B
         * sin ningún subdirectorio. Se aceptan ambas evidencias. */
        if (has_suffix_ci(ent.d_name, ".ii") ||
            has_suffix_ci(ent.d_name, ".ap") ||
            strcmp(ent.d_name, "_suites.dat") == 0) {
            log_line("appdb: suite detectada por fichero: %s",
                     ent.d_name);
            found = 1;
            break;
        }
        /* OJO (bug #23): sceIoDread NO rellena st_mode (queda a 0);
         * el flag de directorio vive en st_attr (bits SCE_SO_*), igual
         * que hace dir.c del propio port. Como red (st_attr podría
         * venir vacío en algún FW), ante la duda se prueba a abrir:
         * sceIoDopen solo tiene éxito contra directorios. */
        if (ent.d_stat.st_attr & SCE_SO_IFDIR) {
            found = 1;
            break;
        }
        {
            char sub[288];
            SceUID tfd;
            snprintf(sub, sizeof(sub), "%s/%s", ADIR, ent.d_name);
            tfd = sceIoDopen(sub);
            if (tfd >= 0) {
                sceIoDclose(tfd);
                log_line("appdb: suite detectada por apertura: %s",
                         ent.d_name);
                found = 1;
                break;
            }
        }
        memset(&ent, 0, sizeof(ent));
    }
    sceIoDclose(dfd);
    return found;
}

/* .mark v2: "<size>\n<intentos>\n<markver>\n" (MARK_VERSION actual 2).
 * Los marks v1/v0 (sin versión, del diseño anterior sin -D) se tratan
 * como frescos: el intento anterior falló sí o sí por el bug #22, así
 * que hay que reintentar sin pedirle al usuario que borre nada. */
#define MARK_VERSION 2

/* Lee un .mark. Devuelve intentos, tamaño por *msize (-1 si no hay) y
 * versión por *mver (0 si formato viejo). */
static int read_mark(const char *mark, long long *msize, int *mver) {
    SceUID mfd;
    char mbuf[64];
    int n, attempts = 0;

    *msize = -1;
    *mver = 0;
    mfd = sceIoOpen(mark, SCE_O_RDONLY, 0777);
    if (mfd < 0) {
        return 0;
    }
    n = sceIoRead(mfd, mbuf, sizeof(mbuf) - 1);
    sceIoClose(mfd);
    if (n > 0) {
        char *nl, *nl2;
        mbuf[n] = '\0';
        *msize = atoll(mbuf);
        nl = strchr(mbuf, '\n');
        if (nl) {
            attempts = atoi(nl + 1);
            if (attempts < 0) {
                attempts = 0;
            }
            nl2 = strchr(nl + 1, '\n');
            if (nl2) {
                *mver = atoi(nl2 + 1);
            }
        }
    }
    return attempts;
}

static void write_mark(const char *mark, long long sz, int attempts) {
    SceUID mfd = sceIoOpen(mark, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC,
                           0777);
    if (mfd >= 0) {
        char mbuf[64];
        int n = snprintf(mbuf, sizeof(mbuf), "%lld\n%d\n%d\n", sz, attempts,
                         MARK_VERSION);
        sceIoWrite(mfd, mbuf, (SceSize)n);
        sceIoClose(mfd);
    }
}

/* Busca UN .jad/.jar pendiente de instalar en games/.
 * Devuelve 1 y rellena out_name (solo fichero) si hay; 0 si toca AMS.
 * Reglas: .jad manda sobre su .jar hermano; instalado = tamaño igual en
 * .mark + appdb con suites; 3 intentos sin suites = imposible (se avisa
 * y se deja pasar al AMS). */
static int pending_install_pick(char *out_name, size_t out_sz) {
    static const char *GDIR = VITA_HOME "/games";
    static const char *ADIR = VITA_HOME "/appdb";
    SceUID dfd;
    SceIoDirent ent;
    int have_suites = appdb_has_suites();
    int found = 0;

    dfd = sceIoDopen(GDIR);
    if (dfd < 0) {
        log_line("games/: no se puede abrir (0x%08x)", (unsigned)dfd);
        return 0;
    }

    memset(&ent, 0, sizeof(ent));
    while (sceIoDread(dfd, &ent) > 0) {
        char fpath[288], mark[288];
        long long sz, msize;
        int attempts, is_jad, mver;
        char *dot;

        if (ent.d_name[0] == '.') {
            memset(&ent, 0, sizeof(ent));
            continue;
        }
        is_jad = has_suffix_ci(ent.d_name, ".jad");
        if (!is_jad && !has_suffix_ci(ent.d_name, ".jar")) {
            memset(&ent, 0, sizeof(ent));
            continue;
        }
        if (!is_jad) {
            char jadpath[288];
            dot = strrchr(ent.d_name, '.');
            snprintf(jadpath, sizeof(jadpath), "%s/%.*s.jad", GDIR,
                     (int)(dot - ent.d_name), ent.d_name);
            if (file_size_abs(jadpath) >= 0) {
                memset(&ent, 0, sizeof(ent));
                continue; /* lo cubre su .jad */
            }
        }

        snprintf(fpath, sizeof(fpath), "%s/%s", GDIR, ent.d_name);
        sz = file_size_abs(fpath);
        if (sz < 0) {
            memset(&ent, 0, sizeof(ent));
            continue;
        }
        snprintf(mark, sizeof(mark), "%s/.autoinst_%s.mark", ADIR,
                 ent.d_name);
        attempts = read_mark(mark, &msize, &mver);

        if (mver < MARK_VERSION) {
            /* Diseño anterior (sin -D, bug #22): reintentar de oficio. */
            log_line("  games/%s: mark v%d obsoleto — se reintenta",
                     ent.d_name, mver);
        } else if (msize == sz && have_suites) {
            memset(&ent, 0, sizeof(ent));
            continue; /* instalado */
        } else if (msize == sz && !have_suites && attempts >= 3) {
            log_line("  games/%s: 3 intentos sin instalar — se omite "
                     "(borra su .mark en appdb/ para reintentar)",
                     ent.d_name);
            memset(&ent, 0, sizeof(ent));
            continue;
        }
        /* Pendiente: sin mark, tamaño cambiado, mark obsoleto, o
         * reintento < 3. El cambio de versión resetea el contador. */
        {
            int now = (mver >= MARK_VERSION && msize == sz) ? attempts + 1
                                                            : 1;
            snprintf(out_name, out_sz, "%s", ent.d_name);
            write_mark(mark, sz, now);
            log_line("  games/%s: pendiente (%lld bytes, intento %d)",
                     ent.d_name, sz, now);
        }
        found = 1;
        memset(&ent, 0, sizeof(ent));
        break; /* uno por arranque */
    }
    sceIoDclose(dfd);
    return found;
}

/* Envío propio de START_ARBITRARY_ARG (bug #22).
 *
 * javanotify_start/install_* viven en libmidp.a (no recompilable) con
 * argv FIJOS sin -D. Pero las tablas estáticas de propiedades de ESTE
 * build están VACÍAS (ningún .o de libmidp.a trae "microedition.profiles":
 * verificado con strings) → System.getProperty("microedition.profiles")
 * da NULL siempre y el instalador muere en matchProfile (jvm-029), y
 * cualquier juego que pregunte su perfil moriría igual.
 * El bypass: el frontend arma el evento a mano con -D...=... (máx 10
 * argv, MIDP_RUNMIDLET_MAXIMUM_ARGS), que JVM_ParseOneArg convierte en
 * JVMSPI_SetSystemProperty → setSystemProperty (el camino del bug #14,
 * ya con veneer PIC). Layout compatible por construcción: type@0,
 * argc@4, argv@8 — los únicos campos que JavaTaskImpl lee.
 * OJO: el parseo ESCRIBE un NUL sobre el '=' (Arguments.cpp), así que
 * los -D van en buffers estáticos mutables, nunca literales. */
#define VITA_START_ARBITRARY_ARG 101
#define VITA_START_MAX_ARGS 10

static char vita_arg_D_profiles[] = "-Dmicroedition.profiles=MIDP-2.1";
static char vita_arg_D_config[] = "-Dmicroedition.configuration=CLDC-1.1";
static char vita_arg_D_encoding[] = "-Dmicroedition.encoding=ISO-8859-1";
static char vita_arg_classpathext[] = "-classpathext";
static char vita_arg_classes[] = "classes.zip";
static char vita_arg_internal[] = "-1";
static char vita_arg_runmidlet[] = "runMidlet";

typedef struct {
    int eventType;
    int argc;
    char *argv[VITA_START_MAX_ARGS];
} vita_start_event;

static void vita_send_start(int argc, char *argv[]) {
    vita_start_event e;
    int i;

    memset(&e, 0, sizeof(e));
    e.eventType = VITA_START_ARBITRARY_ARG;
    e.argc = argc;
    for (i = 0; i < argc && i < VITA_START_MAX_ARGS; i++) {
        e.argv[i] = argv[i];
    }
    log_line("START propio: %d args (%s ... %s)", argc, argv[0],
             argv[argc - 1]);
    javacall_event_send((unsigned char *)&e, sizeof(e));
}

/* Juego elegido en la biblioteca nativa: runMidlet <suiteId> <clase>.
 * MIDletSuiteLoader crea la suite desde el suitestore y la arranca en
 * el isolate PRINCIPAL (createMIDletSuite -> startSuite), sin
 * MVMManager.launchSuite ni Isolate nuevo — el camino que en HW 037
 * dejó la JVM dormida en "Launching MIDlet..." para siempre. */
static void vita_start_game(const vj_game *g, const vj_cfg *c, int gate) {
    static char suite[16];
    static char cls[128];
    /* Lo leen los parches Java: T = táctil (Canvas.hasPointerEvents),
     * G = precarga (AbstractMIDletSuiteLoader espera a appdb/_vjwait). */
    static char flags[32];
    char *argv[] = {
        vita_arg_runmidlet,
        vita_arg_D_profiles,
        vita_arg_D_config,
        vita_arg_D_encoding,
        flags,
        vita_arg_classpathext,
        vita_arg_classes,
        suite,
        cls,
    };
    snprintf(flags, sizeof(flags), "-Dvita.flags=%s%s-", c->touch ? "T" : "",
             gate ? "G" : "");
    snprintf(suite, sizeof(suite), "%d", g->suite_id);
    snprintf(cls, sizeof(cls), "%s", g->cls);
    log_line("vita_start_game() -> suite %s '%s' (%s), directo sin AMS, %s",
             suite, g->name, cls, flags);
    vita_send_start((int)(sizeof(argv) / sizeof(argv[0])), argv);
}

void vj_log(const char *fmt, ...) {
    char buf[480];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    log_line("%s", buf);
}

static void relaunch(const char *a1, const char *a2);

/* MODO INSTALADOR: el único evento del proceso es el install silencioso
 * (CommandLineInstaller "I <url>"), con -D para matchProfile (bug #22).
 * Al terminar, la JVM sale sola: hay que reiniciar la app para jugar. */
static void boot_installer_mode(const char *fname) {
    static char installer[] = "com.sun.midp.scriptutil.CommandLineInstaller";
    static char op_install[] = "I";
    static char url[288];
    char *argv[10];
    int n = 0;

    snprintf(url, sizeof(url), "file:///games/%s", fname);
    if ((int)strlen(url) >= (int)sizeof(url) - 1) {
        log_line("FATAL nombre demasiado largo: %s", fname);
        sceKernelDelayThread(5 * 1000 * 1000);
        sceKernelExitProcess(1);
        return;
    }
    argv[n++] = vita_arg_runmidlet;
    argv[n++] = vita_arg_D_profiles;
    argv[n++] = vita_arg_D_config;
    argv[n++] = vita_arg_D_encoding;
    argv[n++] = vita_arg_classpathext;
    argv[n++] = vita_arg_classes;
    argv[n++] = vita_arg_internal;
    argv[n++] = installer;
    argv[n++] = op_install;
    argv[n++] = url; /* 10/10, el maximo del evento */
    log_line("MODO INSTALADOR: %s (pantalla nativa, se relanza al "
             "terminar; detalle en jvm-%03d.log)", fname,
             vitaj2me_session_id);
    {
        /* Sesión 46: la UI del instalador de PSPKVM (y sus avisos) no se
         * presenta nunca; se ve "Instalando…" con el estilo de la app. */
        char jar[320];
        snprintf(jar, sizeof(jar), VITA_HOME "/games/%s", fname);
        launcher_install_start(jar);
    }
    vita_send_start(n, argv);
    log_line("JavaTask() >> (instalador, unico evento del proceso)");
    JavaTask();
    /* un proceso = un START (bug #20): relanzar instala el siguiente
     * pendiente o, si no queda ninguno, abre la biblioteca */
    log_line("<< instalador termino");
    relaunch(NULL, NULL);
}

/* ======================================================================
 * Biblioteca + precarga
 *
 * Un proceso = un runMidlet (bug #20), y la VM tarda ~20 s en tener MIDP
 * listo (sin ROM ni JIT: clases de classes.zip interpretadas). Así que
 * la VM arranca YA con el juego más probable (el último jugado) mientras
 * la biblioteca sigue en pantalla, y se detiene justo antes de arrancar
 * el MIDlet (parche en AbstractMIDletSuiteLoader, -Dvita.flags=G) hasta
 * que se borra appdb/_vjwait. Si se elige otro juego o se cambia algo
 * que la VM ya fijó (resolución, táctil), la app se relanza con -play.
 * ====================================================================== */

#define GATE_FILE VITA_HOME "/appdb/_vjwait"

static vj_game pre_game;
static vj_cfg pre_cfg;
static volatile int vm_dead = 0, game_started = 0;
static vj_game cur_game;  /* el que corre: lo usa el menú del juego */

void vita_jvmlog_flush(void);
extern void (*vita_lcd_menu_hook)(void);
extern void (*vita_exit_hook)(void);

static void relaunch(const char *a1, const char *a2) {
    char *args[3] = { (char *)a1, (char *)a2, NULL };
    log_line("relanzando app0:eboot.bin %s %s", a1 ? a1 : "",
             a2 ? a2 : "");
    vita_jvmlog_flush();
    sceAppMgrLoadExec("app0:eboot.bin", a1 ? args : NULL, NULL);
    sceKernelExitProcess(0);
}

static void relaunch_play(int suite_id) {
    static char id[16];
    snprintf(id, sizeof(id), "%d", suite_id);
    relaunch("-play", id);
}

/* L+R+X en juego (hilo de teclas): de vuelta a la biblioteca. Las
 * partidas de los juegos ya están en RMS (se escriben al guardar). */
static void exit_hook(void) {
    log_line("L+R+X: salir del juego a la biblioteca");
    relaunch(NULL, NULL);
}

/* L+R+START en juego (hilo de la VM, dentro del flush de lcd.c). */
static void game_menu_hook(void) {
    if (launcher_game_menu(&cur_game)) {
        log_line("menú: salir del juego a la biblioteca");
        relaunch(NULL, NULL);
    }
}

/* ----------------------------------------------------------------------
 * Bug real #30 (crash HW 045 y 050: GC completo -> ROM::oops_do con
 * klass 0 en el primer objeto de _rom_data_block).
 * CLDC se compiló con ROMIZING=false -> UseROM es la constante 0 y
 * ROM::initialize() NO copia _rom_data_block_src a _rom_data_block
 * (`if (UseROM) { if (_rom_is_relaunchable) memcpy... }`). Pero libmidp
 * enlaza una imagen ROM relanzable (_rom_data_block_size = 0x9db4) y el
 * GC recorre ese bloque sin mirar UseROM: los GC jóvenes usan
 * _rom_data_block_scanned_size y no llegan a los objetos, el GC completo
 * (cambio de stage en SoD, arranque de Asphalt Nitro) recorre el bloque
 * entero, todavía a ceros (BSS), y desreferencia klass = 0.
 * Fix: hacer aquí, antes de JavaTask, la copia que la VM se salta.
 * ---------------------------------------------------------------------- */
extern int _rom_data_block[];
extern const int _rom_data_block_src[];
extern const int _rom_data_block_size;

static void rom_data_init(void) {
    memcpy(_rom_data_block, _rom_data_block_src, (size_t)_rom_data_block_size);
    log_line("ROM: _rom_data_block inicializado (%d B); la VM no lo hace "
             "con UseROM=0 (bug #30)", _rom_data_block_size);
}

/* Tras JavaTask: el juego terminó -> volver a la biblioteca. */
static void back_to_library(void) {
    log_line("<< JavaTask() retornó: el juego terminó");
    relaunch(NULL, NULL);
}

/* fallback: si no está, el primero instalado. */
static int find_game(int suite_id, int fallback, vj_game *out) {
    static vj_game list[64];
    int i, n = launcher_list(list, 64);
    for (i = 0; i < n; i++) {
        if (list[i].suite_id == suite_id) {
            *out = list[i];
            return 1;
        }
    }
    if (n > 0 && fallback) {
        *out = list[0];
        return 1;
    }
    return 0;
}

/* Sin precarga (relanzado con -play): directo a la pantalla de carga. */
static void play_direct(int suite_id) {
    static vj_game g;
    static vj_cfg c;
    if (!find_game(suite_id, 0, &g) || !g.cls[0]) {
        log_line("-play %d: juego no encontrado -> biblioteca", suite_id);
        relaunch(NULL, NULL);
        return;
    }
    cfg_load(&g, &c);
    cfg_apply(&c, 0);
    cfg_set_last_suite(g.suite_id);
    cur_game = g;
    vita_lcd_menu_hook = game_menu_hook;
    vita_exit_hook = exit_hook;
    log_line("MODO JUEGO DIRECTO (-play, sin precarga)...");
    vita_start_game(&g, &c, 0);
    launcher_loading_start(&g);
    log_line("JavaTask() >> (toma el hilo principal)");
    JavaTask();
    back_to_library();
}

static int library_thread(SceSize args, void *argp) {
    static vj_game g;
    static vj_cfg c;
    int choice, w0, h0, w1, h1;
    (void)args; (void)argp;
    choice = launcher_run(&g, pre_game.suite_id);
    if (choice == LAUNCHER_EXIT) {
        log_line("biblioteca: salir. sceKernelExitProcess(0)");
        sceKernelExitProcess(0);
    }

    cfg_set_last_suite(g.suite_id);
    cfg_load(&g, &c);
    cfg_screen(&pre_cfg, &w0, &h0);
    cfg_screen(&c, &w1, &h1);
    if (g.suite_id != pre_game.suite_id || w0 != w1 || h0 != h1 ||
        c.touch != pre_cfg.touch || vm_dead) {
        log_line("precarga descartada (%s) -> -play %d",
                 g.suite_id != pre_game.suite_id ? "otro juego"
                 : vm_dead ? "la VM terminó" : "cambió resolución/táctil",
                 g.suite_id);
        relaunch_play(g.suite_id);
    }
    /* Mismo juego: lo demás (escala, filtro, teclado, botones) se aplica
     * en caliente, y se abre la compuerta. */
    cfg_apply(&c, 1);
    cur_game = g;
    vita_lcd_menu_hook = game_menu_hook;
    vita_exit_hook = exit_hook;
    launcher_loading_start(&g);
    game_started = 1;
    suspend_key_input = 0;
    sceIoRemove(GATE_FILE);
    log_line("precarga: '%s' elegido, compuerta abierta", g.name);
    return 0;
}

static void library_with_preload(void) {
    SceUID th;
    int fd;
    if (!find_game(cfg_last_suite(), 1, &pre_game) || !pre_game.cls[0]) {
        /* nada instalado: biblioteca vacía (solo explica y sale) */
        static vj_game g;
        if (launcher_run(&g, -1) == LAUNCHER_PLAY) relaunch_play(g.suite_id);
        sceKernelExitProcess(0);
        return;
    }
    cfg_load(&pre_game, &pre_cfg);
    cfg_apply(&pre_cfg, 0);
    fd = sceIoOpen(GATE_FILE, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd >= 0) sceIoClose(fd);
    launcher_preload_hold();
    suspend_key_input = 1;
    log_line("precarga: VM con '%s' (suite %d) tras la biblioteca",
             pre_game.name, pre_game.suite_id);
    vita_start_game(&pre_game, &pre_cfg, fd >= 0);

    th = sceKernelCreateThread("vj_library", library_thread, 0x10000100,
                               0x10000, 0, SCE_KERNEL_CPU_MASK_USER_1, NULL);
    if (th < 0) {
        log_line("FATAL hilo de la biblioteca (0x%08x)", (unsigned)th);
        sceIoRemove(GATE_FILE);
        sceKernelExitProcess(1);
    }
    sceKernelStartThread(th, 0, NULL);
    log_line("JavaTask() >> (toma el hilo principal, precarga)");
    JavaTask();
    vm_dead = 1;
    log_line("<< JavaTask() retornó (juego %s)",
             game_started ? "terminado" : "aún sin elegir");
    sceKernelWaitThreadEnd(th, NULL, NULL);
    if (game_started) back_to_library();
    /* la VM murió antes de elegir; library_thread ya relanzó con -play */
    sceKernelExitProcess(0);
}

int main(int argc, char *argv[]) {

    /* Este hilo corre la JVM (JavaTask lo toma): el watchdog lo muestrea. */
    vita_main_tid = sceKernelGetThreadId();

    make_dirs();
    boot_session_log();

    log_line("=== VitaJ2ME Fase 2b (sesion %d) ===", vitaj2me_session_id);

    /* Canales de log de la JVM: sin esto javacall_printf no va a ningún
     * lado (bug ciego: sin jvm.log no hay triage posible). STDOUT para
     * consola USB + FILE para logs/jvm-NNN.log. */
    /* Solo FICHERO (HW 043): CONSOLE/STDOUT hacían un printf por
     * carácter (System.out llega letra a letra) hacia un tty que en una
     * Vita retail nadie lee — syscalls puras en el hilo de la JVM. */
    javacall_logging_channel(JAVACALL_LOG_CHANNEL_FILE);
    log_line("frontend nativo (Vita, ARMv7)");
    /* La VM es un intérprete puro (sin JIT): 444 MHz se nota en todo,
     * sobre todo en los ~20 s de arranque de MIDP. */
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);
    log_line("CPU %d MHz, bus %d MHz, GPU %d MHz",
             scePowerGetArmClockFrequency(), scePowerGetBusClockFrequency(),
             scePowerGetGpuClockFrequency());
    log_line("fb objetivo: %dx%d (x2 de PSP 480x272)", FB_W, FB_H);

    // --- Input bringup (Fase 3: javacall keypress.c) ---
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    log_line("sceCtrl sampling OK (analog mode)");

    // --- Audio bringup (Fase 3: JSR135 mmapi -> SceAudioOut MAIN 48kHz) ---
    {
        int port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, 1024,
            48000, SCE_AUDIO_OUT_MODE_STEREO);
        if (port >= 0) {
            log_line("sceAudioOut open OK (port=%d, 48kHz stereo)", port);
            sceAudioOutReleasePort(port);
        } else {
            log_line("WARN sceAudioOutOpenPort -> 0x%08x", port);
        }
    }

    // --- AppUtil + locale (Fase 3 input.c usa este mismo patrón) ---
    {
        SceAppUtilInitParam init;
        SceAppUtilBootParam boot;
        int lang = -1;
        memset(&init, 0, sizeof(init));
        memset(&boot, 0, sizeof(boot));
        if (sceAppUtilInit(&init, &boot) == 0 &&
            sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &lang) == 0) {
            log_line("sceAppUtil locale OK (lang=%d)", lang);
        } else {
            log_line("WARN sceAppUtil locale -> en-US por defecto");
        }
    }

    // --- Memoria (Fase 6): mayor malloc contiguo, como hace la JVM ---
    {
        int sz = 192 * 1024 * 1024;
        void *p = NULL;
        while (sz > 0 && !(p = malloc((size_t)sz))) {
            sz -= 4 * 1024 * 1024;
        }
        if (p) {
            log_line("heap contiguo max ~%d MB (presupuesto CVM OK)", sz / (1024 * 1024));
            free(p);
        } else {
            log_line("WARN sin bloque grande contiguo");
        }
    }

    // --- Home J2ME: todo relativo a ux0:data/vitaj2me ---
    if (chdir(VITA_HOME) != 0) {
        log_line("FATAL chdir(%s)", VITA_HOME);
        sceKernelDelayThread(3 * 1000 * 1000);
        sceKernelExitProcess(1);
        return 1;
    }
    log_line("home J2ME: %s (classes.zip + lib/ + appdb/)", VITA_HOME);
    if (!check_j2me_home()) {
        log_line("");
        log_line("=================================================");
        log_line("NO ARRANCO: falta classes.zip en %s", VITA_HOME);
        log_line("Copiala por FTP desde _prebuilt/lib/classes.zip.");
        log_line("Sin ella la JVM no encuentra NINGUNA clase del AMS y");
        log_line("el bootstrap crashea mas adelante con un sintoma que");
        log_line("no tiene nada que ver (camino de clases 'fake').");
        log_line("=================================================");
        sceKernelDelayThread(8 * 1000 * 1000);
        sceKernelExitProcess(1);
        return 1;
    }

    // --- Arranque JVM: AMS Manager (runMidlet -1 MVMManager) ---
    // NOTA HW (bug real #2): los eventos van a los msgpipes de
    // javacall_events_init(), que JavaTask llama DESPUÉS. Hay que
    // inicializar aquí primero o el evento de arranque se pierde y
    // JavaTask espera eternamente (init idempotente, sin doble coste).
    if (javacall_events_init() != 0) {
        log_line("FATAL javacall_events_init");
        sceKernelExitProcess(1);
        return 1;
    }
    /* Watchdog de sesión (petición explícita: mejores logs): late
     * cada 10 s con FPS y teclas aunque la JVM esté atascada. */
    start_watchdog();
    rom_data_init();
    /* Splash inmediato (bug #21): el bootstrap tarda segundos y el
     * instalador no pinta nada — pantalla negra = "no corre". Best
     * effort: si vita2d falla, se sigue igual (la JVM lo reintentará). */
    if (javacall_lcd_init() == 0) {
        javacall_lcd_show_splash();
        log_line("splash Duke visible (lcd_init OK)");
    } else {
        log_line("WARN lcd_init temprano fallo (splash omitido)");
    }
    /* Un arranque = UN evento START (bug #20): o instalador o AMS. */
    {
        char pending[256];
        if (pending_install_pick(pending, sizeof(pending))) {
            boot_installer_mode(pending);
            return 0; /* inalcanzable (exit dentro), por claridad */
        }
    }
    /* Relanzamientos de la propia app (ver relaunch()). */
    if (argc >= 3 && !strcmp(argv[1], "-play")) {
        play_direct(atoi(argv[2]));
        return 0;
    }
    library_with_preload();
    return 0;
}
