#!/usr/bin/env python3
"""Rasterize the small Lucide SVGs as recolorable LVGL A8 resources."""

from pathlib import Path
import subprocess
import tempfile

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
ICON_DIR = ROOT / "src/assets/icons"
OUTPUT = ICON_DIR / "lucide_icons.c"
SIZE = 24
ICONS = ("wifi", "wifi-off", "battery", "battery-charging")


def render_alpha(name: str) -> bytes:
    with tempfile.NamedTemporaryFile(suffix=".png") as png_file:
        subprocess.run(
            [
                "rsvg-convert",
                f"--width={SIZE}",
                f"--height={SIZE}",
                "--output",
                png_file.name,
                str(ICON_DIR / f"{name}.svg"),
            ],
            check=True,
        )
        with Image.open(png_file.name) as rendered:
            rgba = rendered.convert("RGBA")
            if rgba.size != (SIZE, SIZE):
                raise ValueError(f"Unexpected render size for {name}: {rgba.size}")
            return rgba.getchannel("A").tobytes()


def format_bytes(data: bytes) -> str:
    rows = []
    for offset in range(0, len(data), 12):
        rows.append("    " + ", ".join(f"0x{byte:02X}" for byte in data[offset : offset + 12]))
    return ",\n".join(rows)


def main() -> None:
    lines = [
        '#include "assets/icons/lucide_icons.h"',
        "",
        "#include <stddef.h>",
        "",
    ]
    for name in ICONS:
        symbol = name.replace("-", "_")
        data = render_alpha(name)
        lines.extend(
            [
                f"static const uint8_t {symbol}_alpha[] = {{",
                format_bytes(data),
                "};",
                f"const lv_image_dsc_t nexus_icon_{symbol} = {{",
                "    {LV_IMAGE_HEADER_MAGIC, LV_COLOR_FORMAT_A8, 0, 24, 24, 24, 0},",
                f"    sizeof({symbol}_alpha), {symbol}_alpha, NULL",
                "};",
                "",
            ]
        )
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
