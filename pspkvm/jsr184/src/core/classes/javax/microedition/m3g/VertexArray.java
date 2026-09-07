/*
 * VertexArray.java — JSR-184 (M3G 1.0).
 *
 * El almacenamiento es el array Java crudo (byte[]/short[]/float[]): el
 * motor nativo lo lee en sitio con SNI_GetRawArrayPointer, sin copias ni
 * memoria nativa que liberar.
 */
package javax.microedition.m3g;

public class VertexArray extends Object3D {

    private final int numVertices;
    private final int numComponents;
    private final int componentSize;   /* 1 = byte, 2 = short, 4 = float */

    byte[]  dataB;
    short[] dataS;
    float[] dataF;

    public VertexArray(int numVertices, int numComponents, int componentSize) {
        if (numVertices < 1 || numVertices > 65535) {
            throw new IllegalArgumentException("numVertices fuera de [1,65535]");
        }
        if (numComponents < 2 || numComponents > 4) {
            throw new IllegalArgumentException("numComponents fuera de [2,4]");
        }
        if (componentSize != 1 && componentSize != 2 && componentSize != 4) {
            /* 4 (float) es extension de M3G 1.1; se acepta por comodidad
             * del Loader, el motor la soporta. */
            throw new IllegalArgumentException("componentSize debe ser 1, 2 o 4");
        }
        this.numVertices = numVertices;
        this.numComponents = numComponents;
        this.componentSize = componentSize;
        int n = numVertices * numComponents;
        switch (componentSize) {
        case 1: dataB = new byte[n]; break;
        case 2: dataS = new short[n]; break;
        default: dataF = new float[n]; break;
        }
    }

    public int getVertexCount() {
        return numVertices;
    }

    public int getComponentCount() {
        return numComponents;
    }

    public int getComponentType() {
        return componentSize;
    }

    public void set(int firstVertex, int numVertices, byte[] values) {
        if (values == null) {
            throw new NullPointerException();
        }
        if (componentSize != 1) {
            throw new IllegalStateException("este VertexArray no es de bytes");
        }
        checkRange(firstVertex, numVertices, values.length);
        System.arraycopy(values, 0, dataB, firstVertex * numComponents,
                         numVertices * numComponents);
    }

    public void set(int firstVertex, int numVertices, short[] values) {
        if (values == null) {
            throw new NullPointerException();
        }
        if (componentSize != 2) {
            throw new IllegalStateException("este VertexArray no es de shorts");
        }
        checkRange(firstVertex, numVertices, values.length);
        System.arraycopy(values, 0, dataS, firstVertex * numComponents,
                         numVertices * numComponents);
    }

    public void set(int firstVertex, int numVertices, float[] values) {
        if (values == null) {
            throw new NullPointerException();
        }
        if (componentSize != 4) {
            throw new IllegalStateException("este VertexArray no es de floats");
        }
        checkRange(firstVertex, numVertices, values.length);
        System.arraycopy(values, 0, dataF, firstVertex * numComponents,
                         numVertices * numComponents);
    }

    public void get(int firstVertex, int numVertices, byte[] values) {
        if (values == null) {
            throw new NullPointerException();
        }
        if (componentSize != 1) {
            throw new IllegalStateException("este VertexArray no es de bytes");
        }
        checkRange(firstVertex, numVertices, values.length);
        System.arraycopy(dataB, firstVertex * numComponents, values, 0,
                         numVertices * numComponents);
    }

    public void get(int firstVertex, int numVertices, short[] values) {
        if (values == null) {
            throw new NullPointerException();
        }
        if (componentSize != 2) {
            throw new IllegalStateException("este VertexArray no es de shorts");
        }
        checkRange(firstVertex, numVertices, values.length);
        System.arraycopy(dataS, firstVertex * numComponents, values, 0,
                         numVertices * numComponents);
    }

    private void checkRange(int firstVertex, int count, int arrayLen) {
        if (firstVertex < 0 || count < 0
            || firstVertex + count > this.numVertices) {
            throw new IndexOutOfBoundsException("rango de vertices invalido");
        }
        if (arrayLen < count * numComponents) {
            throw new IllegalArgumentException("array demasiado corto");
        }
    }

    /** Componente como float sin escala/bias (uso interno). */
    float getFloat(int vertex, int component) {
        int i = vertex * numComponents + component;
        switch (componentSize) {
        case 1: return dataB[i];
        case 2: return dataS[i];
        default: return dataF[i];
        }
    }

    /** Array crudo para pasarlo al motor nativo. */
    Object rawData() {
        switch (componentSize) {
        case 1: return dataB;
        case 2: return dataS;
        default: return dataF;
        }
    }

    Object3D createClone() {
        VertexArray c = new VertexArray(numVertices, numComponents,
                                        componentSize);
        if (dataB != null) {
            System.arraycopy(dataB, 0, c.dataB, 0, dataB.length);
        } else if (dataS != null) {
            System.arraycopy(dataS, 0, c.dataS, 0, dataS.length);
        } else {
            System.arraycopy(dataF, 0, c.dataF, 0, dataF.length);
        }
        return c;
    }
}
