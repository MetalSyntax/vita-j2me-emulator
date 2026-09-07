#!/usr/bin/env python3
"""Inyecta el LiveArea en el VPK base (Fase 6).

Motivo: vita-pack-vpk (vitasdk) parte las rutas con espacios y este repo
vive en ".../PSVITA Develop/...". El VPK es un zip: se reescribe con los
ficheros sce_sys/* añadidos. Idempotente (se puede correr N veces).

Uso: patch_vpk.py <ruta.vpk> <dir-livearea>
"""
import sys
import zipfile

VPK, LADIR = sys.argv[1], sys.argv[2]

FILES = {
    "icon0.png": "sce_sys/icon0.png",
    "pic0.png": "sce_sys/pic0.png",
    "bg0.png": "sce_sys/livearea/contents/bg0.png",
    "startup.png": "sce_sys/livearea/contents/startup.png",
    "template.xml": "sce_sys/livearea/contents/template.xml",
}

with zipfile.ZipFile(VPK, "r") as zin:
    keep = [(n, zin.read(n)) for n in zin.namelist()
            if n not in FILES.values()]

with zipfile.ZipFile(VPK, "w", zipfile.ZIP_DEFLATED) as zout:
    for name, data in keep:
        zout.writestr(name, data)
    for src, dst in FILES.items():
        zout.write(f"{LADIR}/{src}", dst)

print(f"livearea OK: {len(keep)} base + {len(FILES)} sce_sys en {VPK}")
