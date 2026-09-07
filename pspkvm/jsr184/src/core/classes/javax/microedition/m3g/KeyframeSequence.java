/*
 * KeyframeSequence.java — JSR-184 (M3G 1.0).
 *
 * Interpolaciones implementadas: STEP, LINEAR, SLERP y SPLINE
 * (Catmull-Rom). SQUAD se evalua con SLERP: la diferencia es solo una
 * transicion de rotacion algo menos suave y no cambia la geometria.
 * Documentado en jsr184/README_VITA.txt.
 */
package javax.microedition.m3g;

public class KeyframeSequence extends Object3D {

    public static final int CONSTANT = 192;
    public static final int LOOP     = 193;
    public static final int LINEAR   = 176;
    public static final int SLERP    = 177;
    public static final int SPLINE   = 178;
    public static final int SQUAD    = 179;
    public static final int STEP     = 180;

    private final int numKeyframes;
    private final int numComponents;
    private final int interpolation;
    private final int[] times;
    private final float[] values;

    private int duration;
    private int repeatMode = CONSTANT;
    private int rangeFirst;
    private int rangeLast;

    public KeyframeSequence(int numKeyframes, int numComponents,
                            int interpolation) {
        if (numKeyframes < 1 || numComponents < 1) {
            throw new IllegalArgumentException(
                "numKeyframes y numComponents deben ser > 0");
        }
        switch (interpolation) {
        case STEP: case LINEAR: case SPLINE:
            break;
        case SLERP: case SQUAD:
            if (numComponents != 4) {
                throw new IllegalArgumentException(
                    "SLERP/SQUAD exigen 4 componentes");
            }
            break;
        default:
            throw new IllegalArgumentException("interpolacion desconocida");
        }
        this.numKeyframes = numKeyframes;
        this.numComponents = numComponents;
        this.interpolation = interpolation;
        this.times = new int[numKeyframes];
        this.values = new float[numKeyframes * numComponents];
        this.rangeFirst = 0;
        this.rangeLast = numKeyframes - 1;
    }

    public int getComponentCount() {
        return numComponents;
    }

    public int getKeyframeCount() {
        return numKeyframes;
    }

    public int getInterpolationType() {
        return interpolation;
    }

    public int getDuration() {
        return duration;
    }

    public void setDuration(int duration) {
        if (duration <= 0) {
            throw new IllegalArgumentException("duration <= 0");
        }
        this.duration = duration;
    }

    public int getRepeatMode() {
        return repeatMode;
    }

    public void setRepeatMode(int mode) {
        if (mode != CONSTANT && mode != LOOP) {
            throw new IllegalArgumentException("repeat mode desconocido");
        }
        this.repeatMode = mode;
    }

    public int getValidRangeFirst() {
        return rangeFirst;
    }

    public int getValidRangeLast() {
        return rangeLast;
    }

    public void setValidRange(int first, int last) {
        if (first < 0 || first >= numKeyframes
            || last < 0 || last >= numKeyframes) {
            throw new IndexOutOfBoundsException("rango fuera de limites");
        }
        this.rangeFirst = first;
        this.rangeLast = last;
    }

    public int getKeyframe(int index, float[] value) {
        if (index < 0 || index >= numKeyframes) {
            throw new IndexOutOfBoundsException();
        }
        if (value != null) {
            if (value.length < numComponents) {
                throw new IllegalArgumentException("value demasiado corto");
            }
            System.arraycopy(values, index * numComponents, value, 0,
                             numComponents);
        }
        return times[index];
    }

    public void setKeyframe(int index, int time, float[] value) {
        if (index < 0 || index >= numKeyframes) {
            throw new IndexOutOfBoundsException();
        }
        if (value == null) {
            throw new NullPointerException();
        }
        if (time < 0) {
            throw new IllegalArgumentException("time < 0");
        }
        if (value.length < numComponents) {
            throw new IllegalArgumentException("value demasiado corto");
        }
        times[index] = time;
        System.arraycopy(value, 0, values, index * numComponents,
                         numComponents);
    }

    public void setKeyframes(int[] times, float[] values) {
        /* Ayuda del Loader: no forma parte del API publico de M3G 1.0. */
        System.arraycopy(times, 0, this.times, 0, numKeyframes);
        System.arraycopy(values, 0, this.values, 0,
                         numKeyframes * numComponents);
    }

    /* ------------------------------------------------------------ */
    /* Evaluacion                                                    */
    /* ------------------------------------------------------------ */

    /** Numero de keyframes del rango valido. */
    private int rangeCount() {
        if (rangeLast >= rangeFirst) {
            return rangeLast - rangeFirst + 1;
        }
        return numKeyframes - rangeFirst + rangeLast + 1;
    }

    /** Convierte una posicion dentro del rango [0, n) a indice absoluto. */
    private int keyAt(int i) {
        int n = rangeCount();
        if (n <= 0) {
            return rangeFirst;
        }
        i = ((i % n) + n) % n;
        return (rangeFirst + i) % numKeyframes;
    }

    /**
     * Muestrea la secuencia en el instante dado (en unidades de la
     * secuencia). Se aplica el modo de repeticion.
     */
    void sample(float time, float[] out) {
        int n = rangeCount();
        if (n <= 0 || out == null) {
            return;
        }
        if (duration <= 0) {
            copyKey(keyAt(0), out);
            return;
        }

        if (repeatMode == LOOP) {
            time = time % duration;
            if (time < 0.0f) {
                time += duration;
            }
        } else {
            if (time < 0.0f) {
                time = 0.0f;
            } else if (time > duration) {
                time = duration;
            }
        }

        if (n == 1) {
            copyKey(keyAt(0), out);
            return;
        }

        /* Localiza el segmento [i, i+1) que contiene time. */
        int seg = -1;
        for (int i = 0; i < n; i++) {
            int k0 = keyAt(i);
            int k1 = keyAt(i + 1);
            int t0 = times[k0];
            int t1 = times[k1];
            if (i == n - 1) {
                /* Ultimo segmento: cierra contra duration (LOOP) o
                 * mantiene el ultimo valor (CONSTANT). */
                if (repeatMode == LOOP) {
                    t1 = duration + times[keyAt(0)];
                } else {
                    if (time >= t0) {
                        copyKey(k0, out);
                        return;
                    }
                    continue;
                }
            }
            if (time >= t0 && time <= t1) {
                seg = i;
                break;
            }
        }
        if (seg < 0) {
            /* Antes del primer keyframe. */
            if (repeatMode == LOOP) {
                seg = n - 1;
            } else {
                copyKey(keyAt(0), out);
                return;
            }
        }

        int k0 = keyAt(seg);
        int k1 = keyAt(seg + 1);
        float t0 = times[k0];
        float t1 = times[k1];
        if (t1 <= t0) {
            t1 = t0 + duration;
        }
        float s = (t1 > t0) ? (time - t0) / (t1 - t0) : 0.0f;
        if (s < 0.0f) {
            s = 0.0f;
        } else if (s > 1.0f) {
            s = 1.0f;
        }

        switch (interpolation) {
        case STEP:
            copyKey(k0, out);
            break;
        case SLERP:
        case SQUAD:
            slerp(k0, k1, s, out);
            break;
        case SPLINE:
            spline(seg, k0, k1, s, out);
            break;
        case LINEAR:
        default:
            for (int c = 0; c < numComponents; c++) {
                float a = values[k0 * numComponents + c];
                float b = values[k1 * numComponents + c];
                out[c] = a + (b - a) * s;
            }
            break;
        }
    }

    private void copyKey(int k, float[] out) {
        System.arraycopy(values, k * numComponents, out, 0, numComponents);
    }

    private void slerp(int k0, int k1, float s, float[] out) {
        float x0 = values[k0 * 4], y0 = values[k0 * 4 + 1];
        float z0 = values[k0 * 4 + 2], w0 = values[k0 * 4 + 3];
        float x1 = values[k1 * 4], y1 = values[k1 * 4 + 1];
        float z1 = values[k1 * 4 + 2], w1 = values[k1 * 4 + 3];

        float dot = x0 * x1 + y0 * y1 + z0 * z1 + w0 * w1;
        if (dot < 0.0f) {
            x1 = -x1; y1 = -y1; z1 = -z1; w1 = -w1;
            dot = -dot;
        }
        float a, b;
        if (dot > 0.9995f) {
            a = 1.0f - s;
            b = s;
        } else {
            float theta = MathUtil.acos(dot);
            float sin = (float) Math.sin(theta);
            a = (float) Math.sin((1.0f - s) * theta) / sin;
            b = (float) Math.sin(s * theta) / sin;
        }
        out[0] = a * x0 + b * x1;
        out[1] = a * y0 + b * y1;
        out[2] = a * z0 + b * z1;
        out[3] = a * w0 + b * w1;
    }

    /** Catmull-Rom sobre los cuatro keyframes que rodean al segmento. */
    private void spline(int seg, int k0, int k1, float s, float[] out) {
        int kPrev = keyAt(seg - 1);
        int kNext = keyAt(seg + 2);
        float s2 = s * s;
        float s3 = s2 * s;
        float h1 =  2.0f * s3 - 3.0f * s2 + 1.0f;
        float h2 = -2.0f * s3 + 3.0f * s2;
        float h3 =         s3 - 2.0f * s2 + s;
        float h4 =         s3 -        s2;

        for (int c = 0; c < numComponents; c++) {
            float p0 = values[kPrev * numComponents + c];
            float p1 = values[k0 * numComponents + c];
            float p2 = values[k1 * numComponents + c];
            float p3 = values[kNext * numComponents + c];
            float m1 = 0.5f * (p2 - p0);
            float m2 = 0.5f * (p3 - p1);
            out[c] = h1 * p1 + h2 * p2 + h3 * m1 + h4 * m2;
        }
    }

    Object3D createClone() {
        KeyframeSequence c = new KeyframeSequence(numKeyframes, numComponents,
                                                  interpolation);
        System.arraycopy(times, 0, c.times, 0, numKeyframes);
        System.arraycopy(values, 0, c.values, 0, values.length);
        c.duration = duration;
        c.repeatMode = repeatMode;
        c.rangeFirst = rangeFirst;
        c.rangeLast = rangeLast;
        return c;
    }
}
