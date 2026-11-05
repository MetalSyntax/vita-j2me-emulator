# See ../README.html for build instructions, an explanation of the
# build system, and how to create a build system for a new
# configuration.

# Fase 2 Vita: el frontend vita/ enlaza con vitasdk, no con pspsdk,
# así que no se incluye ningún .make de PSP aquí. Los flags de
# cross-compile los pone javacall_vita_gcc.cfg (GNU_TOOLS_DIR=$VITASDK).
IsSelfHosted = false
CPP_DEF_FLAGS = $(CXXFLAGS)
LINK_FLAGS = $(LDFLAGS)
include ../share/root.make
