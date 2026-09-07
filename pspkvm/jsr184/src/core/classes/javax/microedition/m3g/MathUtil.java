/*
 * MathUtil.java — utilidades matriciales/cuaterniones (paquete privado).
 * Fila-mayor: m[fila*4 + columna].
 */
package javax.microedition.m3g;

final class MathUtil {

    private MathUtil() { }

    /**
     * arccos para CLDC 1.1, cuya clase Math solo trae sin/cos/tan/sqrt.
     * Aproximacion polinomica clasica; error maximo ~7e-5 rad, muy por
     * debajo de la precision util para orientaciones y slerp.
     */
    static float acos(float x) {
        if (x > 1.0f) {
            x = 1.0f;
        } else if (x < -1.0f) {
            x = -1.0f;
        }
        boolean negate = x < 0.0f;
        if (negate) {
            x = -x;
        }
        float ret = -0.0187293f;
        ret = ret * x + 0.0742610f;
        ret = ret * x - 0.2121144f;
        ret = ret * x + 1.5707288f;
        ret = ret * (float) Math.sqrt(1.0f - x);
        if (negate) {
            return (float) Math.PI - ret;
        }
        return ret;
    }

    static void identity(float[] m) {
        for (int i = 0; i < 16; i++) {
            m[i] = 0.0f;
        }
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static void mul(float[] dst, float[] a, float[] b) {
        float[] t = new float[16];
        for (int r = 0; r < 4; r++) {
            int r4 = r * 4;
            for (int c = 0; c < 4; c++) {
                t[r4 + c] = a[r4] * b[c] + a[r4 + 1] * b[4 + c]
                          + a[r4 + 2] * b[8 + c] + a[r4 + 3] * b[12 + c];
            }
        }
        System.arraycopy(t, 0, dst, 0, 16);
    }

    static void axisAngle(float[] dst, float angle, float ax, float ay, float az) {
        float len = (float) Math.sqrt(ax * ax + ay * ay + az * az);
        if (len == 0.0f) {
            if (angle != 0.0f) {
                throw new IllegalArgumentException("eje de rotacion nulo");
            }
            identity(dst);
            return;
        }
        ax /= len; ay /= len; az /= len;
        float rad = angle * (float) Math.PI / 180.0f;
        float c = (float) Math.cos(rad);
        float s = (float) Math.sin(rad);
        float t = 1.0f - c;
        identity(dst);
        dst[0]  = t * ax * ax + c;
        dst[1]  = t * ax * ay - s * az;
        dst[2]  = t * ax * az + s * ay;
        dst[4]  = t * ax * ay + s * az;
        dst[5]  = t * ay * ay + c;
        dst[6]  = t * ay * az - s * ax;
        dst[8]  = t * ax * az - s * ay;
        dst[9]  = t * ay * az + s * ax;
        dst[10] = t * az * az + c;
    }

    static void quatToMatrix(float[] dst, float x, float y, float z, float w) {
        float len = (float) Math.sqrt(x * x + y * y + z * z + w * w);
        if (len == 0.0f) {
            identity(dst);
            return;
        }
        x /= len; y /= len; z /= len; w /= len;
        identity(dst);
        dst[0]  = 1.0f - 2.0f * (y * y + z * z);
        dst[1]  = 2.0f * (x * y - z * w);
        dst[2]  = 2.0f * (x * z + y * w);
        dst[4]  = 2.0f * (x * y + z * w);
        dst[5]  = 1.0f - 2.0f * (x * x + z * z);
        dst[6]  = 2.0f * (y * z - x * w);
        dst[8]  = 2.0f * (x * z - y * w);
        dst[9]  = 2.0f * (y * z + x * w);
        dst[10] = 1.0f - 2.0f * (x * x + y * y);
    }

    /** Inversa general por cofactores; false si es singular. */
    static boolean invert(float[] dst, float[] m) {
        float[] inv = new float[16];
        inv[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
                 + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
        inv[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
                 - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
        inv[8]  =  m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
                 + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
        inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
                 - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
        inv[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
                 - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
        inv[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
                 + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
        inv[9]  = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
                 - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
        inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
                 + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
        inv[2]  =  m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
                 + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
        inv[6]  = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
                 - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
        inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
                 + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
        inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
                 - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
        inv[3]  = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
                 - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
        inv[7]  =  m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
                 + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
        inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
                 - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
        inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
                 + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

        float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
        if (det == 0.0f) {
            return false;
        }
        det = 1.0f / det;
        for (int i = 0; i < 16; i++) {
            dst[i] = inv[i] * det;
        }
        return true;
    }
}
