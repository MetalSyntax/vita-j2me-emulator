/* vita_arm/jsr179_location/location.c — Fase 3: stub sin GPS.
 *
 * La PSP usaba un accesorio GPS por USB (pspUsbGps + sceUsb). La Vita no
 * tiene API pública de geolocalización para homebrew, así que JSR-179
 * queda deshabilitado de forma segura: provider_open devuelve FAIL y
 * Java trata la localización como no disponible (ningún juego comercial
 * J2ME la exige; tampoco la necesita Soul of Darkness).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_location.h"
#include <math.h>

javacall_result javacall_location_property_get(
        javacall_location_property property,
        javacall_utf16* /*OUT*/outPropertyValue) {
    (void)property; (void)outPropertyValue;
    return JAVACALL_FAIL;
}

javacall_result javacall_location_provider_getinfo(
        const javacall_utf16_string name,
        javacall_location_provider_info* /*OUT*/pInfo) {
    (void)name; (void)pInfo;
    return JAVACALL_FAIL;
}

javacall_result javacall_location_provider_open(
        const javacall_utf16_string name,
        /*OUT*/ javacall_handle* pProvider) {
    (void)name; (void)pProvider;
    return JAVACALL_FAIL;
}

javacall_result javacall_location_provider_close(
        javacall_handle provider) {
    (void)provider;
    return JAVACALL_FAIL;
}

javacall_result javacall_location_provider_state(
        javacall_handle provider,
        /*OUT*/ javacall_location_state* pState) {
    (void)provider; (void)pState;
    return JAVACALL_FAIL;
}

javacall_result javacall_location_update_set(javacall_handle provider, javacall_int64 timeout) {
    (void)provider; (void)timeout;
    return JAVACALL_FAIL;
}

javacall_result javacall_location_update_cancel(javacall_handle provider) {
    (void)provider;
    return JAVACALL_FAIL;
}

javacall_result javacall_location_get(javacall_handle provider,
        /*OUT*/ javacall_location_location* pLocationInfo) {
    (void)provider; (void)pLocationInfo;
    return JAVACALL_FAIL;
}

javacall_result /*OPTIONAL*/ javacall_location_get_extrainfo(
        javacall_handle provider,
        javacall_location_extrainfo_mimetype mimetype,
        int maxUnicodeStringBufferLen,
        /*OUT*/javacall_utf16_string outUnicodeStringBuffer,
        /*OUT*/javacall_utf16_string outMimeTypeBuffer) {
    (void)provider; (void)mimetype; (void)maxUnicodeStringBufferLen;
    (void)outUnicodeStringBuffer; (void)outMimeTypeBuffer;
    return JAVACALL_FAIL;
}

javacall_result /*OPTIONAL*/ javacall_location_get_addressinfo(
        javacall_handle provider,
        /*IN and OUT*/ int* pAddresInfoFieldNumber,
        /*OUT*/javacall_location_addressinfo_fieldinfo fields[]) {
    (void)provider; (void)pAddresInfoFieldNumber; (void)fields;
    return JAVACALL_FAIL;
}

javacall_result /*OPTIONAL*/ javacall_location_orientation_start(
        javacall_handle provider,
        /*OUT*/ javacall_location_orientation* pOrientationInfo,
        /*OUT*/ void **pContext) {
    (void)provider; (void)pOrientationInfo; (void)pContext;
    return JAVACALL_FAIL;
}

javacall_result /*OPTIONAL*/ javacall_location_orientation_finish(
        javacall_handle provider,
        /*OUT*/ javacall_location_orientation* pOrientationInfo,
        void *context) {
    (void)provider; (void)pOrientationInfo; (void)context;
    return JAVACALL_FAIL;
}

javacall_result /*OPTIONAL*/ javacall_location_proximity_monitoring_add(
        javacall_handle provider,
        double latitude,
        double longitude,
        float proximityRadius) {
    (void)provider; (void)latitude; (void)longitude; (void)proximityRadius;
    return JAVACALL_FAIL;
}

javacall_result /*OPTIONAL*/ javacall_location_proximity_monitoring_cancel(
        javacall_handle provider,
        double latitude,
        double longitude,
        float proximityRadius) {
    (void)provider; (void)latitude; (void)longitude; (void)proximityRadius;
    return JAVACALL_FAIL;
}

double /*OPTIONAL*/ javacall_location_atan2(double y, double x) {
    return atan2(y, x);
}

#ifdef __cplusplus
}
#endif
