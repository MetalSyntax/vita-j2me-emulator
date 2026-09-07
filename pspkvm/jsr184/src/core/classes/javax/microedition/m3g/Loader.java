/*
 * Loader.java — JSR-184 (M3G 1.0): lector de ficheros .m3g y de imagenes.
 *
 * Implementa el formato binario del Apendice B de la especificacion:
 * cabecera de 12 bytes, secciones (sin comprimir o zlib) y objetos
 * indexados desde 1. La descompresion la hace el inflater nativo de
 * MIDP a traves de M3GNative.nInflate.
 */
package javax.microedition.m3g;

import java.io.IOException;
import java.io.InputStream;
import java.util.Vector;

import javax.microedition.lcdui.Image;

public final class Loader {

    /* Tipos de objeto del formato binario. */
    private static final int T_HEADER               = 0;
    private static final int T_ANIMATION_CONTROLLER = 1;
    private static final int T_ANIMATION_TRACK      = 2;
    private static final int T_APPEARANCE           = 3;
    private static final int T_BACKGROUND           = 4;
    private static final int T_CAMERA               = 5;
    private static final int T_COMPOSITING_MODE     = 6;
    private static final int T_FOG                  = 7;
    private static final int T_POLYGON_MODE         = 8;
    private static final int T_GROUP                = 9;
    private static final int T_IMAGE2D              = 10;
    private static final int T_TRIANGLE_STRIP_ARRAY = 11;
    private static final int T_LIGHT                = 12;
    private static final int T_MATERIAL             = 13;
    private static final int T_MESH                 = 14;
    private static final int T_MORPHING_MESH        = 15;
    private static final int T_SKINNED_MESH         = 16;
    private static final int T_TEXTURE2D            = 17;
    private static final int T_SPRITE3D             = 18;
    private static final int T_KEYFRAME_SEQUENCE    = 19;
    private static final int T_VERTEX_ARRAY         = 20;
    private static final int T_VERTEX_BUFFER        = 21;
    private static final int T_WORLD                = 22;
    private static final int T_EXTERNAL_REFERENCE   = 255;

    private static final byte[] M3G_ID = {
        (byte) 0xAB, 'J', 'S', 'R', '1', '8', '4', (byte) 0xBB,
        (byte) 0x0D, (byte) 0x0A, (byte) 0x1A, (byte) 0x0A
    };

    private static final byte[] PNG_ID = {
        (byte) 0x89, 'P', 'N', 'G', (byte) 0x0D, (byte) 0x0A,
        (byte) 0x1A, (byte) 0x0A
    };

    /** true si el Header Object ocupa un indice de objeto. */
    private final boolean headerIndexed;

    private Loader(boolean headerIndexed) {
        this.headerIndexed = headerIndexed;
    }

    public static Object3D[] load(byte[] data, int offset) {
        if (data == null) {
            throw new NullPointerException();
        }
        if (offset < 0 || offset >= data.length) {
            throw new IndexOutOfBoundsException("offset invalido");
        }
        if (matches(data, offset, PNG_ID)) {
            return loadImage(data, offset, data.length - offset);
        }
        if (!matches(data, offset, M3G_ID)) {
            /* Puede ser un JPEG u otro formato que MIDP sepa decodificar. */
            return loadImage(data, offset, data.length - offset);
        }
        return parseWithFallback(data, offset, null);
    }

    public static Object3D[] load(String name) {
        if (name == null) {
            throw new NullPointerException();
        }
        byte[] data = readAll(name);
        if (data == null) {
            throw new RuntimeException("no se pudo abrir: " + name);
        }
        if (matches(data, 0, PNG_ID) || !matches(data, 0, M3G_ID)) {
            return loadImage(data, 0, data.length);
        }
        return parseWithFallback(data, 0, name);
    }

    /*
     * El unico punto del formato binario que no se puede resolver sin un
     * fichero real delante es si el Header Object consume un indice de
     * objeto o no: si la convencion elegida fuera la contraria, TODAS las
     * referencias saldrian desplazadas en uno y el analisis reventaria
     * enseguida (cast invalido o indice adelantado). En vez de apostar,
     * se prueba la convencion mas probable y, si falla, la otra.
     */
    private static Object3D[] parseWithFallback(byte[] data, int offset,
                                                String name) {
        try {
            return new Loader(true).parse(data, offset, name);
        } catch (RuntimeException first) {
            try {
                return new Loader(false).parse(data, offset, name);
            } catch (RuntimeException second) {
                throw first;
            }
        }
    }

    private static boolean matches(byte[] data, int off, byte[] sig) {
        if (data.length - off < sig.length) {
            return false;
        }
        for (int i = 0; i < sig.length; i++) {
            if (data[off + i] != sig[i]) {
                return false;
            }
        }
        return true;
    }

    private static Object3D[] loadImage(byte[] data, int off, int len) {
        Image img = Image.createImage(data, off, len);
        boolean hasAlpha = matches(data, off, PNG_ID);
        Image2D i2d = new Image2D(hasAlpha ? Image2D.RGBA : Image2D.RGB, img);
        return new Object3D[] { i2d };
    }

    private static byte[] readAll(String name) {
        InputStream in = null;
        try {
            /* Fase 2b Vita: "Loader.class" no resuelve en javac -source
             * 1.4 (mismo bug del compilador ya visto en PiscesFont.java
             * e ImageDataFactory.java); se usa Class.forName por
             * nombre, que no dispara el helper sintético roto. */
            Class loaderClass;
            try {
                loaderClass = Class.forName("javax.microedition.m3g.Loader");
            } catch (ClassNotFoundException cnfe) {
                return null;
            }
            in = loaderClass.getResourceAsStream(name);
            if (in == null && !name.startsWith("/")) {
                in = loaderClass.getResourceAsStream("/" + name);
            }
            if (in == null) {
                return null;
            }
            byte[] buf = new byte[4096];
            byte[] out = new byte[0];
            int n;
            while ((n = in.read(buf)) > 0) {
                byte[] bigger = new byte[out.length + n];
                System.arraycopy(out, 0, bigger, 0, out.length);
                System.arraycopy(buf, 0, bigger, out.length, n);
                out = bigger;
            }
            return out;
        } catch (IOException e) {
            return null;
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e) {
                    /* ignorado: solo se estaba liberando el recurso */
                }
            }
        }
    }

    /* ------------------------------------------------------------ */
    /* Estado del analizador                                         */
    /* ------------------------------------------------------------ */

    private byte[] buf;
    private int pos;
    private final Vector objects = new Vector();   /* indice 0 = null */
    private boolean[] referenced;
    private final Vector referencedIdx = new Vector();
    private String baseName;

    private Object3D[] parse(byte[] data, int offset, String name) {
        baseName = name;
        objects.addElement(null);

        int p = offset + M3G_ID.length;
        while (p + 9 <= data.length) {
            int scheme = data[p] & 0xFF;
            int totalLen = readU32(data, p + 1);
            int uncompLen = readU32(data, p + 5);
            if (totalLen < 13 || p + totalLen > data.length) {
                throw new RuntimeException("seccion .m3g corrupta");
            }
            int payloadOff = p + 9;
            int payloadLen = totalLen - 13;

            byte[] section;
            if (scheme == 0) {
                section = new byte[payloadLen];
                System.arraycopy(data, payloadOff, section, 0, payloadLen);
            } else if (scheme == 1) {
                section = new byte[uncompLen];
                if (!M3GNative.nInflate(data, payloadOff, payloadLen,
                                        section, uncompLen)) {
                    throw new RuntimeException(
                        "no se pudo descomprimir una seccion .m3g");
                }
            } else {
                throw new RuntimeException(
                    "esquema de compresion .m3g desconocido: " + scheme);
            }

            parseSection(section);
            p += totalLen;
        }

        /* Raices = objetos que nadie referencia (y nunca la cabecera). */
        int n = objects.size();
        referenced = new boolean[n];
        for (int i = 0; i < referencedIdx.size(); i++) {
            int idx = ((Integer) referencedIdx.elementAt(i)).intValue();
            if (idx > 0 && idx < n) {
                referenced[idx] = true;
            }
        }
        Vector roots = new Vector();
        for (int i = 1; i < n; i++) {
            Object3D o = (Object3D) objects.elementAt(i);
            if (o != null && !referenced[i]) {
                roots.addElement(o);
            }
        }
        Object3D[] result = new Object3D[roots.size()];
        for (int i = 0; i < result.length; i++) {
            result[i] = (Object3D) roots.elementAt(i);
        }
        return result;
    }

    private void parseSection(byte[] section) {
        buf = section;
        pos = 0;
        while (pos + 5 <= buf.length) {
            int type = u8();
            int len = u32();
            int end = pos + len;
            if (end > buf.length) {
                throw new RuntimeException("objeto .m3g corrupto");
            }
            Object3D obj = readObject(type);
            if (type != T_HEADER || headerIndexed) {
                objects.addElement(obj);
            }
            pos = end;
        }
    }

    private Object3D readObject(int type) {
        switch (type) {
        case T_HEADER:
            /* version, external refs, tamanos: no aportan nada al grafo */
            return null;
        case T_EXTERNAL_REFERENCE:
            return readExternalReference();
        case T_ANIMATION_CONTROLLER: return readAnimationController();
        case T_ANIMATION_TRACK:      return readAnimationTrack();
        case T_APPEARANCE:           return readAppearance();
        case T_BACKGROUND:           return readBackground();
        case T_CAMERA:               return readCamera();
        case T_COMPOSITING_MODE:     return readCompositingMode();
        case T_FOG:                  return readFog();
        case T_POLYGON_MODE:         return readPolygonMode();
        case T_GROUP:                return readGroup(new Group());
        case T_IMAGE2D:              return readImage2D();
        case T_TRIANGLE_STRIP_ARRAY: return readTriangleStripArray();
        case T_LIGHT:                return readLight();
        case T_MATERIAL:             return readMaterial();
        case T_MESH:                 return readMesh(T_MESH);
        case T_MORPHING_MESH:        return readMesh(T_MORPHING_MESH);
        case T_SKINNED_MESH:         return readMesh(T_SKINNED_MESH);
        case T_TEXTURE2D:            return readTexture2D();
        case T_SPRITE3D:             return readSprite3D();
        case T_KEYFRAME_SEQUENCE:    return readKeyframeSequence();
        case T_VERTEX_ARRAY:         return readVertexArray();
        case T_VERTEX_BUFFER:        return readVertexBuffer();
        case T_WORLD:                return readWorld();
        default:
            /* Tipo desconocido: se ignora el cuerpo (pos ya salta al fin). */
            return null;
        }
    }

    private Object3D readExternalReference() {
        String uri = str();
        byte[] data = readAll(uri);
        if (data == null && baseName != null) {
            int slash = baseName.lastIndexOf('/');
            if (slash >= 0) {
                data = readAll(baseName.substring(0, slash + 1) + uri);
            }
        }
        if (data == null) {
            throw new RuntimeException("referencia externa no encontrada: " + uri);
        }
        Object3D[] r = (matches(data, 0, M3G_ID))
                     ? parseWithFallback(data, 0, uri)
                     : loadImage(data, 0, data.length);
        return (r.length > 0) ? r[0] : null;
    }

    /* ------------------------------------------------------------ */
    /* Lectura de tipos primitivos                                   */
    /* ------------------------------------------------------------ */

    private int u8() {
        return buf[pos++] & 0xFF;
    }

    private boolean bool() {
        return u8() != 0;
    }

    private int u16() {
        int v = (buf[pos] & 0xFF) | ((buf[pos + 1] & 0xFF) << 8);
        pos += 2;
        return v;
    }

    private int u32() {
        int v = readU32(buf, pos);
        pos += 4;
        return v;
    }

    private static int readU32(byte[] b, int p) {
        return (b[p] & 0xFF) | ((b[p + 1] & 0xFF) << 8)
             | ((b[p + 2] & 0xFF) << 16) | ((b[p + 3] & 0xFF) << 24);
    }

    private float f32() {
        return Float.intBitsToFloat(u32());
    }

    private String str() {
        /* UTF-8 terminado en cero. */
        int start = pos;
        while (pos < buf.length && buf[pos] != 0) {
            pos++;
        }
        String s;
        try {
            s = new String(buf, start, pos - start, "UTF-8");
        } catch (java.io.UnsupportedEncodingException e) {
            s = new String(buf, start, pos - start);
        }
        if (pos < buf.length) {
            pos++;       /* consume el terminador */
        }
        return s;
    }

    private int rgb() {
        int r = u8();
        int g = u8();
        int b = u8();
        return (r << 16) | (g << 8) | b;
    }

    private int rgba() {
        int r = u8();
        int g = u8();
        int b = u8();
        int a = u8();
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    private float[] vec3() {
        return new float[] { f32(), f32(), f32() };
    }

    private Transform matrix() {
        float[] m = new float[16];
        for (int i = 0; i < 16; i++) {
            m[i] = f32();
        }
        Transform t = new Transform();
        t.set(m);
        return t;
    }

    private Object3D ref() {
        int idx = u32();
        if (idx == 0) {
            return null;
        }
        referencedIdx.addElement(new Integer(idx));
        if (idx >= objects.size()) {
            throw new RuntimeException("referencia adelantada en .m3g: " + idx);
        }
        return (Object3D) objects.elementAt(idx);
    }

    /* ------------------------------------------------------------ */
    /* Lectura de cada clase                                         */
    /* ------------------------------------------------------------ */

    private void readObject3D(Object3D o) {
        int userID = u32();
        int trackCount = u32();
        Vector tracks = new Vector();
        for (int i = 0; i < trackCount; i++) {
            Object3D t = ref();
            if (t instanceof AnimationTrack) {
                tracks.addElement(t);
            }
        }
        int paramCount = u32();
        for (int i = 0; i < paramCount; i++) {
            u32();                       /* parameterID */
            int len = u32();
            pos += len;                  /* valor: no lo usa esta impl. */
        }
        if (o != null) {
            o.setUserID(userID);
            for (int i = 0; i < tracks.size(); i++) {
                try {
                    o.addAnimationTrack((AnimationTrack) tracks.elementAt(i));
                } catch (IllegalArgumentException e) {
                    /* Track incompatible con el objeto: el fichero manda
                     * pero no podemos aplicarlo; se ignora. */
                }
            }
        }
    }

    private void readTransformable(Transformable t) {
        readObject3D(t);
        if (bool()) {
            float[] tr = vec3();
            float[] sc = vec3();
            float angle = f32();
            float[] axis = vec3();
            t.setTranslation(tr[0], tr[1], tr[2]);
            t.setScale(sc[0], sc[1], sc[2]);
            if (axis[0] != 0.0f || axis[1] != 0.0f || axis[2] != 0.0f) {
                t.setOrientation(angle, axis[0], axis[1], axis[2]);
            }
        }
        if (bool()) {
            t.setTransform(matrix());
        }
    }

    private void readNode(Node n) {
        readTransformable(n);
        n.setRenderingEnable(bool());
        n.setPickingEnable(bool());
        n.setAlphaFactor((u8() & 0xFF) / 255.0f);
        n.setScope(u32());
        if (bool()) {
            int zTarget = u8();
            int yTarget = u8();
            Object3D zRef = ref();
            Object3D yRef = ref();
            n.setAlignment((Node) zRef, zTarget, (Node) yRef, yTarget);
        }
    }

    private Object3D readGroup(Group g) {
        readNode(g);
        int count = u32();
        for (int i = 0; i < count; i++) {
            Object3D c = ref();
            if (c instanceof Node) {
                g.addChild((Node) c);
            }
        }
        return g;
    }

    private Object3D readWorld() {
        World w = new World();
        readGroup(w);
        Object3D cam = ref();
        Object3D bg = ref();
        if (cam instanceof Camera) {
            w.setActiveCamera((Camera) cam);
        }
        if (bg instanceof Background) {
            w.setBackground((Background) bg);
        }
        return w;
    }

    private Object3D readCamera() {
        Camera c = new Camera();
        readNode(c);
        int type = u8();
        if (type == Camera.GENERIC) {
            c.setGeneric(matrix());
        } else {
            float fovy = f32();
            float aspect = f32();
            float near = f32();
            float far = f32();
            if (type == Camera.PARALLEL) {
                c.setParallel(fovy, aspect, near, far);
            } else {
                c.setPerspective(fovy, aspect, near, far);
            }
        }
        return c;
    }

    private Object3D readLight() {
        Light l = new Light();
        readNode(l);
        float ac = f32();
        float al = f32();
        float aq = f32();
        l.setColor(rgb());
        l.setMode(u8());
        l.setIntensity(f32());
        l.setSpotAngle(f32());
        l.setSpotExponent(f32());
        if (ac != 0.0f || al != 0.0f || aq != 0.0f) {
            l.setAttenuation(ac, al, aq);
        }
        return l;
    }

    private Object3D readMesh(int type) {
        /* Las tres clases de malla comparten cabecera, pero los
         * constructores exigen los datos completos: se acumulan primero
         * los campos de Node y luego se construye. */
        MeshHeader mh = new MeshHeader();
        readNodeInto(mh);
        mh.vertices = (VertexBuffer) ref();
        int submeshCount = u32();
        mh.submeshes = new IndexBuffer[submeshCount];
        mh.appearances = new Appearance[submeshCount];
        for (int i = 0; i < submeshCount; i++) {
            mh.submeshes[i] = (IndexBuffer) ref();
            mh.appearances[i] = (Appearance) ref();
        }

        Mesh mesh;
        if (type == T_MORPHING_MESH) {
            int n = u32();
            VertexBuffer[] targets = new VertexBuffer[n];
            float[] weights = new float[n];
            for (int i = 0; i < n; i++) {
                targets[i] = (VertexBuffer) ref();
                weights[i] = f32();
            }
            MorphingMesh mm = new MorphingMesh(mh.vertices, targets,
                                               mh.submeshes, mh.appearances);
            mm.setWeights(weights);
            mesh = mm;
        } else if (type == T_SKINNED_MESH) {
            Object3D skel = ref();
            SkinnedMesh sm = new SkinnedMesh(mh.vertices, mh.submeshes,
                                             mh.appearances, (Group) skel);
            int n = u32();
            for (int i = 0; i < n; i++) {
                Object3D bone = ref();
                int firstVertex = u32();
                int vertexCount = u32();
                int weight = u32();
                if (bone instanceof Node && weight > 0 && vertexCount > 0) {
                    sm.addTransform((Node) bone, weight, firstVertex,
                                    vertexCount);
                }
            }
            mesh = sm;
        } else {
            mesh = new Mesh(mh.vertices, mh.submeshes, mh.appearances);
        }
        mh.applyTo(mesh);
        return mesh;
    }

    /** Portador de los campos de Node leidos antes de crear la malla. */
    private static final class MeshHeader {
        int userID;
        Vector tracks = new Vector();
        float[] translation;
        float[] scale;
        float angle;
        float[] axis;
        Transform generic;
        boolean rendering = true;
        boolean picking = true;
        float alpha = 1.0f;
        int scope = -1;
        VertexBuffer vertices;
        IndexBuffer[] submeshes;
        Appearance[] appearances;

        void applyTo(Node n) {
            n.setUserID(userID);
            for (int i = 0; i < tracks.size(); i++) {
                try {
                    n.addAnimationTrack((AnimationTrack) tracks.elementAt(i));
                } catch (IllegalArgumentException e) {
                    /* incompatible: se ignora, ver readObject3D */
                }
            }
            if (translation != null) {
                n.setTranslation(translation[0], translation[1], translation[2]);
                n.setScale(scale[0], scale[1], scale[2]);
                if (axis[0] != 0.0f || axis[1] != 0.0f || axis[2] != 0.0f) {
                    n.setOrientation(angle, axis[0], axis[1], axis[2]);
                }
            }
            if (generic != null) {
                n.setTransform(generic);
            }
            n.setRenderingEnable(rendering);
            n.setPickingEnable(picking);
            n.setAlphaFactor(alpha);
            n.setScope(scope);
        }
    }

    private void readNodeInto(MeshHeader mh) {
        mh.userID = u32();
        int trackCount = u32();
        for (int i = 0; i < trackCount; i++) {
            Object3D t = ref();
            if (t instanceof AnimationTrack) {
                mh.tracks.addElement(t);
            }
        }
        int paramCount = u32();
        for (int i = 0; i < paramCount; i++) {
            u32();
            int len = u32();
            pos += len;
        }
        if (bool()) {
            mh.translation = vec3();
            mh.scale = vec3();
            mh.angle = f32();
            mh.axis = vec3();
        }
        if (bool()) {
            mh.generic = matrix();
        }
        mh.rendering = bool();
        mh.picking = bool();
        mh.alpha = (u8() & 0xFF) / 255.0f;
        mh.scope = u32();
        if (bool()) {
            u8();       /* zTarget */
            u8();       /* yTarget */
            ref();      /* zReference */
            ref();      /* yReference */
        }
    }

    private Object3D readSprite3D() {
        /* Igual que las mallas: hay que leer la cabecera antes de poder
         * construir el objeto (el constructor exige la imagen). */
        MeshHeader mh = new MeshHeader();
        readNodeInto(mh);
        Object3D img = ref();
        Object3D ap = ref();
        int cropX = u32();
        int cropY = u32();
        int cropW = u32();
        int cropH = u32();
        /* El formato binario de Sprite3D no guarda el flag "scaled";
         * se asume escalado, que es el modo habitual del contenido. */
        Sprite3D s = new Sprite3D(true, (Image2D) img, (Appearance) ap);
        mh.applyTo(s);
        s.setCrop(cropX, cropY, cropW, cropH);
        return s;
    }

    private Object3D readAppearance() {
        Appearance a = new Appearance();
        readObject3D(a);
        a.setLayer(buf[pos++]);          /* Byte con signo */
        a.setCompositingMode((CompositingMode) ref());
        a.setFog((Fog) ref());
        a.setPolygonMode((PolygonMode) ref());
        a.setMaterial((Material) ref());
        int n = u32();
        for (int i = 0; i < n; i++) {
            Object3D t = ref();
            if (i < VertexBuffer.MAX_TEXTURE_UNITS) {
                a.setTexture(i, (Texture2D) t);
            }
        }
        return a;
    }

    private Object3D readCompositingMode() {
        CompositingMode c = new CompositingMode();
        readObject3D(c);
        c.setDepthTestEnable(bool());
        c.setDepthWriteEnable(bool());
        c.setColorWriteEnable(bool());
        c.setAlphaWriteEnable(bool());
        c.setBlending(u8());
        c.setAlphaThreshold((u8() & 0xFF) / 255.0f);
        float factor = f32();
        float units = f32();
        c.setDepthOffset(factor, units);
        return c;
    }

    private Object3D readPolygonMode() {
        PolygonMode p = new PolygonMode();
        readObject3D(p);
        p.setCulling(u8());
        p.setShading(u8());
        p.setWinding(u8());
        p.setTwoSidedLightingEnable(bool());
        p.setLocalCameraLightingEnable(bool());
        p.setPerspectiveCorrectionEnable(bool());
        return p;
    }

    private Object3D readMaterial() {
        Material m = new Material();
        readObject3D(m);
        m.setColor(Material.AMBIENT, rgb());
        m.setColor(Material.DIFFUSE, rgba());
        m.setColor(Material.EMISSIVE, rgb());
        m.setColor(Material.SPECULAR, rgb());
        m.setShininess(f32());
        m.setVertexColorTrackingEnable(bool());
        return m;
    }

    private Object3D readFog() {
        Fog f = new Fog();
        readObject3D(f);
        f.setColor(rgb());
        int mode = u8();
        f.setMode(mode);
        if (mode == Fog.EXPONENTIAL) {
            f.setDensity(f32());
        } else {
            f.setLinear(f32(), f32());
        }
        return f;
    }

    private Object3D readBackground() {
        Background b = new Background();
        readObject3D(b);
        b.setColor(rgba());
        Object3D img = ref();
        if (img instanceof Image2D) {
            b.setImage((Image2D) img);
        }
        int modeX = u8();
        int modeY = u8();
        b.setImageMode(modeX, modeY);
        int cropX = u32();
        int cropY = u32();
        int cropW = u32();
        int cropH = u32();
        b.setCrop(cropX, cropY, cropW, cropH);
        b.setDepthClearEnable(bool());
        b.setColorClearEnable(bool());
        return b;
    }

    private Object3D readTexture2D() {
        /* El constructor exige la imagen, que va despues de la parte
         * Transformable: se lee a mano y se aplica al final. */
        int userID = u32();
        int trackCount = u32();
        Vector tracks = new Vector();
        for (int i = 0; i < trackCount; i++) {
            Object3D t = ref();
            if (t instanceof AnimationTrack) {
                tracks.addElement(t);
            }
        }
        int paramCount = u32();
        for (int i = 0; i < paramCount; i++) {
            u32();
            int len = u32();
            pos += len;
        }
        float[] tr = null;
        float[] sc = null;
        float angle = 0.0f;
        float[] axis = null;
        Transform generic = null;
        if (bool()) {
            tr = vec3();
            sc = vec3();
            angle = f32();
            axis = vec3();
        }
        if (bool()) {
            generic = matrix();
        }

        Image2D img = (Image2D) ref();
        Texture2D tex = new Texture2D(img);
        tex.setUserID(userID);
        for (int i = 0; i < tracks.size(); i++) {
            try {
                tex.addAnimationTrack((AnimationTrack) tracks.elementAt(i));
            } catch (IllegalArgumentException e) {
                /* incompatible: se ignora */
            }
        }
        if (tr != null) {
            tex.setTranslation(tr[0], tr[1], tr[2]);
            tex.setScale(sc[0], sc[1], sc[2]);
            if (axis[0] != 0.0f || axis[1] != 0.0f || axis[2] != 0.0f) {
                tex.setOrientation(angle, axis[0], axis[1], axis[2]);
            }
        }
        if (generic != null) {
            tex.setTransform(generic);
        }
        tex.setBlendColor(rgb());
        tex.setBlending(u8());
        int wrapS = u8();
        int wrapT = u8();
        tex.setWrapping(wrapS, wrapT);
        int levelFilter = u8();
        int imageFilter = u8();
        tex.setFiltering(levelFilter, imageFilter);
        return tex;
    }

    private Object3D readImage2D() {
        /* El formato y el tamano van antes que los pixeles, pero el
         * constructor los necesita todos a la vez. */
        int userID = u32();
        int trackCount = u32();
        for (int i = 0; i < trackCount; i++) {
            ref();
        }
        int paramCount = u32();
        for (int i = 0; i < paramCount; i++) {
            u32();
            int len = u32();
            pos += len;
        }

        int format = u8();
        boolean isMutable = bool();
        int width = u32();
        int height = u32();

        Image2D img;
        if (isMutable) {
            img = new Image2D(format, width, height);
        } else {
            int paletteLen = u32();
            byte[] palette = new byte[paletteLen];
            System.arraycopy(buf, pos, palette, 0, paletteLen);
            pos += paletteLen;
            int pixelLen = u32();
            byte[] pixels = new byte[pixelLen];
            System.arraycopy(buf, pos, pixels, 0, pixelLen);
            pos += pixelLen;
            if (paletteLen > 0) {
                img = new Image2D(format, width, height, pixels, palette);
            } else {
                img = new Image2D(format, width, height, pixels);
            }
        }
        img.setUserID(userID);
        return img;
    }

    private Object3D readVertexArray() {
        int userID = u32();
        int trackCount = u32();
        for (int i = 0; i < trackCount; i++) {
            ref();
        }
        int paramCount = u32();
        for (int i = 0; i < paramCount; i++) {
            u32();
            int len = u32();
            pos += len;
        }

        int componentSize = u8();
        int componentCount = u8();
        int encoding = u8();
        int vertexCount = u16();

        VertexArray va = new VertexArray(vertexCount, componentCount,
                                         componentSize);
        int total = vertexCount * componentCount;
        if (componentSize == 1) {
            byte[] v = new byte[total];
            if (encoding == 0) {
                for (int i = 0; i < total; i++) {
                    v[i] = buf[pos++];
                }
            } else {
                byte[] prev = new byte[componentCount];
                for (int i = 0; i < vertexCount; i++) {
                    for (int c = 0; c < componentCount; c++) {
                        prev[c] = (byte) (prev[c] + buf[pos++]);
                        v[i * componentCount + c] = prev[c];
                    }
                }
            }
            va.set(0, vertexCount, v);
        } else {
            short[] v = new short[total];
            if (encoding == 0) {
                for (int i = 0; i < total; i++) {
                    v[i] = (short) u16();
                }
            } else {
                short[] prev = new short[componentCount];
                for (int i = 0; i < vertexCount; i++) {
                    for (int c = 0; c < componentCount; c++) {
                        prev[c] = (short) (prev[c] + (short) u16());
                        v[i * componentCount + c] = prev[c];
                    }
                }
            }
            va.set(0, vertexCount, v);
        }
        va.setUserID(userID);
        return va;
    }

    private Object3D readVertexBuffer() {
        VertexBuffer vb = new VertexBuffer();
        readObject3D(vb);
        vb.setDefaultColor(rgba());
        VertexArray positions = (VertexArray) ref();
        float[] posBias = vec3();
        float posScale = f32();
        VertexArray normals = (VertexArray) ref();
        VertexArray colors = (VertexArray) ref();
        int texCount = u32();
        VertexArray[] tc = new VertexArray[texCount];
        float[][] tcBias = new float[texCount][];
        float[] tcScale = new float[texCount];
        for (int i = 0; i < texCount; i++) {
            tc[i] = (VertexArray) ref();
            tcBias[i] = vec3();
            tcScale[i] = f32();
        }
        if (positions != null) {
            vb.setPositions(positions, posScale, posBias);
        }
        if (normals != null) {
            vb.setNormals(normals);
        }
        if (colors != null) {
            vb.setColors(colors);
        }
        for (int i = 0; i < texCount && i < VertexBuffer.MAX_TEXTURE_UNITS;
             i++) {
            if (tc[i] != null) {
                vb.setTexCoords(i, tc[i], tcScale[i], tcBias[i]);
            }
        }
        return vb;
    }

    private Object3D readTriangleStripArray() {
        int userID = u32();
        int trackCount = u32();
        for (int i = 0; i < trackCount; i++) {
            ref();
        }
        int paramCount = u32();
        for (int i = 0; i < paramCount; i++) {
            u32();
            int len = u32();
            pos += len;
        }

        int encoding = u8();
        int firstIndex = 0;
        int[] indices = null;
        if (encoding == 0) {
            firstIndex = u32();
        } else if (encoding == 1) {
            firstIndex = u8();
        } else if (encoding == 2) {
            firstIndex = u16();
        } else {
            int n = u32();
            indices = new int[n];
            for (int i = 0; i < n; i++) {
                if (encoding == 128) {
                    indices[i] = u32();
                } else if (encoding == 129) {
                    indices[i] = u8();
                } else {
                    indices[i] = u16();
                }
            }
        }
        int stripCount = u32();
        int[] strips = new int[stripCount];
        for (int i = 0; i < stripCount; i++) {
            strips[i] = u32();
        }

        TriangleStripArray tsa = (indices != null)
            ? new TriangleStripArray(indices, strips)
            : new TriangleStripArray(firstIndex, strips);
        tsa.setUserID(userID);
        return tsa;
    }

    private Object3D readKeyframeSequence() {
        int userID = u32();
        int trackCount = u32();
        for (int i = 0; i < trackCount; i++) {
            ref();
        }
        int paramCount = u32();
        for (int i = 0; i < paramCount; i++) {
            u32();
            int len = u32();
            pos += len;
        }

        int interpolation = u8();
        int repeatMode = u8();
        int encoding = u8();
        int duration = u32();
        int rangeFirst = u32();
        int rangeLast = u32();
        int componentCount = u32();
        int keyframeCount = u32();

        KeyframeSequence ks = new KeyframeSequence(keyframeCount,
                                                   componentCount,
                                                   interpolation);
        float[] value = new float[componentCount];
        if (encoding == 0) {
            for (int i = 0; i < keyframeCount; i++) {
                int time = u32();
                for (int c = 0; c < componentCount; c++) {
                    value[c] = f32();
                }
                ks.setKeyframe(i, time, value);
            }
        } else {
            float[] bias = new float[componentCount];
            float[] scale = new float[componentCount];
            for (int c = 0; c < componentCount; c++) {
                bias[c] = f32();
            }
            for (int c = 0; c < componentCount; c++) {
                scale[c] = f32();
            }
            for (int i = 0; i < keyframeCount; i++) {
                int time = u32();
                for (int c = 0; c < componentCount; c++) {
                    int raw = (encoding == 1) ? u8() : u16();
                    float norm = (encoding == 1) ? raw / 255.0f
                                                 : raw / 65535.0f;
                    value[c] = bias[c] + norm * scale[c];
                }
                ks.setKeyframe(i, time, value);
            }
        }
        if (duration > 0) {
            ks.setDuration(duration);
        }
        ks.setRepeatMode(repeatMode);
        ks.setValidRange(rangeFirst, rangeLast);
        ks.setUserID(userID);
        return ks;
    }

    private Object3D readAnimationTrack() {
        int userID = u32();
        int trackCount = u32();
        for (int i = 0; i < trackCount; i++) {
            ref();
        }
        int paramCount = u32();
        for (int i = 0; i < paramCount; i++) {
            u32();
            int len = u32();
            pos += len;
        }
        KeyframeSequence seq = (KeyframeSequence) ref();
        AnimationController ctrl = (AnimationController) ref();
        int property = u32();
        AnimationTrack t = new AnimationTrack(seq, property);
        t.setController(ctrl);
        t.setUserID(userID);
        return t;
    }

    private Object3D readAnimationController() {
        AnimationController c = new AnimationController();
        readObject3D(c);
        float speed = f32();
        c.setWeight(f32());
        int start = u32();
        int end = u32();
        float refSeqTime = f32();
        int refWorldTime = u32();
        if (start <= end) {
            c.setActiveInterval(start, end);
        }
        c.setPosition(refSeqTime, refWorldTime);
        c.setSpeed(speed, refWorldTime);
        return c;
    }
}
