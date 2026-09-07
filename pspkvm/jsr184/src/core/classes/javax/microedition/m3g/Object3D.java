/*
 * Object3D.java — JSR-184 (M3G 1.0). Raiz de la jerarquia.
 */
package javax.microedition.m3g;

import java.util.Vector;

public abstract class Object3D {

    private int userID;
    private Object userObject;
    private Vector tracks;          /* de AnimationTrack, perezoso */

    /** Marca de pasada para no animar dos veces un objeto compartido. */
    int animatePass;

    /** Contador global de pasadas de animacion. */
    static int currentAnimatePass;

    Object3D() {
    }

    public int getUserID() {
        return userID;
    }

    public void setUserID(int userID) {
        this.userID = userID;
    }

    public Object getUserObject() {
        return userObject;
    }

    public void setUserObject(Object userObject) {
        this.userObject = userObject;
    }

    public void addAnimationTrack(AnimationTrack animationTrack) {
        if (animationTrack == null) {
            throw new NullPointerException();
        }
        if (!isCompatible(animationTrack)) {
            throw new IllegalArgumentException(
                "propiedad de animacion no soportada por este objeto");
        }
        if (tracks == null) {
            tracks = new Vector();
        }
        if (tracks.contains(animationTrack)) {
            throw new IllegalArgumentException("track duplicado");
        }
        tracks.addElement(animationTrack);
    }

    public AnimationTrack getAnimationTrack(int index) {
        if (tracks == null || index < 0 || index >= tracks.size()) {
            throw new IndexOutOfBoundsException();
        }
        return (AnimationTrack) tracks.elementAt(index);
    }

    public int getAnimationTrackCount() {
        return (tracks == null) ? 0 : tracks.size();
    }

    public void removeAnimationTrack(AnimationTrack animationTrack) {
        if (tracks != null && animationTrack != null) {
            tracks.removeElement(animationTrack);
        }
    }

    /**
     * Aplica las animaciones de este objeto y de todos los objetos a los
     * que referencia. Devuelve la validez en milisegundos.
     */
    public final int animate(int time) {
        int pass = ++currentAnimatePass;
        return animateInternal(time, pass);
    }

    int animateInternal(int time, int pass) {
        if (animatePass == pass) {
            return Integer.MAX_VALUE;   /* ya visitado en esta pasada */
        }
        animatePass = pass;

        int validity = Integer.MAX_VALUE;

        if (tracks != null && tracks.size() > 0) {
            validity = applyTracks(time);
        }

        Object3D[] refs = new Object3D[getReferences(null)];
        if (refs.length > 0) {
            getReferences(refs);
            for (int i = 0; i < refs.length; i++) {
                if (refs[i] != null) {
                    int v = refs[i].animateInternal(time, pass);
                    if (v < validity) {
                        validity = v;
                    }
                }
            }
        }
        return validity;
    }

    /**
     * Mezcla todos los tracks por propiedad (suma ponderada normalizada)
     * y aplica el resultado con updateProperty().
     */
    private int applyTracks(int time) {
        int validity = Integer.MAX_VALUE;
        int n = tracks.size();
        boolean[] done = new boolean[n];

        for (int i = 0; i < n; i++) {
            if (done[i]) {
                continue;
            }
            AnimationTrack ti = (AnimationTrack) tracks.elementAt(i);
            int property = ti.getTargetProperty();
            int comps = ti.getKeyframeSequence().getComponentCount();
            float[] acc = new float[comps];
            float[] tmp = new float[comps];
            float weightSum = 0.0f;

            for (int j = i; j < n; j++) {
                AnimationTrack tj = (AnimationTrack) tracks.elementAt(j);
                if (tj.getTargetProperty() != property) {
                    continue;
                }
                done[j] = true;
                AnimationController ctrl = tj.getController();
                if (ctrl == null || !ctrl.isActive(time)) {
                    continue;
                }
                float w = ctrl.getWeight();
                if (w <= 0.0f) {
                    continue;
                }
                KeyframeSequence seq = tj.getKeyframeSequence();
                if (seq.getComponentCount() != comps) {
                    continue;
                }
                seq.sample(ctrl.getPosition(time), tmp);
                for (int c = 0; c < comps; c++) {
                    acc[c] += tmp[c] * w;
                }
                weightSum += w;

                int v = ctrl.validity(time, seq);
                if (v < validity) {
                    validity = v;
                }
            }

            if (weightSum > 0.0f) {
                if (property == AnimationTrack.ORIENTATION) {
                    /* Los cuaterniones se renormalizan, no se promedian. */
                    float len = 0.0f;
                    for (int c = 0; c < comps; c++) {
                        len += acc[c] * acc[c];
                    }
                    len = (float) Math.sqrt(len);
                    if (len > 0.0f) {
                        for (int c = 0; c < comps; c++) {
                            acc[c] /= len;
                        }
                    }
                } else {
                    for (int c = 0; c < comps; c++) {
                        acc[c] /= weightSum;
                    }
                }
                updateProperty(property, acc);
            }
        }
        return validity;
    }

    public Object3D find(int userID) {
        if (this.userID == userID) {
            return this;
        }
        Object3D[] refs = new Object3D[getReferences(null)];
        if (refs.length > 0) {
            getReferences(refs);
            for (int i = 0; i < refs.length; i++) {
                if (refs[i] != null) {
                    Object3D found = refs[i].find(userID);
                    if (found != null) {
                        return found;
                    }
                }
            }
        }
        return null;
    }

    /**
     * Rellena el array con los objetos directamente referenciados y
     * devuelve cuantos hay. Con references == null solo cuenta.
     */
    public int getReferences(Object3D[] references) {
        int n = 0;
        int count = getAnimationTrackCount();
        for (int i = 0; i < count; i++) {
            AnimationTrack t = (AnimationTrack) tracks.elementAt(i);
            if (references != null) {
                references[n] = t;
            }
            n++;
        }
        return n;
    }

    public final Object3D duplicate() {
        Object3D copy = createClone();
        copy.userID = userID;
        copy.userObject = userObject;
        if (tracks != null) {
            copy.tracks = new Vector();
            for (int i = 0; i < tracks.size(); i++) {
                copy.tracks.addElement(tracks.elementAt(i));
            }
        }
        return copy;
    }

    /** Copia superficial especifica de cada subclase. */
    abstract Object3D createClone();

    /** Aplica un valor animado; las subclases sobrescriben lo que soportan. */
    void updateProperty(int property, float[] value) {
    }

    /** true si esta clase soporta la propiedad del track dado. */
    boolean isCompatible(AnimationTrack track) {
        return true;
    }
}
