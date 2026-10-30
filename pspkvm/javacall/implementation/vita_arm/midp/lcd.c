/* vita_arm/midp/lcd.c — Fase 4: port PSP -> Vita (vita2d).
 *
 * Origen: psp_mips/midp/lcd.c (GU textured-quad + swizzle + VRAM directa).
 * Arquitectura conservada: J2ME dibuja en scbuff RGB565 (hasta 864x864,
 * vscr_w/h dinámicos); el flush lo presenta en pantalla.
 * Cambios:
 *  - pspgu/pspge/pspdisplay + swizzle_fast + advancedBlit + vram 0x04000000
 *    -> vita2d: textura 864x864 U5U6U5_BGR (cero conversiones: el 565 va
 *    directo a GPU) + draw_texture_part_scale con los mismos 3 modos PSP
 *    (centrado 1:1, aspect-fit, stretch) + rotación 90.
 *  - Resolución: Vita 960x544 = 2x exacto de 480x272. Un juego clásico
 *    ocupa toda la pantalla con escalado entero 2x (nítido).
 *  - Filtro: propiedad "vita_filter" = "linear" (bilineal, como el
 *    GU_LINEAR de PSP) o cualquier otro valor = "nearest" (defecto:
 *    píxel nítido en 2x). Equivale al GU_TEXFILTER de PSP.
 *  - show_splash: mismo DukeTango (imageRom.c, datos portables) + flush.
 *  - El VKB (teclado virtual) es Java-side: dibuja en scbuff vía Graphics,
 *    así que funciona sin código extra una vez el flush existe.
 */

#include "javacall_lcd.h"
#include "javacall_properties.h"
#include "javacall_devemu.h"
#include <vita2d.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pantalla Vita: doble exacto de la PSP. */
#define VITA_SCR_W 960
#define VITA_SCR_H 544

/* Pantalla virtual máxima: 864 cubre los juegos táctiles de 480x800 y
 * 480x854 (y los Nokia 640x360); offscreen y textura van a ese tamaño. */
#define VSCR_MAX 864

extern const unsigned short DukeTango[];

static int vscr_w = 480;
static int vscr_h = 272;

static int resized = 0;

static unsigned short int __attribute__((aligned(16))) _offscreen[VSCR_MAX*VSCR_MAX];
static javacall_pixel* scbuff = (javacall_pixel*)_offscreen;

static int _enable_lcd_flush = 1;
static int fit_scr = -1;

/* Textura espejo del offscreen: mismo formato 565, subida directa. */
static vita2d_texture *j2me_tex = NULL;
static int tex_linear = 0; /* 0 nearest (defecto), 1 bilinear */
static int vita_vblank_wait = -1; /* espejo de lo seteado (sin getter) */

/* Contador de frames presentados (watchdog del frontend: mide FPS reales
 * y distingue "colgado" de "lento"). Volátil: lo escribe el hilo LCD/JVM
 * y lo lee el hilo HB sin lock (estadística, carreras benignas). */
volatile unsigned long vita_flush_count = 0;

/* Pantalla de carga nativa del frontend (vita/launcher.c, sesión 41).
 * Mientras esté puesto, cada flush pregunta al hook si ya hay contenido:
 * si el framebuffer de Java es todavía uniforme (negro de arranque) el
 * frame NO se presenta y la animación de carga sigue en pantalla; al
 * primer frame con contenido el hook detiene su hilo de dibujo (vuelve
 * con él terminado) y desde ahí manda la JVM. hook(1) también se usa
 * para pararla antes de cualquier reinicio de vita2d (resize). */
int (*vita_lcd_loading_hook)(int has_content) = 0;

/* Ajustes de pantalla por juego (los fija vita/settings.c antes de
 * JavaTask). scale: 0 = política PSP de siempre, 1 ajustar con
 * proporción, 2 escalado entero, 3 estirar, 4 tamaño original.
 * area: zona de 960x544 donde va el juego (el resto es del teclado
 * virtual). filter: -1 = propiedad vita_filter, 0 nítido, 1 suave. */
int vita_lcd_scale = 0;
int vita_lcd_area[4] = {0, 0, VITA_SCR_W, VITA_SCR_H};
int vita_lcd_filter = -1;
/* Rectángulo real del último frame (el táctil lo invierte). */
volatile float vita_lcd_dest[4] = {0, 0, VITA_SCR_W, VITA_SCR_H};
/* Dibuja encima del juego (teclado virtual) dentro del mismo frame. */
void (*vita_lcd_overlay_hook)(void) = 0;
/* Menú del juego (L+R+START, keypress.c): corre AQUÍ, en el hilo de la
 * VM, así el juego queda en pausa mientras está abierto. */
void (*vita_lcd_menu_hook)(void) = 0;
extern volatile int vita_menu_request;

/* Antes de JavaTask: el MIDlet ve esta pantalla desde el primer
 * getWidth(). Sin javanotify_rotation (aún no hay VM). */
void vita_lcd_set_virtual_size(int w, int h) {
    if (w < 16 || h < 16) return;
    vscr_w = w > VSCR_MAX ? VSCR_MAX : w;
    vscr_h = h > VSCR_MAX ? VSCR_MAX : h;
}

/* ¿El frame tiene algo distinto de un color liso? Muestreo 16x16. */
static int vita_frame_has_content(void) {
    const unsigned short *px = (const unsigned short *)scbuff;
    unsigned short first = px[0];
    int x, y;
    for (y = 0; y < 16; y++) {
        int yy = (vscr_h - 1) * y / 15;
        for (x = 0; x < 16; x++) {
            int xx = (vscr_w - 1) * x / 15;
            if (px[yy * vscr_w + xx] != first) return 1;
        }
    }
    return 0;
}

static int vita_loading_should_skip(int force_stop) {
    int (*hook)(int) = vita_lcd_loading_hook;
    if (!hook) return 0;
    if (!force_stop && hook(vita_frame_has_content())) {
        return 1;  /* sigue la pantalla de carga: no presentar */
    }
    if (force_stop) hook(1);
    vita_lcd_loading_hook = 0;
    return 0;
}

static void vita_apply_filter(void) {
    if (!j2me_tex) return;
    if (tex_linear) {
        vita2d_texture_set_filters(j2me_tex,
            SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR);
    } else {
        vita2d_texture_set_filters(j2me_tex,
            SCE_GXM_TEXTURE_FILTER_POINT, SCE_GXM_TEXTURE_FILTER_POINT);
    }
}

/* Sube filas [ystart, yend) de un buffer con pitch dado a la textura.
 * El stride de vita2d puede venir en píxeles o bytes según versión:
 * si es menor que una fila (1024B) se interpreta como píxeles. */
static void vita_upload_rows_ex(const javacall_pixel *src, int pitch,
                                int width, int ystart, int yend) {
    unsigned char *dst = (unsigned char *)vita2d_texture_get_datap(j2me_tex);
    unsigned int stride = vita2d_texture_get_stride(j2me_tex);
    unsigned int row_bytes = (stride < VSCR_MAX * 2u) ? stride * 2u : stride;
    int y;
    if (ystart < 0) ystart = 0;
    if (yend > VSCR_MAX) yend = VSCR_MAX;
    if (width > VSCR_MAX) width = VSCR_MAX;
    for (y = ystart; y < yend; y++) {
        memcpy(dst + (unsigned int)y * row_bytes,
               src + (unsigned int)y * (unsigned int)pitch,
               (size_t)width * sizeof(javacall_pixel));
    }
    /* NOTA coherencia CPU->GPU: vitasdk no expone DcacheWriteback en
     * modo usuario; se sigue la convención vita2d (su propia caché de
     * fuentes y los emuladores escriben la textura por CPU cada frame
     * sin mantenimiento manual). Si en HW real aparecieran tiles
     * obsoletos, el plan B es doble textura alterna. */
}

static void vita_upload_rows(int ystart, int yend) {
    /* NOTA: Java dibuja con pitch = vscr_w (ver path VRAM original),
     * no VSCR_MAX aunque el offscreen sea cuadrado. */
    if (yend > vscr_h) yend = vscr_h;
    vita_upload_rows_ex(scbuff, vscr_w, vscr_w, ystart, yend);
}

/* Buffer para rotación 90° por CPU (el modo rotado es raro; predecible
 * y sin depender de la semántica del hotspot de vita2d). */
static unsigned short int __attribute__((aligned(16))) _rotbuf[VSCR_MAX*VSCR_MAX];

/* Orientación por juego (vita/gamecfg.c). rotate: 0..3 = 0/90/180/270
 * grados en sentido horario; mirror: bit0 espejo horizontal, bit1
 * vertical (sobre la imagen ya girada). gamecfg.c invierte la misma
 * transformación para el táctil y la cruceta. */
int vita_lcd_rotate = 0;
int vita_lcd_mirror = 0;

/* scbuff (W x H) -> _rotbuf (dw x dh, pitch VSCR_MAX) con la orientación
 * pedida. Recorre el destino y calcula el origen de cada píxel. */
static void vita_transform(int rot, int mirror, int *out_w, int *out_h) {
    const unsigned short *src = (const unsigned short *)scbuff;
    int W = vscr_w, H = vscr_h;
    int dw = (rot & 1) ? H : W, dh = (rot & 1) ? W : H;
    int u, v;
    for (v = 0; v < dh; v++) {
        unsigned short *row = _rotbuf + (unsigned int)v * VSCR_MAX;
        int vv = (mirror & 2) ? dh - 1 - v : v;
        for (u = 0; u < dw; u++) {
            int uu = (mirror & 1) ? dw - 1 - u : u, x, y;
            switch (rot) {
            case 1: x = vv; y = H - 1 - uu; break;
            case 2: x = W - 1 - uu; y = H - 1 - vv; break;
            case 3: x = W - 1 - vv; y = uu; break;
            default: x = uu; y = vv; break;
            }
            row[u] = src[y * W + x];
        }
    }
    *out_w = dw;
    *out_h = dh;
}

/* Límite de FPS (0 = sin límite). Con el acelerador de esperas de
 * vm_patches.c los juegos irían todo lo rápido que dé la CPU: 30 tope. */
int vita_lcd_fps_cap = 0;

/* Calcula el rectángulo destino en 960x544 con la misma política PSP:
 * centrado (1:1, o 2x entero si cabe) si cabe y !fit_scr,
 * si no aspect-fit, si no stretch. */
static void vita_dest_rect(int scr_w, int scr_h,
                           float *dx, float *dy, float *dw, float *dh) {
    if (vita_lcd_scale > 0) {
        float ax = vita_lcd_area[0], ay = vita_lcd_area[1];
        float aw = vita_lcd_area[2], ah = vita_lcd_area[3];
        float sx = aw / scr_w, sy = ah / scr_h;
        float s = sx < sy ? sx : sy;
        if (vita_lcd_scale == 2) {
            s = (float)(int)s;
            if (s < 1.0f) s = sx < sy ? sx : sy; /* no cabe ni a 1x */
        } else if (vita_lcd_scale == 4) {
            s = 1.0f;
        }
        if (vita_lcd_scale == 3) {
            *dw = aw; *dh = ah;
        } else {
            *dw = scr_w * s; *dh = scr_h * s;
        }
        *dx = ax + (aw - *dw) / 2.0f;
        *dy = ay + (ah - *dh) / 2.0f;
        return;
    }
    {
        if (scr_w <= VITA_SCR_W && scr_h <= VITA_SCR_H && !fit_scr) {
            /* 480x272 -> x2 entero exacto a pantalla completa. */
            float s = 1.0f;
            if (scr_w * 2 <= VITA_SCR_W && scr_h * 2 <= VITA_SCR_H) {
                s = 2.0f;
            }
            *dw = scr_w * s; *dh = scr_h * s;
            *dx = (VITA_SCR_W - *dw) / 2.0f;
            *dy = (VITA_SCR_H - *dh) / 2.0f;
        } else if (scr_h / (float)VITA_SCR_H >= scr_w / (float)VITA_SCR_W) {
            *dw = scr_w * (float)VITA_SCR_H / (float)scr_h;
            *dh = (float)VITA_SCR_H;
            *dx = (VITA_SCR_W - *dw) / 2.0f;
            *dy = 0.0f;
        } else {
            *dw = (float)VITA_SCR_W;
            *dh = scr_h * (float)VITA_SCR_W / (float)scr_w;
            *dx = 0.0f;
            *dy = (VITA_SCR_H - *dh) / 2.0f;
        }
    }
}

/**
 * The function javacall_lcd_init is called by during Java VM startup, allowing the
 * platform to perform device specific initializations. The function is required to
 * provide the supported screen capabilities:
 * - Display Width
 * - Display Height
 * - Color encoding: Either 32bit ARGB format, 15 bit 565 encoding or 24 bit RGB encoding
 *
 * \par
 *
 * Once this API call is invoked, the VM will receive display focus.\n
 * <b>Note:</b> Consider the case where the platform tries to assume control over the
 * display while the VM is running by pausing the Java platform. In this case, the
 * platform is required to save the VRAM screen buffer: Whenever the Java
 * platform is resumed, the stored screen buffers must be restored to original
 * state.
 *
 * @param screenWidth width of screen
 * @param screenHeight width of screen
 * @param colorEncoding color encoding, one of the following:
 *              -# JAVACALL_LCD_COLOR_RGB565
 *              -# JAVACALL_LCD_COLOR_ARGB
 *              -# JAVACALL_LCD_COLOR_RGB888
 *              -# JAVACALL_LCD_COLOR_OTHER
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_init(void) {
       /* Prueba de qué VPK corre en consola (fingerprint de lib). */
       javacall_printf("VITAJ2ME-TRACE: lcd_init %dx%d vblank_wait=%d\n",
                       vscr_w, vscr_h, vita_vblank_wait);
       if (!j2me_tex) {
           vita2d_init();
           vita2d_set_clear_color(0xFF000000);
           /* Bug real #24 (1 FPS en juego): SIN espera de vblank.
            * Cada flush_partial hace swap_buffers; con wait(1) cada
            * repintado parcial bloquea hasta 16.7 ms y un frame con
            * decenas de parciales (barra de "cargando", sprites) tarda
            * ~1 s. En PSP pspFrameEnd() hace SwapBuffers SIN WaitVblank
            * (línea comentada en psp_mips/midp/lcd.c) — paridad. El
            * coste GPU por parcial (clear + quad 960x544) es trivial;
            * el bloqueo era lo que mataba. Posible tearing ocasional,
            * igual que en PSP. */
           vita_vblank_wait = 0;
           vita2d_set_vblank_wait(vita_vblank_wait);
           /* Bug real #26 (colores R/B invertidos en AMS y juegos):
            * libmidp.a se compila con -DBIG_ENDIAN_FRAMEBUFFER=1
            * (midp/build/javacall_vita/GNUmakefile, heredado de PSP,
            * cuyo GU_PSM_5650 es BGR): gxj empaqueta el ROJO en los bits
            * BAJOS. jsr184 (m3g_pixel.h) y DukeTango siguen la misma
            * convención. La textura tiene que ser BGR565, no R5G6B5:
            * con _RGB el skin azul de 2006 salía naranja (captura
            * 2026-09-26) y todos los juegos con R<->B cambiados. */
           j2me_tex = vita2d_create_empty_texture_format(
               VSCR_MAX, VSCR_MAX, SCE_GXM_TEXTURE_FORMAT_U5U6U5_BGR);
           if (!j2me_tex) {
               return JAVACALL_FAIL;
           }
       }

       /* Propiedades en CADA init (no solo al crear): el frontend puede
        * inicializar temprano para el splash (ver main.c) cuando aún no
        * hay config cargada; al reentrar la JVM con config, los valores
        * reales deben imponerse igual. */
       {
           char* result = NULL;
           int linear = 0;
           if (javacall_get_property("vita_filter", JAVACALL_INTERNAL_PROPERTY,
                                     &result) == JAVACALL_OK) {
               if (result != NULL &&
                   (result[0] == 'l' || result[0] == 'L' ||
                    result[0] == 'b' || result[0] == 'B')) {
                   linear = 1;
               }
           }
           if (vita_lcd_filter >= 0) linear = vita_lcd_filter;
           if (linear != tex_linear) {
               tex_linear = linear;
               vita_apply_filter();
           }
       }

       {
           char* result;
           int fit = 0;
           if (javacall_get_property("fit_screen", JAVACALL_INTERNAL_PROPERTY, &result) == JAVACALL_OK) {
               if (result != NULL && (result[0] == 'y' || result[0] == 'Y')) fit = 1;
           }
           fit_scr = fit;
       }

       return JAVACALL_OK;
}


/**
 * The function javacall_lcd_finalize is called by during Java VM shutdown,
 * allowing the  * platform to perform device specific lcd-related shutdown
 * operations.
 * The VM guarantees not to call other lcd functions before calling
 * javacall_lcd_init( ) again.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_finalize(void){
    //if (scbuff) free(scbuff);
    if (j2me_tex) {
        vita2d_free_texture(j2me_tex);
        j2me_tex = NULL;
        vita2d_fini();
    }
    return JAVACALL_OK;
}

/**
 * Get screen raster pointer
 *
 * @return pointer to video ram mapped memory region of size
 *         ( LCDSGetScreenWidth() * LCDSGetScreenHeight() )
 */
javacall_pixel* javacall_lcd_get_screen(javacall_lcd_screen_type screenType,
                                        int* screenWidth,
                                        int* screenHeight,
                                        javacall_lcd_color_encoding_type* colorEncoding){
    (void)screenType;
    if (resized) {
        vita_loading_should_skip(1);  /* vita2d_fini: parar la carga antes */
        javacall_lcd_finalize();
        javacall_lcd_init();
        resized = 0;
    }
    *screenWidth   = vscr_w;
    *screenHeight  = vscr_h;
    *colorEncoding = JAVACALL_LCD_COLOR_RGB565;
    return (javacall_pixel* )scbuff;
}

/**
 * The following function is used to flush the image from the Video RAM raster to
 * the LCD display. \n
 * The function call should not be CPU time expensive, and should return
 * immediately. It should avoid memory bulk memory copying of the entire raster.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_flush(void) {
    return javacall_lcd_flush_partial(0, vscr_h);
}

/**
 * Set or unset full screen mode.
 *
 * This function should return <code>JAVACALL_FAIL</code> if full screen mode
 * is not supported.
 * Subsequent calls to <code>javacall_lcd_get_screen()</code> will return
 * a pointer to the relevant offscreen pixel buffer of the corresponding screen
 * mode as well s the corresponding screen dimensions, after the screen mode has
 * changed.
 *
 * @param useFullScreen if <code>JAVACALL_TRUE</code>, turn on full screen mode.
 *                      if <code>JAVACALL_FALSE</code>, use normal screen mode.
 *
 * @retval JAVACALL_OK   success
 * @retval JAVACALL_FAIL failure
 */
javacall_result javacall_lcd_set_full_screen_mode(javacall_bool useFullScreen) {
    (void)useFullScreen;
    return JAVACALL_OK;
}


static javacall_result javacall_lcd_flush_partial_internal(javacall_pixel* buf, int ystart, int yend){
    float dx, dy, dw, dh;
    int rot;
    int draw_w, draw_h; /* dimensiones del contenido subido */

    if (!j2me_tex || !_enable_lcd_flush) {
        return JAVACALL_OK;
    }

    if (vita_loading_should_skip(0)) {
        return JAVACALL_OK;
    }

    if (vita_menu_request) {
        vita_menu_request = 0;
        if (vita_lcd_menu_hook) vita_lcd_menu_hook();
    }

    rot = javacall_devemu_get_rotation(javacall_devemu_get_current_device());

    if (buf != scbuff) {
        /* Paridad PSP: el path GU ignoraba el buffer externo (solo lo
         * usaba para el chequeo de alineación) y volcaba scbuff. */
        vita_upload_rows(ystart, yend);
        draw_w = vscr_w; draw_h = yend;
    } else if (rot == 90 || vita_lcd_rotate || vita_lcd_mirror) {
        /* Orientación por CPU: se transforma y sube el frame entero. */
        int r = vita_lcd_rotate ? vita_lcd_rotate : 1;
        vita_transform(r, vita_lcd_mirror, &draw_w, &draw_h);
        vita_upload_rows_ex((javacall_pixel*)_rotbuf, VSCR_MAX, draw_w, 0, draw_h);
    } else {
        vita_upload_rows(ystart, yend);
        draw_w = vscr_w; draw_h = vscr_h;
    }

    vita_dest_rect(draw_w, draw_h, &dx, &dy, &dw, &dh);
    vita_lcd_dest[0] = dx; vita_lcd_dest[1] = dy;
    vita_lcd_dest[2] = dw; vita_lcd_dest[3] = dh;

    {
        static int flush_count = 0;
        if (flush_count < 3) {
            javacall_printf("VITAJ2ME-TRACE: lcd_flush #%d %dx%d -> %.0fx%.0f\n",
                            flush_count + 1, draw_w, draw_h, dw, dh);
            flush_count++;
        }
    }

    vita2d_start_drawing();
    vita2d_clear_screen();
    vita2d_draw_texture_part_scale(j2me_tex,
        dx, dy, 0.0f, 0.0f, (float)draw_w, (float)draw_h,
        dw / (float)draw_w, dh / (float)draw_h);
    if (vita_lcd_overlay_hook) vita_lcd_overlay_hook();
    vita2d_end_drawing();
    if (vita_lcd_fps_cap > 0) {
        static SceUInt64 last = 0;
        SceUInt64 now = sceKernelGetProcessTimeWide();
        SceUInt64 frame = 1000000ull / (unsigned)vita_lcd_fps_cap;
        if (last && now - last < frame) {
            sceKernelDelayThread((SceUInt)(frame - (now - last)));
        }
        last = sceKernelGetProcessTimeWide();
    }
    vita2d_swap_buffers();
    vita_flush_count++;
    return JAVACALL_OK;
}

javacall_bool javacall_lcd_direct_flush(javacall_pixel* buf, int h) {
    if ((unsigned int)(uintptr_t)buf & 0x03) {
    	 //printf("WARNING: javacall_lcd_direct_flush can't be applied for non-aligned buffer\n");
        return JAVACALL_FALSE;
    }

    if (_enable_lcd_flush) {
    	 if (javacall_lcd_flush_partial_internal(buf, 0, h) == JAVACALL_OK) {
            return JAVACALL_TRUE;
        }
    }
    return JAVACALL_FALSE;
}

/**
 * Flush the screen raster to the display.
 * This function should not be CPU intensive and should not perform bulk memory
 * copy operations.
 * The following API uses partial flushing of the VRAM, thus may reduce the
 * runtime of the expensive flush operation: It should be implemented on
 * platforms that support it
 *
 * @param ystart start vertical scan line to start from
 * @param yend last vertical scan line to refresh
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_flush_partial(int ystart, int yend){
        return javacall_lcd_flush_partial_internal(scbuff, ystart, yend);
}

javacall_bool javacall_lcd_reverse_orientation() {
    return JAVACALL_FALSE;
}

javacall_bool javacall_lcd_get_reverse_orientation() {
    return JAVACALL_FALSE;
}

int javacall_lcd_get_screen_width() {
    return vscr_w;
}

int javacall_lcd_get_screen_height() {
    return vscr_h;
}

void javacall_set_new_screen_size(int w, int h) {
    if (w == vscr_w && h == vscr_h) {
        return;
    }

    javacall_printf("javacall_set_new_screen_size: %d, %d\n",w,h);
    /* Guardia: w/h<=0 dejaba vita_dest_rect dividiendo por cero y el
     * resize recreando GXM para nada (pantalla negra 1 frame). */
    if (w <= 0 || h <= 0) {
        javacall_printf("javacall_set_new_screen_size: ignorado (<=0)\n");
        return;
    }
    if (w > VSCR_MAX) w = VSCR_MAX;
    if (h > VSCR_MAX) h = VSCR_MAX;
    vscr_w = w;
    vscr_h = h;
    resized = 1;
    javanotify_rotation();
}

void javacall_lcd_enable_flush(int enable) {
    _enable_lcd_flush = enable;
}

void javacall_lcd_show_splash() {
	unsigned short* p;
	int x, y;
	int w, h;

	memset(scbuff, 0, sizeof(_offscreen));
           p = (unsigned short*)DukeTango + 2;
           /* El splash es 480x272 fijo; si la pantalla es otra se centra
            * y recorta (antes pitch 480 hardcodeado = shear con vscr!=480). */
           w = vscr_w < 480 ? vscr_w : 480;
           h = vscr_h < 272 ? vscr_h : 272;
           for (y = 0; y < h; y++) {
        	for (x = 0; x < w; x++) {
     			scbuff[x + y * vscr_w] = p[x+y*480];
     		}
           }
          javacall_lcd_flush();
}

#ifdef __cplusplus
} //extern "C"
#endif
