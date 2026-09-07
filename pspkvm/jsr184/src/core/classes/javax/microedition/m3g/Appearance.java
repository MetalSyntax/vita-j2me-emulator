/* Appearance.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class Appearance extends Object3D {

    private int layer;
    private CompositingMode compositingMode;
    private Fog fog;
    private PolygonMode polygonMode;
    private Material material;
    private final Texture2D[] textures =
        new Texture2D[VertexBuffer.MAX_TEXTURE_UNITS];

    public Appearance() {
    }

    public int getLayer() {
        return layer;
    }

    public void setLayer(int layer) {
        if (layer < -63 || layer > 63) {
            throw new IndexOutOfBoundsException("layer fuera de [-63,63]");
        }
        this.layer = layer;
    }

    public CompositingMode getCompositingMode() {
        return compositingMode;
    }

    public void setCompositingMode(CompositingMode compositingMode) {
        this.compositingMode = compositingMode;
    }

    public Fog getFog() {
        return fog;
    }

    public void setFog(Fog fog) {
        this.fog = fog;
    }

    public PolygonMode getPolygonMode() {
        return polygonMode;
    }

    public void setPolygonMode(PolygonMode polygonMode) {
        this.polygonMode = polygonMode;
    }

    public Material getMaterial() {
        return material;
    }

    public void setMaterial(Material material) {
        this.material = material;
    }

    public Texture2D getTexture(int index) {
        checkIndex(index);
        return textures[index];
    }

    public void setTexture(int index, Texture2D texture) {
        checkIndex(index);
        textures[index] = texture;
    }

    private static void checkIndex(int index) {
        if (index < 0 || index >= VertexBuffer.MAX_TEXTURE_UNITS) {
            throw new IndexOutOfBoundsException("unidad de textura invalida");
        }
    }

    public int getReferences(Object3D[] references) {
        int n = super.getReferences(references);
        Object3D[] all = { compositingMode, fog, polygonMode, material,
                           textures[0], textures[1] };
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
        Appearance c = new Appearance();
        c.layer = layer;
        c.compositingMode = compositingMode;
        c.fog = fog;
        c.polygonMode = polygonMode;
        c.material = material;
        c.textures[0] = textures[0];
        c.textures[1] = textures[1];
        return c;
    }
}
