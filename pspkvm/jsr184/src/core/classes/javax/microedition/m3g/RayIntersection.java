/* RayIntersection.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class RayIntersection {

    private Node intersected;
    private float distance;
    private int submeshIndex;
    private final float[] normal = { 0.0f, 0.0f, 1.0f };
    private final float[] ray = new float[6];
    private final float[] texS = new float[VertexBuffer.MAX_TEXTURE_UNITS];
    private final float[] texT = new float[VertexBuffer.MAX_TEXTURE_UNITS];

    public RayIntersection() {
    }

    public Node getIntersected() {
        return intersected;
    }

    public float getDistance() {
        return distance;
    }

    public int getSubmeshIndex() {
        return submeshIndex;
    }

    public void getNormal(float[] normal) {
        if (normal == null) {
            throw new NullPointerException();
        }
        if (normal.length < 3) {
            throw new IllegalArgumentException("normal.length < 3");
        }
        System.arraycopy(this.normal, 0, normal, 0, 3);
    }

    public void getRay(float[] ray) {
        if (ray == null) {
            throw new NullPointerException();
        }
        if (ray.length < 6) {
            throw new IllegalArgumentException("ray.length < 6");
        }
        System.arraycopy(this.ray, 0, ray, 0, 6);
    }

    public float getTextureS(int index) {
        checkIndex(index);
        return texS[index];
    }

    public float getTextureT(int index) {
        checkIndex(index);
        return texT[index];
    }

    private static void checkIndex(int index) {
        if (index < 0 || index >= VertexBuffer.MAX_TEXTURE_UNITS) {
            throw new IndexOutOfBoundsException("unidad de textura invalida");
        }
    }

    /* --- relleno interno desde Group.pick --- */
    void fill(Node node, float distance, int submeshIndex,
              float[] normal, float[] ray, float[] s, float[] t) {
        this.intersected = node;
        this.distance = distance;
        this.submeshIndex = submeshIndex;
        System.arraycopy(normal, 0, this.normal, 0, 3);
        System.arraycopy(ray, 0, this.ray, 0, 6);
        System.arraycopy(s, 0, this.texS, 0, texS.length);
        System.arraycopy(t, 0, this.texT, 0, texT.length);
    }
}
