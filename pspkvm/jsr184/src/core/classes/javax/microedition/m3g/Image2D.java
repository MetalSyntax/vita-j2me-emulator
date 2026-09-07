/*
 * Image2D.java — JSR-184 (M3G 1.0).
 *
 * Los texeles se guardan ya convertidos a ARGB8888 en un int[]: es lo
 * que consume el motor nativo, y la conversion se paga una vez al
 * construir la imagen en vez de una vez por fragmento.
 *
 * Semantica de formatos igual que las texturas de OpenGL ES 1.x (que es
 * lo que define JSR-184): ALPHA da RGB = 0, LUMINANCE da A = 255.
 */
package javax.microedition.m3g;

import javax.microedition.lcdui.Image;

public class Image2D extends Object3D {

    public static final int ALPHA           = 96;
    public static final int LUMINANCE       = 97;
    public static final int LUMINANCE_ALPHA = 98;
    public static final int RGB             = 99;
    public static final int RGBA            = 100;

    private final int format;
    private final int width;
    private final int height;
    private final boolean mutable;

    /** Texeles ARGB8888 (lo que lee el motor). */
    final int[] argb;

    public Image2D(int format, int width, int height) {
        checkFormat(format);
        checkSize(width, height);
        this.format = format;
        this.width = width;
        this.height = height;
        this.mutable = true;
        this.argb = new int[width * height];
    }

    public Image2D(int format, int width, int height, byte[] image) {
        checkFormat(format);
        checkSize(width, height);
        if (image == null) {
            throw new NullPointerException();
        }
        int bpp = bytesPerPixel(format);
        if (image.length < width * height * bpp) {
            throw new IllegalArgumentException("image demasiado corto");
        }
        this.format = format;
        this.width = width;
        this.height = height;
        this.mutable = false;
        this.argb = new int[width * height];
        convert(image, 0, null, 0, 0, width, height, width);
    }

    public Image2D(int format, int width, int height, byte[] image,
                   byte[] palette) {
        checkFormat(format);
        checkSize(width, height);
        if (image == null || palette == null) {
            throw new NullPointerException();
        }
        if (image.length < width * height) {
            throw new IllegalArgumentException("image demasiado corto");
        }
        this.format = format;
        this.width = width;
        this.height = height;
        this.mutable = false;
        this.argb = new int[width * height];
        convertPaletted(image, palette);
    }

    public Image2D(int format, Object image) {
        checkFormat(format);
        if (image == null) {
            throw new NullPointerException();
        }
        if (!(image instanceof Image)) {
            throw new IllegalArgumentException(
                "se esperaba javax.microedition.lcdui.Image");
        }
        Image img = (Image) image;
        this.format = format;
        this.width = img.getWidth();
        this.height = img.getHeight();
        this.mutable = false;
        checkSize(width, height);
        int[] src = new int[width * height];
        img.getRGB(src, 0, width, 0, 0, width, height);
        this.argb = new int[width * height];
        for (int i = 0; i < src.length; i++) {
            this.argb[i] = fromARGB(src[i], format);
        }
    }

    public int getFormat() {
        return format;
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public boolean isMutable() {
        return mutable;
    }

    public void set(int x, int y, int width, int height, byte[] image) {
        if (!mutable) {
            throw new IllegalStateException("Image2D inmutable");
        }
        if (image == null) {
            throw new NullPointerException();
        }
        if (x < 0 || y < 0 || width < 0 || height < 0
            || x + width > this.width || y + height > this.height) {
            throw new IllegalArgumentException("region fuera de la imagen");
        }
        int bpp = bytesPerPixel(format);
        if (image.length < width * height * bpp) {
            throw new IllegalArgumentException("image demasiado corto");
        }
        convert(image, 0, null, x, y, width, height, width);
    }

    /* ------------------------------------------------------------ */

    private static void checkFormat(int format) {
        if (format < ALPHA || format > RGBA) {
            throw new IllegalArgumentException("formato desconocido");
        }
    }

    private static void checkSize(int w, int h) {
        if (w < 1 || h < 1) {
            throw new IllegalArgumentException("dimensiones invalidas");
        }
    }

    static int bytesPerPixel(int format) {
        switch (format) {
        case ALPHA:
        case LUMINANCE:          return 1;
        case LUMINANCE_ALPHA:    return 2;
        case RGB:                return 3;
        default:                 return 4;
        }
    }

    /** Empaqueta un pixel del formato dado a ARGB8888. */
    private static int pack(int format, byte[] src, int off) {
        int a, r, g, b;
        switch (format) {
        case ALPHA:
            a = src[off] & 0xFF;
            return (a << 24);
        case LUMINANCE: {
            int l = src[off] & 0xFF;
            return 0xFF000000 | (l << 16) | (l << 8) | l;
        }
        case LUMINANCE_ALPHA: {
            int l = src[off] & 0xFF;
            a = src[off + 1] & 0xFF;
            return (a << 24) | (l << 16) | (l << 8) | l;
        }
        case RGB:
            r = src[off] & 0xFF;
            g = src[off + 1] & 0xFF;
            b = src[off + 2] & 0xFF;
            return 0xFF000000 | (r << 16) | (g << 8) | b;
        default:
            r = src[off] & 0xFF;
            g = src[off + 1] & 0xFF;
            b = src[off + 2] & 0xFF;
            a = src[off + 3] & 0xFF;
            return (a << 24) | (r << 16) | (g << 8) | b;
        }
    }

    /** Aplica la semantica del formato a un ARGB ya existente. */
    private static int fromARGB(int c, int format) {
        int a = (c >>> 24) & 0xFF;
        int r = (c >> 16) & 0xFF;
        int g = (c >> 8) & 0xFF;
        int b = c & 0xFF;
        switch (format) {
        case ALPHA:
            return (a << 24);
        case LUMINANCE: {
            int l = (r * 77 + g * 150 + b * 29) >> 8;
            return 0xFF000000 | (l << 16) | (l << 8) | l;
        }
        case LUMINANCE_ALPHA: {
            int l = (r * 77 + g * 150 + b * 29) >> 8;
            return (a << 24) | (l << 16) | (l << 8) | l;
        }
        case RGB:
            return 0xFF000000 | (r << 16) | (g << 8) | b;
        default:
            return c;
        }
    }

    private void convert(byte[] image, int srcOff, byte[] palette,
                         int dx, int dy, int w, int h, int srcStride) {
        int bpp = bytesPerPixel(format);
        for (int row = 0; row < h; row++) {
            int s = srcOff + row * srcStride * bpp;
            int d = (dy + row) * this.width + dx;
            for (int col = 0; col < w; col++) {
                argb[d + col] = pack(format, image, s + col * bpp);
            }
        }
    }

    private void convertPaletted(byte[] image, byte[] palette) {
        int bpp = bytesPerPixel(format);
        int entries = palette.length / bpp;
        int[] lut = new int[entries];
        for (int i = 0; i < entries; i++) {
            lut[i] = pack(format, palette, i * bpp);
        }
        for (int i = 0; i < width * height; i++) {
            int idx = image[i] & 0xFF;
            argb[i] = (idx < entries) ? lut[idx] : 0;
        }
    }

    Object3D createClone() {
        Image2D c = new Image2D(format, width, height);
        System.arraycopy(argb, 0, c.argb, 0, argb.length);
        return c;
    }
}
