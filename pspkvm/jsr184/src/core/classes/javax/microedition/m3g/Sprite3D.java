/*
 * Sprite3D.java — JSR-184 (M3G 1.0).
 * El dibujado (quad alineado a pantalla) lo resuelve Graphics3D.
 */
package javax.microedition.m3g;

public class Sprite3D extends Node {

    private final boolean scaled;
    private Image2D image;
    private Appearance appearance;
    private int cropX;
    private int cropY;
    private int cropWidth;
    private int cropHeight;

    public Sprite3D(boolean scaled, Image2D image, Appearance appearance) {
        if (image == null) {
            throw new NullPointerException();
        }
        this.scaled = scaled;
        this.appearance = appearance;
        setImage(image);
    }

    public boolean isScaled() {
        return scaled;
    }

    public Image2D getImage() {
        return image;
    }

    public void setImage(Image2D image) {
        if (image == null) {
            throw new NullPointerException();
        }
        this.image = image;
        this.cropX = 0;
        this.cropY = 0;
        this.cropWidth = image.getWidth();
        this.cropHeight = image.getHeight();
    }

    public Appearance getAppearance() {
        return appearance;
    }

    public void setAppearance(Appearance appearance) {
        this.appearance = appearance;
    }

    public int getCropX()      { return cropX; }
    public int getCropY()      { return cropY; }
    public int getCropWidth()  { return cropWidth; }
    public int getCropHeight() { return cropHeight; }

    public void setCrop(int cropX, int cropY, int width, int height) {
        this.cropX = cropX;
        this.cropY = cropY;
        this.cropWidth = width;
        this.cropHeight = height;
    }

    void updateProperty(int property, float[] value) {
        if (property == AnimationTrack.CROP) {
            cropX = (int) value[0];
            cropY = (int) value[1];
            if (value.length >= 4) {
                cropWidth = (int) value[2];
                cropHeight = (int) value[3];
            }
        } else {
            super.updateProperty(property, value);
        }
    }

    public int getReferences(Object3D[] references) {
        int n = super.getReferences(references);
        if (image != null) {
            if (references != null) {
                references[n] = image;
            }
            n++;
        }
        if (appearance != null) {
            if (references != null) {
                references[n] = appearance;
            }
            n++;
        }
        return n;
    }

    Object3D createClone() {
        Sprite3D c = new Sprite3D(scaled, image, appearance);
        copyNodeTo(c);
        c.cropX = cropX;
        c.cropY = cropY;
        c.cropWidth = cropWidth;
        c.cropHeight = cropHeight;
        return c;
    }
}
