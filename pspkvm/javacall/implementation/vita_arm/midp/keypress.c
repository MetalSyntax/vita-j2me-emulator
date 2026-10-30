/* vita_arm/midp/keypress.c — Fase 3: port PSP -> Vita.
 *
 * Origen: psp_mips/midp/keypress.c (sceCtrl PSP + hilos PSP).
 * Cambios:
 *  - <pspthreadman.h>/<pspctrl.h> -> <psp2/ctrl.h> + <psp2/kernel/threadmgr.h>
 *  - SceCtrlData.Buttons/.Lx/.Ly -> .buttons/.lx/.ly (minúsculas en Vita)
 *  - sceCtrlSetSamplingCycle (solo PSP) eliminado;
 *    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG) se conserva
 *  - sceCtrlReadBufferPositive(&pad, 3) -> sceCtrlPeekBufferPositive(0, &pad, 1)
 *    (en Vita el puerto se pasa explícito; 0 = mando integrado)
 *  - Máscaras PSP_CTRL_* -> SCE_CTRL_* (mismo valor de bit, ver keymap.c)
 *  - sceKernelCreateThread: 6 args PSP -> 7 args Vita
 *    (prio 0x11 -> 0x40, stack 0xFA0 -> 0x1000, sin PSP_THREAD_ATTR_USER)
 *  - sceKernelWaitThreadEndCB -> sceKernelWaitThreadEnd
 *  - Rangos del stick: Vita lx/ly 0..255 centro 128, igual que PSP;
 *    se conservan los umbrales originales.
 */

#include <javacall_keypress.h>
#include <javacall_keymap.h>
#include <javacall_devemu.h>

#include <stdio.h> /* snprintf para la traza VITAJ2ME-KEY (Bug #17) */

#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>

extern int suspend_key_input;
static int key_thread_id = -1;
static int done = 0;

/* Transiciones mapeadas vistas (watchdog del frontend). */
volatile unsigned long vita_key_count = 0;

#define VITA_CTRL_ANALOG_LEFT 	0x10000000
#define VITA_CTRL_ANALOG_RIGHT 	0x20000000
#define VITA_CTRL_ANALOG_UP 	0x40000000
#define VITA_CTRL_ANALOG_DOWN 	0x80000000

// Modifications adding RAW_KEY_STATUS send /AJM
// See also higher JVM modifications filtering these from clients that
// won't recognize them (see CLayer, CWindow).


#define MULTITASK_KEY (SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER | SCE_CTRL_TRIANGLE)
#define EXIT_KEY (SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER | SCE_CTRL_CROSS)
#define DEBUG_TRACE_KEY (SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER | SCE_CTRL_START)
#define NETWORK_KEY (SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER | SCE_CTRL_SQUARE)
#define SHIFT_KEY1 SCE_CTRL_LTRIGGER
#define SHIFT_KEY2 SCE_CTRL_RTRIGGER

#define REPEAT_THRESHOLD 20
#define REPEAT_THRESHOLD1 5

// Valores crudos del analógico. En PSP vivían en ext/pspkvm
// (rawstate.c); en Vita se definen aquí (nadie más los usa en v1).
unsigned char ext_pspkvm_rawstate_lx = 128;
unsigned char ext_pspkvm_rawstate_ly = 128;

static int KeyThread(SceSize args, void *argp)
{
	SceCtrlData pad;
	unsigned int vitaKey = 0, lastVitaKey = 0, lastRawVitaKey=0;
	int lx, ly;
	static int hold = 0, raw_hold = 0;
	static int repeat_threshold = REPEAT_THRESHOLD, raw_repeat_threshold = REPEAT_THRESHOLD;
	static javacall_key lastPressedJavakey = 0;

	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	while(!done){

		//pspDebugScreenSetXY(0, 2);

    sceCtrlPeekBufferPositive(0, &pad, 1);

	/* Cadencia de muestreo (Bug real #17): en PSP este mismo hilo giraba
	 * sin delay porque sceCtrlReadBufferPositive() lee del buffer que el
	 * sistema refresca cada sceCtrlSetSamplingCycle(25) -> el loop efectivo
	 * iba a ~40 Hz y los REPEAT_THRESHOLD (20/5) daban ~500 ms de retardo
	 * inicial y ~125 ms de repeticion. En Vita no existe SetSamplingCycle
	 * y Peek lee el HW en vivo: sin pausa el loop gira a MHz, 'hold' llega
	 * a 20 en microsegundos y cada pulsacion inunda la JVM de KEYREPEATED
	 * (cruceta erratica; en un menu la cascada de repeticiones cae en
	 * cascada hasta activar Salir -> cierre limpio como el de la sesion 21).
	 * 25 ms = ~40 Hz, paridad con PSP. Va ANTES del chequeo de 'suspend'
	 * para no quemar CPU en busy-loop cuando la entrada esta suspendida. */
	sceKernelDelayThread(25 * 1000);

		if (suspend_key_input) {
			continue;
		}

    // Emit the raw key event--if it's changed since the last sample.
    if ((lastRawVitaKey != pad.buttons) ||
				((raw_hold == raw_repeat_threshold)&&(pad.buttons != 0))) {
			javanotify_key_event(pad.buttons, JAVACALL_RAW_KEY_STATUS);
			raw_hold = 0;
			lastRawVitaKey = pad.buttons;
			// Set threshold lower if this is a repeat press
			if (raw_hold == raw_repeat_threshold) {
				raw_repeat_threshold = REPEAT_THRESHOLD1; }
			else {
				raw_repeat_threshold = REPEAT_THRESHOLD; } }
		else {
			if (pad.buttons != 0) {
				raw_hold++; } }

		vitaKey = 0;
		lx = pad.lx;
		ext_pspkvm_rawstate_lx = (unsigned char)lx;
		ly = pad.ly;
		ext_pspkvm_rawstate_ly = (unsigned char)ly;
		if (lx < 5) {
			if (ly < 55) {
				vitaKey = VITA_CTRL_ANALOG_LEFT | VITA_CTRL_ANALOG_UP;
			} else if (ly > 200) {
				vitaKey = VITA_CTRL_ANALOG_LEFT | VITA_CTRL_ANALOG_DOWN;
			} else if (lx < 5) {
				vitaKey = VITA_CTRL_ANALOG_LEFT;
			}
		} else if (lx > 251) {
			if (ly < 55) {
				vitaKey = VITA_CTRL_ANALOG_RIGHT | VITA_CTRL_ANALOG_UP;
			} else if (ly > 200) {
				vitaKey = VITA_CTRL_ANALOG_RIGHT | VITA_CTRL_ANALOG_DOWN;
			} else if (lx > 251) {
				vitaKey = VITA_CTRL_ANALOG_RIGHT;
			}
		} else {
			if (ly < 5) {
				vitaKey = VITA_CTRL_ANALOG_UP;
			} else if (ly > 251) {
				vitaKey = VITA_CTRL_ANALOG_DOWN;
			} else {
				//Staying in center, nothing happen
			}
		}


    vitaKey |= pad.buttons;

		if (vitaKey != lastVitaKey || hold == repeat_threshold){
		    int i;
		    int id;
		    int keys;
		    int shift, lastShift; // non-zero if "shift" key is pressed
		    int rotate;
		    javacall_keymap* keymap;

                  if ((vitaKey == lastVitaKey) && lastPressedJavakey != 0) {
                  	//repeat
                  	javanotify_key_event(lastPressedJavakey, JAVACALL_KEYREPEATED);
                  	hold = 0;
                  	repeat_threshold = REPEAT_THRESHOLD1;
                  	continue;
                  } else {
                     lastPressedJavakey = 0;
		       hold = 0;
		       repeat_threshold = REPEAT_THRESHOLD;
                  }

                  if (lastVitaKey == DEBUG_TRACE_KEY) {
                  	if (vitaKey == 0) {
		    		lastVitaKey = 0;
		    	}
		    	continue;
                  }

		    if (lastVitaKey == EXIT_KEY) {
		    	if (vitaKey == 0) {
		    		lastVitaKey = 0;
		    	}
		    	continue;
		    }

		    if (lastVitaKey == MULTITASK_KEY) {
		    	if (vitaKey == 0) {
		    		lastVitaKey = 0;
		    	}
		    	continue;
		    }

		    if (lastVitaKey == NETWORK_KEY) {
		    	if (vitaKey == 0) {
		    		lastVitaKey = 0;
		    	}
		    	continue;
		    }

		    if (vitaKey == NETWORK_KEY) {
		    	javanotify_network_connect();
		    	lastRawVitaKey = pad.buttons;
		    	lastVitaKey = vitaKey;
		    	continue;
		    }

		    if (vitaKey == DEBUG_TRACE_KEY) {
		    	javanotify_key_event(JAVACALL_KEY_DEBUG_TRACE, JAVACALL_KEYPRESSED);
		    	//display_log(1);
		    	lastRawVitaKey = pad.buttons;
		    	lastVitaKey = vitaKey;
		    	continue;
		    }

		    if (vitaKey == EXIT_KEY) {
		    	javanotify_shutdown_current();
		    	lastRawVitaKey = pad.buttons;
		    	lastVitaKey = vitaKey;
		    	continue;
		    }

		    if (vitaKey == MULTITASK_KEY) {
		    	javanotify_switch_to_ams();
		    	lastRawVitaKey = pad.buttons;
		    	lastVitaKey = vitaKey;
		    	continue;
		    }

		    keymap = javacall_keymap_current();
		    keys = javacall_keymap_size();
		    shift = vitaKey & (SHIFT_KEY1 | SHIFT_KEY2);
		    lastShift = lastVitaKey & (SHIFT_KEY1 | SHIFT_KEY2);
		    rotate = javacall_devemu_get_rotation(javacall_devemu_get_current_device());

		    for (i = 0 ; i < keys; i++) {
		    	//printf("keymap[i].nativekey:%08x, javakey:%d\n", keymap[i].nativeKey, keymap[i].javaKey);
		    	if (keymap[i].javaKey == JAVACALL_KEY_INVALID) {
		    		continue;
		    	}

		    	int javaKey = keymap[i].javaKey;
		    	if (rotate == 90) {
		    		switch (javaKey) {
		    			case JAVACALL_KEY_UP:
		    				javaKey = JAVACALL_KEY_LEFT;
		    				break;
		    			case JAVACALL_KEY_DOWN:
		    				javaKey = JAVACALL_KEY_RIGHT;
		    				break;
		    			case JAVACALL_KEY_LEFT:
		    				javaKey = JAVACALL_KEY_DOWN;
		    				break;
		    			case JAVACALL_KEY_RIGHT:
		    				javaKey = JAVACALL_KEY_UP;
		    				break;
		    			case JAVACALL_KEY_2:
		    				javaKey = JAVACALL_KEY_4;
		    				break;
		    			case JAVACALL_KEY_8:
		    				javaKey = JAVACALL_KEY_6;
		    				break;
		    			case JAVACALL_KEY_4:
		    				javaKey = JAVACALL_KEY_8;
		    				break;
		    			case JAVACALL_KEY_6:
		    				javaKey = JAVACALL_KEY_2;
		    		}
		    	}
		    	int pressed = ((keymap[i].nativeKey & vitaKey) && (keymap[i].shift?shift:!shift));
		    	int lastPressed = ((keymap[i].nativeKey& lastVitaKey) && (keymap[i].shift?lastShift:!lastShift));
		    	if (pressed!= lastPressed) {
			    	if (pressed) {
			    		javanotify_key_event(javaKey, JAVACALL_KEYPRESSED);
			    		vita_key_count++;
			    		lastPressedJavakey = javaKey;
					{
						/* Traza Bug #17 (solo transiciones, no repeats):
						 * una pulsacion debe loguear un unico par
						 * press/release. Si el delay de muestreo se pierde,
						 * aqui se veria la inundacion de repeats. */
						char keydbg[64];
						snprintf(keydbg, sizeof(keydbg),
							"VITAJ2ME-KEY: press %d (native 0x%08x)\n",
							javaKey, vitaKey);
						javacall_print(keydbg);
					}
				} else {
				    	javanotify_key_event(javaKey, JAVACALL_KEYRELEASED);
				    	vita_key_count++;
				    	lastPressedJavakey = 0;
					{
						char keydbg[64];
						snprintf(keydbg, sizeof(keydbg),
							"VITAJ2ME-KEY: release %d (native 0x%08x)\n",
							javaKey, vitaKey);
						javacall_print(keydbg);
					}
				}
			}
		    }

		} else {
		    hold++;
		}

		lastVitaKey = vitaKey;
	}
	return 0;
}

javacall_result javacall_key_init() {
    int thid;

    javacall_print("javacall_key_init\n");

    if (key_thread_id >= 0) {
        return JAVACALL_OK;
    }

    done = 0;

    thid = sceKernelCreateThread("key_thread", KeyThread,
				     0x40, 0x1000, 0, 0, 0);
    if (thid >= 0) {
    	 sceKernelStartThread(thid, 0, 0);
    	 /* Fingerprint del fix #17: si el jvm.log trae esta línea, la
    	  * libjavacall.a instalada SÍ muestrea a 25 ms y loguea
    	  * VITAJ2ME-KEY por transición. Sin ella (VPK viejo), la
    	  * cruceta inunda de repeats y ningún menú se puede usar. */
    	 javacall_print("keythread started (muestreo 25ms + traza VITAJ2ME-KEY)\n");
    	 key_thread_id = thid;
        return JAVACALL_OK;
    } else {
        return JAVACALL_FAIL;
    }
}

javacall_result javacall_key_finalize() {
    done = 1;
    if (key_thread_id >= 0) {
        sceKernelWaitThreadEnd(key_thread_id, NULL, NULL);
    }

    key_thread_id = -1;
    return JAVACALL_OK;
}
