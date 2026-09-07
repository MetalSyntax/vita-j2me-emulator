/* vita_arm/common/vmsettings.c — Fase 2b: almacén de settings mínimo.
 *
 * En PSP estos vivían en ext/pspkvm (vmsettings.c con persistencia en
 * fichero). Para v1 basta con defaults compilados:
 *  - keymap "western" (X=aceptar, coherente con mando Vita occidental).
 *  - getint devuelve el default salvo overrides conocidos.
 * Usado por keymap.c (layout) y ft_support.c (tamaños de fuente).
 */

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns true if value v exists for key k and v equals cmp */
int vmsettings_key_equals(const char* k, const char* cmp) {
    if (k == NULL || cmp == NULL) {
        return 0;
    }
    if (strcmp(k, "com.pspkvm.default_keymap") == 0) {
        return strcmp(cmp, "western") == 0 ? 1 : 0;
    }
    return 0;
}

/* Get an integer or default (if there's no such key) */
int vmsettings_getint(const char* k, int d) {
    (void)k;
    return d;
}

#ifdef __cplusplus
}
#endif
