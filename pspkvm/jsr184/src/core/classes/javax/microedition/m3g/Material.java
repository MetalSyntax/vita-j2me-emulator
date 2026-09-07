/* Material.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class Material extends Object3D {

    public static final int AMBIENT  = 1024;
    public static final int DIFFUSE  = 2048;
    public static final int EMISSIVE = 4096;
    public static final int SPECULAR = 8192;

    private int ambient  = 0x00333333;
    private int diffuse  = 0xFFCCCCCC;
    private int emissive = 0x00000000;
    private int specular = 0x00000000;
    private float shininess = 0.0f;
    private boolean vertexColorTracking;

    public Material() {
    }

    public int getColor(int target) {
        switch (target) {
        case AMBIENT:  return ambient;
        case DIFFUSE:  return diffuse;
        case EMISSIVE: return emissive;
        case SPECULAR: return specular;
        default:
            throw new IllegalArgumentException("target invalido");
        }
    }

    public void setColor(int target, int ARGB) {
        if ((target & ~(AMBIENT | DIFFUSE | EMISSIVE | SPECULAR)) != 0
            || target == 0) {
            throw new IllegalArgumentException("target invalido");
        }
        if ((target & AMBIENT) != 0)  { ambient = ARGB; }
        if ((target & DIFFUSE) != 0)  { diffuse = ARGB; }
        if ((target & EMISSIVE) != 0) { emissive = ARGB; }
        if ((target & SPECULAR) != 0) { specular = ARGB; }
    }

    public float getShininess() {
        return shininess;
    }

    public void setShininess(float shininess) {
        if (shininess < 0.0f || shininess > 128.0f) {
            throw new IllegalArgumentException("shininess fuera de [0,128]");
        }
        this.shininess = shininess;
    }

    public boolean isVertexColorTrackingEnabled() {
        return vertexColorTracking;
    }

    public void setVertexColorTrackingEnable(boolean enable) {
        vertexColorTracking = enable;
    }

    void updateProperty(int property, float[] value) {
        switch (property) {
        case AnimationTrack.AMBIENT_COLOR:
            ambient = packRGB(ambient, value);
            break;
        case AnimationTrack.DIFFUSE_COLOR:
            diffuse = packRGBA(diffuse, value);
            break;
        case AnimationTrack.EMISSIVE_COLOR:
            emissive = packRGB(emissive, value);
            break;
        case AnimationTrack.SPECULAR_COLOR:
            specular = packRGB(specular, value);
            break;
        case AnimationTrack.ALPHA:
            diffuse = (diffuse & 0x00FFFFFF) | (clamp255(value[0]) << 24);
            break;
        case AnimationTrack.SHININESS: {
            float s = value[0];
            shininess = (s < 0.0f) ? 0.0f : (s > 128.0f ? 128.0f : s);
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

    private static int packRGB(int old, float[] v) {
        return (old & 0xFF000000) | (clamp255(v[0]) << 16)
             | (clamp255(v[1]) << 8) | clamp255(v[2]);
    }

    private static int packRGBA(int old, float[] v) {
        int a = (v.length > 3) ? clamp255(v[3]) : ((old >>> 24) & 0xFF);
        return (a << 24) | (clamp255(v[0]) << 16)
             | (clamp255(v[1]) << 8) | clamp255(v[2]);
    }

    Object3D createClone() {
        Material c = new Material();
        c.ambient = ambient;
        c.diffuse = diffuse;
        c.emissive = emissive;
        c.specular = specular;
        c.shininess = shininess;
        c.vertexColorTracking = vertexColorTracking;
        return c;
    }
}
