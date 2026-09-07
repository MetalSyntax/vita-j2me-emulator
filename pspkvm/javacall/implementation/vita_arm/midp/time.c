/* vita_arm/midp/time.c — Fase 3: port PSP -> Vita.
 *
 * Origen: psp_mips/midp/time.c
 * Cambios:
 *  - La Vita en modo usuario NO tiene sceKernelSetAlarm/CancelAlarm.
 *    Los 5 slots de timer se gestionan con un hilo worker propio:
 *    duerme hasta el próximo vencimiento (sceKernelDelayThreadCB) y
 *    dispara el callback; los cíclicos se reprograman. La API
 *    javacall_time_initialize/finalize_timer NO cambia, así que
 *    socket.c y el resto del código la usan sin modificaciones.
 *  - Reloj wall-clock: en vez de sceRtc+tick PSP se usa time() de newlib
 *    (segundos UTC desde 1970) + sceKernelGetSystemTimeWide como base
 *    monotónica. Evita depender de la época del RTC de cada consola.
 *  - Zona horaria: diferencia localtime/gmtime (incluye DST del sistema).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_time.h"
#include "javacall_logging.h"
#include <psp2/kernel/threadmgr.h>

#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TIMERS 5

typedef struct _timer{
    int active;               /* 0 libre, 1 armado */
    javacall_bool cyclic;
    SceUInt uSecToWait;
    SceUInt nextFireUsec;     /* en base sceKernelGetSystemTimeWide() */
    javacall_callback_func cbFunc;
} timer;

static timer timers[MAX_TIMERS];
static SceUID timer_sema = 0;   /* protege el array */
static SceUID timer_thread = -1;
static int timer_worker_done = 0;

static SceUInt now_usec(void) {
    return (SceUInt)(sceKernelGetSystemTimeWide() & 0xFFFFFFFFu);
}

static int timer_worker(SceSize args, void *argp) {
    (void)args; (void)argp;
    while (!timer_worker_done) {
        SceUInt now = now_usec();
        SceUInt sleepUsec = 100000; /* 100ms si no hay timers */
        int i;

        sceKernelWaitSema(timer_sema, 1, NULL);
        for (i = 0; i < MAX_TIMERS; i++) {
            if (!timers[i].active) {
                continue;
            }
            if ((int)(now - timers[i].nextFireUsec) >= 0) {
                /* Vence: copiar callback y soltar el semáforo antes de
                 * llamar (el callback puede crear/cancelar timers). */
                javacall_callback_func func = timers[i].cbFunc;
                javacall_bool cyclic = timers[i].cyclic;
                SceUInt period = timers[i].uSecToWait;
                if (!cyclic) {
                    timers[i].active = 0;
                } else {
                    timers[i].nextFireUsec = now + period;
                }
                sceKernelSignalSema(timer_sema, 1);
                func((javacall_handle *)(long)i);
                sceKernelWaitSema(timer_sema, 1, NULL);
                now = now_usec();
            } else {
                SceUInt left = timers[i].nextFireUsec - now;
                if (left < sleepUsec) {
                    sleepUsec = left;
                }
            }
        }
        sceKernelSignalSema(timer_sema, 1);

        if (sleepUsec > 100000) {
            sleepUsec = 100000; /* re-evaluar cada 100ms como máximo */
        }
        sceKernelDelayThreadCB(sleepUsec);
    }
    return 0;
}

static void timer_worker_ensure(void) {
    if (timer_thread >= 0) {
        return;
    }
    memset(timers, 0, sizeof(timers));
    timer_sema = sceKernelCreateSema("JavaTimerMutex", 0, 1, 1, NULL);
    timer_worker_done = 0;
    timer_thread = sceKernelCreateThread("vitaj2me_timer", timer_worker,
                                         0x40, 0x1000, 0, 0, 0);
    if (timer_thread >= 0) {
        sceKernelStartThread(timer_thread, 0, 0);
    }
}

static int get_free_timer_slot() {
    int i;
    for (i = 0; i < MAX_TIMERS; i++) {
    	if (!timers[i].active) {
            timers[i].active = -1; /* reservado */
            return i;
    	}
    }
    return -1;
}
/**
 *
 * Create a native timer to expire in wakeupInSeconds or less seconds.
 *
 * @param wakeupInMilliSecondsFromNow time to wakeup in milli-seconds
 *                              relative to current time
 *                              if -1, then ignore the call
 * @param cyclic <tt>JAVACALL_TRUE</tt>  indicates that the timer should be
 *               repeated cuclically,
 *               <tt>JAVACALL_FALSE</tt> indicates that this is a one-shot
 *               timer that should call the callback function once
 * @param func callback function should be called in platform's context once the timer
 *			   expires
 * @param handle A pointer to the returned handle that on success will be
 *               associated with this timer.
 *
 * @return on success returns <tt>JAVACALL_OK</tt>,
 *         or <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
javacall_result javacall_time_initialize_timer(
                    int                      wakeupInMilliSecondsFromNow,
                    javacall_bool            cyclic,
                    javacall_callback_func   func,
					/*OUT*/ javacall_handle	*handle){
    if (wakeupInMilliSecondsFromNow <= 0) {
        javacall_print("javacall_time_initialize_timer invalid param!\n");
        return JAVACALL_FAIL;
    }
    timer_worker_ensure();
    sceKernelWaitSema(timer_sema, 1, NULL);
    int slot = get_free_timer_slot();
    if (slot < 0) {
    	 javacall_print("[Javacall Error]No available timer!\n");
         sceKernelSignalSema(timer_sema, 1);
         return JAVACALL_FAIL;
    }

    SceUInt uSec = (SceUInt)wakeupInMilliSecondsFromNow*1000;
    timers[slot].active = 1;
    timers[slot].cyclic = cyclic;
    timers[slot].cbFunc = func;
    timers[slot].uSecToWait = uSec;
    timers[slot].nextFireUsec = now_usec() + uSec;
    sceKernelSignalSema(timer_sema, 1);
    *handle = (javacall_handle)(long)slot;
//    printf("javacall_time_initialize_timer:%d, %d\n", cyclic, uSec);
    return JAVACALL_OK;
}

/**
 *
 * Disable a set native timer
 * @param handle The handle of the timer to be finalized
 *
 * @return on success returns <tt>JAVACALL_OK</tt>,
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
javacall_result javacall_time_finalize_timer(javacall_handle handle){
    int i = (int)(long)handle;
    if (i < 0 || i >=MAX_TIMERS) {
    	javacall_print("javacall_time_finalize_timer try to finalize invalid timer\n");
    	return JAVACALL_FAIL;
    }
//    javacall_print("javacall_time_finalize_timer:ok\n");
    sceKernelWaitSema(timer_sema, 1, NULL);
    timers[i].active = 0;
    sceKernelSignalSema(timer_sema, 1);
    return JAVACALL_OK;
}

/**
 *
 * Create a native timer to expire in wakeupInSeconds or less seconds.
 * At least one native timer can be used concurrently.
 * If a later timer exists, cancel it and create a new timer
 *
 * @param type type of alarm to set, either JAVACALL_TIMER_PUSH, JAVACALL_TIMER_EVENT
 *                              or JAVACALL_TIMER_WATCHDOG
 * @param wakeupInMilliSecondsFromNow time to wakeup in milli-seconds
 *                              relative to current time
 *                              if -1, then ignore the call
 *
 * @return <tt>JAVACALL_OK</tt> on success, <tt>JAVACALL_FAIL</tt> on failure
 */
////javacall_result	javacall_time_create_timer(javacall_timer_type type, int wakeupInMilliSecondsFromNow){
//    return JAVACALL_FAIL;
//}

/**
 * Return local timezone ID string. This string is maintained by this
 * function internally. Caller must NOT try to free it.
 *
 * This function should handle daylight saving time properly. For example,
 * for time zone America/Los_Angeles, during summer time, this function
 * should return GMT-07:00 and GMT-08:00 during winter time.
 *
 * @return Local timezone ID string pointer. The ID string should be in the
 *         format of GMT+/-??:??. For example, GMT-08:00 for PST.
 */
char* javacall_time_get_local_timezone(void){
    time_t now = time(NULL);
    /* mktime interpreta el tm como hora local en ambos casos; la diferencia
     * es el offset local (con DST ya aplicado por el sistema). */
    struct tm gm = *gmtime(&now);
    struct tm lc = *localtime(&now);
    long offSec = (long)difftime(mktime(&lc), mktime(&gm));
    long offAbs = offSec < 0 ? -offSec : offSec;
    int hours = (int)(offAbs / 3600);
    int minutes = (int)((offAbs - hours * 3600) / 60);
    static char tz[11];
    snprintf(tz, sizeof(tz), "GMT%s%02i:%02i", offSec < 0 ? "-" : "+",
             hours, minutes);
    return tz;
}


static javacall_int64 time_offset = 0;

static void set_time_offset() {
    /* Base monotónica (µs, sobrevive a cambios de hora del sistema) anclada
     * una vez al reloj UTC de newlib. */
    javacall_int64 base = (javacall_int64)sceKernelGetSystemTimeWide() / 1000LL;
    javacall_int64 wall = (javacall_int64)time(NULL) * 1000LL;
    time_offset = wall - base;
}

/**
 * returns number of milliseconds elapsed since midnight(00:00:00), January 1, 1970,
 *
 * @return milliseconds elapsed since midnight (00:00:00), January 1, 1970
 */
javacall_int64 /*OPTIONAL*/ javacall_time_get_milliseconds_since_1970(void){
    if (time_offset == 0) {
    	 set_time_offset();
    }
    javacall_int64 ret = (javacall_int64)sceKernelGetSystemTimeWide() / 1000LL + time_offset;
    //printf("javacall_time_get_milliseconds_since_1970:%d\n",ret);
    return ret;
}

/**
 * returns the number of seconds elapsed since midnight (00:00:00), January 1, 1970,
 *
 * @return seconds elapsed since midnight (00:00:00), January 1, 1970
 */
javacall_time_seconds /*OPTIONAL*/ javacall_time_get_seconds_since_1970(void){
    if (time_offset == 0) {
    	 set_time_offset();
    }
    return (javacall_time_seconds)(((javacall_int64)sceKernelGetSystemTimeWide() / 1000000LL + time_offset / 1000LL)&0xFFFFFFFF);
}

/**
 * returns the milliseconds elapsed time counter
 *
 * @return elapsed time in milliseconds
 */
javacall_time_milliseconds /*OPTIONAL*/ javacall_time_get_clock_milliseconds(void){
    return 0;
}

#ifdef __cplusplus
}
#endif
