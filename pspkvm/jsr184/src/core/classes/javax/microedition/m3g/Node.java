/*
 * Node.java — JSR-184 (M3G 1.0). Nodo del grafo de escena.
 */
package javax.microedition.m3g;

public abstract class Node extends Transformable {

    public static final int NONE   = 144;
    public static final int ORIGIN = 145;
    public static final int X_AXIS = 146;
    public static final int Y_AXIS = 147;
    public static final int Z_AXIS = 148;

    Node parent;

    private boolean renderingEnabled = true;
    private boolean pickingEnabled = true;
    private float alphaFactor = 1.0f;
    private int scope = -1;

    private Node zReference;
    private Node yReference;
    private int zTarget = NONE;
    private int yTarget = NONE;

    Node() {
    }

    public Node getParent() {
        return parent;
    }

    public boolean isRenderingEnabled() {
        return renderingEnabled;
    }

    public void setRenderingEnable(boolean enable) {
        renderingEnabled = enable;
    }

    public boolean isPickingEnabled() {
        return pickingEnabled;
    }

    public void setPickingEnable(boolean enable) {
        pickingEnabled = enable;
    }

    public float getAlphaFactor() {
        return alphaFactor;
    }

    public void setAlphaFactor(float alphaFactor) {
        if (alphaFactor < 0.0f || alphaFactor > 1.0f) {
            throw new IllegalArgumentException("alphaFactor fuera de [0,1]");
        }
        this.alphaFactor = alphaFactor;
    }

    public int getScope() {
        return scope;
    }

    public void setScope(int scope) {
        this.scope = scope;
    }

    public int getAlignmentTarget(int axis) {
        if (axis == Y_AXIS) {
            return yTarget;
        }
        if (axis == Z_AXIS) {
            return zTarget;
        }
        throw new IllegalArgumentException("axis debe ser Y_AXIS o Z_AXIS");
    }

    public Node getAlignmentReference(int axis) {
        if (axis == Y_AXIS) {
            return yReference;
        }
        if (axis == Z_AXIS) {
            return zReference;
        }
        throw new IllegalArgumentException("axis debe ser Y_AXIS o Z_AXIS");
    }

    public void setAlignment(Node zRef, int zTarget, Node yRef, int yTarget) {
        checkTarget(zTarget);
        checkTarget(yTarget);
        if (zRef == this || yRef == this) {
            throw new IllegalArgumentException("un nodo no puede alinearse consigo mismo");
        }
        this.zReference = zRef;
        this.zTarget = zTarget;
        this.yReference = yRef;
        this.yTarget = yTarget;
    }

    private static void checkTarget(int target) {
        if (target != NONE && target != ORIGIN && target != X_AXIS
            && target != Y_AXIS && target != Z_AXIS) {
            throw new IllegalArgumentException("target de alineacion invalido");
        }
    }

    /**
     * Orienta el nodo segun su alineacion. Se resuelve primero el eje Z
     * y despues se ortogonaliza el Y, como describe la especificacion.
     */
    public void align(Node reference) {
        if (zTarget == NONE && yTarget == NONE) {
            return;
        }
        Node zr = (zReference != null) ? zReference : reference;
        Node yr = (yReference != null) ? yReference : reference;

        float[] zdir = axisTowards(zr, zTarget);
        float[] ydir = axisTowards(yr, yTarget);
        if (zdir == null && ydir == null) {
            return;
        }
        if (zdir == null) {
            zdir = new float[] { 0.0f, 0.0f, 1.0f };
        }
        if (ydir == null) {
            ydir = new float[] { 0.0f, 1.0f, 0.0f };
        }

        float[] x = cross(ydir, zdir);
        if (norm(x) == 0.0f) {
            return;     /* ejes paralelos: alineacion indefinida */
        }
        normalize(x);
        normalize(zdir);
        float[] y = cross(zdir, x);

        float[] m = new float[16];
        MathUtil.identity(m);
        m[0] = x[0];    m[1] = y[0];    m[2] = zdir[0];
        m[4] = x[1];    m[5] = y[1];    m[6] = zdir[1];
        m[8] = x[2];    m[9] = y[2];    m[10] = zdir[2];

        Transform t = new Transform();
        t.set(m);
        applyOrientationMatrix(t);

        /* La alineacion se propaga a los hijos. */
        if (this instanceof Group) {
            Group g = (Group) this;
            for (int i = 0; i < g.getChildCount(); i++) {
                g.getChild(i).align(reference);
            }
        }
    }

    /** Direccion del eje pedido del nodo de referencia, en coords de este. */
    private float[] axisTowards(Node ref, int target) {
        if (ref == null || target == NONE) {
            return null;
        }
        Transform t = new Transform();
        if (!ref.getTransformTo(this, t)) {
            return null;
        }
        float[] v;
        switch (target) {
        case ORIGIN: v = new float[] { 0.0f, 0.0f, 0.0f, 1.0f }; break;
        case X_AXIS: v = new float[] { 1.0f, 0.0f, 0.0f, 0.0f }; break;
        case Y_AXIS: v = new float[] { 0.0f, 1.0f, 0.0f, 0.0f }; break;
        default:     v = new float[] { 0.0f, 0.0f, 1.0f, 0.0f }; break;
        }
        t.transform(v);
        float[] d = { v[0], v[1], v[2] };
        if (norm(d) == 0.0f) {
            return null;
        }
        normalize(d);
        return d;
    }

    private void applyOrientationMatrix(Transform t) {
        float[] axis = new float[4];
        getOrientation(axis);
        /* setOrientation a partir de la matriz de rotacion resultante. */
        float[] m = new float[16];
        t.get(m);
        float trace = m[0] + m[5] + m[10];
        float cos = (trace - 1.0f) * 0.5f;
        if (cos > 1.0f) {
            cos = 1.0f;
        } else if (cos < -1.0f) {
            cos = -1.0f;
        }
        float angle = MathUtil.acos(cos);
        float sin = (float) Math.sin(angle);
        if (Math.abs(sin) < 1.0e-6f) {
            if (cos > 0.0f) {
                setOrientation(0.0f, 0.0f, 0.0f, 1.0f);
            } else {
                setOrientation(180.0f,
                    (float) Math.sqrt(Math.max(0.0f, (m[0] + 1.0f) * 0.5f)),
                    (float) Math.sqrt(Math.max(0.0f, (m[5] + 1.0f) * 0.5f)),
                    (float) Math.sqrt(Math.max(0.0f, (m[10] + 1.0f) * 0.5f)));
            }
            return;
        }
        float k = 1.0f / (2.0f * sin);
        setOrientation(angle * 180.0f / (float) Math.PI,
                       (m[9] - m[6]) * k, (m[2] - m[8]) * k,
                       (m[4] - m[1]) * k);
    }

    private static float[] cross(float[] a, float[] b) {
        return new float[] {
            a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]
        };
    }

    private static float norm(float[] v) {
        return (float) Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    }

    private static void normalize(float[] v) {
        float n = norm(v);
        if (n != 0.0f) {
            v[0] /= n; v[1] /= n; v[2] /= n;
        }
    }

    /**
     * Transformacion de las coordenadas de este nodo a las del destino.
     * false si no comparten arbol.
     */
    public boolean getTransformTo(Node target, Transform transform) {
        if (target == null || transform == null) {
            throw new NullPointerException();
        }
        Node common = findCommonAncestor(this, target);
        if (common == null) {
            return false;
        }
        Transform toCommon = new Transform();
        chainTo(this, common, toCommon);
        Transform targetToCommon = new Transform();
        chainTo(target, common, targetToCommon);
        targetToCommon.invert();
        targetToCommon.postMultiply(toCommon);
        transform.set(targetToCommon);
        return true;
    }

    /** M = C_(hijo de anc) * ... * C_node */
    private static void chainTo(Node node, Node ancestor, Transform out) {
        out.setIdentity();
        Node n = node;
        Transform c = new Transform();
        while (n != null && n != ancestor) {
            n.getCompositeTransform(c);
            /* out = C_n * out  =>  se premultiplica */
            Transform t = new Transform(c);
            t.postMultiply(out);
            out.set(t);
            n = n.parent;
        }
    }

    private static Node findCommonAncestor(Node a, Node b) {
        for (Node p = a; p != null; p = p.parent) {
            for (Node q = b; q != null; q = q.parent) {
                if (p == q) {
                    return p;
                }
            }
        }
        return null;
    }

    void updateProperty(int property, float[] value) {
        switch (property) {
        case AnimationTrack.ALPHA: {
            float v = value[0];
            alphaFactor = (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
            break;
        }
        case AnimationTrack.PICKABILITY:
            pickingEnabled = value[0] >= 0.5f;
            break;
        case AnimationTrack.VISIBILITY:
            renderingEnabled = value[0] >= 0.5f;
            break;
        default:
            super.updateProperty(property, value);
            break;
        }
    }

    void copyNodeTo(Node c) {
        copyTransformableTo(c);
        c.renderingEnabled = renderingEnabled;
        c.pickingEnabled = pickingEnabled;
        c.alphaFactor = alphaFactor;
        c.scope = scope;
        c.zReference = zReference;
        c.yReference = yReference;
        c.zTarget = zTarget;
        c.yTarget = yTarget;
    }
}
