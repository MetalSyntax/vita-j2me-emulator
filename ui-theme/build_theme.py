#!/usr/bin/env python3
"""
build_theme.py — rediseño total de la UI de VitaJ2ME ("Vita Light").

Genera, de forma reproducible y sin recompilar MIDP (romgen sigue
bloqueado, bug #14):

  _prebuilt/data/lib/skin.bin      colores/alturas/fuentes del skin Chameleon
  _prebuilt/data/lib/*.png         tiles del skin (mismas dimensiones que 2006)
  _prebuilt/data/appdb/*.png       iconos, chip de foco y splashes del AMS
  pspkvm/javacall/.../imageRom.c   splash nativo pre-JVM (480x272, BGR565)
  vita/livearea/*.png              LiveArea (PNG indexado, requisito Vita)
  ui-theme/preview/*.png           maquetas para comparar (NO se despliegan)

Por qué funciona sin tocar MIDP (verificado, ver port_progress.md sesión 37):
  * skin.bin se lee en runtime desde lib/ (SkinResources.loadSkin). Su
    formato es trivial (arrays int/string LE). Los IDs de propiedad son las
    constantes compiladas de SkinPropertiesIDs (classes.zip) -> skin_ids.txt.
    El skin.bin desplegado era el skin.xml ORIGINAL de 2006 (190/190 enteros
    coinciden): los cambios de skin.xml de la sesión 23 nunca llegaron.
  * Las barras título/softkeys/ticker/PTI estaban romizadas dentro de
    libmidp.a. SkinResources.getImage() cae a lib/<nombre>.png si el índice
    romizado es -1, así que basta con marcarlas -1 y aportar el PNG.
  * El framebuffer es BGR565 (BIG_ENDIAN_FRAMEBUFFER, bug #26): los PNG van
    en RGB normal (el decoder de MIDP convierte); solo imageRom.c se escribe
    a mano en BGR565.

Restricción de diseño: el AMS dibuja los nombres de la lista con colores
FIJOS en Java (AMSCustomItem: 0x3177E2 normal, 0x0B2876 enfocado) y sin
relleno de fila, directamente sobre el fondo del skin. Por eso el tema es
claro y usa esos dos colores como acento y como navy de marca.

Uso:  python3 ui-theme/build_theme.py      (desde la raíz del proyecto)
"""

import math
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent.parent
HERE = ROOT / "ui-theme"
LIB = ROOT / "_prebuilt/data/lib"
APPDB = ROOT / "_prebuilt/data/appdb"
FONTS = ROOT / "_prebuilt/data/fonts"
LIVEAREA = ROOT / "vita/livearea"
IMAGEROM = ROOT / "pspkvm/javacall/implementation/vita_arm/midp/imageRom.c"
PREVIEW = HERE / "preview"
# Commit con los PNG originales de 2006 (antes del recoloreado de la
# sesión 23): fuente de los assets que se conservan sin redibujar.
ORIG_COMMIT = "c284482"

# --------------------------------------------------------------------------
# Paleta "Vita Light"
# --------------------------------------------------------------------------
BG          = 0xF2F4F8   # fondo de pantalla
SURFACE     = 0xFFFFFF   # tarjetas, barra de softkeys, popups
LINE        = 0xDDE2EA   # divisores
LINE_STRONG = 0xC3CCD9   # bordes de controles
TEXT        = 0x1A2233
TEXT_2      = 0x5D6779
ACCENT      = 0x3177E2   # = ICON_TEXT fijo del AMS
ACCENT_DK   = 0x1F5BC4
ACCENT_SOFT = 0xE3EDFC
ACCENT_EDGE = 0xBCD2F6
NAVY        = 0x0B2876   # = ICON_HL_TEXT fijo del AMS
NAVY_DEEP   = 0x061A52
ON_DARK     = 0xFFFFFF
ON_DARK_2   = 0xB9C8EE
WARN        = 0xE59A1F
ERROR       = 0xD64545
OK          = 0x22A06B


def rgb(c, a=255):
    return ((c >> 16) & 255, (c >> 8) & 255, c & 255, a)


def mix(c1, c2, t):
    a, b = rgb(c1), rgb(c2)
    return (int(a[0] + (b[0] - a[0]) * t) << 16 |
            int(a[1] + (b[1] - a[1]) * t) << 8 |
            int(a[2] + (b[2] - a[2]) * t))


# --------------------------------------------------------------------------
# skin.bin
# --------------------------------------------------------------------------
MAGIC = bytes([0x89]) + b"CHAM"

# Constantes de javax.microedition.lcdui.Graphics
G_HCENTER, G_VCENTER, G_LEFT, G_RIGHT, G_TOP, G_BOTTOM = 1, 2, 4, 8, 16, 32

# FontResourcesConstants: <cara><estilo><tamaño>. SYS = DejaVu Sans
# Condensed (sys*.ttf); PROP es DejaVu SERIF (prop*.ttf), que es lo que
# hacía ver "viejo" el AMS. Todo pasa a SYS. S=14px M=16px L=18px.
SYS_P_S, SYS_B_S, SYS_P_M, SYS_B_M, SYS_B_L = 700, 702, 800, 802, 902

THEME = {
    # Pantalla / formularios
    "SCREEN_PAD_FORM_ITEMS": 5,
    "SCREEN_PAD_LABEL_VERT": 3,
    "SCREEN_PAD_LABEL_HORIZ": 6,
    "SCREEN_COLOR_BG": BG,
    "SCREEN_COLOR_HS_BG": BG,
    "SCREEN_COLOR_FG": TEXT,
    "SCREEN_COLOR_BG_HL": ACCENT_SOFT,
    "SCREEN_COLOR_FG_HL": NAVY,
    "SCREEN_COLOR_BORDER": LINE_STRONG,
    "SCREEN_COLOR_BORDER_HL": ACCENT,
    "SCREEN_COLOR_TRAVERSE_IND": ACCENT,
    "SCREEN_SCROLL_AMOUNT": 24,
    "SCREEN_FONT_LABEL": SYS_B_S,
    "SCREEN_FONT_INPUT_TEXT": SYS_P_S,
    "SCREEN_FONT_STATIC_TEXT": SYS_P_S,
    # Scroll: barra fina tipo "pill", sin rejilla de agarre
    "SCROLL_WIDTH": 8,
    "SCROLL_COLOR_BG": 0xE7EBF2,
    "SCROLL_COLOR_FG": 0xB7C8E6,
    "SCROLL_COLOR_FRAME": 0xB7C8E6,
    "SCROLL_COLOR_UP_ARROW": 0x4E6A9A,
    "SCROLL_COLOR_DN_ARROW": SURFACE,
    "SCROLL_COLOR_AU_BG": 0xE7EBF2,
    "SCROLL_COLOR_AU_FG": 0xB7C8E6,
    # Softkeys: barra blanca, texto acento, sin sombra (FG_SHD == FG)
    "SOFTBTN_HEIGHT": 24,
    "SOFTBTN_COLOR_FG": ACCENT_DK,
    "SOFTBTN_COLOR_FG_SHD": ACCENT_DK,
    "SOFTBTN_COLOR_BG": SURFACE,
    "SOFTBTN_COLOR_MU_FG": NAVY,
    "SOFTBTN_COLOR_MU_FG_SHD": NAVY,
    "SOFTBTN_COLOR_MU_BG": ACCENT_SOFT,
    "SOFTBTN_COLOR_AU_FG": ACCENT_DK,
    "SOFTBTN_COLOR_AU_FG_SHD": ACCENT_DK,
    "SOFTBTN_COLOR_AU_BG": SURFACE,
    "SOFTBTN_FONT": SYS_B_M,
    "SOFTBTN_BUTTON_ANCHOR_X": [10, -10],
    "SOFTBTN_BUTTON_ANCHOR_Y": [3, 3],
    "SOFTBTN_BUTTON_MAX_WIDTH": [150, 150],
    # Ticker
    "TICKER_HEIGHT": 20,
    "TICKER_COLOR_BG": ACCENT_SOFT,
    "TICKER_COLOR_FG": NAVY,
    "TICKER_COLOR_FG_SHD": NAVY,
    "TICKER_TEXT_ANCHOR_Y": 2,
    "TICKER_FONT": SYS_B_S,
    # Texto predictivo / modo de entrada
    "PTI_HEIGHT": 20,
    "PTI_MARGIN": 4,
    "PTI_COLOR_BG": SURFACE,
    "PTI_COLOR_FG": TEXT,
    "PTI_COLOR_FG_HL": ON_DARK,
    "PTI_COLOR_BG_HL": ACCENT,
    "PTI_COLOR_BDR": LINE,
    "PTI_FONT": SYS_P_S,
    "INPUT_MODE_MARGIN": 4,
    "INPUT_MODE_COLOR_BG": ACCENT,
    "INPUT_MODE_COLOR_FG": ON_DARK,
    "INPUT_MODE_COLOR_BDR": ACCENT_DK,
    "INPUT_MODE_FONT": SYS_B_S,
    # Título: barra navy con filete de acento
    "TITLE_HEIGHT": 26,
    "TITLE_MARGIN": 10,
    "TITLE_COLOR_FG": ON_DARK,
    "TITLE_COLOR_FG_SHD": ON_DARK,
    "TITLE_COLOR_BG": NAVY,
    "TITLE_FONT": SYS_B_M,
    # Alertas: tarjeta blanca con franja de acento
    "ALERT_HEIGHT": 132,
    "ALERT_MARGIN_H": 10,
    "ALERT_TITLE_HEIGHT": 36,
    "ALERT_TITLE_MARGIN": 8,
    "ALERT_PAD_HORIZ": 4,
    "ALERT_PAD_VERT": 4,
    "ALERT_COLOR_BG": SURFACE,
    "ALERT_COLOR_TITLE": NAVY,
    "ALERT_COLOR_FG": TEXT,
    "ALERT_FONT_TITLE": SYS_B_M,
    "ALERT_FONT_TEXT": SYS_P_S,
    # Cursor de espera: giro continuo (antes ida y vuelta)
    "BUSYCRSR_WIDTH": 30,
    "BUSYCRSR_HEIGHT": 30,
    "BUSYCRSR_FRAME_SEQU": list(range(1, 17)),
    # ChoiceGroup
    "CHOICE_PAD_H": 5,
    "CHOICE_PAD_V": 3,
    "CHOICE_COLOR_FG": TEXT,
    "CHOICE_COLOR_BG": SURFACE,
    "CHOICE_COLOR_BRDR": LINE_STRONG,
    "CHOICE_COLOR_BRDR_SHD": LINE,
    "CHOICE_COLOR_SCROLL": 0xE7EBF2,
    "CHOICE_COLOR_THUMB": ACCENT,
    "CHOICE_FONT": SYS_P_S,
    "CHOICE_FONT_FOCUS": SYS_B_S,
    # DateEditor / DateField
    "DATEEDITOR_COLOR_BG": SURFACE,
    "DATEEDITOR_COLOR_POPUPS_BG": ACCENT_SOFT,
    "DATEEDITOR_COLOR_BRDR": LINE_STRONG,
    "DATEEDITOR_COLOR_TRAV_IND": ACCENT,
    "DATEEDITOR_COLOR_CLK_LT": SURFACE,
    "DATEEDITOR_COLOR_CLK_DK": NAVY,
    "DATEEDITOR_FONT_POPUPS": SYS_P_S,
    "DATEFIELD_FONT": SYS_P_S,
    "DATEFIELD_COLOR_FG": TEXT,
    "DATEFIELD_COLOR_BG": SURFACE,
    "DATEFIELD_COLOR_BRDR": LINE_STRONG,
    "DATEFIELD_COLOR_BRDR_LT": LINE,
    "DATEFIELD_COLOR_BRDR_DK": LINE_STRONG,
    "DATEFIELD_COLOR_BRDR_SHD": LINE,
    # ImageItem
    "IMAGEITEM_COLOR_BG_LNK_FOC": ACCENT_SOFT,
    "IMAGEITEM_COLOR_BG_BTN": ACCENT_SOFT,
    "IMAGEITEM_COLOR_BORDER_LT": ACCENT_EDGE,
    "IMAGEITEM_COLOR_BORDER_DK": ACCENT,
    # Menú emergente: tarjeta blanca, filas de 18px, selección "pill"
    "MENU_WIDTH": 176,
    "MENU_HEIGHT": 156,
    "MENU_TITLE_X": 12,
    "MENU_TITLE_Y": 1,
    "MENU_TITLE_MAXWIDTH": 150,
    "MENU_TITLE_ALIGN": G_LEFT,
    "MENU_MAX_ITEMS": 7,
    "MENU_ITEM_HEIGHT": 18,
    "MENU_ITEM_TOPOFFSET": 24,
    "MENU_ITEM_INDEX_ANCHOR_X": 8,
    "MENU_ITEM_ANCHOR_X": 14,
    "MENU_COLOR_BG": SURFACE,
    "MENU_COLOR_BG_SEL": ACCENT,
    "MENU_COLOR_TITLE": TEXT_2,
    "MENU_COLOR_INDEX": TEXT_2,
    "MENU_COLOR_INDEX_SEL": ON_DARK,
    "MENU_COLOR_ITEM": TEXT,
    "MENU_COLOR_ITEM_SEL": ON_DARK,
    "MENU_FONT_TITLE": SYS_B_S,
    # MenuLayer mide la selección con FONT_ITEM_SEL y dibuja con
    # FONT_ITEM: tienen que ser la misma o la "pill" no encaja.
    "MENU_FONT_ITEM": SYS_P_S,
    "MENU_FONT_ITEM_SEL": SYS_P_S,
    # ProgressBar
    "PBAR_VALUE_Y": 5,
    # StringItem (links y botones)
    "STRINGITEM_PAD_BUTTON_H": 8,
    "STRINGITEM_PAD_BUTTON_V": 2,
    "STRINGITEM_COLOR_FG_LNK": ACCENT_DK,
    "STRINGITEM_COLOR_FG_LNK_FOC": NAVY,
    "STRINGITEM_COLOR_BG_LNK_FOC": ACCENT_SOFT,
    "STRINGITEM_COLOR_FG_BTN": NAVY,
    "STRINGITEM_COLOR_BG_BTN": ACCENT_SOFT,
    "STRINGITEM_COLOR_BORDER_LT": ACCENT_EDGE,
    "STRINGITEM_COLOR_BORDER_DK": ACCENT,
    "STRINGITEM_FONT": SYS_P_S,
    "STRINGITEM_FONT_LNK": SYS_P_S,
    "STRINGITEM_FONT_BTN": SYS_B_S,
    # TextField
    "TEXTFIELD_PAD_H": 5,
    "TEXTFIELD_PAD_V": 3,
    "TEXTFIELD_COLOR_FG": TEXT,
    "TEXTFIELD_COLOR_BG": SURFACE,
    "TEXTFIELD_COLOR_BRDR": LINE_STRONG,
    "TEXTFIELD_COLOR_BRDR_SHD": LINE,
    "TEXTFIELD_COLOR_FG_UE": TEXT_2,
    "TEXTFIELD_COLOR_BG_UE": 0xE9ECF1,
    "TEXTFIELD_COLOR_BRDR_UE": LINE_STRONG,
    "TEXTFIELD_COLOR_BRDR_SHD_UE": 0xE9ECF1,
}

# Barras romizadas en libmidp.a que pasan a cargarse de lib/*.png.
UNROMIZE = ("title.image_bg", "softbtn.image_", "ticker.image_",
            "pti.image_bg")


def load_ids():
    ids = {}
    for line in (HERE / "skin_ids.txt").read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        i, t, k = line.split()
        ids[k] = (int(i), t)
    return ids


def parse_skin(data):
    p = 0
    assert data[:5] == MAGIC, "skin.bin: magic inválido"
    p = 5

    def ints():
        nonlocal p
        n, = struct.unpack_from("<i", data, p); p += 4
        v = list(struct.unpack_from("<%di" % n, data, p)); p += 4 * n
        return v

    def strs():
        nonlocal p
        n, = struct.unpack_from("<i", data, p); p += 4
        out = []
        for _ in range(n):
            ln, enc = data[p], data[p + 1]; p += 2
            out.append((enc, bytes(data[p:p + ln]))); p += ln
        return out

    s = dict(ver=ints(), props=ints(), intseq=ints(), strings=strs(),
             fonts=ints(), images=strs(), rom=ints())
    assert p == len(data), "skin.bin: bytes sobrantes"
    return s


def write_skin(s):
    out = bytearray(MAGIC)

    def ints(a):
        out.extend(struct.pack("<i", len(a)))
        out.extend(struct.pack("<%di" % len(a), *a))

    def strs(a):
        out.extend(struct.pack("<i", len(a)))
        for enc, b in a:
            assert len(b) < 256
            out.extend(bytes([len(b), enc]) + b)

    ints(s["ver"]); ints(s["props"]); ints(s["intseq"]); strs(s["strings"])
    ints(s["fonts"]); strs(s["images"]); ints(s["rom"])
    return bytes(out)


def build_skin():
    orig = (HERE / "skin.orig.bin").read_bytes()
    s = parse_skin(orig)
    assert write_skin(s) == orig, "writer no reproduce skin.orig.bin"
    ids = load_ids()
    assert len(ids) == len(s["props"]) == 306
    props = s["props"]

    # Decodificar int_seq y fuentes a valores para poder reasignarlos.
    seqs, fonts = {}, {}
    for k, (i, t) in ids.items():
        if t == "integer_seq":
            off = props[i]
            n = s["intseq"][off]
            seqs[k] = s["intseq"][off + 1: off + 1 + n]
        elif t == "font":
            fonts[k] = s["fonts"][props[i]]

    for k, v in THEME.items():
        i, t = ids[k]
        if t == "integer":
            props[i] = v & 0xFFFFFFFF if v >= 0 else v
            if props[i] > 0x7FFFFFFF:
                props[i] -= 1 << 32
        elif t == "integer_seq":
            seqs[k] = list(v)
        elif t == "font":
            fonts[k] = v
        else:
            raise ValueError("tipo no soportado en THEME: %s %s" % (k, t))

    # Todas las PROP_* (serif) restantes -> SYS_* equivalente.
    for k, v in fonts.items():
        if 400 <= v < 700:
            fonts[k] = v + 300 if v < 600 else v + 300

    # Re-serializar int_seq y fuentes en orden de ID.
    s["intseq"], s["fonts"] = [], []
    for k, (i, t) in sorted(ids.items(), key=lambda x: x[1][0]):
        if t == "integer_seq":
            props[i] = len(s["intseq"])
            s["intseq"] += [len(seqs[k])] + seqs[k]
        elif t == "font":
            if fonts[k] not in s["fonts"]:
                s["fonts"].append(fonts[k])
            props[i] = s["fonts"].index(fonts[k])

    unrom = []
    for idx, (enc, name) in enumerate(s["images"]):
        n = name.decode()
        if n.startswith(UNROMIZE) and s["rom"][idx] != -1:
            s["rom"][idx] = -1
            unrom.append(n)

    data = write_skin(s)
    # Releer y comprobar lo escrito, valor a valor.
    chk = parse_skin(data)
    for k, v in THEME.items():
        i, t = ids[k]
        if t == "integer":
            assert chk["props"][i] == (v if v < 0x80000000 else v - (1 << 32)), k
        elif t == "font":
            assert chk["fonts"][chk["props"][i]] == v, k
        else:
            off = chk["props"][i]
            assert chk["intseq"][off + 1: off + 1 + len(v)] == list(v), k
    (LIB / "skin.bin").write_bytes(data)
    print("skin.bin: %d props de THEME, %d imágenes desromizadas, %d bytes"
          % (len(THEME), len(unrom), len(data)))
    return unrom


# --------------------------------------------------------------------------
# Dibujo
# --------------------------------------------------------------------------
SS = 4  # supersampling para bordes suaves


def font(bold=False, size=14):
    f = "sys_b.ttf" if bold else "sys.ttf"
    return ImageFont.truetype(str(FONTS / f), size)


def new(w, h, color=None):
    return Image.new("RGBA", (w, h), rgb(color) if color is not None
                     else (0, 0, 0, 0))


def hi(w, h, color=None):
    return new(w * SS, h * SS, color)


def lo(im, w, h):
    return im.resize((w, h), Image.LANCZOS)


def rrect(d, box, r, fill=None, outline=None, width=1):
    x0, y0, x1, y1 = box
    d.rounded_rectangle([x0, y0, x1, y1], radius=r,
                        fill=rgb(fill) if fill is not None else None,
                        outline=rgb(outline) if outline is not None else None,
                        width=width)


def save(im, path, mode=None):
    path = Path(path)
    if mode == "RGB":
        base = Image.new("RGBA", im.size, rgb(BG))
        base.alpha_composite(im)
        im = base.convert("RGB")
    im.save(path, optimize=True)


def orig_mode(path):
    try:
        return Image.open(path).mode
    except FileNotFoundError:
        return "RGBA"


def git_orig(rel):
    """Bytes del archivo en el commit de 2006 (antes del reskin 23)."""
    return subprocess.run(["git", "-C", str(ROOT), "show",
                           "%s:%s" % (ORIG_COMMIT, rel)],
                          check=True, capture_output=True).stdout


def nine(panel, sizes):
    """Corta un panel en 9 piezas con los tamaños originales.

    panel(w, h) devuelve la imagen completa del control; se renderiza una
    vez por fila con el ancho de ESA fila (las filas del skin de 2006 no
    siempre suman lo mismo) y una altura holgada, y de ahí se recorta:
    fila superior desde arriba, inferior desde abajo, central del medio.
    """
    out = []
    for row in range(3):
        ws = [sizes[row * 3 + c][0] for c in range(3)]
        W = sum(ws)
        H = 64 + max(h for _, h in sizes)
        im = panel(W, H)
        x = 0
        for c in range(3):
            w, h = sizes[row * 3 + c]
            if c == 2:
                x = W - w
            if row == 0:
                y = 0
            elif row == 2:
                y = H - h
            else:
                y = H // 2
            out.append(im.crop((x, y, x + w, y + h)))
            x += w
    return out


def three(panel, cap, mid, h):
    im = panel(cap * 2 + mid, h)
    return [im.crop((0, 0, cap, h)), im.crop((cap, 0, cap + mid, h)),
            im.crop((cap + mid, 0, cap * 2 + mid, h))]


def sizes_of(prefix, n):
    return [Image.open(LIB / ("%s%d.png" % (prefix, i))).size for i in range(n)]


def card(fill, outline, r):
    def panel(w, h):
        im = hi(w, h)
        d = ImageDraw.Draw(im)
        rrect(d, (0, 0, w * SS - 1, h * SS - 1), r * SS, fill=fill,
              outline=outline, width=SS if outline is not None else 0)
        return lo(im, w, h)
    return panel


# ---- iconos (dibujados a SS sobre una rejilla de 16) ----------------------

def icon_canvas(w=16, h=16):
    im = hi(w, h)
    return im, ImageDraw.Draw(im), SS


def ic_folder(open_=False):
    im, d, s = icon_canvas()
    back = mix(ACCENT, 0xFFFFFF, 0.15)
    rrect(d, (1 * s, 2.5 * s, 7 * s, 6 * s), 1.2 * s, fill=ACCENT_DK)
    rrect(d, (1 * s, 4 * s, 15 * s, 13.5 * s), 1.6 * s, fill=back)
    if open_:
        d.polygon([(2.2 * s, 7 * s), (15.6 * s, 7 * s), (14 * s, 13.5 * s),
                   (1 * s, 13.5 * s)], fill=rgb(mix(ACCENT, 0xFFFFFF, 0.45)))
    else:
        d.rectangle((1 * s, 6 * s, 15 * s, 6.8 * s),
                    fill=rgb(mix(ACCENT, 0xFFFFFF, 0.4)))
    return lo(im, 16, 16)


def ic_app(size=16, suite=False):
    im = hi(size, size)
    d = ImageDraw.Draw(im)
    s = SS * size / 16.0
    if suite:
        rrect(d, (5 * s, 1 * s, 15 * s, 11 * s), 2.5 * s,
              fill=mix(ACCENT, 0xFFFFFF, 0.55))
        rrect(d, (1 * s, 5 * s, 11.5 * s, 15 * s), 2.5 * s, fill=ACCENT)
        cx, cy = 6.25 * s, 10 * s
    else:
        rrect(d, (1 * s, 1 * s, 15 * s, 15 * s), 3.5 * s, fill=ACCENT)
        cx, cy = 8.3 * s, 8 * s
    r = 2.8 * s
    d.polygon([(cx - r * 0.7, cy - r), (cx - r * 0.7, cy + r),
               (cx + r * 0.95, cy)], fill=rgb(ON_DARK))
    return lo(im, size, size)


def ic_gear():
    im, d, s = icon_canvas()
    c = (8 * s, 8 * s)
    for k in range(8):
        a = k * math.pi / 4
        x, y = c[0] + math.cos(a) * 5.6 * s, c[1] + math.sin(a) * 5.6 * s
        d.ellipse((x - 1.7 * s, y - 1.7 * s, x + 1.7 * s, y + 1.7 * s),
                  fill=rgb(TEXT_2))
    d.ellipse((c[0] - 5.6 * s, c[1] - 5.6 * s, c[0] + 5.6 * s,
               c[1] + 5.6 * s), fill=rgb(TEXT_2))
    d.ellipse((c[0] - 2.3 * s, c[1] - 2.3 * s, c[0] + 2.3 * s,
               c[1] + 2.3 * s), fill=(0, 0, 0, 0))
    return lo(im, 16, 16)


def ic_wifi(color=ACCENT):
    im, d, s = icon_canvas()
    cx, cy = 8 * s, 13.2 * s
    for r in (11, 7.5):
        d.arc((cx - r * s, cy - r * s, cx + r * s, cy + r * s), 225, 315,
              fill=rgb(color), width=int(1.9 * s))
    d.ellipse((cx - 1.8 * s, cy - 1.8 * s, cx + 1.8 * s, cy + 1.8 * s),
              fill=rgb(color))
    return lo(im, 16, 16)


def ic_download():
    im, d, s = icon_canvas()
    d.rectangle((7 * s, 1.5 * s, 9 * s, 8.5 * s), fill=rgb(ACCENT))
    d.polygon([(3.8 * s, 6.5 * s), (12.2 * s, 6.5 * s), (8 * s, 11 * s)],
              fill=rgb(ACCENT))
    rrect(d, (1.5 * s, 11.5 * s, 14.5 * s, 14.6 * s), 1.2 * s, fill=TEXT_2)
    return lo(im, 16, 16)


def ic_text(label, fg=TEXT_2, size=10, bold=True):
    im, d, s = icon_canvas()
    rrect(d, (0.8 * s, 0.8 * s, 15.2 * s, 15.2 * s), 3 * s, fill=ACCENT_SOFT,
          outline=ACCENT_EDGE, width=s)
    f = font(bold, size * s)
    w = d.textlength(label, font=f)
    d.text(((16 * s - w) / 2, 2.1 * s), label, font=f, fill=rgb(fg))
    return lo(im, 16, 16)


def ic_keyboard():
    im, d, s = icon_canvas()
    rrect(d, (0.5 * s, 3.5 * s, 15.5 * s, 12.5 * s), 2 * s, fill=TEXT_2)
    for row, y in enumerate((5.3, 7.6)):
        for k in range(5):
            x = 2.2 * s + k * 2.4 * s + (row * 1.2 * s)
            if x > 13 * s:
                continue
            d.rectangle((x, y * s, x + 1.4 * s, y * s + 1.4 * s),
                        fill=rgb(SURFACE))
    d.rectangle((4.5 * s, 10 * s, 11.5 * s, 11 * s), fill=rgb(SURFACE))
    return lo(im, 16, 16)


def ic_gamepad():
    im, d, s = icon_canvas()
    rrect(d, (0.5 * s, 4 * s, 15.5 * s, 12.5 * s), 4 * s, fill=TEXT_2)
    d.rectangle((3 * s, 7.6 * s, 6.6 * s, 8.8 * s), fill=rgb(SURFACE))
    d.rectangle((4.2 * s, 6.4 * s, 5.4 * s, 10 * s), fill=rgb(SURFACE))
    for x, y in ((11.2, 6.9), (12.8, 8.6)):
        d.ellipse(((x - 0.9) * s, (y - 0.9) * s, (x + 0.9) * s,
                   (y + 0.9) * s), fill=rgb(ACCENT_SOFT))
    return lo(im, 16, 16)


def ic_grid():
    im, d, s = icon_canvas()
    for (x, y, c) in ((1, 1, ACCENT), (8.6, 1, mix(ACCENT, 0xFFFFFF, 0.4)),
                      (1, 8.6, mix(ACCENT, 0xFFFFFF, 0.4)), (8.6, 8.6, ACCENT)):
        rrect(d, (x * s, y * s, (x + 6.4) * s, (y + 6.4) * s), 1.6 * s, fill=c)
    return lo(im, 16, 16)


def ic_check_badge(w, h, color=OK, ring=None):
    im = hi(w, h)
    d = ImageDraw.Draw(im)
    r = min(w, h) * SS / 2 - SS * 0.5
    cx, cy = w * SS / 2, h * SS / 2
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=rgb(color))
    k = r / 8.0
    d.line([(cx - 3.6 * k, cy + 0.2 * k), (cx - 1 * k, cy + 2.9 * k),
            (cx + 4 * k, cy - 2.8 * k)], fill=rgb(ON_DARK),
           width=max(1, int(1.7 * k)), joint="curve")
    return lo(im, w, h)


def ic_shield(size=12):
    im = hi(size, size)
    d = ImageDraw.Draw(im)
    s = SS * size / 12.0
    d.polygon([(6 * s, 0.4 * s), (11.4 * s, 2.2 * s), (10.6 * s, 7.6 * s),
               (6 * s, 11.6 * s), (1.4 * s, 7.6 * s), (0.6 * s, 2.2 * s)],
              fill=rgb(OK))
    d.line([(3.4 * s, 6 * s), (5.3 * s, 7.8 * s), (8.7 * s, 4.1 * s)],
           fill=rgb(ON_DARK), width=int(1.3 * s))
    return lo(im, size, size)


def ic_dot(w, h, color):
    im = hi(w, h)
    d = ImageDraw.Draw(im)
    r = min(w, h) * SS * 0.32
    cx, cy = w * SS / 2, h * SS / 2
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=rgb(color))
    return lo(im, w, h)


def ic_disabled(w=17, h=13):
    im = hi(w, h)
    d = ImageDraw.Draw(im)
    r = (h - 1) * SS / 2
    cx, cy = w * SS / 2, h * SS / 2
    d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=rgb(TEXT_2),
              width=int(1.5 * SS))
    k = r * 0.7
    d.line([(cx - k, cy + k), (cx + k, cy - k)], fill=rgb(TEXT_2),
           width=int(1.5 * SS))
    return lo(im, w, h)


def alert_icon(kind):
    im = hi(32, 32)
    d = ImageDraw.Draw(im)
    s = SS
    color = {"info": ACCENT, "warn": WARN, "errr": ERROR, "alrm": NAVY,
             "cnfm": OK}[kind]
    soft = mix(color, 0xFFFFFF, 0.82)
    d.ellipse((0, 0, 32 * s - 1, 32 * s - 1), fill=rgb(soft))
    if kind == "warn":
        d.polygon([(16 * s, 6 * s), (27 * s, 25.5 * s), (5 * s, 25.5 * s)],
                  fill=rgb(color))
    else:
        d.ellipse((5 * s, 5 * s, 27 * s, 27 * s), fill=rgb(color))
    w = rgb(ON_DARK)
    if kind == "info":
        d.ellipse((14.6 * s, 9.2 * s, 17.4 * s, 12 * s), fill=w)
        rrect(d, (14.7 * s, 13.6 * s, 17.3 * s, 22.6 * s), 1.2 * s,
              fill=ON_DARK)
    elif kind == "warn":
        rrect(d, (14.8 * s, 12 * s, 17.2 * s, 19.8 * s), 1.2 * s, fill=ON_DARK)
        d.ellipse((14.7 * s, 21 * s, 17.3 * s, 23.6 * s), fill=w)
    elif kind == "errr":
        for a, b in (((11.5, 11.5), (20.5, 20.5)), ((20.5, 11.5), (11.5, 20.5))):
            d.line([(a[0] * s, a[1] * s), (b[0] * s, b[1] * s)], fill=w,
                   width=int(2.6 * s))
    elif kind == "alrm":
        d.ellipse((10.5 * s, 10.5 * s, 21.5 * s, 21.5 * s), outline=w,
                  width=int(2 * s))
        d.line([(16 * s, 13 * s), (16 * s, 16.4 * s), (18.6 * s, 17.8 * s)],
               fill=w, width=int(1.8 * s))
    elif kind == "cnfm":
        f = font(True, 17 * s)
        tw = d.textlength("?", font=f)
        d.text(((32 * s - tw) / 2, 6.2 * s), "?", font=f, fill=w)
    return lo(im, 32, 32)


# --------------------------------------------------------------------------
# lib/: tiles del skin
# --------------------------------------------------------------------------

def build_lib():
    made = []

    def put(name, im, mode=None):
        path = LIB / (name + ".png")
        save(im, path, mode or ("RGB" if orig_mode(path) == "RGB" else None))
        made.append(name)

    # Fondo: liso (antes rayado). 27x27 RGB como el original.
    put("screen.image_bg", new(27, 27, BG), "RGB")

    # Título: navy plano + filete de acento de 2px abajo.
    th = THEME["TITLE_HEIGHT"]

    def title_panel(w, h):
        im = new(w, h, NAVY)
        ImageDraw.Draw(im).rectangle((0, h - 2, w, h), fill=rgb(ACCENT))
        return im
    for i, t in enumerate(three(title_panel, 8, 32, th)):
        put("title.image_bg%d" % i, t)

    # Softkeys: superficie blanca con divisor arriba; MU (menú abierto)
    # en acento suave con filete de acento; AU (alerta) igual que normal.
    sh = THEME["SOFTBTN_HEIGHT"]

    def bar_panel(fill, line, line_w=1, top=True):
        def panel(w, h):
            im = new(w, h, fill)
            y = 0 if top else h - line_w
            ImageDraw.Draw(im).rectangle((0, y, w, y + line_w - 1),
                                         fill=rgb(line))
            return im
        return panel
    for prefix, panel in (("softbtn.image_bg", bar_panel(SURFACE, LINE)),
                          ("softbtn.image_mu_bg",
                           bar_panel(ACCENT_SOFT, ACCENT, 2)),
                          ("softbtn.image_au_bg", bar_panel(SURFACE, LINE))):
        for i, t in enumerate(three(panel, 8, 32, sh)):
            put("%s%d" % (prefix, i), t)

    # Ticker (normal + alerta) y PTI.
    tk = THEME["TICKER_HEIGHT"]
    for prefix in ("ticker.image_bg", "ticker.image_au_bg"):
        for i, t in enumerate(three(bar_panel(ACCENT_SOFT, ACCENT_EDGE,
                                              top=False), 2, 2, tk)):
            put("%s%d" % (prefix, i), t, "RGB" if "au" in prefix else None)
    for i, t in enumerate(three(bar_panel(SURFACE, LINE), 4, 16,
                                THEME["PTI_HEIGHT"])):
        put("pti.image_bg%d" % i, t)
    for name, flip in (("pti.left_arrow", True), ("pti.right_arrow", False)):
        im = hi(6, 9)
        d = ImageDraw.Draw(im)
        pts = [(1 * SS, 1 * SS), (5 * SS, 4.5 * SS), (1 * SS, 8 * SS)]
        d.line(pts, fill=rgb(ACCENT), width=int(1.6 * SS), joint="curve")
        im = lo(im, 6, 9)
        put(name, im.transpose(Image.FLIP_LEFT_RIGHT) if flip else im)

    # Menú emergente: tarjeta blanca radio 7, borde suave.
    menu = nine(card(SURFACE, LINE_STRONG, 7), sizes_of("menu.image_bg", 9))
    for i, t in enumerate(menu):
        put("menu.image_bg%d" % i, t)
    for name, color in (("menu.image_submenu", TEXT_2),
                        ("menu.image_submenu_hl", ON_DARK)):
        im = hi(7, 10)
        ImageDraw.Draw(im).line([(1.5 * SS, 1.5 * SS), (5.2 * SS, 5 * SS),
                                 (1.5 * SS, 8.5 * SS)], fill=rgb(color),
                                width=int(1.6 * SS), joint="curve")
        put(name, lo(im, 7, 10))

    # Alerta: tarjeta blanca, franja de acento arriba, bordes finos.
    def alert_panel(w, h):
        im = new(w, h, SURFACE)
        d = ImageDraw.Draw(im)
        d.rectangle((0, 0, w - 1, h - 1), outline=rgb(LINE_STRONG))
        d.rectangle((0, 0, w, 2), fill=rgb(ACCENT))
        return im
    for i, t in enumerate(nine(alert_panel, sizes_of("alert.image_bg", 9))):
        put("alert.image_bg%d" % i, t)
    for k in ("info", "warn", "errr", "alrm", "cnfm"):
        put("alert.image_icon_" + k, alert_icon(k))

    # Cursor de espera: arco de acento girando sobre anillo gris.
    def spinner(angle, arc=100):
        im = hi(30, 30)
        d = ImageDraw.Draw(im)
        b = (3 * SS, 3 * SS, 27 * SS, 27 * SS)
        d.ellipse((0, 0, 30 * SS - 1, 30 * SS - 1), fill=rgb(SURFACE, 235))
        d.arc(b, 0, 360, fill=rgb(LINE), width=int(3 * SS))
        d.arc(b, angle, angle + arc, fill=rgb(ACCENT), width=int(3 * SS))
        return lo(im, 30, 30)
    put("busycrsr.image_bg", spinner(0, 0))
    put("busycrsr.image_frame0", spinner(-90))
    for i in range(1, 17):
        put("busycrsr.image_frame%d" % i, spinner(-90 + (i - 1) * 22.5))

    # Barra de actualización: pista redondeada con segmento que va y viene
    # (UPDATEBAR_FRAME_SEQU es 1..6..2, ida y vuelta).
    def ubar(pos):
        w, h = 132, 21
        im = hi(w, h)
        d = ImageDraw.Draw(im)
        y0, y1 = 7 * SS, 14 * SS
        rrect(d, (2 * SS, y0, (w - 2) * SS, y1), 3.5 * SS, fill=LINE)
        if pos is not None:
            seg = 34
            x = 2 + pos * (w - 4 - seg) / 5.0
            rrect(d, (x * SS, y0, (x + seg) * SS, y1), 3.5 * SS, fill=ACCENT)
        return lo(im, w, h)
    put("updatebar.image_bg", ubar(None))
    put("updatebar.image_frame0", ubar(0))
    for i in range(1, 7):
        put("updatebar.image_frame%d" % i, ubar(i - 1))

    # ChoiceGroup: radio / checkbox.
    def radio(on):
        im = hi(16, 16)
        d = ImageDraw.Draw(im)
        d.ellipse((1 * SS, 1 * SS, 15 * SS, 15 * SS), fill=rgb(SURFACE),
                  outline=rgb(ACCENT if on else LINE_STRONG),
                  width=int(1.6 * SS))
        if on:
            d.ellipse((4.6 * SS, 4.6 * SS, 11.4 * SS, 11.4 * SS),
                      fill=rgb(ACCENT))
        return lo(im, 16, 16)

    def chk(on):
        im = hi(16, 16)
        d = ImageDraw.Draw(im)
        if on:
            rrect(d, (1 * SS, 1 * SS, 15 * SS, 15 * SS), 3.5 * SS, fill=ACCENT)
            d.line([(4.2 * SS, 8.2 * SS), (6.9 * SS, 10.8 * SS),
                    (11.8 * SS, 5.4 * SS)], fill=rgb(ON_DARK),
                   width=int(1.8 * SS), joint="curve")
        else:
            rrect(d, (1 * SS, 1 * SS, 15 * SS, 15 * SS), 3.5 * SS,
                  fill=SURFACE, outline=LINE_STRONG, width=int(1.6 * SS))
        return lo(im, 16, 16)
    for p in ("choice", "dateeditor"):
        put(p + ".image_radio0", radio(False))
        put(p + ".image_radio1", radio(True))
    put("choice.image_chkbx0", chk(False))
    put("choice.image_chkbx1", chk(True))

    # Campos (popup choice, datefield, stringitem botón): radio 4.
    field = card(SURFACE, LINE_STRONG, 4)
    for i, t in enumerate(nine(field, sizes_of("choice.image_btn_bg", 9))):
        put("choice.image_btn_bg%d" % i, t)
    for i, t in enumerate(nine(field, sizes_of("datefield.image_btn_bg", 9))):
        put("datefield.image_btn_bg%d" % i, t)
    for i, t in enumerate(nine(card(SURFACE, LINE_STRONG, 2),
                               sizes_of("choice.image_popup_bg", 9))):
        put("choice.image_popup_bg%d" % i, t)
    for i, t in enumerate(nine(card(ACCENT_SOFT, ACCENT_EDGE, 5),
                               sizes_of("stringitem.image_btn", 9))):
        put("stringitem.image_btn%d" % i, t)
    put("stringitem.image_lnk", new(4, 1, ACCENT))
    im = hi(16, 15)
    ImageDraw.Draw(im).line([(4 * SS, 5.5 * SS), (8 * SS, 9.5 * SS),
                             (12 * SS, 5.5 * SS)], fill=rgb(TEXT_2),
                            width=int(1.8 * SS), joint="curve")
    put("choice.image_btn_icon", lo(im, 16, 15))

    # DateField: iconos fecha / hora / fecha+hora (25x17).
    def df_icon(date, time):
        im = hi(25, 17)
        d = ImageDraw.Draw(im)
        s = SS
        x = 2
        if date:
            rrect(d, (x * s, 2 * s, (x + 12) * s, 15 * s), 2.2 * s,
                  fill=SURFACE, outline=ACCENT, width=int(1.4 * s))
            d.rectangle((x * s, 2 * s, (x + 12) * s, 6 * s), fill=rgb(ACCENT))
            for cx in (x + 3.5, x + 6.5, x + 9.5):
                d.rectangle((cx * s - s * 0.6, 9 * s, cx * s + s * 0.6,
                             10.2 * s), fill=rgb(ACCENT))
            x += 10 if time else 0
        if time:
            cx = (x + 7) if date else 12.5
            r = 6.2
            d.ellipse(((cx - r) * s, (8.5 - r) * s, (cx + r) * s,
                       (8.5 + r) * s), fill=rgb(SURFACE), outline=rgb(ACCENT),
                      width=int(1.4 * s))
            d.line([(cx * s, 5 * s), (cx * s, 8.7 * s), ((cx + 2.6) * s,
                    10 * s)], fill=rgb(ACCENT), width=int(1.3 * s))
        return lo(im, 25, 17)
    put("datefield.image_icon_date", df_icon(True, False))
    put("datefield.image_icon_time", df_icon(False, True))
    put("datefield.image_icon_datetime", df_icon(True, True))

    # Gauge interactivo y ProgressBar: pista + relleno redondeados.
    def meter(w, h, full):
        im = hi(w, h)
        d = ImageDraw.Draw(im)
        y0, y1 = (h / 2 - 3.5) * SS, (h / 2 + 3.5) * SS
        rrect(d, (1 * SS, y0, (w - 1) * SS, y1), 3.5 * SS,
              fill=ACCENT if full else LINE)
        return lo(im, w, h)
    put("gauge.image_mtr_empty", meter(61, 25, False))
    put("gauge.image_mtr_full", meter(61, 25, True))
    put("pbar.image_mtr_empty", meter(132, 22, False))
    put("pbar.image_mtr_full", meter(132, 22, True))
    put("gauge.image_bg", new(114, 25))
    put("pbar.image_bg", new(36, 22))

    def round_btn(sign):
        im = hi(20, 25)
        d = ImageDraw.Draw(im)
        c = 10 * SS, 12.5 * SS
        r = 8 * SS
        d.ellipse((c[0] - r, c[1] - r, c[0] + r, c[1] + r),
                  fill=rgb(ACCENT_SOFT), outline=rgb(ACCENT_EDGE), width=SS)
        k = 3.6 * SS
        d.line([(c[0] - k, c[1]), (c[0] + k, c[1])], fill=rgb(ACCENT_DK),
               width=int(1.8 * SS))
        if sign == "+":
            d.line([(c[0], c[1] - k), (c[0], c[1] + k)], fill=rgb(ACCENT_DK),
                   width=int(1.8 * SS))
        return lo(im, 20, 25)
    put("gauge.image_inc_btn", round_btn("+"))
    put("gauge.image_dec_btn", round_btn("-"))

    # Assets conservados (tiras de dígitos, editor de fecha, indicadores
    # semánticos): originales de 2006 tal cual. Con el fix de color #26
    # ya se ven con sus colores reales.
    kept = 0
    for f in sorted(LIB.glob("*.png")):
        n = f.name[:-4]
        if f.name.startswith("._"):   # metadatos de macOS (exFAT)
            continue
        if n not in made:
            f.write_bytes(git_orig("_prebuilt/data/lib/" + f.name))
            kept += 1
    print("lib/: %d PNG rediseñados, %d originales restaurados" %
          (len(made), kept))
    return made


# --------------------------------------------------------------------------
# appdb/: AMS
# --------------------------------------------------------------------------

def brand_mark(size, radius_frac=0.22, shadow=False):
    """Icono de marca: tile degradado acento->navy con 'J2ME'."""
    s = SS
    im = hi(size, size)
    grad = Image.new("RGBA", (size * s, size * s))
    gd = ImageDraw.Draw(grad)
    for y in range(size * s):
        t = y / float(size * s)
        gd.line([(0, y), (size * s, y)], fill=rgb(mix(ACCENT, NAVY, t * 0.95)))
    mask = Image.new("L", (size * s, size * s), 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (0, 0, size * s - 1, size * s - 1), radius=size * s * radius_frac,
        fill=255)
    im.paste(grad, (0, 0), mask)
    d = ImageDraw.Draw(im)
    f = font(True, int(size * s * 0.36))
    label = "J2ME"
    tw = d.textlength(label, font=f)
    bb = d.textbbox((0, 0), label, font=f)
    th = bb[3] - bb[1]
    y = (size * s - th) / 2 - bb[1] - size * s * 0.04
    d.text(((size * s - tw) / 2, y), label, font=f, fill=rgb(ON_DARK))
    bar_w = size * s * 0.34
    by = y + bb[3] + size * s * 0.07
    rrect(d, ((size * s - bar_w) / 2, by, (size * s + bar_w) / 2,
              by + size * s * 0.05), size * s * 0.025, fill=0x7FB0FF)
    return lo(im, size, size)


def wordmark(im, cx, y, scale=1.0, dark_bg=True, sub=None):
    d = ImageDraw.Draw(im)
    f1 = font(True, int(30 * scale))
    f2 = font(False, int(30 * scale))
    a, b = "Vita", "J2ME"
    wa, wb = d.textlength(a, font=f1), d.textlength(b, font=f2)
    x = cx - (wa + wb) / 2
    d.text((x, y), a, font=f1, fill=rgb(ON_DARK if dark_bg else NAVY))
    d.text((x + wa, y), b, font=f2,
           fill=rgb(0x7FB0FF if dark_bg else ACCENT))
    if sub:
        f3 = font(False, int(13 * scale))
        ws = d.textlength(sub, font=f3)
        d.text((cx - ws / 2, y + 40 * scale), sub, font=f3,
               fill=rgb(ON_DARK_2 if dark_bg else TEXT_2))


def backdrop(w, h):
    """Fondo de marca: degradado navy con halo de acento."""
    im = Image.new("RGBA", (w, h))
    d = ImageDraw.Draw(im)
    for y in range(h):
        d.line([(0, y), (w, y)], fill=rgb(mix(NAVY, NAVY_DEEP, y / float(h))))
    glow = Image.new("L", (w, h), 0)
    gr = int(min(w, h) * 0.55)
    ImageDraw.Draw(glow).ellipse((w // 2 - gr, int(h * 0.12) - gr // 2,
                                  w // 2 + gr, int(h * 0.12) + gr * 3 // 2),
                                 fill=90)
    glow = glow.filter(ImageFilter.GaussianBlur(min(w, h) * 0.18))
    im.paste(Image.new("RGBA", (w, h), rgb(ACCENT)), (0, 0), glow)
    return im


def splash(w, h, label=None):
    """Splash de marca ESTÁTICO: sin textos de estado ni barras de
    progreso falsas (pedido HW 041). Lo que carga de verdad lo muestra la
    pantalla de carga animada de vita/launcher.c."""
    im = backdrop(w, h)
    k = min(w / 480.0, h / 272.0)
    icon = int(64 * k)
    top = int(h * 0.26)
    im.alpha_composite(brand_mark(icon), ((w - icon) // 2, top))
    wordmark(im, w // 2, top + icon + int(14 * k), k,
             sub="Java ME for PS Vita")
    if label:
        d = ImageDraw.Draw(im)
        f = font(False, max(10, int(11 * k)))
        tw = d.textlength(label, font=f)
        d.text(((w - tw) / 2, h - int(26 * k)), label, font=f,
               fill=rgb(ON_DARK_2))
    return im


def build_appdb():
    out = {
        "_ch_hilight_bg": None,
        "folder": ic_folder(False),
        "folder_open": ic_folder(True),
        "_single8": ic_app(16),
        "_suite8": ic_app(16, suite=True),
        "_ch_single": ic_app(12),
        "_ch_suite": ic_app(12, suite=True),
        "_installer": ic_download(),
        "_wifi": ic_wifi(),
        "wifipt": ic_wifi(TEXT_2),
        "vmsettings": ic_gear(),
        "conffonts": ic_text("Aa", size=9),
        "confkb": ic_keyboard(),
        "keymaps": ic_gamepad(),
        "appgal": ic_grid(),
        "trustedmidlet_icon": ic_shield(12),
        "_marked": ic_check_badge(16, 16, ACCENT),
        "_ch_fg_requested": ic_dot(16, 13, WARN),
        "_ch_disabled": ic_disabled(),
        "_ack8": ic_check_badge(51, 43, OK),
        "_dukeok8": ic_check_badge(50, 38, OK),
    }
    # Chip de foco tras el icono. Define bgIconW/bgIconH del AMS: alto de
    # fila = max(alto chip, alto fuente) -> 24 da filas más aireadas que
    # las 18x17 de 2006, y el texto arranca a 24+2 px.
    im = hi(24, 24)
    rrect(ImageDraw.Draw(im), (0, 0, 24 * SS - 1, 24 * SS - 1), 6 * SS,
          fill=ACCENT_SOFT, outline=ACCENT_EDGE, width=SS)
    out["_ch_hilight_bg"] = lo(im, 24, 24)
    for name, im in out.items():
        save(im, APPDB / (name + ".png"))
    save(splash(480, 272, "Galería de aplicaciones"),
         APPDB / "appgal_splash_screen.png")
    save(splash(320, 240), APPDB / "splash_screen_320x240.png")
    save(splash(240, 320), APPDB / "splash_screen_240x320.png")
    print("appdb/: %d iconos + 3 splashes" % len(out))


# --------------------------------------------------------------------------
# Splash nativo (imageRom.c) y LiveArea
# --------------------------------------------------------------------------

def build_imagerom():
    im = splash(480, 272).convert("RGB")
    px = im.load()
    vals = [0x1E0, 0x110]
    for y in range(272):
        for x in range(480):
            r, g, b = px[x, y]
            # BGR565 (BIG_ENDIAN_FRAMEBUFFER): rojo en los bits BAJOS.
            vals.append(((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3))
    src = IMAGEROM.read_bytes().decode("latin1")
    head = src[:src.index("const unsigned short DukeTango[] = {")]
    tail = src[src.index("};", src.index("DukeTango[] = {")):]
    lines = []
    for i in range(0, len(vals), 30):
        lines.append("".join("0x%X, " % v for v in vals[i:i + 30]))
    body = ("/* VitaJ2ME: splash rediseñado (ui-theme/build_theme.py), "
            "480x272 BGR565. */\r\n"
            "const unsigned short DukeTango[] = {\r\n" +
            "\r\n".join(lines) + "\r\n\r\n")
    IMAGEROM.write_bytes((head + body + tail).encode("latin1"))
    save(im.convert("RGBA"), PREVIEW / "native_splash.png")
    print("imageRom.c: splash nativo regenerado (%d valores)" % len(vals))


def indexed(im):
    return im.convert("RGB").quantize(256, method=Image.MEDIANCUT,
                                      dither=Image.FLOYDSTEINBERG)


def build_livearea():
    icon = brand_mark(128, 0.0)
    indexed(icon).save(LIVEAREA / "icon0.png", optimize=True)
    bg = backdrop(840, 500)
    # Rejilla sutil en capa aparte: ImageDraw sobre RGBA reemplaza el
    # alfa en vez de mezclar (saldrían líneas blancas puras).
    grid = new(840, 500)
    d = ImageDraw.Draw(grid)
    for x in range(0, 840, 40):
        d.line([(x, 0), (x, 500)], fill=(255, 255, 255, 12))
    for y in range(0, 500, 40):
        d.line([(0, y), (840, y)], fill=(255, 255, 255, 12))
    bg.alpha_composite(grid)
    indexed(bg).save(LIVEAREA / "bg0.png", optimize=True)
    st = backdrop(280, 158)
    st.alpha_composite(brand_mark(56), (24, 51))
    d = ImageDraw.Draw(st)
    d.text((94, 52), "VitaJ2ME", font=font(True, 26), fill=rgb(ON_DARK))
    d.text((95, 86), "Java ME for PS Vita", font=font(False, 15),
           fill=rgb(ON_DARK_2))
    indexed(st).save(LIVEAREA / "startup.png", optimize=True)
    indexed(splash(960, 544)).save(LIVEAREA / "pic0.png", optimize=True)
    print("livearea/: icon0, bg0, startup, pic0 (indexados)")


# --------------------------------------------------------------------------
# Vista previa: maqueta del AMS con los assets reales (no se despliega)
# --------------------------------------------------------------------------

def preview_ams(menu_open=False, alert=False):
    W, H = 480, 272
    im = new(W, H, BG)
    lib = lambda n: Image.open(LIB / (n + ".png")).convert("RGBA")
    app = lambda n: Image.open(APPDB / (n + ".png")).convert("RGBA")
    d = ImageDraw.Draw(im)

    def bar3(prefix, y, h):
        a, b, c = (lib("%s%d" % (prefix, i)) for i in range(3))
        im.alpha_composite(a, (0, y))
        for x in range(a.width, W, b.width):
            im.alpha_composite(b, (x, y))
        im.alpha_composite(c, (W - c.width, y))

    th, sh = THEME["TITLE_HEIGHT"], THEME["SOFTBTN_HEIGHT"]
    bar3("title.image_bg", 0, th)
    fm = font(True, 16)
    d.text((THEME["TITLE_MARGIN"], (th - 2 - 19) // 2),
           "PSPKVM - http://www.pspkvm.org", font=fm, fill=rgb(ON_DARK))
    rows = [(0, "folder_open", "System midlets"),
            (1, "_installer", "Find Applications"),
            (1, "_wifi", "Network Setup"),
            (1, "vmsettings", "Configure VM"),
            (0, "folder_open", "Installed midlets"),
            (1, "_single8", "Soul of Darkness")]
    focus = 5
    y = th + THEME["SCREEN_PAD_FORM_ITEMS"]
    fi = font(True, 14)
    chip = app("_ch_hilight_bg")
    for i, (depth, ic, label) in enumerate(rows):
        h = max(chip.height, 17)
        x0 = 2 + depth * 8
        if i == focus:
            d.rectangle((1, y - 1, W - THEME["SCROLL_WIDTH"] - 2, y + h),
                        outline=rgb(ACCENT))
            im.alpha_composite(chip, (x0, y))
        icon = app(ic)
        im.alpha_composite(icon, (x0 + (chip.width - icon.width) // 2,
                                  y + (chip.height - icon.height) // 2))
        d.text((x0 + chip.width + 2, y + (h - 17) // 2), label, font=fi,
               fill=rgb(0x0B2876 if i == focus else 0x3177E2))
        y += h + THEME["SCREEN_PAD_FORM_ITEMS"]
    # scrollbar
    sw = THEME["SCROLL_WIDTH"]
    d.rectangle((W - sw, th, W, H - sh), fill=rgb(THEME["SCROLL_COLOR_BG"]))
    d.rectangle((W - sw + 1, th + 30, W, th + 120),
                fill=rgb(THEME["SCROLL_COLOR_FG"]))

    if menu_open:
        mw, mh = THEME["MENU_WIDTH"], 24 + 4 * 18 + 8
        mx, my = W - mw, H - sh - mh
        pieces = [lib("menu.image_bg%d" % i) for i in range(9)]
        panel = new(mw, mh)
        p = pieces
        panel.alpha_composite(p[0], (0, 0))
        for x in range(p[0].width, mw, p[1].width):
            panel.alpha_composite(p[1], (x, 0))
        panel.alpha_composite(p[2], (mw - p[2].width, 0))
        for yy in range(p[0].height, mh - p[6].height, p[3].height):
            panel.alpha_composite(p[3], (0, yy))
            for x in range(p[3].width, mw, p[4].width):
                panel.alpha_composite(p[4], (x, yy))
            panel.alpha_composite(p[5], (mw - p[5].width, yy))
        panel.alpha_composite(p[6], (0, mh - p[6].height))
        for x in range(p[6].width, mw, p[7].width):
            panel.alpha_composite(p[7], (x, mh - p[7].height))
        panel.alpha_composite(p[8], (mw - p[8].width, mh - p[8].height))
        im.alpha_composite(panel, (mx, my))
        bar3("softbtn.image_mu_bg", H - sh, sh)
        d.text((mx + THEME["MENU_TITLE_X"], my + THEME["MENU_TITLE_Y"]),
               "Menu", font=font(True, 14), fill=rgb(TEXT_2))
        fit = font(False, 14)
        for i, label in enumerate(("Launch", "Information", "Settings",
                                   "Remove")):
            yy = my + THEME["MENU_ITEM_TOPOFFSET"] + i * 18
            tx = mx + THEME["MENU_ITEM_ANCHOR_X"]
            if i == 0:
                rrect(d, (tx - 2, yy, tx + d.textlength(label, font=fit) + 2,
                          yy + 18), 3, fill=ACCENT)
            d.text((tx, yy), label, font=fit,
                   fill=rgb(ON_DARK if i == 0 else TEXT))
        fg = THEME["SOFTBTN_COLOR_MU_FG"]
    else:
        bar3("softbtn.image_bg", H - sh, sh)
        fg = THEME["SOFTBTN_COLOR_FG"]

    if alert:
        ah = THEME["ALERT_HEIGHT"]
        wash = Image.new("RGBA", (W, H), (6, 18, 52, 110))
        im.alpha_composite(wash)
        m = THEME["ALERT_MARGIN_H"]
        box = new(W - 2 * m, ah, SURFACE)
        bd = ImageDraw.Draw(box)
        bd.rectangle((0, 0, box.width - 1, ah - 1), outline=rgb(LINE_STRONG))
        bd.rectangle((0, 0, box.width, 2), fill=rgb(ACCENT))
        box.alpha_composite(lib("alert.image_icon_info"), (10, 8))
        bd.text((50, 13), "Information", font=font(True, 16),
                fill=rgb(NAVY))
        bd.text((12, 48), "Soul of Darkness se instaló correctamente.",
                font=font(False, 14), fill=rgb(TEXT))
        im.alpha_composite(box, (m, H - sh - ah))
    fs = font(True, 16)
    d = ImageDraw.Draw(im)
    d.text((10, H - sh + 3), "Exit", font=fs, fill=rgb(fg))
    tw = d.textlength("Menu", font=fs)
    d.text((W - 10 - tw, H - sh + 3), "Menu", font=fs, fill=rgb(fg))
    return im


def build_preview():
    PREVIEW.mkdir(exist_ok=True)
    for name, kw in (("ams_list", {}), ("ams_menu", {"menu_open": True}),
                     ("ams_alert", {"alert": True})):
        im = preview_ams(**kw)
        im.resize((960, 544), Image.NEAREST).save(PREVIEW / (name + ".png"))
    print("preview/: ams_list, ams_menu, ams_alert, native_splash")


def main():
    os.chdir(ROOT)
    build_skin()
    build_lib()
    build_appdb()
    PREVIEW.mkdir(exist_ok=True)
    build_imagerom()
    build_livearea()
    build_preview()


if __name__ == "__main__":
    main()
