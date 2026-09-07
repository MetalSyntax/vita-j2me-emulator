/* Light.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class Light extends Node {

    public static final int AMBIENT     = 128;
    public static final int DIRECTIONAL = 129;
    public static final int OMNI        = 130;
    public static final int SPOT        = 131;

    private int mode = DIRECTIONAL;
    private int color = 0x00FFFFFF;
    private float intensity = 1.0f;
    private float attConstant = 1.0f;
    private float attLinear = 0.0f;
    private float attQuadratic = 0.0f;
    private float spotAngle = 45.0f;
    private float spotExponent = 0.0f;

    public Light() {
    }

    public int getMode() {
        return mode;
    }

    public void setMode(int mode) {
        if (mode < AMBIENT || mode > SPOT) {
            throw new IllegalArgumentException("modo de luz invalido");
        }
        this.mode = mode;
    }

    public int getColor() {
        return color;
    }

    public void setColor(int RGB) {
        color = RGB & 0x00FFFFFF;
    }

    public float getIntensity() {
        return intensity;
    }

    public void setIntensity(float intensity) {
        this.intensity = intensity;
    }

    public float getConstantAttenuation()  { return attConstant; }
    public float getLinearAttenuation()    { return attLinear; }
    public float getQuadraticAttenuation() { return attQuadratic; }

    public void setAttenuation(float constant, float linear, float quadratic) {
        if (constant < 0.0f || linear < 0.0f || quadratic < 0.0f) {
            throw new IllegalArgumentException("atenuacion negativa");
        }
        if (constant == 0.0f && linear == 0.0f && quadratic == 0.0f) {
            throw new IllegalArgumentException("los tres factores son cero");
        }
        attConstant = constant;
        attLinear = linear;
        attQuadratic = quadratic;
    }

    public float getSpotAngle() {
        return spotAngle;
    }

    public void setSpotAngle(float angle) {
        if (angle < 0.0f || angle > 90.0f) {
            throw new IllegalArgumentException("spotAngle fuera de [0,90]");
        }
        spotAngle = angle;
    }

    public float getSpotExponent() {
        return spotExponent;
    }

    public void setSpotExponent(float exponent) {
        if (exponent < 0.0f || exponent > 128.0f) {
            throw new IllegalArgumentException("spotExponent fuera de [0,128]");
        }
        spotExponent = exponent;
    }

    void updateProperty(int property, float[] value) {
        switch (property) {
        case AnimationTrack.COLOR:
            color = (clamp255(value[0]) << 16) | (clamp255(value[1]) << 8)
                  | clamp255(value[2]);
            break;
        case AnimationTrack.INTENSITY:
            intensity = value[0];
            break;
        case AnimationTrack.SPOT_ANGLE: {
            float a = value[0];
            spotAngle = (a < 0.0f) ? 0.0f : (a > 90.0f ? 90.0f : a);
            break;
        }
        case AnimationTrack.SPOT_EXPONENT: {
            float e = value[0];
            spotExponent = (e < 0.0f) ? 0.0f : (e > 128.0f ? 128.0f : e);
            break;
        }
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
        Light c = new Light();
        copyNodeTo(c);
        c.mode = mode;
        c.color = color;
        c.intensity = intensity;
        c.attConstant = attConstant;
        c.attLinear = attLinear;
        c.attQuadratic = attQuadratic;
        c.spotAngle = spotAngle;
        c.spotExponent = spotExponent;
        return c;
    }
}
