/* Mesh.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class Mesh extends Node {

    VertexBuffer vertices;
    final IndexBuffer[] submeshes;
    final Appearance[] appearances;

    public Mesh(VertexBuffer vertices, IndexBuffer submesh,
                Appearance appearance) {
        if (vertices == null || submesh == null) {
            throw new NullPointerException();
        }
        this.vertices = vertices;
        this.submeshes = new IndexBuffer[] { submesh };
        this.appearances = new Appearance[] { appearance };
    }

    public Mesh(VertexBuffer vertices, IndexBuffer[] submeshes,
                Appearance[] appearances) {
        if (vertices == null || submeshes == null) {
            throw new NullPointerException();
        }
        if (submeshes.length == 0) {
            throw new IllegalArgumentException("sin submallas");
        }
        if (appearances != null && appearances.length < submeshes.length) {
            throw new IllegalArgumentException(
                "appearances mas corto que submeshes");
        }
        for (int i = 0; i < submeshes.length; i++) {
            if (submeshes[i] == null) {
                throw new NullPointerException("submesh nula");
            }
        }
        this.vertices = vertices;
        this.submeshes = new IndexBuffer[submeshes.length];
        System.arraycopy(submeshes, 0, this.submeshes, 0, submeshes.length);
        this.appearances = new Appearance[submeshes.length];
        if (appearances != null) {
            System.arraycopy(appearances, 0, this.appearances, 0,
                             submeshes.length);
        }
    }

    public VertexBuffer getVertexBuffer() {
        return vertices;
    }

    public int getSubmeshCount() {
        return submeshes.length;
    }

    public IndexBuffer getIndexBuffer(int index) {
        if (index < 0 || index >= submeshes.length) {
            throw new IndexOutOfBoundsException();
        }
        return submeshes[index];
    }

    public Appearance getAppearance(int index) {
        if (index < 0 || index >= appearances.length) {
            throw new IndexOutOfBoundsException();
        }
        return appearances[index];
    }

    public void setAppearance(int index, Appearance appearance) {
        if (index < 0 || index >= appearances.length) {
            throw new IndexOutOfBoundsException();
        }
        appearances[index] = appearance;
    }

    /** VertexBuffer efectivo a dibujar (las mallas deformables lo cambian). */
    VertexBuffer renderedVertexBuffer() {
        return vertices;
    }

    public int getReferences(Object3D[] references) {
        int n = super.getReferences(references);
        if (vertices != null) {
            if (references != null) {
                references[n] = vertices;
            }
            n++;
        }
        for (int i = 0; i < submeshes.length; i++) {
            if (submeshes[i] != null) {
                if (references != null) {
                    references[n] = submeshes[i];
                }
                n++;
            }
            if (appearances[i] != null) {
                if (references != null) {
                    references[n] = appearances[i];
                }
                n++;
            }
        }
        return n;
    }

    Object3D createClone() {
        Mesh c = new Mesh(vertices, submeshes, appearances);
        copyNodeTo(c);
        return c;
    }
}
