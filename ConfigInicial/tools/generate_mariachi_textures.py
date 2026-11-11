"""Generate portable mariachi material textures and repair OBJ material maps.

Run with Blender's bundled Python, for example:
    blender --background --python generate_mariachi_textures.py -- --root <Mariachis>

The source OBJ topology and filenames are intentionally left untouched.  Only
the material maps are regenerated so the FSM pivots and hierarchy remain valid.
"""

from __future__ import annotations

import math
import re
import sys
from pathlib import Path

import bpy


WIDTH = 256
HEIGHT = 256


def clamp(value: float) -> float:
    return max(0.0, min(1.0, value))


def material_kind(folder: str, stem: str) -> str:
    stem = stem.lower()
    folder = folder.lower()
    if stem == "mono_rojo":
        return "bowtie"
    if stem == "cuerpo_sombrero":
        return "suit"
    if stem.startswith("brazo"):
        return "sleeve"
    if stem == "arco":
        return "bow"
    if "trumpet" in folder:
        return "trumpet"
    if "guitar" in folder:
        return "guitar"
    return "violin"


def color_for_kind(kind: str, kd: tuple[float, float, float]) -> tuple[float, float, float]:
    if kind in {"suit", "sleeve"}:
        return (0.025, 0.055, 0.14)
    if kind == "bowtie":
        return (0.46, 0.015, 0.025)
    if kind == "trumpet":
        return (0.80, 0.38, 0.035)
    if kind == "guitar":
        return (0.40, 0.12, 0.035)
    if kind == "violin":
        return (0.22, 0.055, 0.018)
    if kind == "bow":
        return (0.12, 0.07, 0.025)
    return tuple(clamp(value) for value in kd)


def texture_pixel(kind: str, base: tuple[float, float, float], x: int, y: int) -> tuple[float, float, float]:
    u = x / float(WIDTH - 1)
    v = y / float(HEIGHT - 1)
    wave = 0.90 + 0.08 * math.sin(u * 26.0 + v * 11.0)
    grain = 0.96 + 0.04 * math.sin(u * 83.0 + v * 17.0)
    color = [clamp(channel * wave * grain) for channel in base]

    if kind in {"suit", "sleeve"}:
        # Charro fabric: subtle navy weave, bright piping, and gold buttons.
        if abs((u * 12.0) % 1.0 - 0.5) < 0.018 or abs((v * 8.0) % 1.0 - 0.5) < 0.014:
            color = [0.16, 0.20, 0.34]
        if ((x - 24) % 58) < 9 and 0.12 < v < 0.88:
            color = [0.72, 0.46, 0.08]
        if ((x - 45) ** 2 + (y - 46) ** 2) < 32 or ((x - 45) ** 2 + (y - 178) ** 2) < 32:
            color = [1.0, 0.73, 0.16]
    elif kind == "bowtie":
        # Red moño with a golden embroidered crosshatch.
        if abs(math.sin((u + v) * math.pi * 16.0)) > 0.93:
            color = [0.95, 0.52, 0.10]
    elif kind == "trumpet":
        # Brass with dark seams and a warm highlight.
        color = [clamp(channel * (0.86 + 0.22 * u)) for channel in color]
        if abs((u * 10.0) % 1.0 - 0.5) < 0.025:
            color = [0.34, 0.12, 0.015]
        if 0.35 < u < 0.65 and abs(v - 0.50) < 0.025:
            color = [1.0, 0.82, 0.30]
    elif kind in {"guitar", "violin"}:
        # Wood grain, light bridge and a dark instrument outline.
        grain_line = abs(math.sin((u * 8.0 + v * 2.2) * math.pi))
        if grain_line > 0.93:
            color = [clamp(channel * 0.55) for channel in color]
        if abs(u - 0.50) < 0.018:
            color = [0.92, 0.66, 0.25]
        if 0.43 < v < 0.57 and 0.25 < u < 0.75:
            color = [0.94, 0.78, 0.42]
    elif kind == "bow":
        # Bow wood and pale hair.
        if v > 0.66:
            color = [0.88, 0.78, 0.52]
        elif abs(math.sin(u * math.pi * 18.0)) > 0.94:
            color = [0.62, 0.26, 0.06]

    return tuple(clamp(channel) for channel in color)


def create_texture(path: Path, kind: str, base: tuple[float, float, float]) -> None:
    image = bpy.data.images.new(path.stem, width=WIDTH, height=HEIGHT, alpha=True)
    pixels = []
    for y in range(HEIGHT):
        for x in range(WIDTH):
            red, green, blue = texture_pixel(kind, base, x, y)
            pixels.extend((red, green, blue, 1.0))
    image.pixels = pixels
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()
    bpy.data.images.remove(image)


def read_kd(mtl_path: Path) -> tuple[float, float, float]:
    for line in mtl_path.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.strip().startswith("Kd "):
            parts = line.split()
            if len(parts) >= 4:
                try:
                    return tuple(float(parts[index]) for index in (1, 2, 3))
                except ValueError:
                    pass
    return (0.72, 0.72, 0.76)


def repair_mtl(mtl_path: Path, texture_name: str) -> None:
    lines = mtl_path.read_text(encoding="utf-8", errors="replace").splitlines()
    replaced = False
    output = []
    for line in lines:
        if re.match(r"^\s*map_Kd\s+", line, flags=re.IGNORECASE):
            output.append(f"map_Kd {texture_name}")
            replaced = True
        else:
            output.append(line.rstrip())
    if not replaced:
        output.append(f"map_Kd {texture_name}")
    mtl_path.write_text("\n".join(output) + "\n", encoding="utf-8")


def create_atlas(path: Path) -> None:
    swatches = [
        ((0.025, 0.055, 0.14), "suit"),
        ((0.46, 0.015, 0.025), "bowtie"),
        ((0.80, 0.38, 0.035), "trumpet"),
        ((0.40, 0.12, 0.035), "guitar"),
        ((0.22, 0.055, 0.018), "violin"),
        ((0.88, 0.68, 0.18), "bow"),
    ]
    image = bpy.data.images.new(path.stem, width=512, height=512, alpha=True)
    pixels = []
    for y in range(512):
        for x in range(512):
            tile_x = min(2, x // 170)
            tile_y = min(1, y // 256)
            index = tile_y * 3 + tile_x
            base, kind = swatches[index]
            local_x = x % 170
            local_y = y % 256
            red, green, blue = texture_pixel(kind, base, local_x, local_y)
            pixels.extend((red, green, blue, 1.0))
    image.pixels = pixels
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()
    bpy.data.images.remove(image)


def main(root: Path) -> None:
    if not root.is_dir():
        raise SystemExit(f"Mariachi asset directory does not exist: {root}")
    mtl_files = sorted(root.rglob("*.mtl"))
    if not mtl_files:
        raise SystemExit(f"No MTL files found under {root}")

    for mtl_path in mtl_files:
        folder_name = mtl_path.parent.name
        stem = mtl_path.stem
        kind = material_kind(folder_name, stem)
        kd = read_kd(mtl_path)
        texture_path = mtl_path.with_name(f"{stem}.png")
        create_texture(texture_path, kind, color_for_kind(kind, kd))
        repair_mtl(mtl_path, texture_path.name)

    create_atlas(root / "mariachi_atlas.png")
    print(f"Generated {len(mtl_files)} material textures and repaired {len(mtl_files)} MTL files under {root}")


def parse_root() -> Path:
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if "--root" not in args:
        raise SystemExit("Usage: blender --background --python generate_mariachi_textures.py -- --root <path>")
    return Path(args[args.index("--root") + 1]).resolve()


if __name__ == "__main__":
    main(parse_root())
