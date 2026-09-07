# javacall/configuration/phoneMEFeature/vita_arm/GNUmakefile
# Fase 2: clon de psp_mips/GNUmakefile adaptado a vitasdk.
# Cambios PSP -> Vita:
#   CC psp-gcc            -> arm-vita-eabi-gcc ($VITASDK/bin)
#   AR psp-ar             -> arm-vita-eabi-ar
#   includes pspsdk       -> $(VITASDK)/arm-vita-eabi/include
#   flags -G0 (MIPS-only) -> eliminados (ARM no los acepta)
#   -DPSP                 -> -DVITA -DARM (el backend CVM será arm, no c/mips)

ifeq ($(JAVACALL_DIR),)
$(error JAVACALL_DIR is not set.)
endif

ifeq ($(VITASDK),)
$(error VITASDK is not set. export VITASDK=~/vitasdk)
endif

CC=$(VITASDK)/bin/arm-vita-eabi-gcc
AR=$(VITASDK)/bin/arm-vita-eabi-ar

CFLAGS += -I$(VITASDK)/arm-vita-eabi/include \
          -I$(VITASDK)/arm-vita-eabi/include/freetype2 \
          -DVITA -DARM \
          -fno-strict-aliasing \
          -mfloat-abi=softfp
include environment.gmk

# Toolchain genérico gcc de phoneME (igual que psp_mips usaba)
include $(JAVACALL_DIR)/build/gcc/rules.gmk

pre_target:
post_target:
