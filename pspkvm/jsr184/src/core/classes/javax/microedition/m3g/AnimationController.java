/*
 * AnimationController.java — JSR-184 (M3G 1.0).
 */
package javax.microedition.m3g;

public class AnimationController extends Object3D {

    private float speed = 1.0f;
    private float weight = 1.0f;
    private int activeStart;
    private int activeEnd;
    private float refSequenceTime;
    private int refWorldTime;

    public AnimationController() {
    }

    public int getActiveIntervalStart() {
        return activeStart;
    }

    public int getActiveIntervalEnd() {
        return activeEnd;
    }

    public void setActiveInterval(int start, int end) {
        if (start > end) {
            throw new IllegalArgumentException("start > end");
        }
        activeStart = start;
        activeEnd = end;
    }

    public float getSpeed() {
        return speed;
    }

    public void setSpeed(float speed, int worldTime) {
        refSequenceTime = getPosition(worldTime);
        refWorldTime = worldTime;
        this.speed = speed;
    }

    public float getWeight() {
        return weight;
    }

    public void setWeight(float weight) {
        if (weight < 0.0f) {
            throw new IllegalArgumentException("weight < 0");
        }
        this.weight = weight;
    }

    public float getPosition(int worldTime) {
        return refSequenceTime + speed * (worldTime - refWorldTime);
    }

    public void setPosition(float sequenceTime, int worldTime) {
        refSequenceTime = sequenceTime;
        refWorldTime = worldTime;
    }

    public int getRefWorldTime() {
        return refWorldTime;
    }

    /** Intervalo activo vacio (start == end) significa "siempre activo". */
    boolean isActive(int worldTime) {
        if (activeStart == activeEnd) {
            return true;
        }
        return worldTime >= activeStart && worldTime < activeEnd;
    }

    /**
     * Cuanto tiempo (ms) sigue siendo valido el valor muestreado.
     * Conservador: un frame salvo que el intervalo activo termine antes.
     */
    int validity(int worldTime, KeyframeSequence seq) {
        if (activeStart != activeEnd) {
            if (worldTime < activeStart) {
                return activeStart - worldTime;
            }
            if (worldTime < activeEnd) {
                int left = activeEnd - worldTime;
                return (left < 1) ? 1 : left;
            }
            return Integer.MAX_VALUE;
        }
        return (speed == 0.0f) ? Integer.MAX_VALUE : 1;
    }

    Object3D createClone() {
        AnimationController c = new AnimationController();
        c.speed = speed;
        c.weight = weight;
        c.activeStart = activeStart;
        c.activeEnd = activeEnd;
        c.refSequenceTime = refSequenceTime;
        c.refWorldTime = refWorldTime;
        return c;
    }
}
