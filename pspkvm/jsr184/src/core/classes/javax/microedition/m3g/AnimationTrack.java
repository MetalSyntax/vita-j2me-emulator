/*
 * AnimationTrack.java — JSR-184 (M3G 1.0).
 */
package javax.microedition.m3g;

public class AnimationTrack extends Object3D {

    public static final int ALPHA           = 256;
    public static final int AMBIENT_COLOR   = 257;
    public static final int COLOR           = 258;
    public static final int CROP            = 259;
    public static final int DENSITY         = 260;
    public static final int DIFFUSE_COLOR   = 261;
    public static final int EMISSIVE_COLOR  = 262;
    public static final int FAR_DISTANCE    = 263;
    public static final int FIELD_OF_VIEW   = 264;
    public static final int INTENSITY       = 265;
    public static final int MORPH_WEIGHTS   = 266;
    public static final int NEAR_DISTANCE   = 267;
    public static final int ORIENTATION     = 268;
    public static final int PICKABILITY     = 269;
    public static final int SCALE           = 270;
    public static final int SHININESS       = 271;
    public static final int SPECULAR_COLOR  = 272;
    public static final int SPOT_ANGLE      = 273;
    public static final int SPOT_EXPONENT   = 274;
    public static final int TRANSLATION     = 275;
    public static final int VISIBILITY      = 276;

    private final KeyframeSequence sequence;
    private final int property;
    private AnimationController controller;

    public AnimationTrack(KeyframeSequence sequence, int property) {
        if (sequence == null) {
            throw new NullPointerException();
        }
        if (property < ALPHA || property > VISIBILITY) {
            throw new IllegalArgumentException("propiedad desconocida");
        }
        int comps = sequence.getComponentCount();
        int required = requiredComponents(property);
        if (required > 0 && comps != required) {
            throw new IllegalArgumentException(
                "numero de componentes incompatible con la propiedad");
        }
        this.sequence = sequence;
        this.property = property;
    }

    /** 0 = numero de componentes libre (CROP, SCALE, MORPH_WEIGHTS...). */
    static int requiredComponents(int property) {
        switch (property) {
        case ALPHA: case DENSITY: case FAR_DISTANCE: case FIELD_OF_VIEW:
        case INTENSITY: case NEAR_DISTANCE: case PICKABILITY:
        case SHININESS: case SPOT_ANGLE: case SPOT_EXPONENT:
        case VISIBILITY:
            return 1;
        case AMBIENT_COLOR: case EMISSIVE_COLOR: case SPECULAR_COLOR:
        case TRANSLATION:
            return 3;
        case ORIENTATION:
            return 4;
        default:
            return 0;   /* COLOR (3 o 4), CROP (2 o 4), SCALE (1 o 3),
                         * DIFFUSE_COLOR (3 o 4), MORPH_WEIGHTS (n) */
        }
    }

    public KeyframeSequence getKeyframeSequence() {
        return sequence;
    }

    public int getTargetProperty() {
        return property;
    }

    public AnimationController getController() {
        return controller;
    }

    public void setController(AnimationController controller) {
        this.controller = controller;
    }

    public int getReferences(Object3D[] references) {
        int n = super.getReferences(references);
        if (sequence != null) {
            if (references != null) {
                references[n] = sequence;
            }
            n++;
        }
        if (controller != null) {
            if (references != null) {
                references[n] = controller;
            }
            n++;
        }
        return n;
    }

    Object3D createClone() {
        AnimationTrack c = new AnimationTrack(sequence, property);
        c.controller = controller;
        return c;
    }
}
