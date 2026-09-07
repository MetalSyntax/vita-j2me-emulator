/* Group.java — JSR-184 (M3G 1.0). */
package javax.microedition.m3g;

import java.util.Vector;

public class Group extends Node {

    private final Vector children = new Vector();

    public Group() {
    }

    public void addChild(Node child) {
        if (child == null) {
            throw new NullPointerException();
        }
        if (child == this) {
            throw new IllegalArgumentException("un grupo no puede contenerse");
        }
        if (child instanceof World) {
            throw new IllegalArgumentException("World no puede ser hijo");
        }
        for (Node p = this; p != null; p = p.parent) {
            if (p == child) {
                throw new IllegalArgumentException("ciclo en el grafo");
            }
        }
        if (child.parent == this) {
            return;
        }
        if (child.parent instanceof Group) {
            ((Group) child.parent).removeChild(child);
        }
        children.addElement(child);
        child.parent = this;
    }

    public void removeChild(Node child) {
        if (child == null) {
            return;
        }
        if (children.removeElement(child)) {
            child.parent = null;
        }
    }

    public int getChildCount() {
        return children.size();
    }

    public Node getChild(int index) {
        if (index < 0 || index >= children.size()) {
            throw new IndexOutOfBoundsException();
        }
        return (Node) children.elementAt(index);
    }

    /**
     * Lanza un rayo desde la camara a traves del punto (x, y) del
     * viewport, en coordenadas normalizadas [0,1].
     */
    public boolean pick(int scope, float x, float y, Camera camera,
                        RayIntersection ri) {
        if (camera == null) {
            throw new NullPointerException();
        }
        Transform proj = new Transform();
        camera.getProjection(proj);
        float[] pm = new float[16];
        proj.get(pm);
        float[] inv = new float[16];
        if (!MathUtil.invert(inv, pm)) {
            return false;
        }

        float ndcX = 2.0f * x - 1.0f;
        float ndcY = 1.0f - 2.0f * y;
        float[] pNear = { ndcX, ndcY, -1.0f, 1.0f };
        float[] pFar  = { ndcX, ndcY,  1.0f, 1.0f };
        Transform it = new Transform();
        it.set(inv);
        it.transform(pNear);
        it.transform(pFar);
        if (pNear[3] != 0.0f) {
            pNear[0] /= pNear[3]; pNear[1] /= pNear[3]; pNear[2] /= pNear[3];
        }
        if (pFar[3] != 0.0f) {
            pFar[0] /= pFar[3]; pFar[1] /= pFar[3]; pFar[2] /= pFar[3];
        }

        /* De coordenadas de camara a las de este grupo. */
        Transform camToThis = new Transform();
        if (!camera.getTransformTo(this, camToThis)) {
            throw new IllegalStateException(
                "la camara no pertenece a este grafo");
        }
        float[] o = { pNear[0], pNear[1], pNear[2], 1.0f };
        float[] f = { pFar[0], pFar[1], pFar[2], 1.0f };
        camToThis.transform(o);
        camToThis.transform(f);

        return pick(scope, o[0], o[1], o[2],
                    f[0] - o[0], f[1] - o[1], f[2] - o[2], ri);
    }

    public boolean pick(int scope, float ox, float oy, float oz,
                        float dx, float dy, float dz, RayIntersection ri) {
        if (dx == 0.0f && dy == 0.0f && dz == 0.0f) {
            throw new IllegalArgumentException("direccion del rayo nula");
        }
        Picker p = new Picker(scope, ox, oy, oz, dx, dy, dz);
        p.traverse(this, new Transform());
        if (!p.found) {
            return false;
        }
        if (ri != null) {
            p.store(ri);
        }
        return true;
    }

    public int getReferences(Object3D[] references) {
        int n = super.getReferences(references);
        for (int i = 0; i < children.size(); i++) {
            if (references != null) {
                references[n] = (Object3D) children.elementAt(i);
            }
            n++;
        }
        return n;
    }

    Object3D createClone() {
        Group c = new Group();
        copyNodeTo(c);
        for (int i = 0; i < children.size(); i++) {
            c.addChild((Node) ((Node) children.elementAt(i)).duplicate());
        }
        return c;
    }
}
