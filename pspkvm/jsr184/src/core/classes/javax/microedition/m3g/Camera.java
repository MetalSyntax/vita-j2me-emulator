/* Camera.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class Camera extends Node {

    public static final int GENERIC     = 48;
    public static final int PARALLEL    = 49;
    public static final int PERSPECTIVE = 50;

    private int type = GENERIC;
    private float fovy = 60.0f;
    private float aspect = 1.0f;
    private float near = 0.1f;
    private float far = 1.0f;
    private final Transform generic = new Transform();

    public Camera() {
    }

    public void setGeneric(Transform transform) {
        if (transform == null) {
            throw new NullPointerException();
        }
        generic.set(transform);
        type = GENERIC;
    }

    public void setParallel(float fovy, float aspectRatio, float near,
                            float far) {
        if (fovy <= 0.0f || aspectRatio <= 0.0f) {
            throw new IllegalArgumentException("fovy y aspectRatio deben ser > 0");
        }
        this.type = PARALLEL;
        this.fovy = fovy;
        this.aspect = aspectRatio;
        this.near = near;
        this.far = far;
        rebuild();
    }

    public void setPerspective(float fovy, float aspectRatio, float near,
                               float far) {
        if (fovy <= 0.0f || fovy >= 180.0f || aspectRatio <= 0.0f
            || near <= 0.0f || far <= 0.0f) {
            throw new IllegalArgumentException("parametros de perspectiva invalidos");
        }
        this.type = PERSPECTIVE;
        this.fovy = fovy;
        this.aspect = aspectRatio;
        this.near = near;
        this.far = far;
        rebuild();
    }

    private void rebuild() {
        float[] m = new float[16];
        MathUtil.identity(m);
        if (type == PERSPECTIVE) {
            float t = 1.0f / (float) Math.tan(fovy * 0.5 * Math.PI / 180.0);
            m[0]  = t / aspect;
            m[5]  = t;
            m[10] = (far + near) / (near - far);
            m[11] = 2.0f * far * near / (near - far);
            m[14] = -1.0f;
            m[15] = 0.0f;
        } else {
            /* PARALLEL: fovy es la altura de la vista, no un angulo. */
            float h = fovy * 0.5f;
            float w = h * aspect;
            m[0]  = 1.0f / w;
            m[5]  = 1.0f / h;
            m[10] = 2.0f / (near - far);
            m[11] = (far + near) / (near - far);
        }
        generic.set(m);
    }

    public int getProjection(Transform transform) {
        if (transform != null) {
            transform.set(generic);
        }
        return type;
    }

    public int getProjection(float[] params) {
        if (params != null) {
            if (params.length < 4) {
                throw new IllegalArgumentException("params.length < 4");
            }
            if (type == GENERIC) {
                throw new IllegalStateException(
                    "la camara es GENERIC: usar getProjection(Transform)");
            }
            params[0] = fovy;
            params[1] = aspect;
            params[2] = near;
            params[3] = far;
        }
        return type;
    }

    void updateProperty(int property, float[] value) {
        switch (property) {
        case AnimationTrack.FIELD_OF_VIEW:
            if (type != GENERIC) {
                fovy = value[0];
                rebuild();
            }
            break;
        case AnimationTrack.NEAR_DISTANCE:
            if (type != GENERIC) {
                near = value[0];
                rebuild();
            }
            break;
        case AnimationTrack.FAR_DISTANCE:
            if (type != GENERIC) {
                far = value[0];
                rebuild();
            }
            break;
        default:
            super.updateProperty(property, value);
            break;
        }
    }

    Object3D createClone() {
        Camera c = new Camera();
        copyNodeTo(c);
        c.type = type;
        c.fovy = fovy;
        c.aspect = aspect;
        c.near = near;
        c.far = far;
        c.generic.set(generic);
        return c;
    }
}
