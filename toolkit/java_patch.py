#!/usr/bin/env python3
"""java_patch.py — parchear clases Java de classes.zip (CLDC/MIDP) sin
recompilar MIDP entero (romgen/Docker, ver bug #14).

Receta probada en HW (sesiones 43-51):
  1. ANTES de editar:  python3 toolkit/java_patch.py verify <Fuente.java>...
     Compila la fuente tal cual y compara su bytecode (javap -c,
     ignorando índices del constant pool) con el de classes.zip. Si no da
     0 diferencias, la fuente NO es la que se desplegó: no parchear.
  2. Editar la fuente.
  3. python3 toolkit/java_patch.py apply <Fuente.java>...
     javac -source/-target 1.4 contra classes.zip -> ProGuard -microedition
     (preverifica: versión 48 + StackMap, lo que exige el verificador
     CLDC) -> reemplaza en _prebuilt/lib/classes.zip y
     _prebuilt/data/classes.zip SOLO las clases cuyo bytecode cambió.
     Imprime el diff de javap de cada una para revisarlo.

Requiere un JDK 8 (javac con -source 1.4) y toolkit/tools/proguard.jar
(ProGuard 7.4.2, release oficial; zip sha256 164b26a6ca65...21bce5fa).
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLASSES = os.path.join(ROOT, "_prebuilt", "lib", "classes.zip")
DATA_CLASSES = os.path.join(ROOT, "_prebuilt", "data", "classes.zip")
PROGUARD = os.path.join(ROOT, "toolkit", "tools", "proguard.jar")
JDK8 = os.environ.get(
    "JDK8_HOME",
    "/Library/Java/JavaVirtualMachines/adoptopenjdk-8.jdk/Contents/Home")


def tool(name):
    return os.path.join(JDK8, "bin", name)


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if r.returncode != 0:
        sys.exit("ERROR: %s\n%s%s" % (" ".join(cmd), r.stdout, r.stderr))
    return r.stdout


def compile_sources(sources, out):
    os.makedirs(out)
    run([tool("javac"), "-source", "1.4", "-target", "1.4", "-encoding",
         "ISO-8859-1", "-nowarn", "-bootclasspath", CLASSES, "-d", out]
        + [os.path.abspath(s) for s in sources])
    found = []
    for d, _, files in os.walk(out):
        for f in files:
            if f.endswith(".class"):
                found.append(os.path.relpath(os.path.join(d, f), out))
    return sorted(found)


def javap(cp, entry):
    cls = entry[:-6].replace("/", ".")
    text = run([tool("javap"), "-c", "-p", "-cp", cp, cls])
    lines = []
    for line in text.splitlines():
        if line.startswith("Compiled from"):
            continue
        lines.append(re.sub(r"\s+", " ", re.sub(r"#\d+", "", line)))
    return lines


def diff(a, b):
    import difflib
    return [l for l in difflib.unified_diff(a, b, "desplegado", "nuevo",
                                            lineterm="", n=1)]


def verify(sources):
    tmp = tempfile.mkdtemp()
    try:
        entries = compile_sources(sources, os.path.join(tmp, "o"))
        bad = 0
        with zipfile.ZipFile(CLASSES) as z:
            names = set(z.namelist())
        for e in entries:
            if e not in names:
                print("  %s: no está en classes.zip (clase nueva)" % e)
                continue
            d = diff(javap(CLASSES, e), javap(os.path.join(tmp, "o"), e))
            print("  %s: %d líneas distintas" % (e, len(d)))
            bad += bool(d)
        if bad:
            sys.exit("NO coincide: la fuente no es la desplegada. No parchear.")
        print("OK: la fuente coincide con el bytecode desplegado.")
    finally:
        shutil.rmtree(tmp)


def apply(sources):
    if not os.path.exists(PROGUARD):
        sys.exit("Falta %s (ver docstring)" % PROGUARD)
    tmp = tempfile.mkdtemp()
    try:
        out, pv = os.path.join(tmp, "out"), os.path.join(tmp, "pv")
        entries = compile_sources(sources, out)
        run([tool("java"), "-jar", PROGUARD, "-injars", out, "-outjars", pv,
             "-libraryjars", CLASSES, "-microedition", "-dontshrink",
             "-dontoptimize", "-dontobfuscate", "-dontwarn", "-dontnote",
             "-keep", "class ** { *; }"])
        with zipfile.ZipFile(CLASSES) as z:
            names = set(z.namelist())
        changed = []
        for e in entries:
            major = run([tool("javap"), "-v", "-cp", pv,
                         e[:-6].replace("/", ".")])
            if "major version: 48" not in major:
                sys.exit("%s no quedó en versión 48 (preverificación)" % e)
            if e in names:
                d = diff(javap(CLASSES, e), javap(pv, e))
                if not d:
                    continue
                print("=== %s (%d líneas)" % (e, len(d)))
                print("\n".join(d[:120]))
            else:
                print("=== %s (clase NUEVA)" % e)
            changed.append(e)
        if not changed:
            print("Nada cambió.")
            return
        for target in (CLASSES, DATA_CLASSES):
            src = target + ".tmp"
            with zipfile.ZipFile(target) as zi, \
                    zipfile.ZipFile(src, "w") as zo:
                for info in zi.infolist():
                    data = (open(os.path.join(pv, info.filename), "rb").read()
                            if info.filename in changed else
                            zi.read(info.filename))
                    zinfo = zipfile.ZipInfo(info.filename, info.date_time)
                    zinfo.compress_type = info.compress_type
                    zinfo.external_attr = info.external_attr
                    zo.writestr(zinfo, data)
                for e in changed:
                    if e not in names:
                        zo.write(os.path.join(pv, e), e)
            with zipfile.ZipFile(src) as z:
                if z.testzip() is not None:
                    sys.exit("zip corrupto: %s" % src)
            os.replace(src, target)
        print("Reemplazadas %d clase(s) en classes.zip (lib y data): %s"
              % (len(changed), ", ".join(changed)))
        print("Siguiente: python3 toolkit/deploy.py classes")
    finally:
        shutil.rmtree(tmp)


if __name__ == "__main__":
    if len(sys.argv) < 3 or sys.argv[1] not in ("verify", "apply"):
        sys.exit(__doc__)
    (verify if sys.argv[1] == "verify" else apply)(sys.argv[2:])
