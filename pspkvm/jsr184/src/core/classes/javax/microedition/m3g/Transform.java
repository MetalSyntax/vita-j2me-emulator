/*
 * Transform.java — JSR-184 (M3G 1.0)
 * Implementacion para el port PS Vita de PSPKVM.
 * Matriz 4x4 en orden fila-mayor, igual que exige get(float[]).
 */
package javax.microedition.m3g;

public class Transform {

    /** m[fila*4 + columna] */
    final float[] m = new float[16];

    public Transform() {
        setIdentity();
    }

    public Transform(Transform transform) {
        if (transform == null) {
            throw new NullPointerException();
        }
        System.arraycopy(transform.m, 0, m, 0, 16);
    }

    public void setIdentity() {
        for (int i = 0; i < 16; i++) {
            m[i] = 0.0f;
        }
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    public void set(Transform transform) {
        if (transform == null) {
            throw new NullPointerException();
        }
        System.arraycopy(transform.m, 0, m, 0, 16);
    }

    public void set(float[] matrix) {
        if (matrix == null) {
            throw new NullPointerException();
        }
        if (matrix.length < 16) {
            throw new IllegalArgumentException("matrix.length < 16");
        }
        System.arraycopy(matrix, 0, m, 0, 16);
    }

    public void get(float[] matrix) {
        if (matrix == null) {
            throw new NullPointerException();
        }
        if (matrix.length < 16) {
            throw new IllegalArgumentException("matrix.length < 16");
        }
        System.arraycopy(m, 0, matrix, 0, 16);
    }

    public void transpose() {
        for (int r = 0; r < 4; r++) {
            for (int c = r + 1; c < 4; c++) {
                float t = m[r * 4 + c];
                m[r * 4 + c] = m[c * 4 + r];
                m[c * 4 + r] = t;
            }
        }
    }

    public void invert() {
        if (!MathUtil.invert(m, m)) {
            throw new ArithmeticException("matriz singular");
        }
    }

    public void postMultiply(Transform transform) {
        if (transform == null) {
            throw new NullPointerException();
        }
        MathUtil.mul(m, m, transform.m);
    }

    public void postScale(float sx, float sy, float sz) {
        float[] s = new float[16];
        MathUtil.identity(s);
        s[0] = sx; s[5] = sy; s[10] = sz;
        MathUtil.mul(m, m, s);
    }

    public void postTranslate(float tx, float ty, float tz) {
        float[] t = new float[16];
        MathUtil.identity(t);
        t[3] = tx; t[7] = ty; t[11] = tz;
        MathUtil.mul(m, m, t);
    }

    public void postRotate(float angle, float ax, float ay, float az) {
        float[] r = new float[16];
        MathUtil.axisAngle(r, angle, ax, ay, az);
        MathUtil.mul(m, m, r);
    }

    public void postRotateQuat(float qx, float qy, float qz, float qw) {
        float[] r = new float[16];
        MathUtil.quatToMatrix(r, qx, qy, qz, qw);
        MathUtil.mul(m, m, r);
    }

    /**
     * Transforma en sitio un array de vectores de 4 componentes.
     */
    public void transform(float[] values) {
        if (values == null) {
            throw new NullPointerException();
        }
        if ((values.length % 4) != 0) {
            throw new IllegalArgumentException("values.length no multiplo de 4");
        }
        float[] v = new float[4];
        for (int i = 0; i < values.length; i += 4) {
            v[0] = values[i];
            v[1] = values[i + 1];
            v[2] = values[i + 2];
            v[3] = values[i + 3];
            values[i]     = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3] * v[3];
            values[i + 1] = m[4] * v[0] + m[5] * v[1] + m[6] * v[2] + m[7] * v[3];
            values[i + 2] = m[8] * v[0] + m[9] * v[1] + m[10] * v[2] + m[11] * v[3];
            values[i + 3] = m[12] * v[0] + m[13] * v[1] + m[14] * v[2] + m[15] * v[3];
        }
    }

    /**
     * Transforma los vertices de un VertexArray al array de salida.
     * Si W es true se escriben 4 componentes por vertice, si no 3.
     */
    public void transform(VertexArray in, float[] out, boolean W) {
        if (in == null || out == null) {
            throw new NullPointerException();
        }
        int n = in.getVertexCount();
        int comps = in.getComponentCount();
        int stride = W ? 4 : 3;
        if (out.length < n * stride) {
            throw new IllegalArgumentException("out demasiado corto");
        }
        float[] v = new float[4];
        for (int i = 0; i < n; i++) {
            v[0] = in.getFloat(i, 0);
            v[1] = (comps > 1) ? in.getFloat(i, 1) : 0.0f;
            v[2] = (comps > 2) ? in.getFloat(i, 2) : 0.0f;
            v[3] = (comps > 3) ? in.getFloat(i, 3) : 1.0f;
            out[i * stride]     = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3] * v[3];
            out[i * stride + 1] = m[4] * v[0] + m[5] * v[1] + m[6] * v[2] + m[7] * v[3];
            out[i * stride + 2] = m[8] * v[0] + m[9] * v[1] + m[10] * v[2] + m[11] * v[3];
            if (W) {
                out[i * stride + 3] =
                    m[12] * v[0] + m[13] * v[1] + m[14] * v[2] + m[15] * v[3];
            }
        }
    }
}
