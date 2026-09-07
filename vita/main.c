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
// L+R+Triángulo cambia a AMS (ver keypress.c/keymap.c).
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "javacall_logging.h"

/* Entrypoints de libmidp.a (jams_port/javacall + javacall_common). */
void JavaTask(void);
void javanotify_start(void);
void javanotify_start_suite(char *suiteId);
javacall_result javacall_events_init(void);
void javacall_logging_channel(int channel);

/* Requerido por keypress.c (hilo de teclas): 1 = input suspendido.
 * En v1 siempre 0 (sin diálogos nativos que lo necesiten). */
int suspend_key_input = 0;

/* Sesión de logs (la lee logging.c para el jvm-*.log). Se fija en main. */
int vitaj2me_session_id = 0;

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

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    make_dirs();
    boot_session_log();

    log_line("=== VitaJ2ME Fase 2b (sesion %d) ===", vitaj2me_session_id);

    /* Canales de log de la JVM: sin esto javacall_printf no va a ningún
     * lado (bug ciego: sin jvm.log no hay triage posible). STDOUT para
     * consola USB + FILE para logs/jvm-NNN.log. */
    javacall_logging_channel(JAVACALL_LOG_CHANNEL_CONSOLE |
                             JAVACALL_LOG_CHANNEL_STDOUT |
                             JAVACALL_LOG_CHANNEL_FILE);
    log_line("frontend nativo (Vita, ARMv7)");
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
    {
        SceIoStat stat_cz;
        if (sceIoGetstat("classes.zip", &stat_cz) >= 0) {
            log_line("classes.zip OK (%lld bytes)", (long long)stat_cz.st_size);
        } else {
            log_line("ERROR FATAL: classes.zip NO EXISTE en %s!", VITA_HOME);
        }
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
    log_line("javanotify_start() -> AMS Manager...");
    javanotify_start();
    log_line("JavaTask() >> (toma el hilo principal)");
    JavaTask();
    log_line("<< JavaTask() retornó (shutdown)");

    log_line("bringup OK. sceKernelExitProcess(0)");
    sceKernelExitProcess(0);
    return 0;
}
