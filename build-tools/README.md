# build-tools/ — Helpers del build Fase 2b (solo máquina de build, macOS/arm64)
#
# Estos scripts NO van a la Vita. `build-vita-cldc.sh` los instala en
# /tmp/vitaj2me/{hostbin,*.jar} y los usa para compilar phoneME (2006)
# con toolchains modernos. Sin ellos el build no reproduce.
#
# | Fichero         | Uso                                                    |
# |-----------------|--------------------------------------------------------|
# | hostcxx.sh      | cc/gcc/c++/g++ -> clang filtrando flags solo-GCC       |
# |                 | (-fno-optional-diags, -fexpensive-optimizations)       |
# | java8wrap.sh    | java JDK8 + -Dmakedep.objSuffix=.obj (MakeDeps emite |
# |                 | .o y el make espera .obj con host_os=cygwin)           |
# | javac14.sh      | javac JDK8 + -source/-target 1.4 (MIDP es Java 1.4)   |
# | xalan-java.sh   | java JDK8; para Configurator.jar fuerza Xalan-J        |
# |                 | (el XSLTC de JDK8 no indexa xsl:key sobre RTFs y el   |
# |                 | merged.xml sale vacío) + legacyMergeSort para l10n     |
# | preverify.sh    | preverify Sun -> ProGuard -microedition con staging    |
# |                 | (IN!=OUT, sin solape input/library)                    |
# | deb-setup.sh    | setup del contenedor i386 (gcc + JDK8) para romgen     |
# | gen_livearea.py | genera los PNG del LiveArea sin dependencias           |
#
# Descargas externas (una vez, a /tmp/vitaj2me/):
#   proguard.jar  <- proguard-7.4.2.zip (GitHub Guardsquare, lib/)
#   xalan.jar     <- com/xalan/xalan/2.7.3 (Maven Central)
#   serializer.jar<- com/xalan/serializer/2.7.3 (Maven Central)
#
# Docker (solo para OopMaps.cpp + ROMImage.cpp de MIDP):
#   romgen es una HOST tool de 32 bits por diseño; en macOS/arm64 se
#   genera con Debian i386 (ver BUILDING_VITA.txt, sección Docker).
