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

static void log_to_file(const char *s) {
    char path[128];
    if (vitaj2me_session_id > 0) {
        snprintf(path, sizeof(path), "%s/jvm-%03d.log",
                 VITA_JVM_LOG_DIR, vitaj2me_session_id);
    } else {
        snprintf(path, sizeof(path), "%s", VITA_JVM_LOG_LEGACY);
    }
    SceUID fd = sceIoOpen(path,
        SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    vita_jvmlog_writes++;
    if (fd >= 0) {
        sceIoWrite(fd, s, (SceSize)strlen(s));
        sceIoClose(fd);
    } else {
        vita_jvmlog_fails++;
        vita_jvmlog_lasterr = (int)fd;
    }
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
