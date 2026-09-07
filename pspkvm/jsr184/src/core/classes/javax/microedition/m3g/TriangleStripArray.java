/*
 * TriangleStripArray.java — JSR-184 (M3G 1.0).
 */
package javax.microedition.m3g;

public class TriangleStripArray extends IndexBuffer {

    public TriangleStripArray(int firstIndex, int[] stripLengths) {
        if (stripLengths == null) {
            throw new NullPointerException();
        }
        if (firstIndex < 0) {
            throw new IndexOutOfBoundsException("firstIndex < 0");
        }
        validateStrips(stripLengths);
        this.firstIndex = firstIndex;
        this.stripLengths = copy(stripLengths);
        this.indices = null;
    }

    public TriangleStripArray(int[] indices, int[] stripLengths) {
        if (indices == null || stripLengths == null) {
            throw new NullPointerException();
        }
        validateStrips(stripLengths);
        int total = 0;
        for (int i = 0; i < stripLengths.length; i++) {
            total += stripLengths[i];
        }
        if (indices.length < total) {
            throw new IllegalArgumentException(
                "indices.length menor que la suma de stripLengths");
        }
        for (int i = 0; i < total; i++) {
            if (indices[i] < 0) {
                throw new IndexOutOfBoundsException("indice negativo");
            }
        }
        this.indices = new int[total];
        System.arraycopy(indices, 0, this.indices, 0, total);
        this.stripLengths = copy(stripLengths);
        this.firstIndex = 0;
    }

    private static void validateStrips(int[] stripLengths) {
        if (stripLengths.length == 0) {
            throw new IllegalArgumentException("stripLengths vacio");
        }
        for (int i = 0; i < stripLengths.length; i++) {
            if (stripLengths[i] < 3) {
                throw new IllegalArgumentException(
                    "cada tira necesita al menos 3 indices");
            }
        }
    }

    private static int[] copy(int[] src) {
        int[] d = new int[src.length];
        System.arraycopy(src, 0, d, 0, src.length);
        return d;
    }

    Object3D createClone() {
        if (indices != null) {
            return new TriangleStripArray(indices, stripLengths);
        }
        return new TriangleStripArray(firstIndex, stripLengths);
    }
}
