/* World.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

public class World extends Group {

    private Camera activeCamera;
    private Background background;

    public World() {
    }

    public Camera getActiveCamera() {
        return activeCamera;
    }

    public void setActiveCamera(Camera camera) {
        if (camera == null) {
            throw new NullPointerException();
        }
        this.activeCamera = camera;
    }

    public Background getBackground() {
        return background;
    }

    public void setBackground(Background background) {
        this.background = background;
    }

    public int getReferences(Object3D[] references) {
        int n = super.getReferences(references);
        if (background != null) {
            if (references != null) {
                references[n] = background;
            }
            n++;
        }
        return n;
    }

    Object3D createClone() {
        World c = new World();
        copyNodeTo(c);
        for (int i = 0; i < getChildCount(); i++) {
            c.addChild((Node) getChild(i).duplicate());
        }
        c.background = background;
        c.activeCamera = activeCamera;
        return c;
    }
}
