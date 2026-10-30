/* vita_arm/common/logging.c — Fase 3: port PSP -> Vita.
 *
 * Origen: psp_mips/common/logging.c
 *  (pspDebugScreenPrintf + pspkvm_screen_log del frontend PSP).
 * En Vita: stdout (visible por consola/Vitacompanion) + fichero persistente
 * ux0:data/vitaj2me/jvm.log para triage sin debugger. El canal STARTUP
 * queda reservado para el overlay de Fase 4 (vita2d) y de momento va al
 * mismo fichero.
 */

#include "javacall_logging.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VITA_JVM_LOG_DIR "ux0:data/vitaj2me/logs"
#define VITA_JVM_LOG_LEGACY "ux0:data/vitaj2me/jvm.log"

/* Lo fija el frontend (main.c) en cada arranque; 0 = sin sesión
 * (fallback al path histórico para otros entry points). */
extern int vitaj2me_session_id;

static int log_channel_consle = 0;
static int log_channel_startup = 0;
static int log_channel_stdout = 0;
static int log_channel_file = 0;

/* Sesión 38: en jvm-036/037 el log deja de recibir líneas tras "ready
 * to go" (ni setDeviceToRun ni VITAJ2ME-KEY) mientras el bringup sigue
 * vivo. El HB del frontend imprime estos contadores: si fails sube, el
 * sceIoOpen falla (y lasterr dice por qué); si writes no sube, nadie
 * llama a javacall_print. */
volatile unsigned long vita_jvmlog_writes = 0;
volatile unsigned long vita_jvmlog_fails = 0;
volatile int vita_jvmlog_lasterr = 0;
/* Sesión 41: con el juego corriendo, 3461 llamadas en 100 s dejaron un
 * fichero de 6 KB (casi todas cadenas vacías) y el hilo de la JVM usó
 * ~4% de CPU: vivía bloqueado en sceIoOpen/Close sobre la memory card
 * (1 FPS). Ahora el fd se abre UNA vez y "" no toca el disco. Estos
 * contadores lo miden en el HB: llamadas vacías, bytes y µs de E/S. */
volatile unsigned long vita_jvmlog_empty = 0;
volatile unsigned long vita_jvmlog_bytes = 0;
volatile unsigned long long vita_jvmlog_io_us = 0;

static SceUID log_fd = -1;

/* HW 043: con el fd abierto una sola vez seguían ~68 sceIoWrite/s de ~1
 * byte (io 600 ms de cada segundo): System.out de CLDC llega por
 * JVMSPI_PrintRaw CARÁCTER A CARÁCTER. Buffer por línea: se escribe al
 * ver '\n' o al llenarse. Mutex porque escriben la JVM y key_thread. */
/* HW 045: con el juego corriendo, io = 1-1,5 s de cada 10 s en el hilo
 * de la VM (SoD imprime "heart by liteeeee..." en cada golpe). Ahora:
 * buffer de 8 KB que se vuelca al llenarse o, en un '\n', si pasó más
 * de 1 s desde el último volcado; y las líneas idénticas seguidas se
 * cuentan en vez de escribirse. Se pierde como mucho ~1 s de log si el
 * proceso muere (los dumps siguen teniendo el resto). */
#define LOG_BUF_SIZE 8192
#define LOG_FLUSH_US 1000000ull
static char log_buf[LOG_BUF_SIZE];
static int log_buf_len = 0;
static SceUID log_mutex = -1;
static SceUInt64 log_last_flush = 0;
/* línea en curso y la última completa, para colapsar repeticiones */
static char line_cur[256], line_prev[256];
static int line_cur_len = 0, line_rep = 0;

static int log_open(void) {
    char path[128];
    if (log_fd >= 0) return 1;
    if (vitaj2me_session_id > 0) {
        snprintf(path, sizeof(path), "%s/jvm-%03d.log",
                 VITA_JVM_LOG_DIR, vitaj2me_session_id);
    } else {
        snprintf(path, sizeof(path), "%s", VITA_JVM_LOG_LEGACY);
    }
    log_fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (log_fd < 0) {
        vita_jvmlog_fails++;
        vita_jvmlog_lasterr = (int)log_fd;
        log_fd = -1;
        return 0;
    }
    return 1;
}

/* Con el mutex cogido. */
static void log_flush_locked(void) {
    SceUInt64 t0;
    if (log_buf_len == 0 || !log_open()) {
        log_buf_len = 0;
        return;
    }
    t0 = sceKernelGetProcessTimeWide();
    /* sceIoWrite es una syscall sin buffer de usuario: lo escrito queda
     * en la tarjeta aunque el proceso muera después (dumps). */
    if (sceIoWrite(log_fd, log_buf, (SceSize)log_buf_len) < 0) {
        vita_jvmlog_fails++;
    } else {
        vita_jvmlog_bytes += (unsigned long)log_buf_len;
    }
    vita_jvmlog_io_us += sceKernelGetProcessTimeWide() - t0;
    log_buf_len = 0;
}

static void log_put_locked(const char *s, int n) {
    if (log_buf_len + n > LOG_BUF_SIZE) log_flush_locked();
    if (n > LOG_BUF_SIZE) n = LOG_BUF_SIZE;
    memcpy(log_buf + log_buf_len, s, (size_t)n);
    log_buf_len += n;
}

/* Una línea completa (con '\n') en line_cur. */
static void log_line_done_locked(void) {
    SceUInt64 now;
    if (line_cur_len == (int)strlen(line_prev) &&
        memcmp(line_cur, line_prev, (size_t)line_cur_len) == 0) {
        line_rep++;
    } else {
        if (line_rep > 0) {
            char note[64];
            int k = snprintf(note, sizeof(note),
                             "  (x%d la línea anterior)\n", line_rep);
            log_put_locked(note, k);
            line_rep = 0;
        }
        log_put_locked(line_cur, line_cur_len);
        memcpy(line_prev, line_cur, (size_t)line_cur_len);
        line_prev[line_cur_len] = 0;
    }
    line_cur_len = 0;
    now = sceKernelGetProcessTimeWide();
    if (now - log_last_flush >= LOG_FLUSH_US) {
        log_flush_locked();
        log_last_flush = now;
    }
}

static void log_to_file(const char *s) {
    size_t n = strlen(s), i;
    vita_jvmlog_writes++;
    if (n == 0) {
        vita_jvmlog_empty++;
        return;
    }
    if (log_mutex < 0) {
        log_mutex = sceKernelCreateMutex("VitaJ2meLog", 0, 0, NULL);
    }
    if (log_mutex >= 0) sceKernelLockMutex(log_mutex, 1, NULL);
    for (i = 0; i < n; i++) {
        if (line_cur_len < (int)sizeof(line_cur) - 1)
            line_cur[line_cur_len++] = s[i];
        if (s[i] == '\n' || line_cur_len == (int)sizeof(line_cur) - 1) {
            if (s[i] != '\n') line_cur[line_cur_len++] = '\n';
            log_line_done_locked();
        }
    }
    if (log_mutex >= 0) sceKernelUnlockMutex(log_mutex, 1);
}

/* Para el frontend (salida ordenada, menú, relanzar): vuelca ya. */
void vita_jvmlog_flush(void) {
    if (log_mutex >= 0) sceKernelLockMutex(log_mutex, 1, NULL);
    log_flush_locked();
    if (log_mutex >= 0) sceKernelUnlockMutex(log_mutex, 1);
}

/**
 * Prints out a string to a system specific output strream
 *
 * @param s a NULL terminated character buffer to be printed
*/
void javacall_print(const char *s) {

    /* Fichero PRIMERO: el jvm.log es la evidencia de triage y no debe
     * depender de que el stdout de newlib vuelva. */
    if (log_channel_startup || log_channel_file) log_to_file(s);
    if (!s[0]) return;  /* nada que imprimir: no tocar stdout tampoco */
    if (log_channel_consle) printf("%s", s);       /* consola USB */
    if (log_channel_stdout && !log_channel_consle) printf("%s", s);

}

void javacall_printf (const char* format, ...) {
#define log_buf_size 65536
  static char logs[log_buf_size]={0};

  va_list ap;
  va_start(ap, format);
  vsnprintf(logs, log_buf_size, format, ap);
  va_end(ap);

  javacall_print(logs);
}

/* HW 039: AppManagerUI.sizeChanged() llama JVM.setLogChannel(NONE) en
 * cuanto el AMS se pinta (PSPKVM silenciaba la consola en release) y
 * JVM_SetLogChannel -> aquí. Eso cegaba el jvm.log justo antes del
 * Launch (ni setDeviceToRun ni VITAJ2ME-KEY ni el volcado pss en
 * 036-039). Una vez que el frontend activa el FICHERO, se mantiene. */
static int log_file_latched = 0;

void javacall_logging_channel(int channel) {
  if (log_file_latched && !(channel & JAVACALL_LOG_CHANNEL_FILE)) {
    log_to_file("VITAJ2ME-LOG: setLogChannel sin FILE ignorado "
                "(el jvm.log sigue activo)\n");
    channel |= JAVACALL_LOG_CHANNEL_FILE;
  }
  if (channel & JAVACALL_LOG_CHANNEL_FILE) log_file_latched = 1;
  if (channel & JAVACALL_LOG_CHANNEL_CONSOLE) log_channel_consle = 1;
  else log_channel_consle = 0;
  if (channel & JAVACALL_LOG_CHANNEL_STARTUP) log_channel_startup = 1;
  else log_channel_startup = 0;
  if (channel & JAVACALL_LOG_CHANNEL_STDOUT) log_channel_stdout = 1;
  else log_channel_stdout = 0;
  if (channel & JAVACALL_LOG_CHANNEL_FILE) log_channel_file = 1;
  else log_channel_file = 0;
}

#ifdef __cplusplus
}
#endif
