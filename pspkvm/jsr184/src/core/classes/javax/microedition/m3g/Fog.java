/* Fog.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class Fog extends Object3D {

    public static final int EXPONENTIAL = 80;
    public static final int LINEAR      = 81;

    private int mode = LINEAR;
    private int color;                 /* 0xRRGGBB */
    private float density = 1.0f;
    private float near = 0.0f;
    private float far = 1.0f;

    public Fog() {
    }

    public int getMode() {
        return mode;
    }

    public void setMode(int mode) {
        if (mode != EXPONENTIAL && mode != LINEAR) {
            throw new IllegalArgumentException("modo de niebla invalido");
        }
        this.mode = mode;
    }

    public int getColor() {
        return color;
    }

    public void setColor(int RGB) {
        color = RGB & 0x00FFFFFF;
    }

    public float getDensity() {
        return density;
    }

    public void setDensity(float density) {
        if (density < 0.0f) {
            throw new IllegalArgumentException("density < 0");
        }
        this.density = density;
    }

    public float getNearDistance() {
        return near;
    }

    public float getFarDistance() {
        return far;
    }

    public void setLinear(float near, float far) {
        this.near = near;
        this.far = far;
    }

    void updateProperty(int property, float[] value) {
        switch (property) {
        case AnimationTrack.COLOR:
            color = (clamp255(value[0]) << 16) | (clamp255(value[1]) << 8)
                  | clamp255(value[2]);
            break;
        case AnimationTrack.DENSITY:
            density = (value[0] < 0.0f) ? 0.0f : value[0];
            break;
        case AnimationTrack.NEAR_DISTANCE:
            near = value[0];
            break;
        case AnimationTrack.FAR_DISTANCE:
            far = value[0];
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

    Object3D createClone() {
        Fog c = new Fog();
        c.mode = mode;
        c.color = color;
        c.density = density;
        c.near = near;
        c.far = far;
        return c;
    }
}
