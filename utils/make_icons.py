#!/usr/bin/env python3
"""Regenerate x-AMP's application icons from logo.png.

The source logo is the X over its waveform with AMP lettering underneath.
Below about 48 pixels that lettering turns to mud, so the small sizes use the
X alone -- it is the recognisable part and it fills the square better, the
full logo being noticeably taller than it is wide.

Run from the repository root:

    python3 utils/make_icons.py

Writes src/app/images/<size>/qmmp.png. The file names stay as upstream has
them; the fork's name is applied at install time (see CLAUDE.md).
"""

import base64
import gzip
import io
import pathlib
import sys

from PIL import Image

#Sizes CMake installs, from src/app/CMakeLists.txt.
SIZES = [16, 32, 48, 56, 64, 128, 256]

#Under this, drop the lettering and use the X on its own.
LETTERING_MIN = 48

#The blue X ends and the lettering begins here in the 500px source. Measured
#by colour: the X is blue, the lettering near-white.
X_BOTTOM = 344

#Breathing room around the artwork, as a fraction of the icon.
PADDING = 0.04

ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE = ROOT / "logo.png"
TARGET = ROOT / "src" / "app" / "images"


def fitted(art: Image.Image, size: int) -> Image.Image:
    """Centres art on a transparent square of side size, keeping its aspect."""
    box = art.getbbox()
    if box:
        art = art.crop(box)

    inner = max(1, round(size * (1.0 - 2 * PADDING)))
    scale = min(inner / art.width, inner / art.height)
    scaled = art.resize((max(1, round(art.width * scale)),
                         max(1, round(art.height * scale))),
                        Image.LANCZOS)

    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    canvas.paste(scaled, ((size - scaled.width) // 2,
                          (size - scaled.height) // 2))
    return canvas


def write_svgz(art: Image.Image, path: pathlib.Path, size: int = 256) -> None:
    """Wraps art in an SVG and gzips it, matching the .svgz the tree ships.

    The artwork is a raster, so the result is an SVG only as a container: it
    scales by interpolation, not by redrawing. Shipping one anyway matters --
    the scalable directory is what desktops reach for above the largest PNG,
    and leaving upstream's vector there would show Qmmp's logo instead of
    this one.
    """
    square = fitted(art, size)
    buffer = io.BytesIO()
    square.save(buffer, format="PNG", optimize=True)
    encoded = base64.b64encode(buffer.getvalue()).decode("ascii")

    svg = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        f'<svg xmlns="http://www.w3.org/2000/svg" '
        f'xmlns:xlink="http://www.w3.org/1999/xlink" '
        f'width="{size}" height="{size}" viewBox="0 0 {size} {size}">\n'
        f'  <image width="{size}" height="{size}" '
        f'xlink:href="data:image/png;base64,{encoded}"/>\n'
        '</svg>\n'
    )
    with gzip.open(path, "wb") as out:
        out.write(svg.encode("utf-8"))
    print(f"{path.relative_to(ROOT)}  {size}x{size}  embedded raster")


def main() -> int:
    if not SOURCE.exists():
        print(f"missing {SOURCE}", file=sys.stderr)
        return 1

    logo = Image.open(SOURCE).convert("RGBA")
    mark = logo.crop((0, 0, logo.width, X_BOTTOM))  #the X, without lettering

    for size in SIZES:
        art = logo if size >= LETTERING_MIN else mark
        out = TARGET / f"{size}x{size}" / "qmmp.png"
        out.parent.mkdir(parents=True, exist_ok=True)
        fitted(art, size).save(out, optimize=True)
        print(f"{out.relative_to(ROOT)}  {size}x{size}"
              f"  {'full logo' if art is logo else 'mark only'}")

    #The scalable pair: the full logo for the application, the mark alone for
    #the "simple" icon that notifications and the volume OSD ask for.
    scalable = TARGET / "scalable"
    write_svgz(logo, scalable / "qmmp.svgz")
    write_svgz(mark, scalable / "qmmp-simple.svgz")
    return 0


if __name__ == "__main__":
    sys.exit(main())
