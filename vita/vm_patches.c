// vita-j2me-emulator/vita/vm_patches.c — parches a la JVM aplicados en
// tiempo de LINK, sin recompilar libcldc_vm_r.a / libmidp.a.
//
// POR QUE ACA Y NO EN EL FUENTE DE CLDC:
// recompilar CLDC o MIDP exige regenerar antes OopMaps.cpp/ROMImage.cpp
// con romgen.exe (host tool i386 de 2006, solo corre en Docker emulando
// i386 sobre esta Mac ARM64) y ese romizer crashea con un SIGSEGV propio
// sin resolver (ver Bug real #14 en port_progress.md). Mientras ese
// bloqueador siga en pie, `ld --wrap=<simbolo>` permite interponer una
// funcion nuestra delante de una de la JVM: todas las referencias a
// <simbolo> desde OTROS objetos pasan a resolver a __wrap_<simbolo>, y
// la definicion original sigue disponible como __real_<simbolo>.
//
// El fuente de CLDC TAMBIEN lleva el fix equivalente (comentado con la
// misma referencia), asi que el dia que se pueda recompilar alcanza con
// sacar el --wrap de CMakeLists.txt y este archivo entero.
//
// Los nombres son los mangled de C++ tal cual salen de
// `arm-vita-eabi-nm vita/build/vitaj2me`; phoneME prefija las clases con
// JVM via macros, de ahi JVMTaskMirror/JVMOop.

#include <stddef.h>
#include <stdio.h>

#include "javacall_logging.h"   /* javacall_print() -> jvm-NNN.log */

/* ------------------------------------------------------------------ *
 * Bug real #15: TaskMirror::clinit_list_remove() desreferencia NULL
 *               cuando la clase no esta en la clinit list.
 *
 *   ReturnOop TaskMirror::clinit_list_remove(Oop *containing_class) {
 *     ...
 *     TaskMirror::Raw found = list.obj();
 *     GUARANTEE(found.not_null(), "clinit list must not be null");
 *     do {
 *       if (cc == found().containing_class()) break;   // <-- deref sin
 *       ...                                            //     chequear
 *     } while (found.not_null());
 *
 * El `do{}while` entra SIEMPRE una vez, asi que con la lista vacia el
 * primer `found().containing_class()` lee de 0x0. Los dos GUARANTEE que
 * documentan la precondicion son no-ops en build PRODUCT.
 *
 * Lo dispara SystemDictionary::update_fake_class(): pregunta si el
 * TaskMirror de la clase "fake" es el marcador is_being_initialized y,
 * si lo es, llama a clinit_list_remove(). Pero ese marcador es un
 * singleton compartido (Universe::task_class_init_marker()), NO un
 * TaskMirror real dado de alta con clinit_list_add() -> la clase nunca
 * estuvo en la lista. Sin ROM image (ROMIZING=false en este port) todo
 * el camino de clases "fake" se ejercita muchisimo mas que en las
 * plataformas donde phoneME se probaba en 2006, de ahi que aparezca
 * recien ahora (misma familia que los bugs #8-#11).
 *
 * Fix: si la clase no esta en la lista, no hay nada que sacar -> NULL.
 * clinit_list_lookup() ya recorre la lista con `while(not_null())`, asi
 * que es seguro con lista vacia. Los tres call sites toleran NULL:
 *   - SystemDictionary::update_fake_class() ignora el valor de retorno
 *     (este es el que crasheaba).
 *   - Thread::thread_task_cleanup() solo llama sobre elementos que
 *     acaba de encontrar recorriendo la propia lista.
 *   - InstanceClass::set_initialized() hace clinit_list_lookup() justo
 *     antes, asi que si el lookup encuentra algo el remove tambien.
 * ------------------------------------------------------------------ */

#ifndef VJ_VM_ASM  /* VM asm: fix #15 compilado en TaskMirror.cpp */
/* ReturnOop == OopDesc*; los dos son static member functions (sin this),
 * asi que desde C son punteros a funcion normales. */
extern void *__real__ZN13JVMTaskMirror18clinit_list_removeEP6JVMOop(void *containing_class);
extern void *_ZN13JVMTaskMirror18clinit_list_lookupEPK6JVMOop(const void *containing_class);

/* Cuantas veces logueamos el caso raro antes de callarnos (si el camino
 * de clases fake se dispara seguido, no queremos inundar el jvm-NNN.log
 * ni frenar el arranque con I/O). */
#define VITAJ2ME_SKIP_LOG_MAX 16
static int vitaj2me_clinit_skips = 0;

void *__wrap__ZN13JVMTaskMirror18clinit_list_removeEP6JVMOop(void *containing_class) {
    if (_ZN13JVMTaskMirror18clinit_list_lookupEPK6JVMOop(containing_class) == NULL) {
        vitaj2me_clinit_skips++;
        if (vitaj2me_clinit_skips <= VITAJ2ME_SKIP_LOG_MAX) {
            char msg[192];
            snprintf(msg, sizeof(msg),
                     "VITAJ2ME-PATCH: clinit_list_remove(%p) omitido, la clase "
                     "no esta en la clinit list (Bug real #15, skip #%d)\n",
                     containing_class, vitaj2me_clinit_skips);
            javacall_print(msg);
            if (vitaj2me_clinit_skips == VITAJ2ME_SKIP_LOG_MAX) {
                javacall_print("VITAJ2ME-PATCH: (no vuelvo a loguear este "
                               "caso, sigue contando en silencio)\n");
            }
        }
        return NULL;
    }
    return __real__ZN13JVMTaskMirror18clinit_list_removeEP6JVMOop(containing_class);
}

#endif
/* Para poder leerlo desde el log de cierre / un futuro dump. */
int vitaj2me_clinit_skip_count(void) {
#ifndef VJ_VM_ASM
    return vitaj2me_clinit_skips;
#else
    return 0;
#endif
}

/* Bug real #28 (HW 040): MIDP arma el classpath de una suite como
 * <jar> + storageGetPathSeparator() + "classes.zip", y ese separador sale
 * de pcsl_file_getpathseparator() = ':' (pcsl/file/javacall). Pero CLDC
 * lo parte con OsFile_path_separator_char = ';' (OsFile_javacall.hpp), así
 * que "file:///games/SoD.jar:classes.zip" era UNA sola entrada inexistente
 * y el bootstrap moría con "class not found: java/lang/Object". El AMS
 * nunca lo notó: su suite interna no tiene jar y no lleva separador.
 * Además ':' chocaría con "file:" y "ux0:". Se alinea con CLDC. */
unsigned short __wrap_pcsl_file_getpathseparator(void) {
    return (unsigned short)';';
}

/* Acelerador (ajuste "Velocidad" por juego, sesión 48). Muchos juegos
 * J2ME regulan sus FPS con Thread.sleep(); aquí se acortan esas esperas
 * a vita_sleep_pct %. Mínimo 1 ms: con hilos verdes de CLDC dormir es lo
 * que deja correr al hilo de eventos/pintado. El tope de FPS lo pone
 * lcd.c (vita_lcd_fps_cap). nativeFunctionTable.o (libmidp) referencia
 * el nativo por símbolo desde otro objeto: --wrap basta. Con 100 % o
 * casos raros (negativo, 0) se usa el original con sus comprobaciones. */
int vita_sleep_pct = 100;

extern void __real_Java_java_lang_Thread_sleep(void *traps);
extern long long KNI_GetParameterAsLong(int index);
extern void _ZN12JVMScheduler20sleep_current_threadEx(long long millis);

void __wrap_Java_java_lang_Thread_sleep(void *traps) {
    long long ms = KNI_GetParameterAsLong(1);
    if (vita_sleep_pct >= 100 || ms <= 0) {
        __real_Java_java_lang_Thread_sleep(traps);
        return;
    }
    ms = ms * vita_sleep_pct / 100;
    _ZN12JVMScheduler20sleep_current_threadEx(ms < 1 ? 1 : ms);
}

#ifdef VJ_VM_ASM
/* VM asm: el segmento de texto terminaba justo en un borde de 64 KB
 * (0x81510000) y vita-elf-create no tenia sitio para sus datos SCE
 * ("Cannot allocate 3364 bytes ... segment 1 overlaps"). Este relleno de
 * solo lectura lo aleja del borde. Si el error vuelve tras otro cambio,
 * ajustar el tamano. */
__attribute__((used)) const char vj_sce_pad[0x2000] = { 1 };
#endif
