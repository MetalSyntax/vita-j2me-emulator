Herramientas de verificacion del modulo JSR-184
===============================================

check_contract.py
  Comprueba que los indices IP_*/FP_*/LP_* de M3GNative.java coincidan
  con los de m3g_engine.h. Estan duplicados a mano (KNI no permite
  compartir constantes) y un desajuste daria fallos silenciosos.

      python3 jsr184/tools/check_contract.py

m3g_host_test.c
  Prueba funcional del motor compilada para el host (no para la Vita):
  renderiza un cubo texturizado con iluminacion, z-buffer, correccion
  perspectiva y culling, vuelca out.ppm y lanza un rayo de picking.

      cd jsr184/src/engine
      cc -O2 -Wall -DBIG_ENDIAN_FRAMEBUFFER=1 -I. \
         ../../tools/m3g_host_test.c \
         m3g_math.c m3g_target.c m3g_texture.c m3g_raster.c m3g_pipeline.c \
         -lm -o /tmp/m3gtest
      cd /tmp && ./m3gtest      # imprime OK y escribe out.ppm

Verificacion de compilacion cruzada (lo que se ejecuta en la Vita):

      export VITASDK=~/vitasdk; export PATH=$VITASDK/bin:$PATH
      cd jsr184/src/engine
      for f in m3g_*.c; do
          arm-vita-eabi-gcc -fsyntax-only -Wall -DBIG_ENDIAN_FRAMEBUFFER=1 \
              -I. $f || echo "FALLO $f"
      done
