/*
 * Transformable.java — JSR-184 (M3G 1.0).
 * Transformacion compuesta = T * R * S * M (traslacion, orientacion,
 * escala y matriz generica), en ese orden, como exige la especificacion.
 */
package javax.microedition.m3g;

public abstract class Transformable extends Object3D {

    private final float[] translation = new float[3];
    private float orientationAngle;
    private final float[] orientationAxis = { 0.0f, 0.0f, 1.0f };
    private final float[] scale = { 1.0f, 1.0f, 1.0f };
    private final Transform generic = new Transform();

    Transformable() {
    }

    public void getTranslation(float[] xyz) {
        require3(xyz);
        xyz[0] = translation[0];
        xyz[1] = translation[1];
        xyz[2] = translation[2];
    }

    public void setTranslation(float tx, float ty, float tz) {
        translation[0] = tx;
        translation[1] = ty;
        translation[2] = tz;
    }

    public void translate(float tx, float ty, float tz) {
        translation[0] += tx;
        translation[1] += ty;
        translation[2] += tz;
    }

    public void getScale(float[] xyz) {
        require3(xyz);
        xyz[0] = scale[0];
        xyz[1] = scale[1];
        xyz[2] = scale[2];
    }

    public void setScale(float sx, float sy, float sz) {
        scale[0] = sx;
        scale[1] = sy;
        scale[2] = sz;
    }

    public void scale(float sx, float sy, float sz) {
        scale[0] *= sx;
        scale[1] *= sy;
        scale[2] *= sz;
    }

    public void getOrientation(float[] angleAxis) {
        if (angleAxis == null) {
            throw new NullPointerException();
        }
        if (angleAxis.length < 4) {
            throw new IllegalArgumentException("angleAxis.length < 4");
        }
        angleAxis[0] = orientationAngle;
        angleAxis[1] = orientationAxis[0];
        angleAxis[2] = orientationAxis[1];
        angleAxis[3] = orientationAxis[2];
    }

    public void setOrientation(float angle, float ax, float ay, float az) {
        if (ax == 0.0f && ay == 0.0f && az == 0.0f && angle != 0.0f) {
            throw new IllegalArgumentException("eje nulo con angulo no nulo");
        }
        orientationAngle = angle;
        orientationAxis[0] = ax;
        orientationAxis[1] = ay;
        orientationAxis[2] = az;
    }

    public void postRotate(float angle, float ax, float ay, float az) {
        Transform r = new Transform();
        float[] m = new float[16];
        MathUtil.axisAngle(m, angle, ax, ay, az);
        r.set(m);
        Transform cur = orientationTransform();
        cur.postMultiply(r);
        setOrientationFromMatrix(cur);
    }

    public void preRotate(float angle, float ax, float ay, float az) {
        Transform r = new Transform();
        float[] m = new float[16];
        MathUtil.axisAngle(m, angle, ax, ay, az);
        r.set(m);
        Transform cur = orientationTransform();
        r.postMultiply(cur);
        setOrientationFromMatrix(r);
    }

    public void getTransform(Transform transform) {
        if (transform == null) {
            throw new NullPointerException();
        }
        transform.set(generic);
    }

    public void setTransform(Transform transform) {
        if (transform == null) {
            generic.setIdentity();
        } else {
            generic.set(transform);
        }
    }

    public void getCompositeTransform(Transform transform) {
        if (transform == null) {
            throw new NullPointerException();
        }
        float[] t = new float[16];
        float[] tmp = new float[16];
        MathUtil.identity(t);
        t[3] = translation[0];
        t[7] = translation[1];
        t[11] = translation[2];

        float[] r = new float[16];
        MathUtil.axisAngle(r, orientationAngle, orientationAxis[0],
                           orientationAxis[1], orientationAxis[2]);

        float[] s = new float[16];
        MathUtil.identity(s);
        s[0] = scale[0];
        s[5] = scale[1];
        s[10] = scale[2];

        MathUtil.mul(tmp, t, r);
        MathUtil.mul(tmp, tmp, s);
        MathUtil.mul(tmp, tmp, generic.m);
        transform.set(tmp);
    }

    private Transform orientationTransform() {
        Transform t = new Transform();
        float[] m = new float[16];
        MathUtil.axisAngle(m, orientationAngle, orientationAxis[0],
                           orientationAxis[1], orientationAxis[2]);
        t.set(m);
        return t;
    }

    /** Extrae angulo/eje de una matriz de rotacion pura. */
    private void setOrientationFromMatrix(Transform t) {
        float[] m = new float[16];
        t.get(m);
        float trace = m[0] + m[5] + m[10];
        float cos = (trace - 1.0f) * 0.5f;
        if (cos > 1.0f) {
            cos = 1.0f;
        } else if (cos < -1.0f) {
            cos = -1.0f;
        }
        float angle = MathUtil.acos(cos);
        float sin = (float) Math.sin(angle);
        if (Math.abs(sin) < 1.0e-6f) {
            if (cos > 0.0f) {
                setOrientation(0.0f, 0.0f, 0.0f, 1.0f);
                return;
            }
            /* 180 grados: el eje sale de la diagonal. */
            float ax = (float) Math.sqrt(Math.max(0.0f, (m[0] + 1.0f) * 0.5f));
            float ay = (float) Math.sqrt(Math.max(0.0f, (m[5] + 1.0f) * 0.5f));
            float az = (float) Math.sqrt(Math.max(0.0f, (m[10] + 1.0f) * 0.5f));
            setOrientation(180.0f, ax, ay, az);
            return;
        }
        float k = 1.0f / (2.0f * sin);
        setOrientation(angle * 180.0f / (float) Math.PI,
                       (m[9] - m[6]) * k,
                       (m[2] - m[8]) * k,
                       (m[4] - m[1]) * k);
    }

    private static void require3(float[] a) {
        if (a == null) {
            throw new NullPointerException();
        }
        if (a.length < 3) {
            throw new IllegalArgumentException("array.length < 3");
        }
    }

    void updateProperty(int property, float[] value) {
        switch (property) {
        case AnimationTrack.TRANSLATION:
            setTranslation(value[0], value[1], value[2]);
            break;
        case AnimationTrack.SCALE:
            if (value.length >= 3) {
                setScale(value[0], value[1], value[2]);
            } else {
                setScale(value[0], value[0], value[0]);
            }
            break;
        case AnimationTrack.ORIENTATION: {
            /* Los keyframes de ORIENTATION son cuaterniones (x,y,z,w). */
            float[] m = new float[16];
            MathUtil.quatToMatrix(m, value[0], value[1], value[2], value[3]);
            Transform t = new Transform();
            t.set(m);
            setOrientationFromMatrix(t);
            break;
        }
        default:
            super.updateProperty(property, value);
            break;
        }
    }

    void copyTransformableTo(Transformable c) {
        System.arraycopy(translation, 0, c.translation, 0, 3);
        System.arraycopy(orientationAxis, 0, c.orientationAxis, 0, 3);
        System.arraycopy(scale, 0, c.scale, 0, 3);
        c.orientationAngle = orientationAngle;
        c.generic.set(generic);
    }
}
