/*
 * m3g_pixel.h — empaquetado RGB565 con la MISMA convencion que el
 * framebuffer de MIDP (gxj_putpixel.h). El makefile de la Vita compila
 * con -DBIG_ENDIAN_FRAMEBUFFER=1 (heredado de javacall_psp), asi que el
 * orden de campos debe replicarse aqui o el 3D saldria con R y B
 * intercambiados respecto al 2D.
 */
#ifndef _M3G_PIXEL_H_
#define _M3G_PIXEL_H_

#include "m3g_engine.h"

#ifdef BIG_ENDIAN_FRAMEBUFFER
/* campo bajo = rojo */
#define M3G_PACK565(r, g, b) \
    ((m3g_u16)((((r) & 0xF8) >> 3) | (((g) & 0xFC) << 3) | (((b) & 0xF8) << 8)))
#define M3G_R8(p) (m3g_u8)((((p) << 3) & 0xF8))
#define M3G_G8(p) (m3g_u8)((((p) >> 3) & 0xFC))
#define M3G_B8(p) (m3g_u8)((((p) >> 8) & 0xF8))
#else
/* campo bajo = azul (565 clasico) */
#define M3G_PACK565(r, g, b) \
    ((m3g_u16)((((b) & 0xF8) >> 3) | (((g) & 0xFC) << 3) | (((r) & 0xF8) << 8)))
#define M3G_R8(p) (m3g_u8)((((p) >> 8) & 0xF8))
#define M3G_G8(p) (m3g_u8)((((p) >> 3) & 0xFC))
#define M3G_B8(p) (m3g_u8)((((p) << 3) & 0xF8))
#endif

#define M3G_CLAMP255(v) ((v) < 0 ? 0 : ((v) > 255 ? 255 : (v)))

#endif /* _M3G_PIXEL_H_ */
