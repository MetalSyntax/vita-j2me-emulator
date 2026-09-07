/* CompositingMode.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class CompositingMode extends Object3D {

    public static final int ALPHA        = 64;
    public static final int ALPHA_ADD    = 65;
    public static final int MODULATE     = 66;
    public static final int MODULATE_X2  = 67;
    public static final int REPLACE      = 68;

    private int blending = REPLACE;
    private float alphaThreshold = 0.0f;
    private boolean depthTest = true;
    private boolean depthWrite = true;
    private boolean colorWrite = true;
    private boolean alphaWrite = true;
    private float depthOffsetFactor;
    private float depthOffsetUnits;

    public CompositingMode() {
    }

    public int getBlending() {
        return blending;
    }

    public void setBlending(int mode) {
        if (mode < ALPHA || mode > REPLACE) {
            throw new IllegalArgumentException("modo de mezcla invalido");
        }
        blending = mode;
    }

    public float getAlphaThreshold() {
        return alphaThreshold;
    }

    public void setAlphaThreshold(float threshold) {
        if (threshold < 0.0f || threshold > 1.0f) {
            throw new IllegalArgumentException("threshold fuera de [0,1]");
        }
        alphaThreshold = threshold;
    }

    public boolean isDepthTestEnabled()  { return depthTest; }
    public boolean isDepthWriteEnabled() { return depthWrite; }
    public boolean isColorWriteEnabled() { return colorWrite; }
    public boolean isAlphaWriteEnabled() { return alphaWrite; }

    public void setDepthTestEnable(boolean enable)  { depthTest = enable; }
    public void setDepthWriteEnable(boolean enable) { depthWrite = enable; }
    public void setColorWriteEnable(boolean enable) { colorWrite = enable; }
    public void setAlphaWriteEnable(boolean enable) { alphaWrite = enable; }

    public float getDepthOffsetFactor() { return depthOffsetFactor; }
    public float getDepthOffsetUnits()  { return depthOffsetUnits; }

    public void setDepthOffset(float factor, float units) {
        depthOffsetFactor = factor;
        depthOffsetUnits = units;
    }

    void updateProperty(int property, float[] value) {
        if (property == AnimationTrack.ALPHA) {
            float v = value[0];
            alphaThreshold = (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
        } else {
            super.updateProperty(property, value);
        }
    }

    Object3D createClone() {
        CompositingMode c = new CompositingMode();
        c.blending = blending;
        c.alphaThreshold = alphaThreshold;
        c.depthTest = depthTest;
        c.depthWrite = depthWrite;
        c.colorWrite = colorWrite;
        c.alphaWrite = alphaWrite;
        c.depthOffsetFactor = depthOffsetFactor;
        c.depthOffsetUnits = depthOffsetUnits;
        return c;
    }
}
