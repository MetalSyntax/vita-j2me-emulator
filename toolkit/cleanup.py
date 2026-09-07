"""
cleanup.py — barre ._* (AppleDouble) y .DS_Store del árbol del proyecto.
En este disco (exFAT/red) macOS los genera todo el tiempo y ensucian los
listados/diffs; nada de esto se commitea nunca (ya está en .gitignore
del repo raíz), así que borrarlos es siempre seguro.
"""

from pathlib import Path

from tui import C

JUNK_NAMES = {".DS_Store"}


def _is_junk(path):
    return path.name in JUNK_NAMES or path.name.startswith("._")


def find_junk(root):
    root = Path(root)
    return [p for p in root.rglob("*") if p.is_file() and _is_junk(p)]


def clean(root, dry_run=False):
    junk = find_junk(root)
    if not junk:
        print(f"{C.GREEN}No hay archivos basura de macOS.{C.RESET}")
        return []

    total_bytes = sum(p.stat().st_size for p in junk)
    print(f"Encontrados {len(junk)} archivos ({total_bytes / 1024:.1f} KB):")
    for p in junk[:20]:
        print(f"  {p.relative_to(root)}")
    if len(junk) > 20:
        print(f"  ... y {len(junk) - 20} más")

    if dry_run:
        return junk

    deleted = []
    for p in junk:
        try:
            p.unlink()
            deleted.append(p)
        except OSError as e:
            print(f"{C.RED}No se pudo borrar {p}: {e}{C.RESET}")
    print(f"{C.GREEN}Borrados {len(deleted)}/{len(junk)}.{C.RESET}")
    return deleted
