"""Generate a material palette atlas and remap ofrenda.obj UVs.

This script intentionally uses only the Python standard library so it can run
inside Blender's Python runtime as well as from a normal Python installation.
Each OBJ face gets fresh UVs pointing at the horizontal atlas stripe assigned
to its current usemtl material. This avoids shared vt indices spanning colors.
"""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path


MATERIAL_COLORS = {
    "Barro_Jarron": (133, 69, 42, 255),
    "Calavera_Azucar_Blanca": (250, 243, 222, 255),
    "Calavera_Detalle": (53, 25, 58, 255),
    "Camino_Blanco": (247, 238, 224, 255),
    "Camino_Morado": (71, 25, 108, 255),
    "Camino_Turquesa": (24, 157, 158, 255),
    "Cempasuchil_Altar": (242, 91, 25, 255),
    "Comida_Tradicional": (170, 78, 31, 255),
    "Detalle_Oro": (216, 151, 27, 255),
    "Flama_Naranja_Emision": (255, 136, 0, 255),
    "Follaje_Altar": (55, 116, 48, 255),
    "Fotografia": (80, 52, 38, 255),
    "Mantel_Negro_Morado": (25, 14, 43, 255),
    "Marco_Fotografia": (175, 111, 32, 255),
    "Pan_Muerto_Altar": (181, 103, 43, 255),
    "Veladora_Blanca": (255, 249, 218, 255),
}


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return (struct.pack(">I", len(payload)) + kind + payload
            + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))


def write_palette_png(path: Path, width: int = 256, height: int = 256) -> None:
    stripe_height = height // len(MATERIAL_COLORS)
    colors = list(MATERIAL_COLORS.values())
    rows = []
    for y in range(height):
        stripe = min(y // stripe_height, len(colors) - 1)
        color = colors[stripe]
        row = bytearray([0])
        for _ in range(width):
            if y % stripe_height == 0 and y != 0:
                row.extend((max(color[0] - 20, 0), max(color[1] - 20, 0),
                            max(color[2] - 20, 0), color[3]))
            else:
                row.extend(color)
        rows.append(bytes(row))

    raw = b"".join(rows)
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    data = b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", header)
    data += png_chunk(b"IDAT", zlib.compress(raw, 9)) + png_chunk(b"IEND", b"")
    path.write_bytes(data)


def material_stripe(material: str) -> int:
    names = list(MATERIAL_COLORS)
    return names.index(material) if material in MATERIAL_COLORS else 0


def remap_obj(obj_path: Path, output_path: Path) -> None:
    lines = obj_path.read_text(encoding="utf-8", errors="replace").splitlines()
    original_vt_count = sum(1 for line in lines if line.startswith("vt "))
    generated_uvs = []
    output_lines = []
    current_material = "Mantel_Negro_Morado"

    for line in lines:
        if line.startswith("usemtl "):
            current_material = line.split(None, 1)[1].strip()
            output_lines.append(line)
            continue
        if line.startswith("f "):
            stripe = material_stripe(current_material)
            # Assimp loads the OBJ with aiProcess_FlipUVs. Write the inverse
            # coordinate so the post-flip sample still lands on this stripe.
            v = 1.0 - (stripe + 0.5) / len(MATERIAL_COLORS)
            remapped_face = []
            for corner in line.split()[1:]:
                parts = corner.split("/")
                generated_uvs.append((0.5, v))
                uv_index = original_vt_count + len(generated_uvs)
                if len(parts) <= 2:
                    remapped_face.append(f"{parts[0]}/{uv_index}")
                else:
                    remapped_face.append(f"{parts[0]}/{uv_index}/{parts[2]}")
            output_lines.append("f " + " ".join(remapped_face))
            continue
        output_lines.append(line)

    insert_at = next((i for i, line in enumerate(output_lines) if line.startswith("f ")),
                     len(output_lines))
    uv_lines = [f"vt {u:.8f} {v:.8f}" for u, v in generated_uvs]
    output_lines[insert_at:insert_at] = uv_lines
    output_path.write_text("\n".join(output_lines) + "\n", encoding="utf-8")


def rewrite_mtl(mtl_path: Path) -> None:
    lines = mtl_path.read_text(encoding="utf-8", errors="replace").splitlines()
    output = []
    in_material = False
    has_map = False
    for line in lines:
        if line.startswith("newmtl "):
            if in_material and not has_map:
                output.append("map_Kd ofrenda_palette.png")
            in_material = True
            has_map = False
            output.append(line)
        elif line.startswith("map_Kd "):
            if not has_map:
                output.append("map_Kd ofrenda_palette.png")
                has_map = True
        else:
            output.append(line)
    if in_material and not has_map:
        output.append("map_Kd ofrenda_palette.png")
    mtl_path.write_text("\n".join(output) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--models", type=Path, required=True)
    args = parser.parse_args()
    models = args.models
    obj_path = models / "ofrenda.obj"
    mtl_path = models / "ofrenda.mtl"
    temp_obj = models / "ofrenda.obj.remapped"
    write_palette_png(models / "ofrenda_palette.png")
    remap_obj(obj_path, temp_obj)
    temp_obj.replace(obj_path)
    rewrite_mtl(mtl_path)
    print(f"generated {models / 'ofrenda_palette.png'}")
    print(f"remapped {obj_path} and {mtl_path}")


if __name__ == "__main__":
    main()
