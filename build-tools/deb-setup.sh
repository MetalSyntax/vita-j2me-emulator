#!/bin/bash
# Setup del contenedor i386/debian:buster para generar OopMaps.cpp (Fase 2b).
# Buster está EOL: se usan los archivos de archive.debian.org.
set -e
cat > /etc/apt/sources.list <<'EOF'
deb http://archive.debian.org/debian buster main contrib non-free
deb http://archive.debian.org/debian-security buster/updates main contrib non-free
EOF
echo 'Acquire::Check-Valid-Until "false";' > /etc/apt/apt.conf.d/99-no-check
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    gcc make openjdk-8-jdk ca-certificates
java -version 2>&1 | head -2
gcc --version | head -1
echo SETUP_OK
