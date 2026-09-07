/*
 * Graphics3D.java — JSR-184 (M3G 1.0).
 *
 * Recorre el grafo de escena, ordena por capa de Appearance y empaqueta
 * cada submalla en los tres arrays que consume el motor nativo
 * (arrays de atributos, int[] de estado y float[] de estado).
 */
package javax.microedition.m3g;

import java.util.Hashtable;
import java.util.Vector;

public final class Graphics3D {

    public static final int ANTIALIAS  = 2;
    public static final int DITHER     = 4;
    public static final int TRUE_COLOR = 8;
    public static final int OVERWRITE  = 16;

    private static final int MAX_LIGHTS = M3GNative.MAX_LIGHTS;

    private static Graphics3D instance;

    private Object target;
    private boolean depthEnabled = true;
    private int hints;

    private int vpX;
    private int vpY;
    private int vpW = 1;
    private int vpH = 1;
    private float depthNear = 0.0f;
    private float depthFar = 1.0f;

    private Camera camera;
    private final Transform cameraToWorld = new Transform();
    private final float[] view = new float[16];
    private final float[] projection = new float[16];

    private final Light[] lights = new Light[MAX_LIGHTS];
    private final Transform[] lightTransforms = new Transform[MAX_LIGHTS];
    private int lightCount;

    /* Buffers reutilizados: evitan basura por submalla y por frame. */
    private final int[] ip = new int[M3GNative.IP_COUNT];
    private final float[] fp = new float[M3GNative.FP_COUNT];
    private final Object[] arrays = new Object[M3GNative.ARR_COUNT];
    private final Vector renderables = new Vector();

    private Graphics3D() {
        MathUtil.identity(view);
        MathUtil.identity(projection);
    }

    public static final Graphics3D getInstance() {
        if (instance == null) {
            instance = new Graphics3D();
        }
        return instance;
    }

    /* ------------------------------------------------------------ */
    /* Destino de render                                             */
    /* ------------------------------------------------------------ */

    public void bindTarget(Object target) {
        bindTarget(target, true, 0);
    }

    public void bindTarget(Object target, boolean depthBuffer, int hints) {
        if (target == null) {
            throw new NullPointerException();
        }
        if (this.target != null) {
            throw new IllegalStateException("ya hay un destino ligado");
        }
        if ((hints & ~(ANTIALIAS | DITHER | TRUE_COLOR | OVERWRITE)) != 0) {
            throw new IllegalArgumentException("hints desconocidos");
        }

        boolean ok;
        int w;
        int h;
        if (target instanceof Image2D) {
            Image2D img = (Image2D) target;
            int f = img.getFormat();
            if (f != Image2D.RGB && f != Image2D.RGBA) {
                throw new IllegalArgumentException(
                    "el destino Image2D debe ser RGB o RGBA");
            }
            if (!img.isMutable()) {
                throw new IllegalArgumentException("Image2D inmutable");
            }
            w = img.getWidth();
            h = img.getHeight();
            ok = M3GNative.nBindImage(img.argb, w, h);
        } else if (target instanceof javax.microedition.lcdui.Graphics) {
            ok = M3GNative.nBindGraphics(target);
            w = M3GNative.nGetTargetWidth();
            h = M3GNative.nGetTargetHeight();
        } else {
            throw new IllegalArgumentException(
                "destino no soportado: se espera Graphics o Image2D");
        }
        if (!ok) {
            throw new IllegalArgumentException("no se pudo ligar el destino");
        }

        this.target = target;
        this.depthEnabled = depthBuffer;
        this.hints = hints;
        setViewport(0, 0, w, h);
        setDepthRange(0.0f, 1.0f);
    }

    public void releaseTarget() {
        if (target != null) {
            if (target instanceof Image2D) {
                /* El motor rasteriza en RGB565: al soltar el destino hay
                 * que devolver los pixeles al int[] del Image2D. */
                M3GNative.nFlushImage(((Image2D) target).argb);
            }
            M3GNative.nReleaseTarget();
            target = null;
        }
    }

    public Object getTarget() {
        return target;
    }

    public int getHints() {
        return hints;
    }

    public boolean isDepthBufferEnabled() {
        return depthEnabled;
    }

    public void setViewport(int x, int y, int width, int height) {
        if (width <= 0 || height <= 0 || width > 1024 || height > 1024) {
            throw new IllegalArgumentException("viewport fuera de rango");
        }
        vpX = x;
        vpY = y;
        vpW = width;
        vpH = height;
        M3GNative.nSetViewport(x, y, width, height);
    }

    public int getViewportX()      { return vpX; }
    public int getViewportY()      { return vpY; }
    public int getViewportWidth()  { return vpW; }
    public int getViewportHeight() { return vpH; }

    public void setDepthRange(float near, float far) {
        if (near < 0.0f || near > 1.0f || far < 0.0f || far > 1.0f) {
            throw new IllegalArgumentException("rango de profundidad invalido");
        }
        depthNear = near;
        depthFar = far;
        M3GNative.nSetDepthRange(near, far);
    }

    public float getDepthRangeNear() { return depthNear; }
    public float getDepthRangeFar()  { return depthFar; }

    /* ------------------------------------------------------------ */
    /* Camara y luces                                                */
    /* ------------------------------------------------------------ */

    public void setCamera(Camera camera, Transform transform) {
        this.camera = camera;
        if (transform != null) {
            cameraToWorld.set(transform);
        } else {
            cameraToWorld.setIdentity();
        }
        Transform inv = new Transform(cameraToWorld);
        inv.invert();
        inv.get(view);
        if (camera != null) {
            Transform p = new Transform();
            camera.getProjection(p);
            p.get(projection);
        } else {
            MathUtil.identity(projection);
        }
    }

    public Camera getCamera(Transform transform) {
        if (transform != null) {
            transform.set(cameraToWorld);
        }
        return camera;
    }

    public int addLight(Light light, Transform transform) {
        if (light == null) {
            throw new NullPointerException();
        }
        if (lightCount >= MAX_LIGHTS) {
            /* M3G no limita el numero de luces del API, pero solo
             * MAX_LIGHTS afectan al render (igual que en GL ES). */
            return -1;
        }
        int index = lightCount++;
        lights[index] = light;
        lightTransforms[index] = (transform != null)
                               ? new Transform(transform) : new Transform();
        return index;
    }

    public void setLight(int index, Light light, Transform transform) {
        if (index < 0 || index >= lightCount) {
            throw new IndexOutOfBoundsException();
        }
        lights[index] = light;
        lightTransforms[index] = (transform != null)
                               ? new Transform(transform) : new Transform();
    }

    public Light getLight(int index, Transform transform) {
        if (index < 0 || index >= lightCount) {
            throw new IndexOutOfBoundsException();
        }
        if (transform != null && lightTransforms[index] != null) {
            transform.set(lightTransforms[index]);
        }
        return lights[index];
    }

    public int getLightCount() {
        return lightCount;
    }

    public void resetLights() {
        for (int i = 0; i < MAX_LIGHTS; i++) {
            lights[i] = null;
            lightTransforms[i] = null;
        }
        lightCount = 0;
    }

    /* ------------------------------------------------------------ */
    /* Borrado                                                       */
    /* ------------------------------------------------------------ */

    public void clear(Background background) {
        checkBound();
        if (background == null) {
            M3GNative.nClear(0xFF000000, true, depthEnabled, 1.0f);
            return;
        }
        boolean clearColor = background.isColorClearEnabled();
        boolean clearDepth = background.isDepthClearEnabled() && depthEnabled;
        M3GNative.nClear(background.getColor(), clearColor, clearDepth, 1.0f);

        Image2D img = background.getImage();
        if (clearColor && img != null) {
            M3GNative.nBlitBackground(img.argb, img.getWidth(), img.getHeight(),
                background.getCropX(), background.getCropY(),
                background.getCropWidth(), background.getCropHeight(),
                background.getImageModeX() == Background.REPEAT,
                background.getImageModeY() == Background.REPEAT);
        }
    }

    /* ------------------------------------------------------------ */
    /* Render                                                        */
    /* ------------------------------------------------------------ */

    public void render(World world) {
        checkBound();
        if (world == null) {
            throw new NullPointerException();
        }
        Camera cam = world.getActiveCamera();
        if (cam == null) {
            throw new IllegalStateException("el World no tiene camara activa");
        }
        Transform camToWorld = new Transform();
        if (!cam.getTransformTo(world, camToWorld)) {
            throw new IllegalStateException(
                "la camara activa no pertenece al World");
        }
        setCamera(cam, camToWorld);
        resetLights();

        renderables.removeAllElements();
        collect(world, new Transform(), 1.0f, true);
        sortByLayer();
        drawCollected();
    }

    public void render(Node node, Transform transform) {
        checkBound();
        if (node == null) {
            throw new NullPointerException();
        }
        if (!(node instanceof Mesh) && !(node instanceof Sprite3D)
            && !(node instanceof Group)) {
            throw new IllegalArgumentException(
                "solo se pueden renderizar Mesh, Sprite3D y Group");
        }
        renderables.removeAllElements();
        Transform base = (transform != null) ? new Transform(transform)
                                             : new Transform();
        collect(node, base, 1.0f, false);
        sortByLayer();
        drawCollected();
    }

    public void render(VertexBuffer vertices, IndexBuffer triangles,
                       Appearance appearance, Transform transform) {
        render(vertices, triangles, appearance, transform, -1);
    }

    public void render(VertexBuffer vertices, IndexBuffer triangles,
                       Appearance appearance, Transform transform, int scope) {
        checkBound();
        if (vertices == null || triangles == null) {
            throw new NullPointerException();
        }
        if (camera != null && (camera.getScope() & scope) == 0) {
            return;
        }
        float[] model = new float[16];
        if (transform != null) {
            transform.get(model);
        } else {
            MathUtil.identity(model);
        }
        drawSubmesh(vertices, triangles, appearance, model, 1.0f, scope);
    }

    /* --- recoleccion --- */

    private static final class Renderable {
        Node node;
        VertexBuffer vb;
        IndexBuffer ib;
        Appearance ap;
        float[] toWorld;
        float alpha;
        int layer;
        int scope;
    }

    private void collect(Node node, Transform toParent, float alphaFactor,
                         boolean gatherLights) {
        if (!node.isRenderingEnabled()) {
            return;
        }
        Transform composite = new Transform();
        node.getCompositeTransform(composite);
        Transform toWorld = new Transform(toParent);
        toWorld.postMultiply(composite);

        float alpha = alphaFactor * node.getAlphaFactor();

        if (gatherLights && node instanceof Light) {
            addLight((Light) node, toWorld);
            return;
        }

        if (camera != null && (node.getScope() & camera.getScope()) == 0) {
            return;
        }

        if (node instanceof Group) {
            Group g = (Group) node;
            int n = g.getChildCount();
            for (int i = 0; i < n; i++) {
                collect(g.getChild(i), toWorld, alpha, gatherLights);
            }
        } else if (node instanceof Mesh) {
            Mesh mesh = (Mesh) node;
            VertexBuffer vb = mesh.renderedVertexBuffer();
            float[] m = new float[16];
            toWorld.get(m);
            for (int i = 0; i < mesh.getSubmeshCount(); i++) {
                Renderable r = new Renderable();
                r.node = mesh;
                r.vb = vb;
                r.ib = mesh.getIndexBuffer(i);
                r.ap = mesh.getAppearance(i);
                r.toWorld = m;
                r.alpha = alpha;
                r.layer = (r.ap != null) ? r.ap.getLayer() : 0;
                r.scope = mesh.getScope();
                renderables.addElement(r);
            }
        } else if (node instanceof Sprite3D) {
            Sprite3D s = (Sprite3D) node;
            Renderable r = new Renderable();
            r.node = s;
            r.ap = s.getAppearance();
            float[] m = new float[16];
            toWorld.get(m);
            r.toWorld = m;
            r.alpha = alpha;
            r.layer = (r.ap != null) ? r.ap.getLayer() : 0;
            r.scope = s.getScope();
            renderables.addElement(r);
        }
    }

    /** Ordenacion estable por capa (insercion: las listas son cortas). */
    private void sortByLayer() {
        int n = renderables.size();
        for (int i = 1; i < n; i++) {
            Renderable key = (Renderable) renderables.elementAt(i);
            int j = i - 1;
            while (j >= 0
                   && ((Renderable) renderables.elementAt(j)).layer > key.layer) {
                renderables.setElementAt(renderables.elementAt(j), j + 1);
                j--;
            }
            renderables.setElementAt(key, j + 1);
        }
    }

    private void drawCollected() {
        int n = renderables.size();
        for (int i = 0; i < n; i++) {
            Renderable r = (Renderable) renderables.elementAt(i);
            if (r.vb != null) {
                drawSubmesh(r.vb, r.ib, r.ap, r.toWorld, r.alpha, r.scope);
            } else {
                drawSprite((Sprite3D) r.node, r.ap, r.toWorld, r.alpha);
            }
        }
        renderables.removeAllElements();
    }

    /* --- empaquetado y llamada al motor --- */

    private void drawSubmesh(VertexBuffer vb, IndexBuffer ib, Appearance ap,
                             float[] toWorld, float alphaFactor, int scope) {
        VertexArray pos = vb.getPositions(null);
        if (pos == null || ib == null) {
            return;
        }

        for (int i = 0; i < ip.length; i++) {
            ip[i] = 0;
        }
        for (int i = 0; i < fp.length; i++) {
            fp[i] = 0.0f;
        }
        for (int i = 0; i < arrays.length; i++) {
            arrays[i] = null;
        }

        ip[M3GNative.IP_VERTEX_COUNT]     = vb.getVertexCount();
        ip[M3GNative.IP_POS_COMPONENTS]   = pos.getComponentCount();
        ip[M3GNative.IP_POS_TYPE]         = pos.getComponentType();
        ip[M3GNative.IP_DEFAULT_COLOR]    = applyAlpha(vb.getDefaultColor(),
                                                       alphaFactor);
        arrays[M3GNative.ARR_POSITIONS]   = pos.rawData();
        fp[M3GNative.FP_POS_SCALE]        = vb.positionScale();
        fp[M3GNative.FP_POS_BIAS]         = vb.positionBias(0);
        fp[M3GNative.FP_POS_BIAS + 1]     = vb.positionBias(1);
        fp[M3GNative.FP_POS_BIAS + 2]     = vb.positionBias(2);

        VertexArray nrm = vb.getNormals();
        if (nrm != null) {
            ip[M3GNative.IP_NORMAL_TYPE] = nrm.getComponentType();
            arrays[M3GNative.ARR_NORMALS] = nrm.rawData();
        }

        VertexArray col = vb.getColors();
        if (col != null) {
            ip[M3GNative.IP_COLOR_COMPONENTS] = col.getComponentCount();
            arrays[M3GNative.ARR_COLORS] = col.rawData();
        }

        /* Indices y tiras. */
        arrays[M3GNative.ARR_STRIPS] = ib.stripLengths;
        ip[M3GNative.IP_STRIP_COUNT] = ib.stripLengths.length;
        ip[M3GNative.IP_INDEX_COUNT] = ib.getIndexCount();
        if (ib.indices != null) {
            arrays[M3GNative.ARR_INDICES] = ib.indices;
        } else if (ib.firstIndex != 0) {
            /* Tira implicita desplazada: se materializa una vez. */
            int n = ib.getIndexCount();
            int[] idx = new int[n];
            for (int i = 0; i < n; i++) {
                idx[i] = ib.firstIndex + i;
            }
            ib.indices = idx;
            arrays[M3GNative.ARR_INDICES] = idx;
        }

        packAppearance(ap, alphaFactor);

        /* modelView = view * toWorld */
        float[] mv = new float[16];
        MathUtil.mul(mv, view, toWorld);
        System.arraycopy(mv, 0, fp, M3GNative.FP_MODELVIEW, 16);
        System.arraycopy(projection, 0, fp, M3GNative.FP_PROJECTION, 16);

        packTexCoords(vb, ap);
        packLights(scope);

        M3GNative.nRender(arrays, ip, fp);
    }

    private void packTexCoords(VertexBuffer vb, Appearance ap) {
        for (int u = 0; u < VertexBuffer.MAX_TEXTURE_UNITS; u++) {
            int typeSlot = (u == 0) ? M3GNative.IP_TC0_TYPE
                                    : M3GNative.IP_TC1_TYPE;
            int compSlot = (u == 0) ? M3GNative.IP_TC0_COMPONENTS
                                    : M3GNative.IP_TC1_COMPONENTS;
            int scaleSlot = (u == 0) ? M3GNative.FP_TC0_SCALE
                                     : M3GNative.FP_TC1_SCALE;
            int biasSlot = (u == 0) ? M3GNative.FP_TC0_BIAS
                                    : M3GNative.FP_TC1_BIAS;
            int arrSlot = (u == 0) ? M3GNative.ARR_TC0 : M3GNative.ARR_TC1;

            VertexArray tc = vb.getTexCoords(u, null);
            if (tc == null) {
                continue;
            }
            ip[typeSlot] = tc.getComponentType();
            ip[compSlot] = tc.getComponentCount();
            arrays[arrSlot] = tc.rawData();
            fp[scaleSlot] = vb.texCoordScale(u);
            fp[biasSlot] = vb.texCoordBias(u, 0);
            fp[biasSlot + 1] = vb.texCoordBias(u, 1);
            fp[biasSlot + 2] = vb.texCoordBias(u, 2);
        }
    }

    private void packAppearance(Appearance ap, float alphaFactor) {
        /* Valores por defecto (Appearance nula = estado por defecto). */
        ip[M3GNative.IP_CULLING]     = PolygonMode.CULL_BACK;
        ip[M3GNative.IP_WINDING]     = PolygonMode.WINDING_CCW;
        ip[M3GNative.IP_SHADING]     = PolygonMode.SHADE_SMOOTH;
        ip[M3GNative.IP_PERSPECTIVE] = 1;
        ip[M3GNative.IP_BLENDING]    = CompositingMode.REPLACE;
        ip[M3GNative.IP_DEPTH_TEST]  = depthEnabled ? 1 : 0;
        ip[M3GNative.IP_DEPTH_WRITE] = depthEnabled ? 1 : 0;
        ip[M3GNative.IP_COLOR_WRITE] = 1;
        ip[M3GNative.IP_ALPHA_WRITE] = 1;
        MathUtil.identity(tmp16);
        System.arraycopy(tmp16, 0, fp, M3GNative.FP_TEX0_MATRIX, 16);
        System.arraycopy(tmp16, 0, fp, M3GNative.FP_TEX1_MATRIX, 16);

        if (ap == null) {
            return;
        }

        PolygonMode pm = ap.getPolygonMode();
        if (pm != null) {
            ip[M3GNative.IP_CULLING]   = pm.getCulling();
            ip[M3GNative.IP_WINDING]   = pm.getWinding();
            ip[M3GNative.IP_SHADING]   = pm.getShading();
            ip[M3GNative.IP_TWO_SIDED] = pm.isTwoSidedLightingEnabled() ? 1 : 0;
            ip[M3GNative.IP_PERSPECTIVE] =
                pm.isPerspectiveCorrectionEnabled() ? 1 : 0;
        }

        CompositingMode cm = ap.getCompositingMode();
        if (cm != null) {
            ip[M3GNative.IP_BLENDING]    = cm.getBlending();
            ip[M3GNative.IP_DEPTH_TEST]  =
                (depthEnabled && cm.isDepthTestEnabled()) ? 1 : 0;
            ip[M3GNative.IP_DEPTH_WRITE] =
                (depthEnabled && cm.isDepthWriteEnabled()) ? 1 : 0;
            ip[M3GNative.IP_COLOR_WRITE] = cm.isColorWriteEnabled() ? 1 : 0;
            ip[M3GNative.IP_ALPHA_WRITE] = cm.isAlphaWriteEnabled() ? 1 : 0;
            fp[M3GNative.FP_ALPHA_THRESHOLD] = cm.getAlphaThreshold();
            fp[M3GNative.FP_DEPTH_OFFSET_FACTOR] = cm.getDepthOffsetFactor();
            fp[M3GNative.FP_DEPTH_OFFSET_UNITS] = cm.getDepthOffsetUnits();
        }

        Fog fog = ap.getFog();
        if (fog != null) {
            ip[M3GNative.IP_FOG_MODE]  = fog.getMode();
            ip[M3GNative.IP_FOG_COLOR] = fog.getColor();
            fp[M3GNative.FP_FOG_DENSITY] = fog.getDensity();
            fp[M3GNative.FP_FOG_NEAR]    = fog.getNearDistance();
            fp[M3GNative.FP_FOG_FAR]     = fog.getFarDistance();
        }

        Material mat = ap.getMaterial();
        if (mat != null) {
            ip[M3GNative.IP_LIGHTING] = 1;
            ip[M3GNative.IP_VERTEX_COLOR_TRACK] =
                mat.isVertexColorTrackingEnabled() ? 1 : 0;
            unpackColor(mat.getColor(Material.AMBIENT),
                        M3GNative.FP_MAT_AMBIENT);
            unpackColor(applyAlpha(mat.getColor(Material.DIFFUSE), alphaFactor),
                        M3GNative.FP_MAT_DIFFUSE);
            unpackColor(mat.getColor(Material.EMISSIVE),
                        M3GNative.FP_MAT_EMISSIVE);
            unpackColor(mat.getColor(Material.SPECULAR),
                        M3GNative.FP_MAT_SPECULAR);
            fp[M3GNative.FP_MAT_SHININESS] = mat.getShininess();
        }

        for (int u = 0; u < VertexBuffer.MAX_TEXTURE_UNITS; u++) {
            Texture2D tex = ap.getTexture(u);
            if (tex == null || tex.getImage() == null) {
                continue;
            }
            Image2D img = tex.getImage();
            int enSlot = (u == 0) ? M3GNative.IP_TEX0_ENABLED
                                  : M3GNative.IP_TEX1_ENABLED;
            int wSlot = (u == 0) ? M3GNative.IP_TEX0_WIDTH
                                 : M3GNative.IP_TEX1_WIDTH;
            int hSlot = (u == 0) ? M3GNative.IP_TEX0_HEIGHT
                                 : M3GNative.IP_TEX1_HEIGHT;
            int sSlot = (u == 0) ? M3GNative.IP_TEX0_WRAP_S
                                 : M3GNative.IP_TEX1_WRAP_S;
            int tSlot = (u == 0) ? M3GNative.IP_TEX0_WRAP_T
                                 : M3GNative.IP_TEX1_WRAP_T;
            int fSlot = (u == 0) ? M3GNative.IP_TEX0_FILTER
                                 : M3GNative.IP_TEX1_FILTER;
            int bSlot = (u == 0) ? M3GNative.IP_TEX0_BLEND
                                 : M3GNative.IP_TEX1_BLEND;
            int bcSlot = (u == 0) ? M3GNative.IP_TEX0_BLEND_COLOR
                                  : M3GNative.IP_TEX1_BLEND_COLOR;
            int mSlot = (u == 0) ? M3GNative.FP_TEX0_MATRIX
                                 : M3GNative.FP_TEX1_MATRIX;
            int arrSlot = (u == 0) ? M3GNative.ARR_TEX0 : M3GNative.ARR_TEX1;

            ip[enSlot] = 1;
            ip[wSlot] = img.getWidth();
            ip[hSlot] = img.getHeight();
            ip[sSlot] = tex.getWrappingS();
            ip[tSlot] = tex.getWrappingT();
            ip[fSlot] = tex.getImageFilter();
            ip[bSlot] = tex.getBlending();
            ip[bcSlot] = tex.getBlendColor();
            arrays[arrSlot] = img.argb;

            Transform tt = new Transform();
            tex.getCompositeTransform(tt);
            tt.get(tmp16);
            System.arraycopy(tmp16, 0, fp, mSlot, 16);
        }
    }

    private final float[] tmp16 = new float[16];

    private void packLights(int scope) {
        int n = 0;
        float[] p = new float[4];
        float[] d = new float[4];
        float[] lm = new float[16];
        for (int i = 0; i < lightCount && n < MAX_LIGHTS; i++) {
            Light l = lights[i];
            if (l == null || !l.isRenderingEnabled()) {
                continue;
            }
            if ((l.getScope() & scope) == 0) {
                continue;
            }
            Transform t = lightTransforms[i];
            float[] toWorld = new float[16];
            if (t != null) {
                t.get(toWorld);
            } else {
                MathUtil.identity(toWorld);
            }
            MathUtil.mul(lm, view, toWorld);

            p[0] = 0.0f; p[1] = 0.0f; p[2] = 0.0f; p[3] = 1.0f;
            d[0] = 0.0f; d[1] = 0.0f; d[2] = -1.0f; d[3] = 0.0f;
            Transform lt = new Transform();
            lt.set(lm);
            lt.transform(p);
            lt.transform(d);
            float dl = (float) Math.sqrt(d[0] * d[0] + d[1] * d[1]
                                       + d[2] * d[2]);
            if (dl > 0.0f) {
                d[0] /= dl; d[1] /= dl; d[2] /= dl;
            }

            int base = M3GNative.FP_LIGHTS + n * M3GNative.LP_STRIDE;
            int rgb = l.getColor();
            float in = l.getIntensity();
            fp[base + M3GNative.LP_MODE] = l.getMode();
            fp[base + M3GNative.LP_R] = ((rgb >> 16) & 0xFF) / 255.0f * in;
            fp[base + M3GNative.LP_G] = ((rgb >> 8) & 0xFF) / 255.0f * in;
            fp[base + M3GNative.LP_B] = (rgb & 0xFF) / 255.0f * in;
            fp[base + M3GNative.LP_PX] = p[0];
            fp[base + M3GNative.LP_PY] = p[1];
            fp[base + M3GNative.LP_PZ] = p[2];
            fp[base + M3GNative.LP_DX] = d[0];
            fp[base + M3GNative.LP_DY] = d[1];
            fp[base + M3GNative.LP_DZ] = d[2];
            fp[base + M3GNative.LP_SPOT_COS] =
                (float) Math.cos(l.getSpotAngle() * Math.PI / 180.0);
            fp[base + M3GNative.LP_SPOT_EXP] = l.getSpotExponent();
            fp[base + M3GNative.LP_ATT_C] = l.getConstantAttenuation();
            fp[base + M3GNative.LP_ATT_L] = l.getLinearAttenuation();
            fp[base + M3GNative.LP_ATT_Q] = l.getQuadraticAttenuation();
            n++;
        }
        ip[M3GNative.IP_LIGHT_COUNT] = n;
    }

    private void unpackColor(int argb, int slot) {
        fp[slot]     = ((argb >> 16) & 0xFF) / 255.0f;
        fp[slot + 1] = ((argb >> 8) & 0xFF) / 255.0f;
        fp[slot + 2] = (argb & 0xFF) / 255.0f;
        fp[slot + 3] = ((argb >>> 24) & 0xFF) / 255.0f;
    }

    private static int applyAlpha(int argb, float factor) {
        if (factor >= 1.0f) {
            return argb;
        }
        int a = (int) (((argb >>> 24) & 0xFF) * factor);
        if (a < 0) {
            a = 0;
        } else if (a > 255) {
            a = 255;
        }
        return (a << 24) | (argb & 0x00FFFFFF);
    }

    /* --- Sprite3D --- */

    private final float[] spritePos = new float[16];   /* 4 vertices xyzw */
    private final float[] spriteTc  = new float[8];
    private final int[]   spriteStrip = { 4 };

    private void drawSprite(Sprite3D sprite, Appearance ap, float[] toWorld,
                            float alphaFactor) {
        Image2D img = sprite.getImage();
        if (img == null) {
            return;
        }
        float[] mv = new float[16];
        MathUtil.mul(mv, view, toWorld);

        /* Centro en espacio de camara. */
        float cx = mv[3];
        float cy = mv[7];
        float cz = mv[11];

        float halfW;
        float halfH;
        if (sprite.isScaled()) {
            /* El cuadrado unidad transformado da el tamano en mundo. */
            halfW = 0.5f * length3(mv[0], mv[4], mv[8]);
            halfH = 0.5f * length3(mv[1], mv[5], mv[9]);
        } else {
            /* Tamano en pixeles: se convierte a espacio de camara usando
             * la w del centro y la escala de la proyeccion. */
            /* w de recorte = fila 3 de la proyeccion (para perspectiva
             * sale -z; para paralela, 1). */
            float w = projection[12] * cx + projection[13] * cy
                    + projection[14] * cz + projection[15];
            if (w == 0.0f) {
                return;
            }
            float aw = (w < 0.0f) ? -w : w;
            int cw = sprite.getCropWidth();
            int ch = sprite.getCropHeight();
            float ndcW = (cw < 0 ? -cw : cw) / (float) vpW;
            float ndcH = (ch < 0 ? -ch : ch) / (float) vpH;
            halfW = (projection[0] != 0.0f) ? ndcW * aw / projection[0] : 0.0f;
            halfH = (projection[5] != 0.0f) ? ndcH * aw / projection[5] : 0.0f;
        }

        setSpriteVertex(0, cx - halfW, cy - halfH, cz);
        setSpriteVertex(1, cx + halfW, cy - halfH, cz);
        setSpriteVertex(2, cx - halfW, cy + halfH, cz);
        setSpriteVertex(3, cx + halfW, cy + halfH, cz);

        int iw = img.getWidth();
        int ih = img.getHeight();
        float u0 = sprite.getCropX() / (float) iw;
        float v0 = sprite.getCropY() / (float) ih;
        float u1 = (sprite.getCropX() + sprite.getCropWidth()) / (float) iw;
        float v1 = (sprite.getCropY() + sprite.getCropHeight()) / (float) ih;
        /* El eje V de la imagen crece hacia abajo; el quad se construye
         * con Y hacia arriba, asi que se invierte. */
        spriteTc[0] = u0; spriteTc[1] = v1;
        spriteTc[2] = u1; spriteTc[3] = v1;
        spriteTc[4] = u0; spriteTc[5] = v0;
        spriteTc[6] = u1; spriteTc[7] = v0;

        for (int i = 0; i < ip.length; i++) {
            ip[i] = 0;
        }
        for (int i = 0; i < fp.length; i++) {
            fp[i] = 0.0f;
        }
        for (int i = 0; i < arrays.length; i++) {
            arrays[i] = null;
        }

        ip[M3GNative.IP_VERTEX_COUNT] = 4;
        ip[M3GNative.IP_POS_COMPONENTS] = 4;
        ip[M3GNative.IP_POS_TYPE] = 4;
        ip[M3GNative.IP_DEFAULT_COLOR] = applyAlpha(0xFFFFFFFF, alphaFactor);
        ip[M3GNative.IP_STRIP_COUNT] = 1;
        ip[M3GNative.IP_INDEX_COUNT] = 4;
        ip[M3GNative.IP_TC0_TYPE] = 4;
        ip[M3GNative.IP_TC0_COMPONENTS] = 2;
        arrays[M3GNative.ARR_POSITIONS] = spritePos;
        arrays[M3GNative.ARR_TC0] = spriteTc;
        arrays[M3GNative.ARR_STRIPS] = spriteStrip;
        fp[M3GNative.FP_POS_SCALE] = 1.0f;
        fp[M3GNative.FP_TC0_SCALE] = 1.0f;

        packAppearance(ap, alphaFactor);
        /* Los sprites nunca se descartan por orientacion. */
        ip[M3GNative.IP_CULLING] = PolygonMode.CULL_NONE;
        ip[M3GNative.IP_LIGHTING] = 0;
        ip[M3GNative.IP_TEX0_ENABLED] = 1;
        ip[M3GNative.IP_TEX0_WIDTH] = iw;
        ip[M3GNative.IP_TEX0_HEIGHT] = ih;
        ip[M3GNative.IP_TEX0_WRAP_S] = Texture2D.WRAP_CLAMP;
        ip[M3GNative.IP_TEX0_WRAP_T] = Texture2D.WRAP_CLAMP;
        ip[M3GNative.IP_TEX0_FILTER] = Texture2D.FILTER_NEAREST;
        ip[M3GNative.IP_TEX0_BLEND] = Texture2D.FUNC_MODULATE;
        arrays[M3GNative.ARR_TEX0] = img.argb;
        MathUtil.identity(tmp16);
        System.arraycopy(tmp16, 0, fp, M3GNative.FP_TEX0_MATRIX, 16);

        /* El quad ya esta en espacio de camara: modelview identidad. */
        MathUtil.identity(tmp16);
        System.arraycopy(tmp16, 0, fp, M3GNative.FP_MODELVIEW, 16);
        System.arraycopy(projection, 0, fp, M3GNative.FP_PROJECTION, 16);

        M3GNative.nRender(arrays, ip, fp);
    }

    private void setSpriteVertex(int i, float x, float y, float z) {
        spritePos[i * 4] = x;
        spritePos[i * 4 + 1] = y;
        spritePos[i * 4 + 2] = z;
        spritePos[i * 4 + 3] = 1.0f;
    }

    private static float length3(float x, float y, float z) {
        return (float) Math.sqrt(x * x + y * y + z * z);
    }

    private void checkBound() {
        if (target == null) {
            throw new IllegalStateException("no hay destino ligado");
        }
    }

    /* ------------------------------------------------------------ */
    /* Propiedades de la implementacion                              */
    /* ------------------------------------------------------------ */

    public static final Hashtable getProperties() {
        Hashtable h = new Hashtable();
        h.put("supportAntialiasing", new Boolean(false));
        h.put("supportTrueColor", new Boolean(false));
        h.put("supportDithering", new Boolean(false));
        h.put("supportMipmapping", new Boolean(false));
        h.put("supportPerspectiveCorrection", new Boolean(true));
        h.put("supportLocalCameraLighting", new Boolean(false));
        h.put("maxLights", new Integer(MAX_LIGHTS));
        h.put("maxViewportWidth", new Integer(1024));
        h.put("maxViewportHeight", new Integer(1024));
        h.put("maxViewportDimension", new Integer(1024));
        h.put("maxTextureDimension", new Integer(256));
        h.put("maxSpriteCropDimension", new Integer(1024));
        h.put("maxTransformsPerVertex", new Integer(4));
        h.put("numTextureUnits", new Integer(
            VertexBuffer.MAX_TEXTURE_UNITS));
        h.put("m3gRelease", new Integer(1));
        return h;
    }
}
