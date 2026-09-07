/*
 * VertexBuffer.java — JSR-184 (M3G 1.0).
 */
package javax.microedition.m3g;

public class VertexBuffer extends Object3D {

    static final int MAX_TEXTURE_UNITS = 2;

    private VertexArray positions;
    private float posScale = 1.0f;
    private final float[] posBias = new float[3];

    private VertexArray normals;
    private VertexArray colors;

    private final VertexArray[] texCoords = new VertexArray[MAX_TEXTURE_UNITS];
    private final float[] texScale = new float[MAX_TEXTURE_UNITS];
    private final float[][] texBias = new float[MAX_TEXTURE_UNITS][3];

    private int defaultColor = 0xFFFFFFFF;

    public VertexBuffer() {
        for (int i = 0; i < MAX_TEXTURE_UNITS; i++) {
            texScale[i] = 1.0f;
        }
    }

    public int getVertexCount() {
        return (positions != null) ? positions.getVertexCount() : 0;
    }

    public int getDefaultColor() {
        return defaultColor;
    }

    public void setDefaultColor(int ARGB) {
        defaultColor = ARGB;
    }

    public VertexArray getPositions(float[] scaleBias) {
        if (scaleBias != null) {
            if (scaleBias.length < 4) {
                throw new IllegalArgumentException("scaleBias.length < 4");
            }
            scaleBias[0] = posScale;
            scaleBias[1] = posBias[0];
            scaleBias[2] = posBias[1];
            scaleBias[3] = posBias[2];
        }
        return positions;
    }

    public void setPositions(VertexArray positions, float scale, float[] bias) {
        if (positions != null) {
            if (positions.getComponentCount() != 3) {
                throw new IllegalArgumentException(
                    "las posiciones requieren 3 componentes");
            }
            checkVertexCount(positions);
        }
        this.positions = positions;
        this.posScale = scale;
        if (bias != null) {
            if (bias.length < 3) {
                throw new IllegalArgumentException("bias.length < 3");
            }
            posBias[0] = bias[0];
            posBias[1] = bias[1];
            posBias[2] = bias[2];
        } else {
            posBias[0] = posBias[1] = posBias[2] = 0.0f;
        }
    }

    public VertexArray getNormals() {
        return normals;
    }

    public void setNormals(VertexArray normals) {
        if (normals != null) {
            if (normals.getComponentCount() != 3) {
                throw new IllegalArgumentException(
                    "las normales requieren 3 componentes");
            }
            if (normals.getComponentType() == 4) {
                throw new IllegalArgumentException(
                    "las normales deben ser byte o short");
            }
            checkVertexCount(normals);
        }
        this.normals = normals;
    }

    public VertexArray getColors() {
        return colors;
    }

    public void setColors(VertexArray colors) {
        if (colors != null) {
            int c = colors.getComponentCount();
            if (c != 3 && c != 4) {
                throw new IllegalArgumentException(
                    "los colores requieren 3 o 4 componentes");
            }
            if (colors.getComponentType() != 1) {
                throw new IllegalArgumentException("los colores deben ser byte");
            }
            checkVertexCount(colors);
        }
        this.colors = colors;
    }

    public int getTexCoordArrayCount() {
        return MAX_TEXTURE_UNITS;
    }

    public VertexArray getTexCoords(int index, float[] scaleBias) {
        checkTexIndex(index);
        if (scaleBias != null) {
            int comps = (texCoords[index] != null)
                      ? texCoords[index].getComponentCount() : 2;
            if (scaleBias.length < comps + 1) {
                throw new IllegalArgumentException("scaleBias demasiado corto");
            }
            scaleBias[0] = texScale[index];
            scaleBias[1] = texBias[index][0];
            scaleBias[2] = texBias[index][1];
            if (comps > 2 && scaleBias.length > 3) {
                scaleBias[3] = texBias[index][2];
            }
        }
        return texCoords[index];
    }

    public void setTexCoords(int index, VertexArray texCoords, float scale,
                             float[] bias) {
        checkTexIndex(index);
        if (texCoords != null) {
            int c = texCoords.getComponentCount();
            if (c != 2 && c != 3) {
                throw new IllegalArgumentException(
                    "las coordenadas de textura requieren 2 o 3 componentes");
            }
            checkVertexCount(texCoords);
        }
        this.texCoords[index] = texCoords;
        this.texScale[index] = scale;
        if (bias != null) {
            texBias[index][0] = bias[0];
            texBias[index][1] = bias[1];
            texBias[index][2] = (bias.length > 2) ? bias[2] : 0.0f;
        } else {
            texBias[index][0] = texBias[index][1] = texBias[index][2] = 0.0f;
        }
    }

    private void checkTexIndex(int index) {
        if (index < 0 || index >= MAX_TEXTURE_UNITS) {
            throw new IndexOutOfBoundsException("unidad de textura invalida");
        }
    }

    /** Todos los arrays de un VertexBuffer deben tener el mismo tamano. */
    private void checkVertexCount(VertexArray va) {
        int n = getVertexCount();
        if (n != 0 && va.getVertexCount() != n) {
            throw new IllegalArgumentException(
                "numero de vertices distinto del resto del VertexBuffer");
        }
    }

    /* --- accesos internos para Graphics3D --- */
    float positionScale() { return posScale; }
    float positionBias(int i) { return posBias[i]; }
    float texCoordScale(int u) { return texScale[u]; }
    float texCoordBias(int u, int i) { return texBias[u][i]; }

    public int getReferences(Object3D[] references) {
        int n = super.getReferences(references);
        Object3D[] all = { positions, normals, colors,
                           texCoords[0], texCoords[1] };
        for (int i = 0; i < all.length; i++) {
            if (all[i] != null) {
                if (references != null) {
                    references[n] = all[i];
                }
                n++;
            }
        }
        return n;
    }

    Object3D createClone() {
        VertexBuffer c = new VertexBuffer();
        c.positions = positions;
        c.posScale = posScale;
        System.arraycopy(posBias, 0, c.posBias, 0, 3);
        c.normals = normals;
        c.colors = colors;
        c.defaultColor = defaultColor;
        for (int i = 0; i < MAX_TEXTURE_UNITS; i++) {
            c.texCoords[i] = texCoords[i];
            c.texScale[i] = texScale[i];
            System.arraycopy(texBias[i], 0, c.texBias[i], 0, 3);
        }
        return c;
    }
}
