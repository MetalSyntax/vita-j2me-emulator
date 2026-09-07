"""
ftp_ops.py — subir el VPK/eboot y bajar logs/crash dumps de la Vita por
FTP (servidor de VitaShell: FTP anónimo, sin cifrar, puerto configurable
-- 1337 por defecto). Sin dependencias externas, solo ftplib.
"""

import re
import socket
import sys
import time
from datetime import datetime, timedelta
from ftplib import FTP, all_errors
from pathlib import Path

from tui import C


def _local_ip_for_route(vita_ip):
    """IP local en la misma subred /24 que la Vita, para forzar la ruta
    y evitar que una VPN activa se meta por medio."""
    prefix = ".".join(vita_ip.split(".")[:3]) + "."
    try:
        for ip in socket.gethostbyname_ex(socket.gethostname())[2]:
            if ip.startswith(prefix):
                return ip
    except Exception:
        pass
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect((vita_ip, 9))
        ip = s.getsockname()[0]
        s.close()
        return ip if ip.startswith(prefix) else None
    except Exception:
        return None


def connect(cfg):
    ip = cfg["vita_ip"]
    port = cfg.get("vita_port", 1337)
    print(f"Conectando a {ip}:{port}...")
    local_ip = _local_ip_for_route(ip)
    source_addr = (local_ip, 0) if local_ip else None
    if local_ip:
        print(f"{C.DIM}(forzando ruta por {local_ip}){C.RESET}")
    try:
        ftp = FTP()
        ftp.connect(ip, port, timeout=10, source_address=source_addr)
        ftp.login()
        print(f"{C.GREEN}Conectado.{C.RESET}")
        return ftp
    except all_errors as e:
        print(f"{C.RED}No se pudo conectar: {e}{C.RESET}")
        return None


def connect_with_retry(cfg, retries=2, delay=2.0):
    """Como connect(), pero reintenta un par de veces: VitaShell a veces
    rechaza una conexión nueva justo tras cerrar la anterior."""
    ftp = connect(cfg)
    attempt = 0
    while ftp is None and attempt < retries:
        attempt += 1
        print(f"Reintentando ({attempt}/{retries})...")
        time.sleep(delay)
        ftp = connect(cfg)
    return ftp


def quit_(ftp):
    try:
        ftp.quit()
    except Exception:
        pass


def mkdir_p(ftp, path):
    """mkdir recursivo, ignorando "ya existe"."""
    parts = [p for p in path.split("/") if p]
    current = ""
    for part in parts:
        current = f"{current}/{part}"
        try:
            ftp.mkd(current)
        except all_errors:
            pass


def _progress(total_size, label=""):
    state = {"done": 0, "start": time.time(), "last": 0.0}

    def cb(block):
        state["done"] += len(block)
        now = time.time()
        finished = bool(total_size) and state["done"] >= total_size
        if not finished and now - state["last"] < 0.15:
            return
        state["last"] = now
        elapsed = max(now - state["start"], 0.001)
        kbps = (state["done"] / 1024) / elapsed
        if total_size:
            pct = min(100.0, state["done"] * 100 / total_size)
            bar_len = 24
            filled = int(bar_len * pct / 100)
            bar = "#" * filled + "-" * (bar_len - filled)
            line = f"\r  [{bar}] {pct:5.1f}%  {kbps:7.1f} KB/s  {label}"
        else:
            line = f"\r  {state['done'] / 1024:9.1f} KB  {kbps:7.1f} KB/s  {label}"
        sys.stdout.write(line)
        sys.stdout.flush()
        if finished:
            sys.stdout.write("\n")

    return cb


def _remote_size(ftp, path):
    try:
        ftp.voidcmd("TYPE I")
        return ftp.size(path)
    except all_errors:
        return None


def upload_file(ftp, local_path, remote_dir, remote_name=None):
    remote_name = remote_name or local_path.name
    mkdir_p(ftp, remote_dir)
    dest = f"{remote_dir}/{remote_name}"
    total = local_path.stat().st_size
    with open(local_path, "rb") as f:
        ftp.storbinary(f"STOR {dest}", f, callback=_progress(total, label=remote_name))
    return dest


def _parse_list_line(line):
    """Parsea una línea de LIST (formato UNIX de VitaShell): devuelve
    (nombre, es_dir, mtime_str) o None si no se pudo interpretar."""
    parts = line.split(None, 8)
    if len(parts) < 9:
        return None
    perms = parts[0]
    name = parts[8]
    is_dir = perms.startswith("d")
    mtime = " ".join(parts[5:8])
    return name, is_dir, mtime


def list_dir(ftp, path):
    lines = []
    try:
        ftp.cwd(path)
        ftp.retrlines("LIST", lines.append)
    except all_errors:
        return []
    entries = []
    for line in lines:
        parsed = _parse_list_line(line)
        if parsed and parsed[0] not in (".", ".."):
            entries.append(parsed)
    return entries


# El FTPVita de VitaShell no implementa MDTM, así que para ordenar por
# antigüedad no queda otra que: (1) el número más largo embebido en el
# nombre -- un epoch Unix real en los crash dumps (psp2core-<epoch>-...)
# o un contador incremental en logs numerados --, y (2) como desempate,
# la fecha en texto de LIST (resolución de minuto, año adivinado).

_LIST_MONTHS = {m: i + 1 for i, m in enumerate(
    ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"])}

_DIGIT_RUN_RE = re.compile(r"\d+")


def _name_number(name):
    runs = _DIGIT_RUN_RE.findall(name)
    return int(max(runs, key=len)) if runs else -1


def _parse_list_mtime(mtime_str):
    parts = mtime_str.split()
    if len(parts) != 3:
        return ""
    month = _LIST_MONTHS.get(parts[0])
    if month is None:
        return ""
    try:
        day = int(parts[1])
    except ValueError:
        return ""
    if ":" in parts[2]:
        hh, _, mm = parts[2].partition(":")
        try:
            hour, minute = int(hh), int(mm)
        except ValueError:
            return ""
        now = datetime.now()
        year = now.year
        try:
            guessed = datetime(year, month, day)
        except ValueError:
            guessed = None
        if guessed is not None and guessed - now > timedelta(days=1):
            year -= 1
    else:
        try:
            year = int(parts[2])
        except ValueError:
            return ""
        hour = minute = 0
    return f"{year:04d}{month:02d}{day:02d}{hour:02d}{minute:02d}"


_DUMP_EPOCH_RE = re.compile(r"^psp2core-(\d+)-")


def dump_epoch(name):
    """Epoch Unix embebido en un nombre `psp2core-<epoch>-...`, o None.

    Importante: estos dumps se guardan sueltos en la raíz de `ux0:data`,
    compartidos con TODO homebrew instalado en la consola -- el nombre
    no dice de qué app es. El epoch es la única forma barata de avisar
    "esto es viejo, seguro no es de esta sesión" antes de bajarlo."""
    m = _DUMP_EPOCH_RE.match(name)
    return int(m.group(1)) if m else None


def find_files(ftp, dirs, predicate):
    """Busca en varios directorios remotos (sin bajar a subcarpetas) los
    ficheros que cumplen `predicate(nombre)`. Devuelve lista de
    (directorio, nombre, mtime_str), ordenada del más antiguo al más
    reciente (ver nota sobre MDTM arriba)."""
    found = []
    for d in dirs:
        for name, is_dir, mtime in list_dir(ftp, d):
            if not is_dir and predicate(name):
                found.append((d, name, mtime))
    found.sort(key=lambda e: (_name_number(e[1]), _parse_list_mtime(e[2])))
    return found


def download_file(ftp, remote_dir, remote_name, local_path):
    ftp.cwd(remote_dir)
    total = _remote_size(ftp, remote_name)
    progress = _progress(total, label=remote_name)
    with open(local_path, "wb") as f:
        def write_and_report(block):
            f.write(block)
            progress(block)
        ftp.retrbinary(f"RETR {remote_name}", write_and_report)
