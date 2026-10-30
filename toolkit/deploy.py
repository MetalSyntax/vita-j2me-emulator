#!/usr/bin/env python3
"""deploy.py — versión NO interactiva del toolkit (para scripts e IA).

  python3 toolkit/deploy.py eboot             sube vita/build/eboot.bin
  python3 toolkit/deploy.py classes [tag]     classes.zip -> consola; la
                                              actual queda classes.zip.bak-<tag>
  python3 toolkit/deploy.py jars <dir>        sube los .jar/.jad nuevos a games/
  python3 toolkit/deploy.py livearea          pic0/startup a la app Y a la
                                              caché ur0:appmeta (sin reinstalar)
  python3 toolkit/deploy.py file <local> <remoto>
  python3 toolkit/deploy.py logs [N]          baja los N logs más nuevos (def. 2 sesiones)
  python3 toolkit/deploy.py dump              baja el crash dump más nuevo y lo
                                              analiza (vita-parse-core, ELF local)

Cada subida se verifica comparando tamaños. Usa toolkit/config.json
(IP/puerto de la Vita), igual que el toolkit interactivo.
"""
import os
import sys
import time
from ftplib import error_perm
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import config as cfgmod  # noqa: E402
import crash_analyzer  # noqa: E402
import ftp_ops  # noqa: E402

ROOT = cfgmod.PROJECT_DIR
HOME = "/ux0:/data/vitaj2me/"
APP = "/ux0:/app/VITAJ2ME0/"
META = "/ur0:/appmeta/VITAJ2ME0/"


def connect():
    ftp = ftp_ops.connect_with_retry(cfgmod.load())
    if not ftp:
        sys.exit("Sin FTP: activa el servidor FTP de VitaShell (SELECT).")
    return ftp


def put(ftp, local, remote):
    with open(local, "rb") as fh:
        ftp.storbinary("STOR " + remote, fh)
    got, want = ftp.size(remote), os.path.getsize(local)
    print("%s %s (%d B)" % ("OK " if got == want else "MAL", remote, got))
    if got != want:
        sys.exit("Tamaño distinto: %d vs %d" % (got, want))


def names(ftp, d):
    out = []
    ftp.cwd(d)
    ftp.retrlines("LIST", out.append)
    return {l.split(None, 8)[-1]: (l[0] == "d", int(l.split()[4]))
            for l in out}


def main(args):
    if not args:
        sys.exit(__doc__)
    cmd = args[0]
    ftp = connect()
    try:
        if cmd == "eboot":
            put(ftp, ROOT / "vita/build/eboot.bin", APP + "eboot.bin")
        elif cmd == "classes":
            tag = args[1] if len(args) > 1 else time.strftime("%Y%m%d-%H%M")
            if "classes.zip.bak-" + tag not in names(ftp, HOME):
                ftp.rename(HOME + "classes.zip",
                           HOME + "classes.zip.bak-" + tag)
                print("backup: classes.zip.bak-" + tag)
            put(ftp, ROOT / "_prebuilt/lib/classes.zip", HOME + "classes.zip")
        elif cmd == "jars":
            src = Path(args[1]).expanduser()
            have = names(ftp, HOME + "games/")
            for f in sorted(src.iterdir()):
                if f.suffix.lower() not in (".jar", ".jad"):
                    continue
                if have.get(f.name, (0, -1))[1] == f.stat().st_size:
                    print("ya estaba", f.name)
                    continue
                put(ftp, f, HOME + "games/" + f.name)
        elif cmd == "livearea":
            for name, rel in (("pic0.png", "pic0.png"),
                              ("startup.png",
                               "livearea/contents/startup.png")):
                local = ROOT / "vita/livearea" / name
                put(ftp, local, APP + "sce_sys/" + rel)
                try:
                    put(ftp, local, META + rel)
                except error_perm as e:
                    print("caché appmeta no accesible:", e)
        elif cmd == "file":
            put(ftp, args[1], args[2])
        elif cmd == "logs":
            n = int(args[1]) if len(args) > 1 else 4
            logs = sorted((k for k in names(ftp, HOME + "logs/")
                           if k.endswith(".log")),
                          key=lambda k: int("".join(c for c in k
                                                    if c.isdigit()) or 0))
            for k in logs[-n:]:
                ftp_ops.download_file(ftp, HOME + "logs", k,
                                      cfgmod.LOGS_DIR / k)
                print("bajado", cfgmod.LOGS_DIR / k)
        elif cmd == "dump":
            dumps = [k for k in names(ftp, "/ux0:/data/")
                     if k.startswith("psp2core-") and k.endswith(".psp2dmp")]
            if not dumps:
                sys.exit("No hay crash dumps.")
            k = max(dumps, key=ftp_ops.dump_epoch)
            local = cfgmod.DUMPS_DIR / k
            ftp_ops.download_file(ftp, "/ux0:/data", k, local)
            ftp_ops.quit_(ftp)
            ftp = None
            cfg = cfgmod.load()
            ok, res = crash_analyzer.analyze(cfg, local,
                                             cfgmod.resolve(cfg, "local_elf"))
            crash_analyzer.print_report(res) if ok else print(res)
        else:
            sys.exit(__doc__)
    finally:
        if ftp:
            ftp_ops.quit_(ftp)


if __name__ == "__main__":
    main(sys.argv[1:])
