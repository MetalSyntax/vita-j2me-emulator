/* Background.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class Background extends Object3D {

    public static final int BORDER = 32;
    public static final int REPEAT = 33;

    private int color;                  /* 0xAARRGGBB */
    private Image2D image;
    private int imageModeX = BORDER;
    private int imageModeY = BORDER;
    private int cropX;
    private int cropY;
    private int cropWidth;
    private int cropHeight;
    private boolean colorClear = true;
    private boolean depthClear = true;

    public Background() {
    }

    public int getColor() {
        return color;
    }

    public void setColor(int ARGB) {
        color = ARGB;
    }

    public Image2D getImage() {
        return image;
    }

    public void setImage(Image2D image) {
        if (image != null) {
            int f = image.getFormat();
            if (f != Image2D.RGB && f != Image2D.RGBA) {
                throw new IllegalArgumentException(
                    "el fondo requiere una imagen RGB o RGBA");
            }
            if (this.image == null) {
                cropX = 0;
                cropY = 0;
                cropWidth = image.getWidth();
                cropHeight = image.getHeight();
            }
        }
        this.image = image;
    }

    public int getImageModeX() { return imageModeX; }
    public int getImageModeY() { return imageModeY; }

    public void setImageMode(int modeX, int modeY) {
        if ((modeX != BORDER && modeX != REPEAT)
            || (modeY != BORDER && modeY != REPEAT)) {
            throw new IllegalArgumentException("modo de imagen invalido");
        }
        imageModeX = modeX;
        imageModeY = modeY;
    }

    public int getCropX()      { return cropX; }
    public int getCropY()      { return cropY; }
    public int getCropWidth()  { return cropWidth; }
    public int getCropHeight() { return cropHeight; }

    public void setCrop(int cropX, int cropY, int width, int height) {
        if (width < 0 || height < 0) {
            throw new IllegalArgumentException("crop negativo");
        }
        this.cropX = cropX;
        this.cropY = cropY;
        this.cropWidth = width;
        this.cropHeight = height;
    }

    public boolean isColorClearEnabled() { return colorClear; }
    public boolean isDepthClearEnabled() { return depthClear; }

    public void setColorClearEnable(boolean enable) { colorClear = enable; }
    public void setDepthClearEnable(boolean enable) { depthClear = enable; }

    void updateProperty(int property, float[] value) {
        switch (property) {
        case AnimationTrack.COLOR:
            color = (color & 0xFF000000) | (clamp255(value[0]) << 16)
                  | (clamp255(value[1]) << 8) | clamp255(value[2]);
            break;
        case AnimationTrack.ALPHA:
            color = (color & 0x00FFFFFF) | (clamp255(value[0]) << 24);
            break;
        case AnimationTrack.CROP:
            cropX = (int) value[0];
            cropY = (int) value[1];
            if (value.length >= 4) {
                cropWidth = (int) value[2];
                cropHeight = (int) value[3];
            }
            break;
        default:
            super.updateProperty(property, value);
            break;
        }
    }

    private static int clamp255(float v) {
        int i = (int) (v * 255.0f + 0.5f);
        return (i < 0) ? 0 : (i > 255 ? 255 : i);
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

    Object3D createClone() {
        Background c = new Background();
        c.color = color;
        c.image = image;
        c.imageModeX = imageModeX;
        c.imageModeY = imageModeY;
        c.cropX = cropX;
        c.cropY = cropY;
        c.cropWidth = cropWidth;
        c.cropHeight = cropHeight;
        c.colorClear = colorClear;
        c.depthClear = depthClear;
        return c;
    }
}
