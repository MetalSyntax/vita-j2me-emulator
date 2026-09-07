JSR-184 (M3G 1.0) para PS Vita — Fase 5 de vita-j2me-emulator
==============================================================

Que es esto
-----------
Implementacion completa y autocontenida de javax.microedition.m3g para
el port de PSPKVM/phoneME a PS Vita. PSPKVM no traia JSR-184 (el
JSR-184 de phoneME dependia del motor Swerve de Superscape, que no esta
en el arbol), asi que este modulo aporta las dos mitades:

  src/core/classes/  API Java completa (33 clases, CLDC 1.1)
  src/core/native/   puente KNI (m3g_kni.c)
  src/engine/        rasterizador por software (5 .c + 3 .h)

Reparto Java / nativo
---------------------
Todo lo que es POR NODO vive en Java (grafo de escena, transformaciones,
animacion, ordenacion por capa, carga de .m3g). Todo lo que es POR
VERTICE o POR PIXEL vive en C. Es una decision de rendimiento: este port
usa el interprete en C de la CVM (arch=c, ver vita/BUILDING_VITA.txt),
asi que un bucle por vertice en Java costaria un orden de magnitud mas.

Los datos masivos (vertices, indices, texeles) NO se copian a memoria
nativa: siguen en arrays Java y el lado C los lee en sitio con
SNI_GetRawArrayPointer dentro de un bloque SNI_BEGIN_RAW_POINTERS. Esto
resuelve de raiz el problema de que CLDC 1.1 no tenga finalize(): no hay
memoria nativa colgando de objetos Java, y por tanto nada que liberar
cuando el GC los recoge. La unica reserva nativa persistente es el
z-buffer (una sola, reutilizada entre frames).

Por que software y no GPU
-------------------------
1. La Fase 4 (javacall/implementation/vita_arm/midp/lcd.c) ya es duena
   del contexto GXM a traves de vita2d, y sceGxmInitialize() solo admite
   un contexto por proceso: meter vitaGL encima obligaria a rehacer una
   Fase 4 que ya esta verificada.
2. M3G compone con lo que Graphics haya dibujado antes y despues, y debe
   respetar el clip del Graphics. Rasterizando sobre el mismo buffer
   RGB565 eso sale exacto y gratis; con GPU haria falta un render target
   aparte y una lectura de vuelta por frame.
3. El contenido M3G de J2ME es de muy baja densidad geometrica (decenas
   o pocos cientos de triangulos por frame, pensados para telefonos de
   2005). La Vita a 444 MHz con VFP va sobrada.

La puerta a la GPU queda abierta: el contrato entre Java y el motor
(M3GBatch + los arrays de estado de m3g_engine.h) no presupone software.

Que hay implementado
--------------------
API:  Object3D, Transform, Transformable, Node, Group, World, Camera,
      Light, Mesh, MorphingMesh, SkinnedMesh, Sprite3D, VertexArray,
      VertexBuffer, IndexBuffer, TriangleStripArray, Appearance,
      Material, PolygonMode, CompositingMode, Fog, Texture2D, Image2D,
      Background, KeyframeSequence, AnimationTrack, AnimationController,
      RayIntersection, Graphics3D, Loader.

Render: transformacion, iluminacion por vertice (ambiental, direccional,
      omni y foco, con atenuacion y especular), niebla lineal y
      exponencial, recorte por los 6 planos del frustum, culling con
      winding configurable, sombreado plano y Gouraud, z-buffer de 16
      bits con depth offset, dos unidades de textura con los cinco
      modos de texture env de GL ES, wrap REPEAT/CLAMP, filtrado
      nearest y bilineal, matriz de textura, los cinco modos de mezcla
      de CompositingMode, alpha test, mascaras de escritura de color,
      alfa y profundidad, iluminacion a dos caras, Background con
      imagen y crop, y Sprite3D escalado y sin escalar.

Otros: picking con RayIntersection (Moller-Trumbore en C, atributos
      interpolados en Java), animacion con STEP/LINEAR/SLERP/SPLINE,
      destino de render Graphics o Image2D, y Loader para .m3g
      (secciones sin comprimir y zlib) y para PNG/JPEG.

Limitaciones conocidas (a proposito, y en este orden de importancia)
--------------------------------------------------------------------
1. El Loader NO se ha podido probar contra un .m3g real todavia: no hay
   ninguno en el repo. El unico punto ambiguo del formato es si el
   Header Object consume un indice de objeto; si se acertara al reves,
   TODAS las referencias saldrian desplazadas en uno. Por eso
   Loader.parseWithFallback() prueba las dos convenciones y se queda
   con la que analiza sin error. Aun asi, este es el primer sitio donde
   mirar si un .m3g no carga.
2. SQUAD se evalua como SLERP. La diferencia es una transicion de
   rotacion algo menos suave; no afecta a la geometria.
3. Sin mipmaps: solo nivel base (FILTER_BASE_LEVEL). El contenido J2ME
   usa texturas de 64x64 o 128x128, donde el aliasing por minificacion
   apenas se nota a 480x272.
4. localCameraLighting se acepta en el API pero se ignora: el especular
   usa el observador en el infinito, que es el modo por defecto de
   GL ES 1.x.
5. El color y la niebla se interpolan lineales en pantalla, no con
   correccion perspectiva (las coordenadas de textura SI la llevan).
   Invisible con triangulos pequenos.
6. El skinning y el morphing se calculan en Java. Con mallas de cientos
   de vertices es despreciable; si algun contenido resulta pesado, el
   paso natural es mover ese bucle a m3g_pipeline.c.
7. La textura se muestrea con una division por pixel. La subdivision
   afin de 8 pixeles es la primera optimizacion pendiente si hace falta.
8. Image2D.duplicate() devuelve una imagen mutable aunque el original
   fuera inmutable (desviacion menor del API).

Activacion en el build de la Vita — YA APLICADA
------------------------------------------------
El modulo es autocontenido y NO toca midp/build/common. La activacion
son estos cambios (ya aplicados, Fase 2B ya estaba completada cuando
se hizo esto asi que no habia riesgo de romperla):

1) midp/build/javacall_vita/Options.gmk

       -USE_JSR_184             = false
       +USE_JSR_184             = true

2) build-vita-cldc.sh, en el paso [5/6] (midp), en la linea de make:

       USE_JSR_184=true JSR_184_DIR="$R/jsr184" SWERVE_DIR="$R/jsr184" \

   (SWERVE_DIR se pasa explicito por las dudas; subsystem.gmk tambien
   lo fija con ?= apuntando al propio modulo, porque Verify.gmk lo
   exige por herencia del motor original — con el ?= alcanzaba, pero
   no cuesta nada ser explicito.)

Nada mas: subsystem.gmk se encarga de anadir las clases al
MIDP_CLASSES_ZIP, los .c a JTWI_NATIVE_FILES y los -I a EXTRA_INCLUDES.

Dos bugs reales que aparecieron SOLO al activarlo de verdad (ninguno
del motor/API — los dos eran del toolchain de epoca), documentados en
detalle en port_progress.md Fase 5:

1. Loader.java usaba "Loader.class.getResourceAsStream(...)". El
   helper sintetico que genera javac -source 1.4 para literales
   ".class" invoca NoClassDefFoundError.initCause(...), que no existe
   en CLDC 1.1 -> "Fatal Error: Unable to find method initCause". Ya
   habia precedente exacto en este mismo arbol (PiscesFont.java,
   ImageDataFactory.java): se cambio a
   Class.forName("javax.microedition.m3g.Loader") por nombre, que no
   dispara el helper roto.
2. ROMImage.cpp (romize de MIDP) tenia timestamp viejo a proposito
   para no regenerarse (romgen es un binario Linux 32-bit que no
   corre en macOS/arm64). Al entrar las 33 clases nuevas a classes.zip,
   make detecto que estaba desactualizado e intento regenerarlo,
   fallando ("cannot execute binary file"). Se regenero de verdad
   reutilizando el contenedor Docker ya preparado por la Fase 2B
   (i386/debian:stretch-slim, con gcc+JDK8+romgen compilado, montado
   en /tmp/vitaj2me-real como /work) — 33661 objetos, ROM de 20MB,
   80 segundos.

Verificacion hecha
------------------
- Motor: los 5 .c compilan con arm-vita-eabi-gcc -fsyntax-only -Wall
  sin ningun aviso.
- Motor, prueba funcional en host: se renderizo un cubo texturizado con
  iluminacion Gouraud, z-buffer, correccion perspectiva y culling, y se
  volco a PPM. Salida correcta (3 caras visibles, 14100 pixeles
  pintados de 30000). El picking devolvio t exacto (9.0 para un rayo
  desde z=10 contra la cara en z=1) y los tres vertices correctos.
- Puente KNI: compila con arm-vita-eabi-gcc -fsyntax-only -Wall contra
  las cabeceras reales de CLDC y MIDP (solo se sustituyeron por stubs
  ROMStructs.h y jvmconfig.h, que los genera el propio build).
- Java: las 33 clases compilan con javac; se evito toda API que CLDC 1.1
  no tenga (Math.acos se implementa en MathUtil, no hay generics, no hay
  java.util.zip — de ahi que el inflate sea nativo).
- YA ENLAZADO DE VERDAD (no solo verificacion aislada): build completo
  de la Fase 2B con USE_JSR_184=true de punta a punta. Confirmado con
  arm-vita-eabi-nm: 93 simbolos m3g_*/M3GNative en _prebuilt/lib/libmidp.a,
  42 en el eboot.bin final enlazado; unzip -l confirma 34 .class de
  javax/microedition/m3g/ en _prebuilt/lib/classes.zip. VPK final:
  3527KB (subio desde 3434KB sin M3G). Los 7 "indefinidos" que muestra
  nm -u son todos weak (newlib/libgcc: _ITM_*, __register_frame_info,
  etc.), no una regresion.

Lo que falta por probar es lo unico que solo se puede probar con la
consola delante: subir el eboot.bin nuevo (ya compilado, listo en
vita/build/eboot.bin) y confirmar que el bringup sigue arrancando bien
con M3G enlazado — la consola estaba offline al momento de subirlo. No
deberia cambiar nada visible todavia (ningun .jar real usa M3G aun),
solo confirma que enlazarlo no rompe nada en runtime.
