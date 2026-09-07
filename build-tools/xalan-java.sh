#!/bin/sh
# xalan-java wrapper (Fase 2b Vita): el Configurator de phoneME usa
# xsl:key sobre RTFs, que el XSLTC de JDK8+ no indexa (merged.xml vacío).
# Solo para Configurator.jar se fuerza Xalan-J; el resto pasa intacto.
# Traduce: xalan-java -jar JAR ARGS... (solo si JAR=Configurator.jar)
#       -> java -Dfactory -cp xalan:serializer:JAR MAIN ARGS...
XALAN=/tmp/vitaj2me/xalan.jar
SER=/tmp/vitaj2me/serializer.jar
REALJDK=/Library/Java/JavaVirtualMachines/adoptopenjdk-8.jdk/Contents/Home/bin/java
if [ "$1" = "-jar" ]; then
    case "$2" in
        *Configurator.jar)
            JAR="$2"
            shift 2
            exec "$REALJDK" -Djavax.xml.transform.TransformerFactory=org.apache.xalan.processor.TransformerFactoryImpl -cp "$XALAN:$SER:$JAR" com.sun.xml.transform.CodeTransformer "$@"
            ;;
    esac
fi
exec "$REALJDK" -Djava.util.Arrays.useLegacyMergeSort=true "$@"
