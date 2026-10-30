/* vita_arm/common/events.c — Fase 3 (rev. 2b-bug#3): port PSP -> Vita.
 *
 * Origen: psp_mips/common/events.c
 * La Vita en modo usuario NO tiene mailboxes (sceKernelCreateMbx es
 * API de kernel) y sceKernelCreateMsgPipe falló en HW real (parámetros
 * type/attr sin documentar). Se reimplementa el mismo anillo de 10
 * ranuras con Mutex+CondVar del kernel (APIs estándar de homebrew):
 *  - send guarda el puntero del evento en la ranura y señaliza.
 *    Si el anillo está lleno descarta (sin bloquear jamás).
 *  - receive: <0 espera infinita, 0 poll, >0 espera N ms (timeouts en
 *    microsegundos, igual que antes). Bucle ante wakeups espurios.
 */

#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/error.h>
#include "javacall_events.h"
#include "javacall_logging.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_MBX 10
static void *_vita_event_ring[NUM_MBX];
static SceUID _vita_ev_mutex = 0;
static SceUID _vita_ev_cond = 0;
static int _vita_events_inited = 0;

/* Para el watchdog del frontend: si el hilo de la JVM está WAITING con
 * waitId == este UID, el VM está ocioso esperando un evento nativo
 * (ningún hilo Java ejecutable) — no atascado en nativo. */
int vita_event_cond_uid(void) {
    return (int)_vita_ev_cond;
}

#define NEXT(i) (i >= (NUM_MBX - 1) ? 0 : i+1)
static int _mbxid_recv,  _mbxid_send;

/**
 * Waits for an incoming event message and copies it to user supplied
 * data buffer
 * @param waitForever indicate if the function should block forever
 * @param timeTowaitInMillisec max number of seconds to wait
 *              if waitForever is false
 * @param binaryBuffer user-supplied buffer to copy event to
 * @param binaryBufferMaxLen maximum buffer size that an event can be
 *              copied to.
 *              If an event exceeds the binaryBufferMaxLen, then the first
 *              binaryBufferMaxLen bytes of the events will be copied
 *              to user-supplied binaryBuffer, and JAVACALL_OUT_OF_MEMORY will
 *              be returned
 * @param outEventLen user-supplied pointer to variable that will hold actual
 *              event size received
 *              Platform is responsible to set this value on success to the
 *              size of the event received, or 0 on failure.
 *              If outEventLen is NULL, the event size is not returned.
 * @return <tt>JAVACALL_OK</tt> if an event successfully received,
 *         <tt>JAVACALL_FAIL</tt> or if failed or no messages are avaialable
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> If an event's size exceeds the
 *         binaryBufferMaxLen
 */
javacall_result javacall_event_receive(
                            long                     timeTowaitInMillisec,
                            /*OUT*/ unsigned char*  binaryBuffer,
                            /*IN*/  int             binaryBufferMaxLen,
                            /*OUT*/ int*            outEventLen){
    void* msg;
    int ret;
    static int busy_count = 0;

    if (busy_count > 40 && timeTowaitInMillisec == 0) {
        timeTowaitInMillisec = 30;
        busy_count = 0;
    }

    /* Bug real #25: antes se soltaba el mutex ANTES de sceKernelWaitCond.
     * WaitCond exige tener cogido el mutex asociado (lo suelta y lo
     * re-coge atómicamente, como pthread_cond_wait). Sin él: error
     * inmediato (espera infinita -> FAIL -> la JVM re-entra en bucle;
     * espera acotada -> iteraciones sin dormir = spin al 100%) y, aun si
     * esperara, un send entre el unlock y el wait se perdía (lost
     * wakeup: JVM dormida con un evento ya en cola). Ahora el chequeo y
     * la espera van bajo el mismo mutex. */
    if (timeTowaitInMillisec < 0) {
    	busy_count = 0;
        sceKernelLockMutex(_vita_ev_mutex, 1, NULL);
        while ((msg = _vita_event_ring[_mbxid_recv]) == NULL) {
            ret = sceKernelWaitCond(_vita_ev_cond, NULL);
            if (ret < 0) {
                sceKernelUnlockMutex(_vita_ev_mutex, 1);
                return JAVACALL_FAIL;
            }
        }
        /* con mutex cogido */
    } else if (timeTowaitInMillisec == 0) {
       busy_count++;
       if (sceKernelTryLockMutex(_vita_ev_mutex, 1) < 0) {
           return JAVACALL_FAIL;
       }
       msg = _vita_event_ring[_mbxid_recv];
       if (msg == NULL) {
           sceKernelUnlockMutex(_vita_ev_mutex, 1);
           return JAVACALL_FAIL;
       }
       /* con mutex cogido */
    } else {
       busy_count = 0;
       {
           /* Deadline absoluto: los wakeups espurios/por otro evento no
            * alargan ni acortan la espera total. */
           SceUInt64 deadline = sceKernelGetProcessTimeWide() +
                                (SceUInt64)timeTowaitInMillisec * 1000u;
           sceKernelLockMutex(_vita_ev_mutex, 1, NULL);
           while ((msg = _vita_event_ring[_mbxid_recv]) == NULL) {
               SceUInt64 now = sceKernelGetProcessTimeWide();
               SceUInt left;
               if (now >= deadline) {
                   sceKernelUnlockMutex(_vita_ev_mutex, 1);
                   return JAVACALL_FAIL;
               }
               left = (SceUInt)(deadline - now);
               ret = sceKernelWaitCond(_vita_ev_cond, &left);
               if (ret < 0 && ret != (int)SCE_KERNEL_ERROR_WAIT_TIMEOUT) {
                   /* Error real (no timeout): no girar en vacío. */
                   sceKernelUnlockMutex(_vita_ev_mutex, 1);
                   return JAVACALL_FAIL;
               }
           }
           /* con mutex cogido */
       }
    }

    /* Aquí: mutex cogido y msg != NULL. */
    {
        int len = *((int*)msg+1);
        _vita_event_ring[_mbxid_recv] = NULL;
        _mbxid_recv = NEXT(_mbxid_recv);
        sceKernelUnlockMutex(_vita_ev_mutex, 1);

        if (binaryBufferMaxLen < len) {
            free(msg);
            return JAVACALL_OUT_OF_MEMORY;
        } else {
            memcpy(binaryBuffer, (char*)msg+sizeof(int)+sizeof(int), len);
            *outEventLen = len;
            free(msg);
            return JAVACALL_OK;
        }
    }
    return JAVACALL_FAIL;
}
/**
 * copies a user supplied event message to a queue of messages
 *
 * @param binaryBuffer a pointer to binary event buffer to send
 *        The platform should make a private copy of this buffer as
 *        access to it is not allowed after the function call.
 * @param binaryBufferLen size of binary event buffer to send
 * @return <tt>JAVACALL_OK</tt> if an event successfully sent,
 *         <tt>JAVACALL_FAIL</tt> or negative value if failed
 */
javacall_result javacall_event_send(unsigned char* binaryBuffer,
                                    int binaryBufferLen){
    char* p = NULL;
    int id;

    p = (char*)malloc(binaryBufferLen+sizeof(int)+sizeof(int));
    if (p) {
    	 *((int*)(p+sizeof(int))) = binaryBufferLen;
    	 memcpy(p+sizeof(int)+sizeof(int), binaryBuffer, binaryBufferLen);

        sceKernelLockMutex(_vita_ev_mutex, 1, NULL);
        if (NEXT(_mbxid_send) == _mbxid_recv) {
        	javacall_printf("javacall_event_send: discard event\n");
        	sceKernelUnlockMutex(_vita_ev_mutex, 1);
        	free(p);
        	return JAVACALL_FAIL;
        }
        id = _mbxid_send;
        _vita_event_ring[id] = p;
        _mbxid_send = NEXT(_mbxid_send);
        sceKernelUnlockMutex(_vita_ev_mutex, 1);

        sceKernelSignalCond(_vita_ev_cond);
        return JAVACALL_OK;
    } else {
        return JAVACALL_FAIL;
    }
}

//PSPGlobalINFO _psp_global_info;
javacall_result javacall_events_init(void) {
    int i;
    /* Idempotente: el frontend la llama ANTES de javanotify_start
     * (si no, el primer evento se pierde) y JavaTask la vuelve a llamar. */
    if (_vita_events_inited) {
        return JAVACALL_OK;
    }
    for (i = 0; i < NUM_MBX; i++) {
        _vita_event_ring[i] = NULL;
    }
    _mbxid_send = _mbxid_recv = 0;
    _vita_ev_mutex = sceKernelCreateMutex("JavaEventQMutex", 0, 1, NULL);
    if (_vita_ev_mutex < 0) {
        return JAVACALL_FAIL;
    }
    _vita_ev_cond = sceKernelCreateCond("JavaEventQCond", 0,
                                        _vita_ev_mutex, NULL);
    if (_vita_ev_cond < 0) {
        sceKernelDeleteMutex(_vita_ev_mutex);
        _vita_ev_mutex = 0;
        return JAVACALL_FAIL;
    }
    _vita_events_inited = 1;

    return JAVACALL_OK;
}

javacall_result javacall_events_finalize(void) {
    if (_vita_ev_cond) {
        sceKernelDeleteCond(_vita_ev_cond);
        _vita_ev_cond = 0;
    }
    if (_vita_ev_mutex) {
        sceKernelDeleteMutex(_vita_ev_mutex);
        _vita_ev_mutex = 0;
    }
    _mbxid_send = _mbxid_recv = 0;
    _vita_events_inited = 0;
    return JAVACALL_OK;
}

/*************************** KEYPRESS *************************************/



/***************************** SMS *************************************/
/*                   1. INCOMING SMS CALLBACK                          */
/*                   2. SMS SENDING RESULT CALLBACK                    */
/***********************************************************************/




/*************************** SOCKET *************************************/

/*************************** NETWORK *************************************/


/*************************** PAUSE/RESUME *************************************/

/*************************** TEXTFIELD **************************************/




/*************************** MMAPI *************************************/
/*                   1. END-OF-MEDIA CALLBACK                          */
/*                   2. DURATION UPDATE CALLBACK                       */
/***********************************************************************/

/******************** MMAPI (2) : DURATION UPDATE CALLBACK *************/



#ifdef __cplusplus
}
#endif
