/*
 * MorphingMesh.java — JSR-184 (M3G 1.0).
 *
 * La deformacion se calcula en Java sobre un VertexBuffer de floats que
 * se reconstruye solo cuando cambian los pesos. Las mallas de morphing
 * de J2ME son de pocos cientos de vertices, asi que el coste es
 * despreciable frente al rasterizado.
 */
package javax.microedition.m3g;

public class MorphingMesh extends Mesh {

    private final VertexBuffer[] targets;
    private final float[] weights;
    private VertexBuffer deformed;
    private boolean dirty = true;

    public MorphingMesh(VertexBuffer base, VertexBuffer[] targets,
                        IndexBuffer submesh, Appearance appearance) {
        super(base, submesh, appearance);
        this.targets = checkTargets(base, targets);
        this.weights = new float[this.targets.length];
    }

    public MorphingMesh(VertexBuffer base, VertexBuffer[] targets,
                        IndexBuffer[] submeshes, Appearance[] appearances) {
        super(base, submeshes, appearances);
        this.targets = checkTargets(base, targets);
        this.weights = new float[this.targets.length];
    }

    private static VertexBuffer[] checkTargets(VertexBuffer base,
                                               VertexBuffer[] targets) {
        if (targets == null) {
            throw new NullPointerException();
        }
        if (targets.length == 0) {
            throw new IllegalArgumentException("sin targets de morphing");
        }
        for (int i = 0; i < targets.length; i++) {
            if (targets[i] == null) {
                throw new NullPointerException("target nulo");
            }
            if (targets[i].getVertexCount() != base.getVertexCount()) {
                throw new IllegalArgumentException(
                    "los targets deben tener el mismo numero de vertices");
            }
        }
        VertexBuffer[] c = new VertexBuffer[targets.length];
        System.arraycopy(targets, 0, c, 0, targets.length);
        return c;
    }

    public int getMorphTargetCount() {
        return targets.length;
    }

    public VertexBuffer getMorphTarget(int index) {
        if (index < 0 || index >= targets.length) {
            throw new IndexOutOfBoundsException();
        }
        return targets[index];
    }

    public void getWeights(float[] weights) {
        if (weights == null) {
            throw new NullPointerException();
        }
        if (weights.length < this.weights.length) {
            throw new IllegalArgumentException("weights demasiado corto");
        }
        System.arraycopy(this.weights, 0, weights, 0, this.weights.length);
    }

    public void setWeights(float[] weights) {
        if (weights == null) {
            throw new NullPointerException();
        }
        if (weights.length < this.weights.length) {
            throw new IllegalArgumentException("weights demasiado corto");
        }
        System.arraycopy(weights, 0, this.weights, 0, this.weights.length);
        dirty = true;
    }

    void updateProperty(int property, float[] value) {
        if (property == AnimationTrack.MORPH_WEIGHTS) {
            int n = Math.min(value.length, weights.length);
            System.arraycopy(value, 0, weights, 0, n);
            dirty = true;
        } else {
            super.updateProperty(property, value);
        }
    }

    VertexBuffer renderedVertexBuffer() {
        if (dirty) {
            rebuild();
            dirty = false;
        }
        return (deformed != null) ? deformed : vertices;
    }

    private void rebuild() {
        VertexBuffer base = vertices;
        int n = base.getVertexCount();
        if (n == 0) {
            return;
        }
        if (deformed == null) {
            deformed = new VertexBuffer();
            deformed.setDefaultColor(base.getDefaultColor());
        }

        VertexArray bp = base.getPositions(null);
        if (bp != null) {
            VertexArray out = new VertexArray(n, 3, 4);
            morph(bp, base.positionScale(), base, out, 0, true);
            deformed.setPositions(out, 1.0f, null);
        }

        VertexArray bn = base.getNormals();
        if (bn != null) {
            /* Las normales deben seguir siendo enteras (JSR-184), asi que
             * se mezclan y se re-cuantizan al rango de shorts. */
            VertexArray out = new VertexArray(n, 3, 2);
            short[] tmp = new short[n * 3];
            for (int v = 0; v < n; v++) {
                for (int c = 0; c < 3; c++) {
                    float acc = bn.getFloat(v, c);
                    float scaleBase = (bn.getComponentType() == 1)
                                    ? 1.0f / 127.0f : 1.0f / 32767.0f;
                    acc *= scaleBase;
                    for (int t = 0; t < targets.length; t++) {
                        VertexArray tn = targets[t].getNormals();
                        if (tn == null) {
                            continue;
                        }
                        float ts = (tn.getComponentType() == 1)
                                 ? 1.0f / 127.0f : 1.0f / 32767.0f;
                        acc += weights[t] * (tn.getFloat(v, c) * ts
                                             - bn.getFloat(v, c) * scaleBase);
                    }
                    int q = (int) (acc * 32767.0f);
                    if (q > 32767) {
                        q = 32767;
                    } else if (q < -32767) {
                        q = -32767;
                    }
                    tmp[v * 3 + c] = (short) q;
                }
            }
            out.set(0, n, tmp);
            deformed.setNormals(out);
        }

        deformed.setColors(base.getColors());
        for (int u = 0; u < VertexBuffer.MAX_TEXTURE_UNITS; u++) {
            float[] sb = new float[4];
            VertexArray tc = base.getTexCoords(u, sb);
            if (tc != null) {
                deformed.setTexCoords(u, tc, sb[0],
                    new float[] { sb[1], sb[2], sb[3] });
            }
        }
    }

    /** result = base + sum(w_i * (target_i - base)), en unidades absolutas. */
    private void morph(VertexArray bp, float baseScale, VertexBuffer base,
                       VertexArray out, int unused, boolean positions) {
        int n = bp.getVertexCount();
        float[] tmp = new float[n * 3];
        float[] sb = new float[4];
        for (int v = 0; v < n; v++) {
            for (int c = 0; c < 3; c++) {
                float b = bp.getFloat(v, c) * baseScale + base.positionBias(c);
                float acc = b;
                for (int t = 0; t < targets.length; t++) {
                    VertexArray tp = targets[t].getPositions(sb);
                    if (tp == null) {
                        continue;
                    }
                    float tv = tp.getFloat(v, c) * sb[0] + sb[1 + c];
                    acc += weights[t] * (tv - b);
                }
                tmp[v * 3 + c] = acc;
            }
        }
        out.set(0, n, tmp);
    }

    public int getReferences(Object3D[] references) {
        int n = super.getReferences(references);
        for (int i = 0; i < targets.length; i++) {
            if (references != null) {
                references[n] = targets[i];
            }
            n++;
        }
        return n;
    }

    Object3D createClone() {
        MorphingMesh c = new MorphingMesh(vertices, targets, submeshes,
                                          appearances);
        copyNodeTo(c);
        System.arraycopy(weights, 0, c.weights, 0, weights.length);
        return c;
    }
}
