"""
tui.py — menú TUI mínimo por flechas (sin curses, sin dependencias
externas). Versión reducida del framework de psvita-port-toolkit-cli,
solo con lo que usa vitaj2me_toolkit.py: navegación con flechas, atajos
numéricos 1-9, "0"/"q" para volver, Ctrl+C siempre a salvo.
"""

import os
import shutil
import sys


class C:
    HEADER = "\033[95m"
    BLUE = "\033[94m"
    CYAN = "\033[96m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    BOLD = "\033[1m"
    DIM = "\033[2m"
    RESET = "\033[0m"


class GoBack(Exception):
    """Se lanza para abandonar el menú actual y volver al anterior."""


def clear():
    sys.stdout.write("\033[H\033[2J\033[3J")
    sys.stdout.flush()


def term_width(default=80):
    return shutil.get_terminal_size((default, 24)).columns


def getch():
    """Lee una tecla (o una secuencia de escape completa: flechas) sin
    esperar Enter. Lectura cruda vía os.read() para no perder bytes de
    secuencias multi-byte en el buffer de stdin."""
    import termios
    import tty
    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        raw = os.read(fd, 32)
        return raw.decode("utf-8", errors="replace")
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old)


def pause(msg="Presiona ENTER para continuar..."):
    print(f"\n{C.DIM}[ {C.RESET}{C.BOLD}{msg}{C.RESET}{C.DIM} ]{C.RESET}")
    try:
        input()
    except (EOFError, KeyboardInterrupt):
        pass


def confirm(prompt, default=False):
    suffix = "[S/n]" if default else "[s/N]"
    try:
        raw = input(f"{C.BOLD}{prompt}{C.RESET} {suffix} ").strip().lower()
    except (EOFError, KeyboardInterrupt):
        return False
    if not raw:
        return default
    return raw in ("s", "si", "sí", "y", "yes")


def banner(title, subtitle=None):
    width = min(term_width(), 78)
    bar = "═" * width
    print(f"{C.CYAN}{C.BOLD}╔{bar}╗{C.RESET}")
    print(f"{C.CYAN}{C.BOLD}║{C.RESET}{C.BOLD}{title.center(width)}{C.RESET}{C.CYAN}{C.BOLD}║{C.RESET}")
    if subtitle:
        print(f"{C.CYAN}{C.BOLD}║{C.RESET}{C.DIM}{subtitle.center(width)}{C.RESET}{C.CYAN}{C.BOLD}║{C.RESET}")
    print(f"{C.CYAN}{C.BOLD}╚{bar}╝{C.RESET}")
    print()


def select_list(title, items, label_fn=str, subtitle=None, allow_back=True):
    """
    Menú de selección por flechas. `items` es una lista de valores
    cualesquiera; `label_fn` decide cómo se muestran.

    Devuelve el item elegido, o None si el usuario vuelve atrás
    (flecha con "0"/"q"/Ctrl+C).
    """
    if not items:
        clear()
        banner(title, subtitle)
        print(f"{C.YELLOW}(nada para mostrar){C.RESET}")
        pause()
        return None

    idx = 0
    n = len(items)
    interactive = sys.stdin.isatty() and sys.stdout.isatty()

    if not interactive:
        # Sin TTY (pipe/CI): listado numerado simple por stdin.
        clear()
        banner(title, subtitle)
        for i, item in enumerate(items, start=1):
            print(f"  {i}. {label_fn(item)}")
        if allow_back:
            print(f"  0. Volver")
        try:
            raw = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            return None
        if raw in ("0", "q", ""):
            return None
        try:
            i = int(raw) - 1
            if 0 <= i < n:
                return items[i]
        except ValueError:
            pass
        return None

    sys.stdout.write("\033[?25l")
    sys.stdout.flush()
    try:
        while True:
            clear()
            banner(title, subtitle)
            for i, item in enumerate(items):
                label = label_fn(item)
                num = f"{i + 1:>2}." if i < 9 else "   "
                if i == idx:
                    print(f"{C.CYAN}{C.BOLD} › {num} {label}{C.RESET}")
                else:
                    print(f"   {C.DIM}{num}{C.RESET} {label}")
            print()
            back_hint = " · 0/Q volver" if allow_back else ""
            print(f"{C.DIM}↑/↓ mover · Enter elegir · 1-9 salto directo{back_hint} · Ctrl+C salir{C.RESET}")

            key = getch()

            if key in ("\x03",):  # Ctrl+C
                return None
            if key in ("\r", "\n"):
                return items[idx]
            if key in ("\x1b[A", "\x1bOA", "k"):  # arriba
                idx = (idx - 1) % n
            elif key in ("\x1b[B", "\x1bOB", "j"):  # abajo
                idx = (idx + 1) % n
            elif allow_back and key in ("0", "q", "Q", "\x1b"):
                return None
            elif key.isdigit() and key != "0":
                pick = int(key) - 1
                if 0 <= pick < n:
                    return items[pick]
    finally:
        sys.stdout.write("\033[?25h")
        sys.stdout.flush()


def run_menu(title, entries, subtitle=None):
    """
    Bucle de menú principal: `entries` es una lista de tuplas
    (etiqueta, callback). callback() se ejecuta al elegir la entrada;
    tras terminar, se pausa y se vuelve a mostrar el mismo menú.
    Elegir "volver" (None) sale del bucle.
    """
    while True:
        chosen = select_list(title, entries, label_fn=lambda e: e[0], subtitle=subtitle)
        if chosen is None:
            return
        _, callback = chosen
        clear()
        try:
            callback()
        except GoBack:
            pass
        except KeyboardInterrupt:
            print(f"\n{C.YELLOW}[!] Operación interrumpida.{C.RESET}")
        except Exception as e:
            print(f"\n{C.RED}[-] Error inesperado: {e}{C.RESET}")
        pause()
