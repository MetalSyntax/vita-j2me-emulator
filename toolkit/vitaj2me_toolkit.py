#!/usr/bin/env python3
"""
vitaj2me-toolkit — TUI mínima para agilizar el ciclo de pruebas de
vita-j2me-emulator en consola real: subir el VPK/eboot por FTP, bajar
logs y crash dumps, analizarlos, compilar el bringup y limpiar la
basura de macOS del proyecto.

No es un port de una APK Android (para eso está psvita-port-toolkit,
la herramienta standalone grande) -- esto es un emulador J2ME nativo,
así que el conjunto de herramientas es deliberadamente más chico: solo
lo que hace falta para el ciclo compilar -> subir -> ejecutar -> bajar
log/dump -> analizar, sin nada de lo específico a ports Android (JNI,
.so, jadx/Ghidra, etc).

Uso:
    python3 toolkit/vitaj2me_toolkit.py
    ./toolkit/vitaj2me-toolkit          (wrapper ejecutable)
"""

import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import build as buildmod
import cleanup
import config as cfgmod
import crash_analyzer
import ftp_ops
import tui
from tui import C


def _log_predicate(name):
    lower = name.lower()
    if lower == "session.txt":
        # Contador interno de main.c (numera bringup-NNN.log/jvm-NNN.log),
        # no un log en sí.
        return False
    return lower.endswith(".log") or lower.endswith(".txt")


def _dump_predicate(name):
    lower = name.lower()
    return (
        lower.startswith("psp2core")
        or lower.endswith(".psp2dmp")
        or lower.endswith(".dmp")
    ) and not lower.endswith(".tmp")


def _log_search_dirs(cfg):
    # logging.c/main.c escriben en vita_logs_dir (ux0:data/vitaj2me/logs);
    # vita_home_dir queda como fallback por si hay logs sueltos de antes
    # de que existiera la subcarpeta (bringup.log/jvm.log históricos).
    return [cfg["vita_logs_dir"], cfg["vita_home_dir"]]


def _dump_search_dirs(cfg):
    # Los crash dumps NO usan vita_logs_dir: SceCoredump los deja sueltos
    # en la raíz de ux0:data, compartidos con cualquier otro homebrew.
    return [cfg["vita_home_dir"], cfg["vita_data_root"]]


# ---------------------------------------------------------------------------
# Acciones de menú
# ---------------------------------------------------------------------------

def action_upload_vpk(cfg):
    vpk = cfgmod.resolve(cfg, "local_vpk")
    if not vpk.exists():
        print(f"{C.RED}No existe: {vpk}{C.RESET}")
        print("Compilá primero (opción del menú) o revisá la ruta en la configuración.")
        return
    size_kb = vpk.stat().st_size / 1024
    print(f"VPK local: {vpk} ({size_kb:.0f} KB)")
    if not tui.confirm(f"¿Subir a {cfg['vita_downloads_dir']}/{vpk.name}?", default=True):
        print("Cancelado.")
        return
    ftp = ftp_ops.connect_with_retry(cfg)
    if not ftp:
        return
    try:
        dest = ftp_ops.upload_file(ftp, vpk, cfg["vita_downloads_dir"])
        print(f"\n{C.GREEN}Subido a {dest}{C.RESET}")
        print(f"{C.DIM}Instalalo desde VitaShell (ux0:downloads) si es la primera vez.{C.RESET}")
    except Exception as e:
        print(f"{C.RED}Falló la subida: {e}{C.RESET}")
    finally:
        ftp_ops.quit_(ftp)


def action_upload_home_data(cfg):
    """Sube al home J2ME de la consola lo que la JVM necesita para arrancar.

    Sin `classes.zip` en `ux0:data/vitaj2me/` la JVM no encuentra NINGUNA
    clase del AMS y el bootstrap muere mucho mas adelante, con un sintoma
    que no tiene nada que ver (camino de clases "fake"). Hasta ahora habia
    que copiarla a mano con VitaShell; esta opcion cierra el ciclo dentro
    del toolkit. Tambien sube un .jar/.jad a games/ para poder probar la
    deteccion de MIDlets desde el AMS.
    """
    home = cfg["vita_home_dir"]
    root = cfgmod.PROJECT_DIR
    classes = root / "_prebuilt" / "lib" / "classes.zip"

    items = []
    if classes.exists():
        items.append((classes, home,
                      f"classes.zip ({classes.stat().st_size / 1024:.0f} KB)"))
    else:
        print(f"{C.RED}No existe {classes} — falta compilar la Fase 2b.{C.RESET}")

    # .jar/.jad sueltos en la raiz del repo o en games/ (para probar la
    # deteccion de MIDlets desde el AMS).
    seen = set()
    for pattern_dir in (root, root / "games"):
        if not pattern_dir.is_dir():
            continue
        for j in sorted(pattern_dir.glob("*.jar")) + sorted(pattern_dir.glob("*.jad")):
            if j.name in seen:
                continue
            seen.add(j.name)
            items.append((j, f"{home}/games",
                          f"{j.name} ({j.stat().st_size / 1024:.0f} KB)"))

    if not items:
        print("Nada para subir.")
        return

    print("Se va a subir:")
    for _local, dest, label in items:
        print(f"  {label}  ->  {dest}/")
    if not tui.confirm("¿Continuar?", default=True):
        print("Cancelado.")
        return

    ftp = ftp_ops.connect_with_retry(cfg)
    if not ftp:
        return
    try:
        for local, dest, _label in items:
            print(f"\n{local.name}:")
            ftp_ops.upload_file(ftp, local, dest)
        print(f"\n{C.GREEN}Listo.{C.RESET}")
        print(f"{C.DIM}El frontend loguea un inventario de {home} al arrancar: "
              f"revisa bringup-NNN.log para confirmar que lo ve.{C.RESET}")
    except Exception as e:
        print(f"{C.RED}Falló la subida: {e}{C.RESET}")
    finally:
        ftp_ops.quit_(ftp)


def action_upload_eboot(cfg):
    eboot = cfgmod.resolve(cfg, "local_eboot")
    if not eboot.exists():
        print(f"{C.RED}No existe: {eboot}{C.RESET}")
        return
    size_kb = eboot.stat().st_size / 1024
    dest_dir = cfg["vita_app_dir"]
    print(f"eboot.bin local: {eboot} ({size_kb:.0f} KB)")
    print(f"{C.YELLOW}Esto sobrescribe el eboot.bin ya instalado en {dest_dir}.{C.RESET}")
    print("Solo funciona si el .vpk ya fue instalado antes al menos una vez.")
    if not tui.confirm("¿Continuar?", default=False):
        print("Cancelado.")
        return
    ftp = ftp_ops.connect_with_retry(cfg)
    if not ftp:
        return
    try:
        dest = ftp_ops.upload_file(ftp, eboot, dest_dir, remote_name="eboot.bin")
        print(f"\n{C.GREEN}Subido a {dest}{C.RESET}")
    except Exception as e:
        print(f"{C.RED}Falló la subida: {e}{C.RESET}")
    finally:
        ftp_ops.quit_(ftp)


def _dump_label(name, mtime):
    """Fecha exacta si el nombre trae un epoch embebido (psp2core-<epoch>-...),
    si no la fecha aproximada de LIST. Los crash dumps se guardan sueltos en
    ux0:data/, compartidos con cualquier otro homebrew de la consola -- esta
    fecha es la única pista de si el dump es realmente de esta sesión."""
    epoch = ftp_ops.dump_epoch(name)
    if epoch is not None:
        date_str = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(epoch))
        return f"{name:<44} {C.DIM}{date_str}{C.RESET}"
    return f"{name:<44} {C.DIM}{mtime}{C.RESET}"


def _warn_if_stale_dump(name, max_age_seconds=3600):
    epoch = ftp_ops.dump_epoch(name)
    if epoch is None:
        return
    age = time.time() - epoch
    if age > max_age_seconds:
        when = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(epoch))
        age_txt = f"{age / 3600:.1f} h" if age < 86400 else f"{age / 86400:.1f} días"
        print(
            f"{C.YELLOW}[!] Este dump es de {when} ({age_txt} atrás) -- "
            f"probablemente no es de esta sesión.{C.RESET}\n"
            f"{C.DIM}    (ux0:data/ guarda los dumps de TODO homebrew de la "
            f"consola, sin distinguir de qué app son){C.RESET}"
        )


def _download_one(ftp, cfg, directory, name, dest_dir, label):
    dest_dir.mkdir(parents=True, exist_ok=True)
    local_path = dest_dir / name
    print(f"Descargando {label}: {name}")
    ftp_ops.download_file(ftp, directory, name, local_path)
    print(f"{C.GREEN}Guardado en {local_path}{C.RESET}")
    return local_path


def action_download_latest(cfg):
    ftp = ftp_ops.connect_with_retry(cfg)
    if not ftp:
        return
    try:
        logs = ftp_ops.find_files(ftp, _log_search_dirs(cfg), _log_predicate)
        if logs:
            d, name, mtime = logs[-1]
            _download_one(ftp, cfg, d, name, cfgmod.LOGS_DIR, "log")
        else:
            print(f"{C.YELLOW}No hay logs en {cfg['vita_logs_dir']}{C.RESET}")

        dumps = ftp_ops.find_files(ftp, _dump_search_dirs(cfg), _dump_predicate)
        if dumps:
            d, name, mtime = dumps[-1]
            _warn_if_stale_dump(name)
            local_dump = _download_one(ftp, cfg, d, name, cfgmod.DUMPS_DIR, "crash dump")
            if tui.confirm("\n¿Analizar este crash dump ahora?", default=True):
                _run_analysis(cfg, local_dump)
        else:
            print(f"{C.DIM}No hay crash dumps pendientes.{C.RESET}")
    except Exception as e:
        print(f"{C.RED}Error: {e}{C.RESET}")
    finally:
        ftp_ops.quit_(ftp)


def action_pick_remote_file(cfg):
    kind = tui.select_list(
        "¿Qué querés bajar?",
        [("log", "Un log específico"), ("dump", "Un crash dump específico")],
        label_fn=lambda e: e[1],
    )
    if kind is None:
        return
    kind, _ = kind

    ftp = ftp_ops.connect_with_retry(cfg)
    if not ftp:
        return
    try:
        if kind == "log":
            entries = ftp_ops.find_files(ftp, _log_search_dirs(cfg), _log_predicate)
            dest_dir = cfgmod.LOGS_DIR
        else:
            entries = ftp_ops.find_files(ftp, _dump_search_dirs(cfg), _dump_predicate)
            dest_dir = cfgmod.DUMPS_DIR

        if not entries:
            print(f"{C.YELLOW}No se encontró nada.{C.RESET}")
            return

        label_fn = (lambda e: _dump_label(e[1], e[2])) if kind == "dump" \
            else (lambda e: f"{e[1]:<32} {C.DIM}{e[2]}{C.RESET}")
        picked = tui.select_list(
            "Elegí uno (el más reciente al final)",
            entries,
            label_fn=label_fn,
        )
        if picked is None:
            return
        d, name, mtime = picked
        if kind == "dump":
            _warn_if_stale_dump(name)
        local_path = _download_one(ftp, cfg, d, name, dest_dir, kind)
        if kind == "dump" and tui.confirm("\n¿Analizar este crash dump ahora?", default=True):
            _run_analysis(cfg, local_path)
    except Exception as e:
        print(f"{C.RED}Error: {e}{C.RESET}")
    finally:
        ftp_ops.quit_(ftp)


def _run_analysis(cfg, dump_path):
    elf_path = cfgmod.resolve(cfg, "local_elf")
    print()
    ok, result = crash_analyzer.analyze(cfg, dump_path, elf_path)
    if not ok:
        print(f"{C.RED}{result}{C.RESET}")
        return
    crash_analyzer.print_report(result)


def action_analyze_local(cfg):
    dumps = sorted(cfgmod.DUMPS_DIR.glob("*"), key=lambda p: p.stat().st_mtime, reverse=True)
    dumps = [p for p in dumps if p.is_file() and not p.name.endswith(".analysis.txt")]
    if not dumps:
        print(f"{C.YELLOW}No hay dumps descargados en {cfgmod.DUMPS_DIR}{C.RESET}")
        print("Bajá uno primero con las opciones de log/crash.")
        return
    picked = tui.select_list(
        "Elegí un dump local para analizar",
        dumps,
        label_fn=lambda p: f"{p.name:<40} {C.DIM}{time.strftime('%Y-%m-%d %H:%M', time.localtime(p.stat().st_mtime))}{C.RESET}",
    )
    if picked is None:
        return
    _run_analysis(cfg, picked)


def action_cleanup(cfg):
    junk = cleanup.find_junk(cfgmod.PROJECT_DIR)
    if not junk:
        print(f"{C.GREEN}No hay archivos basura de macOS.{C.RESET}")
        return
    total_kb = sum(p.stat().st_size for p in junk) / 1024
    print(f"{len(junk)} archivos ({total_kb:.1f} KB) bajo {cfgmod.PROJECT_DIR}")
    for p in junk[:20]:
        print(f"  {p.relative_to(cfgmod.PROJECT_DIR)}")
    if len(junk) > 20:
        print(f"  ... y {len(junk) - 20} más")
    if tui.confirm("\n¿Borrarlos?", default=True):
        cleanup.clean(cfgmod.PROJECT_DIR)


def action_build(cfg):
    clean = tui.confirm("¿Limpiar vita/build/ antes de compilar?", default=False)
    buildmod.build(cfg, clean=clean)


def action_test_connection(cfg):
    ftp = ftp_ops.connect(cfg)
    if not ftp:
        return
    try:
        print(ftp.getwelcome())
        entries = ftp_ops.list_dir(ftp, cfg["vita_logs_dir"])
        print(f"\n{cfg['vita_logs_dir']} ({len(entries)} archivos):")
        for name, is_dir, mtime in entries[:15]:
            marker = "/" if is_dir else ""
            print(f"  {name}{marker}  {C.DIM}{mtime}{C.RESET}")
    finally:
        ftp_ops.quit_(ftp)


def action_settings(cfg):
    editable = [
        ("vita_ip", "IP de la Vita"),
        ("vita_port", "Puerto FTP (VitaShell)"),
        ("vita_logs_dir", "Carpeta remota de logs"),
        ("vita_home_dir", "Carpeta remota de datos (fallback de logs)"),
        ("vitasdk", "Ruta a VITASDK"),
        ("vita_parse_core_dir", "Ruta a vita-parse-core"),
    ]
    while True:
        print(f"{C.BOLD}Configuración actual{C.RESET} ({cfgmod.CONFIG_PATH})\n")
        for key, label in editable:
            print(f"  {label:<32} {C.CYAN}{cfg[key]}{C.RESET}")
        print()
        picked = tui.select_list(
            "Editar un valor (o volver)",
            editable,
            label_fn=lambda e: e[1],
        )
        if picked is None:
            return
        key, label = picked
        try:
            new_val = input(f"{label} [{cfg[key]}]: ").strip()
        except (EOFError, KeyboardInterrupt):
            continue
        if not new_val:
            continue
        if key == "vita_port":
            try:
                new_val = int(new_val)
            except ValueError:
                print(f"{C.RED}Puerto inválido.{C.RESET}")
                continue
        cfg[key] = new_val
        cfgmod.save(cfg)
        print(f"{C.GREEN}Guardado.{C.RESET}\n")


def main():
    cfg = cfgmod.load()

    menu = [
        ("Subir VPK (instalar/reinstalar)", lambda: action_upload_vpk(cfg)),
        ("Subir solo eboot.bin (iteración rápida)", lambda: action_upload_eboot(cfg)),
        ("Subir datos del home J2ME (classes.zip + .jar)", lambda: action_upload_home_data(cfg)),
        ("Descargar log + crash dump más recientes", lambda: action_download_latest(cfg)),
        ("Elegir un log/dump específico para bajar", lambda: action_pick_remote_file(cfg)),
        ("Analizar un crash dump ya descargado", lambda: action_analyze_local(cfg)),
        ("Compilar bringup (Fase 2a)", lambda: action_build(cfg)),
        ("Probar conexión FTP", lambda: action_test_connection(cfg)),
        ("Limpiar archivos basura de macOS (._* / .DS_Store)", lambda: action_cleanup(cfg)),
        ("Configuración", lambda: action_settings(cfg)),
    ]
    subtitle = f"{cfg['vita_ip']}:{cfg['vita_port']}  ·  {cfg['titleid']}"
    tui.run_menu("VitaJ2ME Toolkit", menu, subtitle=subtitle)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(f"\n{C.DIM}Hasta luego.{C.RESET}")
        sys.exit(0)
