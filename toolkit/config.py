"""
config.py — configuración local del toolkit (IP de la Vita, rutas
remotas y locales). Se guarda en toolkit/config.json, que está en
.gitignore porque contiene la IP de tu red local.
"""

import json
import os
from pathlib import Path

TOOLKIT_DIR = Path(__file__).resolve().parent
PROJECT_DIR = TOOLKIT_DIR.parent          # vita-j2me-emulator/
CONFIG_PATH = TOOLKIT_DIR / "config.json"
LOGS_DIR = TOOLKIT_DIR / "logs"
DUMPS_DIR = TOOLKIT_DIR / "dumps"

DEFAULTS = {
    # Consola de pruebas
    "vita_ip": "192.168.3.15",
    "vita_port": 1337,
    "titleid": "VITAJ2ME0",

    # Rutas remotas (ver vita/main.c, logging.c y vita/CMakeLists.txt)
    "vita_downloads_dir": "/ux0:/downloads",
    "vita_app_dir": "/ux0:/app/VITAJ2ME0",
    "vita_home_dir": "/ux0:/data/vitaj2me",
    "vita_logs_dir": "/ux0:/data/vitaj2me/logs",
    "vita_data_root": "/ux0:/data",

    # Rutas locales (relativas a vita-j2me-emulator/)
    "local_vpk": "vita/build/vitaj2me-bringup.vpk",
    "local_eboot": "vita/build/eboot.bin",
    "local_elf": "vita/build/vitaj2me",
    "local_build_dir": "vita/build",
    "local_cmake_src": "vita",

    # Herramientas externas
    "vitasdk": os.environ.get("VITASDK", str(Path.home() / "vitasdk")),
    "vita_parse_core_dir": str(Path.home() / "vita-tools" / "vita-parse-core"),
}


def load():
    cfg = dict(DEFAULTS)
    if CONFIG_PATH.exists():
        try:
            with open(CONFIG_PATH, "r", encoding="utf-8") as f:
                cfg.update(json.load(f))
        except (json.JSONDecodeError, OSError):
            pass
    LOGS_DIR.mkdir(parents=True, exist_ok=True)
    DUMPS_DIR.mkdir(parents=True, exist_ok=True)
    return cfg


def save(cfg):
    with open(CONFIG_PATH, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=2, ensure_ascii=False, sort_keys=True)
        f.write("\n")


def resolve(cfg, key):
    """Ruta local absoluta a partir de una clave relativa a PROJECT_DIR."""
    return PROJECT_DIR / cfg[key]
