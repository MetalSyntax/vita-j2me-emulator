/* vita_arm/midp/input.c — Fase 3: port PSP -> Vita (locale).
 *
 * Origen: psp_mips/midp/input.c (sceUtilityGetSystemParamInt).
 * Cambios:
 *  - psputility_sysparam.h -> psp2/apputil.h + psp2/system_param.h
 *  - sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE)
 *    -> sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG)
 *  - Mapa de idiomas ampliado (la Vita distingue más locales que la PSP).
 *    buffer locale[6]: caben tags "xx-YY".
 *  NOTA: el frontend (vita/main.c, Fase 2b) debe llamar a sceAppUtilInit()
 *  antes de arrancar la JVM para que la query funcione; si falla -> en-US.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_input.h"
#include <psp2/apputil.h>
#include <psp2/system_param.h>

javacall_result javacall_input_get_locale(/*OUT*/ char locale[6]) {
    int lang = SCE_SYSTEM_PARAM_LANG_ENGLISH_US;
    if (sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &lang) == 0) {
        switch (lang) {
        case SCE_SYSTEM_PARAM_LANG_CHINESE_S:
            locale[0]='z'; locale[1]='h'; locale[2]='-';
            locale[3]='C'; locale[4]='N'; locale[5]='\0';
            return JAVACALL_OK;
        case SCE_SYSTEM_PARAM_LANG_CHINESE_T:
            locale[0]='z'; locale[1]='h'; locale[2]='-';
            locale[3]='T'; locale[4]='W'; locale[5]='\0';
            return JAVACALL_OK;
        case SCE_SYSTEM_PARAM_LANG_JAPANESE:
            locale[0]='j'; locale[1]='a'; locale[2]='-';
            locale[3]='J'; locale[4]='P'; locale[5]='\0';
            return JAVACALL_OK;
        case SCE_SYSTEM_PARAM_LANG_KOREAN:
            locale[0]='k'; locale[1]='o'; locale[2]='-';
            locale[3]='K'; locale[4]='R'; locale[5]='\0';
            return JAVACALL_OK;
        case SCE_SYSTEM_PARAM_LANG_RUSSIAN:
            locale[0]='r'; locale[1]='u'; locale[2]='-';
            locale[3]='R'; locale[4]='U'; locale[5]='\0';
            return JAVACALL_OK;
        default:
            break;
        }
    }
    //set to "en-US"
    locale[0]='e';
    locale[1]='n';
    locale[2]='-';
    locale[3]='U';
    locale[4]='S';
    locale[5]='\0';
    return JAVACALL_OK;
}

javacall_textfield_status javacall_input_textfield_launch(
        /*IN-OUT*/javacall_utf16*	            textBuffer,
		/*IN-OUT*/int*				            textBufferLength,
        /*IN*/ int					            textBufferMaxLength,
        /*IN-OUT*/int*				            caretPos,
        /*IN*/ const javacall_utf16*	        title,
        /*IN*/ int					            titleLength,
        /*IN*/ const javacall_utf16*	        inputMode,
        /*IN*/ int					            inputModeLength,
        /*IN*/ javacall_textfield_constraint    constraint,
        /*IN*/ int					            modifiers,
        /*IN*/ int                              keyCode) {

    return JAVACALL_TEXTFIELD_ERROR;
}

javacall_result /* OPTIONAL */ javacall_textfield_display_phonebook() {
    return JAVACALL_FAIL;
}

javacall_result /* OPTIONAL */
javacall_textfield_initiate_voicecall(const char* phoneNumber) {
    return JAVACALL_FAIL;
}

#ifdef __cplusplus
}
#endif
