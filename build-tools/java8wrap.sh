#!/bin/sh
# java8 wrapper (Fase 2b Vita): añade la propiedad del sufijo de objetos
# de MakeDeps. Inocuo para el resto de tools Java (jcc, etc.).
exec /Library/Java/JavaVirtualMachines/adoptopenjdk-8.jdk/Contents/Home/bin/java -Dmakedep.objSuffix=.obj "$@"
