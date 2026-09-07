/* Texture2D.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class Texture2D extends Transformable {

    public static final int FILTER_BASE_LEVEL = 208;
    public static final int FILTER_LINEAR     = 209;
    public static final int FILTER_NEAREST    = 210;
    public static final int FUNC_ADD          = 224;
    public static final int FUNC_BLEND        = 225;
    public static final int FUNC_DECAL        = 226;
    public static final int FUNC_MODULATE     = 227;
    public static final int FUNC_REPLACE      = 228;
    public static final int WRAP_CLAMP        = 240;
    public static final int WRAP_REPEAT       = 241;

    private Image2D image;
    private int blending = FUNC_MODULATE;
    private int blendColor;             /* 0xRRGGBB */
    private int wrapS = WRAP_REPEAT;
    private int wrapT = WRAP_REPEAT;
    private int levelFilter = FILTER_BASE_LEVEL;
    private int imageFilter = FILTER_NEAREST;

    public Texture2D(Image2D image) {
        setImage(image);
    }

    public Image2D getImage() {
        return image;
    }

    public void setImage(Image2D image) {
        if (image == null) {
            throw new NullPointerException();
        }
        int w = image.getWidth();
        int h = image.getHeight();
        if (!isPowerOfTwo(w) || !isPowerOfTwo(h)) {
            throw new IllegalArgumentException(
                "las dimensiones de la textura deben ser potencias de dos");
        }
        this.image = image;
    }

    private static boolean isPowerOfTwo(int v) {
        return v > 0 && (v & (v - 1)) == 0;
    }

    public int getBlending() {
        return blending;
    }

    public void setBlending(int func) {
        if (func < FUNC_ADD || func > FUNC_REPLACE) {
            throw new IllegalArgumentException("funcion de mezcla invalida");
        }
        blending = func;
    }

    public int getBlendColor() {
        return blendColor;
    }

    public void setBlendColor(int RGB) {
        blendColor = RGB & 0x00FFFFFF;
    }

    public int getWrappingS() {
        return wrapS;
    }

    public int getWrappingT() {
        return wrapT;
    }

    public void setWrapping(int wrapS, int wrapT) {
        if ((wrapS != WRAP_CLAMP && wrapS != WRAP_REPEAT)
            || (wrapT != WRAP_CLAMP && wrapT != WRAP_REPEAT)) {
            throw new IllegalArgumentException("modo de wrapping invalido");
        }
        this.wrapS = wrapS;
        this.wrapT = wrapT;
    }

    public int getLevelFilter() {
        return levelFilter;
    }

    public int getImageFilter() {
        return imageFilter;
    }

    public void setFiltering(int levelFilter, int imageFilter) {
        if (levelFilter != FILTER_BASE_LEVEL && levelFilter != FILTER_LINEAR
            && levelFilter != FILTER_NEAREST) {
            throw new IllegalArgumentException("levelFilter invalido");
        }
        if (imageFilter != FILTER_LINEAR && imageFilter != FILTER_NEAREST) {
            throw new IllegalArgumentException("imageFilter invalido");
        }
        this.levelFilter = levelFilter;
        this.imageFilter = imageFilter;
    }

    public int getReferences(Object3D[] references) {
        int n = super.getReferences(references);
        if (image != null) {
            if (references != null) {
                references[n] = image;
            }
            n++;
        }
        return n;
    }

    void updateProperty(int property, float[] value) {
        if (property == AnimationTrack.COLOR) {
            blendColor = (clamp255(value[0]) << 16) | (clamp255(value[1]) << 8)
                       | clamp255(value[2]);
        } else {
            super.updateProperty(property, value);
        }
    }

    private static int clamp255(float v) {
        int i = (int) (v * 255.0f + 0.5f);
        return (i < 0) ? 0 : (i > 255 ? 255 : i);
    }

    Object3D createClone() {
        Texture2D c = new Texture2D(image);
        copyTransformableTo(c);
        c.blending = blending;
        c.blendColor = blendColor;
        c.wrapS = wrapS;
        c.wrapT = wrapT;
        c.levelFilter = levelFilter;
        c.imageFilter = imageFilter;
        return c;
    }
}
