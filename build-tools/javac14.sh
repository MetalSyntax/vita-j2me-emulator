#!/bin/sh
# javac wrapper (Fase 2b Vita): phoneME es Java 1.4 ('enum' como
# identificador, etc.). JDK8 compila 1.4 con warnings (obsoleto pero vale).
exec /Library/Java/JavaVirtualMachines/adoptopenjdk-8.jdk/Contents/Home/bin/javac -source 1.4 -target 1.4 "$@"
