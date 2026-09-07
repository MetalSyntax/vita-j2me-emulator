/* PolygonMode.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class PolygonMode extends Object3D {

    public static final int CULL_BACK    = 160;
    public static final int CULL_FRONT   = 161;
    public static final int CULL_NONE    = 162;
    public static final int SHADE_FLAT   = 164;
    public static final int SHADE_SMOOTH = 165;
    public static final int WINDING_CCW  = 168;
    public static final int WINDING_CW   = 169;

    private int culling = CULL_BACK;
    private int winding = WINDING_CCW;
    private int shading = SHADE_SMOOTH;
    private boolean twoSidedLighting;
    private boolean localCameraLighting;
    private boolean perspectiveCorrection;

    public PolygonMode() {
    }

    public int getCulling() {
        return culling;
    }

    public void setCulling(int mode) {
        if (mode != CULL_BACK && mode != CULL_FRONT && mode != CULL_NONE) {
            throw new IllegalArgumentException("modo de culling invalido");
        }
        culling = mode;
    }

    public int getWinding() {
        return winding;
    }

    public void setWinding(int mode) {
        if (mode != WINDING_CCW && mode != WINDING_CW) {
            throw new IllegalArgumentException("winding invalido");
        }
        winding = mode;
    }

    public int getShading() {
        return shading;
    }

    public void setShading(int mode) {
        if (mode != SHADE_FLAT && mode != SHADE_SMOOTH) {
            throw new IllegalArgumentException("shading invalido");
        }
        shading = mode;
    }

    public boolean isTwoSidedLightingEnabled() {
        return twoSidedLighting;
    }

    public void setTwoSidedLightingEnable(boolean enable) {
        twoSidedLighting = enable;
    }

    public boolean isLocalCameraLightingEnabled() {
        return localCameraLighting;
    }

    public void setLocalCameraLightingEnable(boolean enable) {
        localCameraLighting = enable;
    }

    public boolean isPerspectiveCorrectionEnabled() {
        return perspectiveCorrection;
    }

    public void setPerspectiveCorrectionEnable(boolean enable) {
        perspectiveCorrection = enable;
    }

    Object3D createClone() {
        PolygonMode c = new PolygonMode();
        c.culling = culling;
        c.winding = winding;
        c.shading = shading;
        c.twoSidedLighting = twoSidedLighting;
        c.localCameraLighting = localCameraLighting;
        c.perspectiveCorrection = perspectiveCorrection;
        return c;
    }
}
