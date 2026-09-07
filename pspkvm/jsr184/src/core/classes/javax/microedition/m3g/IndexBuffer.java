/*
 * IndexBuffer.java — JSR-184 (M3G 1.0).
 */
package javax.microedition.m3g;

public abstract class IndexBuffer extends Object3D {

    /** Indices explicitos, o null si la tira es implicita. */
    int[] indices;
    /** Longitud de cada tira. */
    int[] stripLengths;
    /** Primer indice cuando la tira es implicita. */
    int firstIndex;

    IndexBuffer() {
    }

    public int getIndexCount() {
        int n = 0;
        for (int i = 0; i < stripLengths.length; i++) {
            n += stripLengths[i];
        }
        return n;
    }

    public void getIndices(int[] indices) {
        if (indices == null) {
            throw new NullPointerException();
        }
        int n = getIndexCount();
        if (indices.length < n) {
            throw new IllegalArgumentException("array demasiado corto");
        }
        if (this.indices != null) {
            System.arraycopy(this.indices, 0, indices, 0, n);
        } else {
            for (int i = 0; i < n; i++) {
                indices[i] = firstIndex + i;
            }
        }
    }

    /** Numero de triangulos que generan las tiras. */
    int getTriangleCount() {
        int n = 0;
        for (int i = 0; i < stripLengths.length; i++) {
            if (stripLengths[i] >= 3) {
                n += stripLengths[i] - 2;
            }
        }
        return n;
    }
}
