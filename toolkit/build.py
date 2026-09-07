"""
build.py — compila el bringup (Fase 2a) con los mismos comandos de
vita/BUILDING_VITA.txt, para no tener que teclearlos a mano en cada
ciclo de prueba.

    cmake -S vita -B vita/build -DCMAKE_BUILD_TYPE=Release \\
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    cmake --build vita/build

No toca nada de la Fase 2b (javacall/pcsl/cldc/midp): esta build es
solo el frontend nativo sin JVM, que es lo que build-vita-cldc.sh llama
"paso [0/4]".
"""

import os
import subprocess
from pathlib import Path

from tui import C
import config as cfgmod


def build(cfg, clean=False):
    vitasdk = Path(cfg["vitasdk"])
    if not vitasdk.is_dir():
        print(f"{C.RED}VITASDK no encontrado en: {vitasdk}{C.RESET}")
        print("Editá la configuración (opción de menú) o exportá VITASDK "
              "antes de lanzar el toolkit.")
        return False

    src_dir = cfgmod.resolve(cfg, "local_cmake_src")
    build_dir = cfgmod.resolve(cfg, "local_build_dir")

    if clean and build_dir.exists():
        import shutil
        print(f"Limpiando {build_dir}...")
        shutil.rmtree(build_dir)

    env = dict(os.environ)
    env["VITASDK"] = str(vitasdk)
    env["PATH"] = f"{vitasdk / 'bin'}:{env.get('PATH', '')}"

    configure_cmd = [
        "cmake", "-S", str(src_dir), "-B", str(build_dir),
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
    ]
    build_cmd = ["cmake", "--build", str(build_dir)]

    print(f"{C.CYAN}$ {' '.join(configure_cmd)}{C.RESET}")
    r1 = subprocess.run(configure_cmd, env=env)
    if r1.returncode != 0:
        print(f"{C.RED}Falló cmake (configure).{C.RESET}")
        return False

    print(f"\n{C.CYAN}$ {' '.join(build_cmd)}{C.RESET}")
    r2 = subprocess.run(build_cmd, env=env)
    if r2.returncode != 0:
        print(f"{C.RED}Falló cmake --build.{C.RESET}")
        return False

    vpk = cfgmod.resolve(cfg, "local_vpk")
    if vpk.exists():
        size_kb = vpk.stat().st_size / 1024
        print(f"\n{C.GREEN}OK: {vpk} ({size_kb:.0f} KB){C.RESET}")
    else:
        print(f"\n{C.YELLOW}Build sin errores, pero no encuentro {vpk}{C.RESET}")
    return True
