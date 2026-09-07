/* vita_arm/midp/image.c — Fase 4: decode síncrono PNG/JPEG vía
 * libpng/libjpeg de vitasdk.
 *
 * En PSP este fichero eran stubs (JAVACALL_FAIL): el decode recaía en el
 * decodificador Java (lento, tira de heap). En Vita hay libpng16 + libjpeg
 * turbo, así que se decodifica en nativo a RGB565 + plano alfa.
 * Modelo síncrono según la doc del API: start guarda el puntero, finish
 * decodifica a decodeBuf (javacall_pixel RGB565) y alphaBuf (1B/píxel).
 */

#include "javacall_image.h"

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <png.h>
#include <jpeglib.h>

#ifdef __cplusplus
extern "C" {
#endif

static const void *img_source = NULL;
static long img_source_size = 0;

#define RGB2PIXEL565(r,g,b) \
    (javacall_pixel)(((((r) >> 3) & 0x1f) << 11) | ((((g) >> 2) & 0x3f) << 5) | (((b) >> 3) & 0x1f))

static int is_png(const void *src, long len) {
    static const unsigned char magic[8] =
        { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    return len > 8 && memcmp(src, magic, 8) == 0;
}

static int is_jpeg(const void *src, long len) {
    const unsigned char *d = (const unsigned char *)src;
    return len > 2 && d[0] == 0xFF && d[1] == 0xD8;
}

/* --- PNG --- */
static int png_get_info(const void *src, long len, long *w, long *h, int *alpha) {
    /* IHDR: offset 16 (w BE32), 20 (h BE32), 25 (color type). */
    const unsigned char *d = (const unsigned char *)src;
    if (len < 30) return -1;
    *w = (long)((d[16] << 24) | (d[17] << 16) | (d[18] << 8) | d[19]);
    *h = (long)((d[20] << 24) | (d[21] << 16) | (d[22] << 8) | d[23]);
    {
        int ctype = d[25];
        *alpha = (ctype == 4 || ctype == 6) ? 1 : 0;
    }
    return (*w > 0 && *h > 0 && *w <= 2048 && *h <= 2048) ? 0 : -1;
}

static int png_decode(const void *src, long len,
                      javacall_pixel *out, long outPixels,
                      unsigned char *alphaOut, long alphaPixels) {
    png_image img;
    unsigned char *rgba;
    long n;
    memset(&img, 0, sizeof(img));
    img.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_memory(&img, src, (size_t)len)) {
        return -1;
    }
    img.format = PNG_FORMAT_RGBA;
    n = (long)img.width * (long)img.height;
    if (n <= 0 || n > outPixels) {
        png_image_free(&img);
        return -1;
    }
    rgba = (unsigned char *)malloc((size_t)n * 4);
    if (!rgba) {
        png_image_free(&img);
        return -1;
    }
    if (!png_image_finish_read(&img, NULL, rgba, 0, NULL)) {
        free(rgba);
        png_image_free(&img);
        return -1;
    }
    {
        long i;
        for (i = 0; i < n; i++) {
            out[i] = RGB2PIXEL565(rgba[i*4], rgba[i*4+1], rgba[i*4+2]);
        }
        if (alphaOut && alphaPixels >= n) {
            for (i = 0; i < n; i++) {
                alphaOut[i] = rgba[i*4+3];
            }
        }
    }
    free(rgba);
    png_image_free(&img);
    return 0;
}

/* --- JPEG --- */
struct vita_jpeg_err {
    struct jpeg_error_mgr pub;
    jmp_buf jb;
};

static void vita_jpeg_error(j_common_ptr cinfo) {
    struct vita_jpeg_err *e = (struct vita_jpeg_err *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(e->jb, 1);
}

static int jpeg_get_info(const void *src, long len, long *w, long *h) {
    /* Escanea marcadores hasta SOF0/1/2 (largo+precisión+h(16)+w(16)). */
    const unsigned char *d = (const unsigned char *)src;
    long off = 2;
    *w = *h = 0;
    while (off + 4 < len) {
        if (d[off] != 0xFF) break;
        unsigned char m = d[off + 1];
        if (m == 0xC0 || m == 0xC1 || m == 0xC2) {
            *h = (long)((d[off+5] << 8) | d[off+6]);
            *w = (long)((d[off+7] << 8) | d[off+8]);
            break;
        }
        if (m == 0xD8 || m == 0xD9 || (m >= 0xD0 && m <= 0xD7) || m == 0x01) {
            off += 2;
        } else {
            off += 2 + ((d[off+2] << 8) | d[off+3]);
        }
    }
    return (*w > 0 && *h > 0 && *w <= 2048 && *h <= 2048) ? 0 : -1;
}

static int jpeg_decode(const void *src, long len,
                       javacall_pixel *out, long outPixels) {
    struct jpeg_decompress_struct cinfo;
    struct vita_jpeg_err jerr;
    unsigned char *row = NULL;
    long n;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = vita_jpeg_error;
    if (setjmp(jerr.jb)) {
        jpeg_destroy_decompress(&cinfo);
        free(row);
        return -1;
    }
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, (unsigned char *)src, (unsigned long)len);
    jpeg_read_header(&cinfo, TRUE);
    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);
    n = (long)cinfo.output_width * (long)cinfo.output_height;
    if (n <= 0 || n > outPixels) {
        jpeg_destroy_decompress(&cinfo);
        return -1;
    }
    row = (unsigned char *)malloc((size_t)cinfo.output_width * 3);
    if (!row) {
        jpeg_destroy_decompress(&cinfo);
        return -1;
    }
    {
        long y = 0;
        while (cinfo.output_scanline < cinfo.output_height) {
            unsigned char *r = row;
            jpeg_read_scanlines(&cinfo, &r, 1);
            {
                unsigned int x;
                for (x = 0; x < cinfo.output_width; x++) {
                    out[y * cinfo.output_width + x] =
                        RGB2PIXEL565(row[x*3], row[x*3+1], row[x*3+2]);
                }
            }
            y++;
        }
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    free(row);
    return 0;
}

/**
 * Get image information
 *
 * @param source        Pointer to image data source
 * @param sourceSize    Size of source in bytes
 * @param imageInfo     Pointer to javacall_image_info type buffer
 *
 * @retval JAVACALL_OK
 *          imageData is acceptable format and image information returned by imageInfo argument
 * @retval JAVACALL_FAIL
 *          imageData is not acceptable
 */
javacall_result javacall_image_get_info(const void* source, long sourceSize,
                                        /*OUT*/ javacall_image_info* imageInfo)
{
    long w = 0, h = 0;
    int alpha = 0;
    if (!source || sourceSize <= 0 || !imageInfo) {
        return JAVACALL_FAIL;
    }
    if (is_png(source, sourceSize)) {
        if (png_get_info(source, sourceSize, &w, &h, &alpha) != 0) {
            return JAVACALL_FAIL;
        }
    } else if (is_jpeg(source, sourceSize)) {
        if (jpeg_get_info(source, sourceSize, &w, &h) != 0) {
            return JAVACALL_FAIL;
        }
        alpha = 0;
    } else {
        return JAVACALL_FAIL;
    }
    imageInfo->width = w;
    imageInfo->height = h;
    imageInfo->hasAlpha = alpha ? JAVACALL_TRUE : JAVACALL_FALSE;
    return JAVACALL_OK;
}

/**
 * Start a decoding transaction of the source image to array of javacall_pixel type
 *
 * (Síncrono: se guarda el puntero y se devuelve OK; finish decodifica.)
 */
javacall_result javacall_image_decode_start(const void* source,
                                            long sourceSize,
                                            long width,
                                            long height,
                                            /*OUT*/ javacall_handle* handle){
    (void)width; (void)height;
    if (!source || sourceSize <= 0) {
        return JAVACALL_FAIL;
    }
    if (!is_png(source, sourceSize) && !is_jpeg(source, sourceSize)) {
        return JAVACALL_FAIL;
    }
    img_source = source;
    img_source_size = sourceSize;
    if (handle) {
        *handle = 0;
    }
    return JAVACALL_OK;
}

/**
 * Finalize the image decoding transaction
 */
javacall_result javacall_image_decode_finish(javacall_handle handle,
                                             /*OUT*/ javacall_pixel* decodeBuf, long decodeBufSize,
                                             /*OUT*/ char* alphaBuf, long alphaBufSize)
{
    long maxPixels;
    (void)handle;
    if (!img_source || !decodeBuf || decodeBufSize <= 0) {
        return JAVACALL_FAIL;
    }
    maxPixels = decodeBufSize / (long)sizeof(javacall_pixel);
    if (is_png(img_source, img_source_size)) {
        if (png_decode(img_source, img_source_size, decodeBuf, maxPixels,
                       (unsigned char *)alphaBuf,
                       alphaBuf ? alphaBufSize : 0) != 0) {
            return JAVACALL_FAIL;
        }
    } else if (is_jpeg(img_source, img_source_size)) {
        if (jpeg_decode(img_source, img_source_size, decodeBuf, maxPixels) != 0) {
            return JAVACALL_FAIL;
        }
    } else {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

#ifdef __cplusplus
}
#endif
