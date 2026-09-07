/* vita_arm/midp/misc.c — Fase 3: port PSP -> Vita.
 *
 * Origen: psp_mips/midp/misc.c (scePowerSetClockFrequency).
 * La Vita en modo usuario NO puede cambiar frecuencias (haría falta un
 * plugin de kernel / overclock externo). La CPU corre a 444MHz fijos,
 * ~7x los 66-333MHz de la PSP: ignorar la petición es correcto.
 */

#include <javacall_misc.h>
#include "javacall_logging.h"

void javacall_misc_set_cpu_speed(int speed) {
    javacall_printf("javacall_misc_set_cpu_speed(%d): ignorado en Vita (reloj fijo)\n", speed);
}
