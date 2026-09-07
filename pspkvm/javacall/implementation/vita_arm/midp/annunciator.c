/* vita_arm/midp/annunciator.c — Fase 2b: LEDs/vibración/backlight.
 *
 * En PSP todo eran stubs FAIL. En Vita: vibrate usa el actuador
 * (mando emparejado; la Vita integrada no tiene motor y devuelve error,
 * igual que PSP). El resto FAIL (sin LEDs de notificación en Vita).
 */

#include "javacall_annunciator.h"

#include <psp2/ctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

javacall_result javacall_annunciator_vibrate(javacall_bool enableVibrate) {
    SceCtrlActuator act;
    act.small = enableVibrate ? 255 : 0;
    act.large = enableVibrate ? 255 : 0;
    if (sceCtrlSetActuator(0, &act) < 0) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

javacall_result javacall_annunciator_flash_backlight(javacall_bool enableBrightBack) {
    (void)enableBrightBack;
    return JAVACALL_FAIL;
}

javacall_result javacall_annunciator_display_trusted_icon(javacall_bool enableTrustedIcon) {
    (void)enableTrustedIcon;
    return JAVACALL_FAIL;
}

javacall_result javacall_annunciator_display_network_icon(javacall_bool enableNetworkIndicator) {
    (void)enableNetworkIndicator;
    return JAVACALL_FAIL;
}

javacall_result javacall_annunciator_display_input_mode_icon(
                     javacall_input_mode_type mode) {
    (void)mode;
    return JAVACALL_FAIL;
}

javacall_result javacall_annunciator_display_home_icon(javacall_bool enableHomeIndicator) {
    (void)enableHomeIndicator;
    return JAVACALL_FAIL;
}

javacall_result javacall_annunciator_play_audible_tone(javacall_audible_tone_type soundType) {
    (void)soundType;
    /* Los tonos van por JSR-135 (multimedia.c), no por aquí. */
    return JAVACALL_FAIL;
}

#ifdef __cplusplus
} //extern "C"
#endif
