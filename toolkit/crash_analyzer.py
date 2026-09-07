"""
crash_analyzer.py — envuelve vita-parse-core (github.com/xerpi, clonado
en ~/vita-tools/vita-parse-core) para analizar un .psp2dmp contra el ELF
del bringup (vita/build/vitaj2me, compilado con -g).

A diferencia del analizador del toolkit grande (pensado para ports
Android con un .so JNI aparte), aquí solo hay UN ejecutable nativo, así
que no hace falta detectar base de carga de ningún .so ni cruzar con
jadx/Ghidra: basta con vita-parse-core + arm-vita-eabi-addr2line/objdump.
"""

import os
import subprocess
import sys
import time
from pathlib import Path

from tui import C
from ftp_ops import dump_epoch

STOP_REASONS = {
    0: "No reason (ejecución normal)",
    0x30002: "Undefined instruction exception (instrucción inválida/corrupta)",
    0x30003: "Prefetch abort exception (fallo al buscar instrucción en memoria)",
    0x30004: "Data abort exception (fallo de acceso a memoria / puntero nulo o inválido)",
    0x60080: "Division by zero",
}

# Cuántas palabras de la pila (por encima y por debajo de SP) se
# escanean buscando direcciones que caigan dentro de vitaj2me, igual
# que el rango por defecto de vita-parse-core/main.py.
_STACK_SCAN_BELOW = 16
_STACK_SCAN_ABOVE = 36


def toolchain_ready(cfg):
    """Comprueba que estén vita-parse-core y el toolchain de vitasdk."""
    problems = []

    vpc_dir = Path(cfg["vita_parse_core_dir"])
    main_py = vpc_dir / "main.py"
    if not main_py.exists():
        problems.append(f"No se encontró vita-parse-core en: {vpc_dir}")

    venv_python = vpc_dir / "venv" / "bin" / "python"
    python_bin = str(venv_python) if venv_python.exists() else None
    if python_bin is None:
        problems.append(
            f"No se encontró el venv de vita-parse-core en {vpc_dir}/venv "
            "(python del sistema, sin pyelftools instalado, probablemente falle)"
        )
        python_bin = "python3"

    vitasdk_bin = Path(cfg["vitasdk"]) / "bin"
    for tool in ("arm-vita-eabi-addr2line", "arm-vita-eabi-objdump"):
        if not (vitasdk_bin / tool).exists():
            problems.append(f"Falta {tool} en {vitasdk_bin}")

    return python_bin, str(main_py), str(vitasdk_bin), problems


def _elf_freshness_warning(dump_path, elf_path):
    """
    El ELF se recompila todo el tiempo durante el desarrollo (Fase 2b en
    curso). Si vita/build/vitaj2me es MÁS NUEVO que el dump, la
    resolución de símbolos queda mal sin avisar: el código pudo haberse
    movido de lugar entre el crash y la recompilación, así que
    addr2line/objdump "resuelven" nombres de función reales pero
    incorrectos (comprobado en la práctica: la misma dirección resolvió
    a dos funciones distintas en dos corridas, separadas por un rebuild
    de por medio).

    Devuelve el texto del aviso, o None si el ELF es anterior al dump
    (o si no se pudo determinar la fecha del dump por el nombre).
    """
    epoch = dump_epoch(Path(dump_path).name)
    if epoch is None:
        return (
            "No se pudo extraer la fecha del dump de su nombre de archivo "
            "(no sigue el patrón psp2core-<epoch>-...): no hay forma de "
            "saber si el ELF actual es el que estaba corriendo cuando "
            "crasheó. Si lo recompilaste después del crash, la resolución "
            "de símbolos de abajo puede ser incorrecta."
        )
    try:
        elf_mtime = Path(elf_path).stat().st_mtime
    except OSError:
        return None
    if elf_mtime > epoch:
        dump_when = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(epoch))
        elf_when = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(elf_mtime))
        return (
            f"El ELF ({elf_when}) es MÁS NUEVO que el dump ({dump_when}): "
            "se recompiló después de este crash. La resolución de "
            "símbolos de abajo puede no corresponder al código que "
            "realmente crasheó -- las funciones pudieron moverse de "
            "lugar entre medio. Para un análisis confiable hace falta el "
            "ELF exacto con el que se generó este dump."
        )
    return None


def analyze(cfg, dump_path, elf_path):
    """Ejecuta vita-parse-core y guarda la salida en
    <dump>.analysis.txt junto al dump. También genera un resumen corto
    en <dump>.triage_summary.md. Devuelve (ok, texto_o_error)."""
    dump_path = Path(dump_path)
    elf_path = Path(elf_path)

    if not dump_path.exists():
        return False, f"No existe el dump: {dump_path}"
    if not elf_path.exists():
        return False, (
            f"No existe el ELF: {elf_path}\n"
            "(¿Se compiló el bringup? El ELF con símbolos es "
            "vita/build/vitaj2me, ANTES de convertirlo a eboot.bin/self)"
        )

    python_bin, main_py, vitasdk_bin, problems = toolchain_ready(cfg)
    if not Path(main_py).exists():
        return False, "\n".join(problems)

    freshness_warning = _elf_freshness_warning(dump_path, elf_path)

    env = dict(os.environ)
    env["PATH"] = f"{vitasdk_bin}:{env.get('PATH', '')}"

    try:
        result = subprocess.run(
            [python_bin, main_py, str(dump_path), str(elf_path)],
            cwd=str(Path(main_py).parent),
            env=env,
            capture_output=True,
            text=True,
            timeout=60,
        )
    except FileNotFoundError as e:
        return False, f"No se pudo ejecutar vita-parse-core: {e}"
    except subprocess.TimeoutExpired:
        return False, "vita-parse-core tardó demasiado (¿addr2line colgado?)"

    output = result.stdout
    if result.returncode != 0:
        output += f"\n\n[vita-parse-core salió con código {result.returncode}]\n{result.stderr}"
    if freshness_warning:
        output = f"[!] {freshness_warning}\n\n{output}"

    report_path = dump_path.with_suffix(dump_path.suffix + ".analysis.txt")
    try:
        report_path.write_text(output, encoding="utf-8")
    except OSError:
        report_path = None

    triage_ok, triage_result = build_triage_summary(
        cfg, dump_path, elf_path, freshness_warning=freshness_warning
    )
    triage_path = triage_result[1] if triage_ok else None
    triage_error = None if triage_ok else triage_result

    return True, (output, report_path, triage_path, triage_error)


# ---------------------------------------------------------------------------
# Resumen corto (triage_summary.md): se construye aparte, importando
# directamente las clases de vita-parse-core en vez de scrapear el texto
# (con colores ANSI) que imprime main.py -- así el resumen usa datos
# estructurados de verdad, no un parseo frágil de la salida coloreada.
# ---------------------------------------------------------------------------

_vpc_cache = {}


def _import_vpc(cfg):
    """Importa core.CoreParser y elf.ElfParser desde vita-parse-core.
    Cachea el resultado (misma sesión, mismo vpc_dir)."""
    vpc_dir = str(Path(cfg["vita_parse_core_dir"]).resolve())
    if vpc_dir in _vpc_cache:
        return _vpc_cache[vpc_dir]
    if vpc_dir not in sys.path:
        sys.path.insert(0, vpc_dir)
    from core import CoreParser
    from elf import ElfParser
    _vpc_cache[vpc_dir] = (CoreParser, ElfParser)
    return CoreParser, ElfParser


def _stop_reason_text(code):
    return STOP_REASONS.get(code, f"Código desconocido: 0x{code:x}")


def _resolve(core, elf, exe_name, vaddr):
    """Notación 'módulo + offset (función @ archivo:línea)' para una
    dirección. addr2line solo se intenta contra el segmento ejecutable
    (seg 1) del propio vitaj2me -- para módulos del sistema (SceX) no
    tenemos símbolos ni tendría sentido."""
    notation = core.get_address_notation("", vaddr)
    if not notation.is_located():
        return f"0x{vaddr:x}", False
    mod = notation._VitaAddress__module
    seg = notation._VitaAddress__segment
    off = notation._VitaAddress__offset
    base = f"0x{vaddr:x} [{mod.name}@{seg.num} + 0x{off:x}]"
    if mod.name == exe_name and seg.num == 1:
        try:
            line_info = elf.addr2line(off)
            line_str = line_info.decode("utf-8", "ignore") if isinstance(line_info, bytes) else str(line_info)
        except Exception:
            line_str = ""
        if line_str and "?? ??:0" not in line_str:
            base += f" ({line_str.strip()})"
        return base, True
    return base, (mod.name == exe_name)


def build_triage_summary(cfg, dump_path, elf_path, freshness_warning=None):
    """Genera <dump_path>.triage_summary.md: causa del crash, PC/LR
    resueltos, registros en 0x0 (posible puntero nulo) y la cadena de
    direcciones de la pila que caen dentro de vitaj2me. Devuelve
    (True, (texto, ruta)) o (False, mensaje_de_error)."""
    dump_path = Path(dump_path)
    elf_path = Path(elf_path)
    exe_name = elf_path.name

    try:
        CoreParser, ElfParser = _import_vpc(cfg)
    except ImportError as e:
        return False, f"No se pudo importar vita-parse-core para el resumen: {e}"

    vitasdk_bin = str(Path(cfg["vitasdk"]) / "bin")
    old_path = os.environ.get("PATH", "")
    if vitasdk_bin not in old_path:
        os.environ["PATH"] = f"{vitasdk_bin}:{old_path}"

    try:
        core = CoreParser(str(dump_path))
        elf = ElfParser(str(elf_path))
    except Exception as e:
        return False, f"No se pudo parsear el dump o el ELF: {e}"

    try:
        lines = _triage_lines(core, elf, exe_name, dump_path, freshness_warning)
    finally:
        # ElfParser mantiene un subproceso addr2line abierto de por vida;
        # sin esto, cada análisis en una misma sesión del TUI deja un
        # proceso huérfano más.
        elf.close_addr2line()

    text = "\n".join(lines)
    out_path = dump_path.with_suffix(dump_path.suffix + ".triage_summary.md")
    try:
        out_path.write_text(text, encoding="utf-8")
    except OSError as e:
        return False, f"No se pudo guardar el resumen: {e}"

    return True, (text, out_path)


def _triage_lines(core, elf, exe_name, dump_path, freshness_warning):
    crashed = [t for t in core.threads if t.stop_reason != 0] or core.threads[:1]

    lines = [f"# Triage: {dump_path.name}", ""]
    if freshness_warning:
        lines += ["> [!] " + freshness_warning, ""]

    for thread in crashed:
        lines.append(f"## Hilo `{thread.name}` (0x{thread.uid:x})")
        lines.append("")
        lines.append(f"- **Causa:** {_stop_reason_text(thread.stop_reason)}")

        pc_str, _ = _resolve(core, elf, exe_name, thread.pc)
        lr = thread.regs.gpr[14]
        lr_str, _ = _resolve(core, elf, exe_name, lr)
        lines.append(f"- **PC:** `{pc_str}`")
        lines.append(f"- **LR:** `{lr_str}`")
        if thread.pc == 0:
            lines.append(
                "  - PC en 0x0: se saltó a través de un puntero a función "
                "nulo o inválido. El sitio real a revisar es LR (de dónde "
                "vino el salto), no PC."
            )
        lines.append("")

        zero_regs = [i for i in range(13) if thread.regs.gpr[i] == 0]
        if zero_regs:
            regs_str = ", ".join(f"r{i}" for i in zero_regs)
            lines.append(f"- **Registros en 0x0 (posible puntero nulo):** {regs_str}")
            lines.append("")

        sp = thread.regs.gpr[13]
        chain = []
        seen = set()
        for x in range(-_STACK_SCAN_BELOW, _STACK_SCAN_ABOVE):
            addr = sp + 4 * x
            data = core.read_vaddr(addr, 4)
            if not data:
                continue
            val = int.from_bytes(data[:4], "little")
            resolved, in_exe = _resolve(core, elf, exe_name, val)
            if in_exe and resolved not in seen:
                seen.add(resolved)
                chain.append(resolved)

        if chain:
            lines.append("### Cadena de llamadas (direcciones de la pila dentro de vitaj2me)")
            lines.append("")
            for frame in chain[:20]:
                lines.append(f"- `{frame}`")
            lines.append("")

        lines.append("### Sugerencia")
        lines.append("")
        if zero_regs and thread.stop_reason == 0x30004:
            lines.append(
                "Data abort con algún registro en 0x0: muy probablemente "
                "un puntero nulo desreferenciado. Revisar la línea "
                "resuelta de LR/PC de arriba."
            )
        elif thread.pc == 0:
            lines.append(
                "Salto a dirección nula: revisar qué puntero a función "
                "se llamó desde LR -- probablemente una tabla de "
                "callbacks sin inicializar o un puntero liberado."
            )
        elif thread.stop_reason == 0x60080:
            lines.append("División por cero: revisar la línea resuelta de PC.")
        else:
            lines.append(
                "Revisar la línea resuelta de PC/LR y la cadena de "
                "llamadas de arriba; el informe completo "
                f"(`{dump_path.name}.analysis.txt`) trae además el "
                "desensamblado alrededor del crash y todos los registros."
            )
        lines.append("")

    return lines


def print_report(result):
    """`result` es la tupla que devuelve analyze(): (texto, report_path,
    triage_path, triage_error)."""
    if isinstance(result, tuple) and len(result) == 4:
        text, report_path, triage_path, triage_error = result
    elif isinstance(result, tuple):
        text, report_path = result
        triage_path = triage_error = None
    else:
        text, report_path, triage_path, triage_error = result, None, None, None

    print(text)
    if report_path:
        print(f"\n{C.GREEN}Informe completo guardado en: {report_path}{C.RESET}")
    if triage_path:
        print(f"{C.GREEN}Resumen guardado en: {triage_path}{C.RESET}")
    elif triage_error:
        print(f"{C.YELLOW}No se pudo generar el resumen: {triage_error}{C.RESET}")
