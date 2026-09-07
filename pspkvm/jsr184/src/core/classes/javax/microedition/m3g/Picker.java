/*
 * Picker.java — recorrido de picking (paquete privado).
 *
 * El rayo se transforma al espacio local de cada nodo y la interseccion
 * rayo/triangulo la resuelve el motor nativo (m3g_pick). Como la
 * direccion se transforma sin normalizar, el parametro t es comparable
 * entre nodos aunque haya escalados por medio.
 */
package javax.microedition.m3g;

final class Picker {

    private final int scope;
    private final float[] ray = new float[6];

    boolean found;
    private float bestT;
    private Mesh bestMesh;
    private int bestSubmesh;
    private final float[] bestNormal = new float[3];
    private final float[] bestS = new float[VertexBuffer.MAX_TEXTURE_UNITS];
    private final float[] bestT2 = new float[VertexBuffer.MAX_TEXTURE_UNITS];

    private final int[] outIdx = new int[4];
    private final float[] outHit = new float[3];

    Picker(int scope, float ox, float oy, float oz,
           float dx, float dy, float dz) {
        this.scope = scope;
        ray[0] = ox; ray[1] = oy; ray[2] = oz;
        ray[3] = dx; ray[4] = dy; ray[5] = dz;
    }

    /** Recorre el subarbol con el rayo expresado en coordenadas de root. */
    void traverse(Node root, Transform unused) {
        visit(root, ray[0], ray[1], ray[2], ray[3], ray[4], ray[5],
              new Transform(), true);
    }

    /**
     * @param toRoot transformacion de las coordenadas del padre a las de
     *               la raiz del picking (para devolver la normal).
     * @param isRoot la raiz no aplica su propia transformacion compuesta.
     */
    private void visit(Node node, float ox, float oy, float oz,
                       float dx, float dy, float dz,
                       Transform toRoot, boolean isRoot) {
        if (!node.isPickingEnabled()) {
            return;
        }

        Transform nodeToParent = new Transform();
        if (!isRoot) {
            node.getCompositeTransform(nodeToParent);
        }

        Transform toLocal = new Transform(nodeToParent);
        float[] o = { ox, oy, oz, 1.0f };
        float[] d = { dx, dy, dz, 0.0f };
        if (!isRoot) {
            try {
                toLocal.invert();
            } catch (ArithmeticException e) {
                return;         /* transformacion no invertible */
            }
            toLocal.transform(o);
            toLocal.transform(d);
        }

        Transform localToRoot = new Transform(toRoot);
        localToRoot.postMultiply(nodeToParent);

        if (node instanceof Group) {
            Group g = (Group) node;
            for (int i = 0; i < g.getChildCount(); i++) {
                visit(g.getChild(i), o[0], o[1], o[2], d[0], d[1], d[2],
                      localToRoot, false);
            }
            return;
        }

        if (!(node instanceof Mesh)) {
            return;             /* Sprite3D y Camera no son pickables */
        }
        Mesh mesh = (Mesh) node;
        if ((mesh.getScope() & scope) == 0) {
            return;
        }

        VertexBuffer vb = mesh.renderedVertexBuffer();
        VertexArray pos = vb.getPositions(null);
        if (pos == null) {
            return;
        }

        int[] ip = new int[M3GNative.IP_COUNT];
        float[] fp = new float[M3GNative.FP_COUNT];
        ip[M3GNative.IP_VERTEX_COUNT] = vb.getVertexCount();
        ip[M3GNative.IP_POS_COMPONENTS] = pos.getComponentCount();
        ip[M3GNative.IP_POS_TYPE] = pos.getComponentType();
        fp[M3GNative.FP_POS_SCALE] = vb.positionScale();
        fp[M3GNative.FP_POS_BIAS] = vb.positionBias(0);
        fp[M3GNative.FP_POS_BIAS + 1] = vb.positionBias(1);
        fp[M3GNative.FP_POS_BIAS + 2] = vb.positionBias(2);

        float[] localRay = { o[0], o[1], o[2], d[0], d[1], d[2] };

        for (int sm = 0; sm < mesh.getSubmeshCount(); sm++) {
            IndexBuffer ib = mesh.getIndexBuffer(sm);
            ip[M3GNative.IP_STRIP_COUNT] = ib.stripLengths.length;
            ip[M3GNative.IP_INDEX_COUNT] = ib.getIndexCount();

            Object indices = ib.indices;
            if (indices == null && ib.firstIndex != 0) {
                int n = ib.getIndexCount();
                int[] idx = new int[n];
                for (int i = 0; i < n; i++) {
                    idx[i] = ib.firstIndex + i;
                }
                ib.indices = idx;
                indices = idx;
            }

            if (!M3GNative.nPick(pos.rawData(), ip, fp, indices,
                                 ib.stripLengths, localRay, outIdx, outHit)) {
                continue;
            }
            float t = outHit[0];
            if (found && t >= bestT) {
                continue;
            }
            found = true;
            bestT = t;
            bestMesh = mesh;
            bestSubmesh = sm;
            computeHitAttributes(vb, localToRoot, outIdx, outHit);
        }
    }

    private void computeHitAttributes(VertexBuffer vb, Transform localToRoot,
                                      int[] idx, float[] hit) {
        float u = hit[1];
        float v = hit[2];
        float w = 1.0f - u - v;

        /* Normal: interpolada si hay array, si no la de la cara. */
        VertexArray na = vb.getNormals();
        float nx;
        float ny;
        float nz;
        if (na != null) {
            nx = w * na.getFloat(idx[0], 0) + u * na.getFloat(idx[1], 0)
               + v * na.getFloat(idx[2], 0);
            ny = w * na.getFloat(idx[0], 1) + u * na.getFloat(idx[1], 1)
               + v * na.getFloat(idx[2], 1);
            nz = w * na.getFloat(idx[0], 2) + u * na.getFloat(idx[1], 2)
               + v * na.getFloat(idx[2], 2);
        } else {
            VertexArray pa = vb.getPositions(null);
            float ax = pa.getFloat(idx[1], 0) - pa.getFloat(idx[0], 0);
            float ay = pa.getFloat(idx[1], 1) - pa.getFloat(idx[0], 1);
            float az = pa.getFloat(idx[1], 2) - pa.getFloat(idx[0], 2);
            float bx = pa.getFloat(idx[2], 0) - pa.getFloat(idx[0], 0);
            float by = pa.getFloat(idx[2], 1) - pa.getFloat(idx[0], 1);
            float bz = pa.getFloat(idx[2], 2) - pa.getFloat(idx[0], 2);
            nx = ay * bz - az * by;
            ny = az * bx - ax * bz;
            nz = ax * by - ay * bx;
        }

        float[] n4 = { nx, ny, nz, 0.0f };
        localToRoot.transform(n4);
        float len = (float) Math.sqrt(n4[0] * n4[0] + n4[1] * n4[1]
                                    + n4[2] * n4[2]);
        if (len > 0.0f) {
            n4[0] /= len; n4[1] /= len; n4[2] /= len;
        }
        bestNormal[0] = n4[0];
        bestNormal[1] = n4[1];
        bestNormal[2] = n4[2];

        for (int unit = 0; unit < VertexBuffer.MAX_TEXTURE_UNITS; unit++) {
            float[] sb = new float[4];
            VertexArray tc = vb.getTexCoords(unit, sb);
            if (tc == null) {
                bestS[unit] = 0.0f;
                bestT2[unit] = 0.0f;
                continue;
            }
            float s = w * tc.getFloat(idx[0], 0) + u * tc.getFloat(idx[1], 0)
                    + v * tc.getFloat(idx[2], 0);
            float tt = w * tc.getFloat(idx[0], 1) + u * tc.getFloat(idx[1], 1)
                     + v * tc.getFloat(idx[2], 1);
            bestS[unit] = s * sb[0] + sb[1];
            bestT2[unit] = tt * sb[0] + sb[2];
        }
    }

    void store(RayIntersection ri) {
        ri.fill(bestMesh, bestT, bestSubmesh, bestNormal, ray, bestS, bestT2);
    }
}
