/*
 * SkinnedMesh.java — JSR-184 (M3G 1.0).
 *
 * Skinning por CPU en Java: para cada vertice se acumula la suma
 * ponderada de (hueso -> malla actual) * (malla -> hueso en reposo).
 * Se recalcula una vez por frame (al pedir el VertexBuffer a dibujar).
 * Las mallas con esqueleto de J2ME rondan los cientos de vertices; si
 * algun contenido resultara pesado, el paso natural es mover este bucle
 * al motor nativo (ver README_VITA.txt).
 */
package javax.microedition.m3g;

import java.util.Vector;

public class SkinnedMesh extends Mesh {

    private static final class Bone {
        Node node;
        int weight;
        int firstVertex;
        int numVertices;
        Transform meshToBoneAtRest;   /* R_i */
    }

    private final Group skeleton;
    private final Vector bones = new Vector();
    private VertexBuffer deformed;

    public SkinnedMesh(VertexBuffer vertices, IndexBuffer submesh,
                       Appearance appearance, Group skeleton) {
        super(vertices, submesh, appearance);
        this.skeleton = attach(skeleton);
    }

    public SkinnedMesh(VertexBuffer vertices, IndexBuffer[] submeshes,
                       Appearance[] appearances, Group skeleton) {
        super(vertices, submeshes, appearances);
        this.skeleton = attach(skeleton);
    }

    private Group attach(Group skeleton) {
        if (skeleton == null) {
            throw new NullPointerException();
        }
        if (skeleton.getParent() != null) {
            throw new IllegalArgumentException("el esqueleto ya tiene padre");
        }
        skeleton.parent = this;
        return skeleton;
    }

    public Group getSkeleton() {
        return skeleton;
    }

    public void addTransform(Node bone, int weight, int firstVertex,
                             int numVertices) {
        if (bone == null) {
            throw new NullPointerException();
        }
        if (weight <= 0) {
            throw new IllegalArgumentException("weight <= 0");
        }
        if (firstVertex < 0 || numVertices <= 0
            || firstVertex + numVertices > 65535) {
            throw new IllegalArgumentException("rango de vertices invalido");
        }
        Transform rest = new Transform();
        if (!getTransformTo(bone, rest)) {
            throw new IllegalArgumentException(
                "el hueso no pertenece al esqueleto de esta malla");
        }
        Bone b = new Bone();
        b.node = bone;
        b.weight = weight;
        b.firstVertex = firstVertex;
        b.numVertices = numVertices;
        b.meshToBoneAtRest = rest;
        bones.addElement(b);
    }

    public void getBoneTransform(Node bone, Transform transform) {
        if (bone == null || transform == null) {
            throw new NullPointerException();
        }
        for (int i = 0; i < bones.size(); i++) {
            Bone b = (Bone) bones.elementAt(i);
            if (b.node == bone) {
                Transform t = new Transform(b.meshToBoneAtRest);
                t.invert();
                transform.set(t);
                return;
            }
        }
        throw new IllegalArgumentException("el hueso no esta asociado");
    }

    public int getBoneVertices(Node bone, int[] indices, float[] weights) {
        if (bone == null) {
            throw new NullPointerException();
        }
        int count = 0;
        for (int i = 0; i < bones.size(); i++) {
            Bone b = (Bone) bones.elementAt(i);
            if (b.node != bone) {
                continue;
            }
            for (int v = 0; v < b.numVertices; v++) {
                if (indices != null && count < indices.length) {
                    indices[count] = b.firstVertex + v;
                }
                if (weights != null && count < weights.length) {
                    weights[count] = normalizedWeight(b, b.firstVertex + v);
                }
                count++;
            }
        }
        return count;
    }

    private float normalizedWeight(Bone bone, int vertex) {
        float total = 0.0f;
        for (int i = 0; i < bones.size(); i++) {
            Bone b = (Bone) bones.elementAt(i);
            if (vertex >= b.firstVertex
                && vertex < b.firstVertex + b.numVertices) {
                total += b.weight;
            }
        }
        return (total > 0.0f) ? bone.weight / total : 0.0f;
    }

    VertexBuffer renderedVertexBuffer() {
        if (bones.size() == 0) {
            return vertices;
        }
        skin();
        return (deformed != null) ? deformed : vertices;
    }

    private void skin() {
        VertexBuffer base = vertices;
        VertexArray bp = base.getPositions(null);
        if (bp == null) {
            return;
        }
        int n = base.getVertexCount();

        /* Matriz por hueso: (hueso -> malla) * (malla -> hueso en reposo) */
        int nb = bones.size();
        float[][] mats = new float[nb][];
        for (int i = 0; i < nb; i++) {
            Bone b = (Bone) bones.elementAt(i);
            Transform boneToMesh = new Transform();
            if (!b.node.getTransformTo(this, boneToMesh)) {
                mats[i] = null;
                continue;
            }
            boneToMesh.postMultiply(b.meshToBoneAtRest);
            float[] m = new float[16];
            boneToMesh.get(m);
            mats[i] = m;
        }

        float scale = base.positionScale();
        float[] outPos = new float[n * 3];
        float[] totalW = new float[n];
        float[] v4 = new float[4];

        for (int i = 0; i < nb; i++) {
            Bone b = (Bone) bones.elementAt(i);
            float[] m = mats[i];
            if (m == null) {
                continue;
            }
            int end = b.firstVertex + b.numVertices;
            if (end > n) {
                end = n;
            }
            for (int v = b.firstVertex; v < end; v++) {
                v4[0] = bp.getFloat(v, 0) * scale + base.positionBias(0);
                v4[1] = bp.getFloat(v, 1) * scale + base.positionBias(1);
                v4[2] = bp.getFloat(v, 2) * scale + base.positionBias(2);
                v4[3] = 1.0f;
                float x = m[0] * v4[0] + m[1] * v4[1] + m[2] * v4[2] + m[3];
                float y = m[4] * v4[0] + m[5] * v4[1] + m[6] * v4[2] + m[7];
                float z = m[8] * v4[0] + m[9] * v4[1] + m[10] * v4[2] + m[11];
                float w = b.weight;
                outPos[v * 3]     += x * w;
                outPos[v * 3 + 1] += y * w;
                outPos[v * 3 + 2] += z * w;
                totalW[v] += w;
            }
        }

        for (int v = 0; v < n; v++) {
            if (totalW[v] > 0.0f) {
                outPos[v * 3]     /= totalW[v];
                outPos[v * 3 + 1] /= totalW[v];
                outPos[v * 3 + 2] /= totalW[v];
            } else {
                /* Vertice sin huesos: se queda en su posicion de reposo. */
                outPos[v * 3]     = bp.getFloat(v, 0) * scale + base.positionBias(0);
                outPos[v * 3 + 1] = bp.getFloat(v, 1) * scale + base.positionBias(1);
                outPos[v * 3 + 2] = bp.getFloat(v, 2) * scale + base.positionBias(2);
            }
        }

        if (deformed == null) {
            deformed = new VertexBuffer();
            deformed.setDefaultColor(base.getDefaultColor());
            deformed.setNormals(base.getNormals());
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
        VertexArray out = new VertexArray(n, 3, 4);
        out.set(0, n, outPos);
        deformed.setPositions(out, 1.0f, null);
    }

    public int getReferences(Object3D[] references) {
        int n = super.getReferences(references);
        if (skeleton != null) {
            if (references != null) {
                references[n] = skeleton;
            }
            n++;
        }
        return n;
    }

    Object3D createClone() {
        SkinnedMesh c = new SkinnedMesh(vertices, submeshes, appearances,
                                        (Group) skeleton.duplicate());
        copyNodeTo(c);
        for (int i = 0; i < bones.size(); i++) {
            Bone b = (Bone) bones.elementAt(i);
            Bone nb2 = new Bone();
            nb2.node = b.node;
            nb2.weight = b.weight;
            nb2.firstVertex = b.firstVertex;
            nb2.numVertices = b.numVertices;
            nb2.meshToBoneAtRest = new Transform(b.meshToBoneAtRest);
            c.bones.addElement(nb2);
        }
        return c;
    }
}
