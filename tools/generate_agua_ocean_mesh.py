"""Generate the subdivided ocean-like water mesh used by the OpenGL scene.

The source mesh is intentionally a single, upward-facing surface.  Its
subdivision gives the vertex shader enough samples for smooth waves while
keeping the water from behaving like a solid box that can occlude the world.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path


def wave(x: float, z: float) -> float:
    return (0.012 * math.sin(x * 9.0 + z * 4.0)
            + 0.007 * math.cos(x * 5.0 - z * 11.0)
            + 0.004 * math.sin((x + z) * 17.0))


def wave_derivative_x(x: float, z: float) -> float:
    return (0.012 * 9.0 * math.cos(x * 9.0 + z * 4.0)
            - 0.007 * 5.0 * math.sin(x * 5.0 - z * 11.0)
            + 0.004 * 17.0 * math.cos((x + z) * 17.0))


def wave_derivative_z(x: float, z: float) -> float:
    return (0.012 * 4.0 * math.cos(x * 9.0 + z * 4.0)
            + 0.007 * 11.0 * math.sin(x * 5.0 - z * 11.0)
            + 0.004 * 17.0 * math.cos((x + z) * 17.0))


def normalize(x: float, y: float, z: float) -> tuple[float, float, float]:
    length = math.sqrt(x * x + y * y + z * z)
    return x / length, y / length, z / length


def generate(path: Path, subdivisions: int, uv_repeats: float) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    vertices: list[str] = []
    texcoords: list[str] = []
    normals: list[str] = []

    for row in range(subdivisions + 1):
        z = -1.0 + 2.0 * row / subdivisions
        for column in range(subdivisions + 1):
            x = -1.0 + 2.0 * column / subdivisions
            vertices.append(f"v {x:.7f} {wave(x, z):.7f} {z:.7f}")
            texcoords.append(
                f"vt {column / subdivisions * uv_repeats:.7f} "
                f"{row / subdivisions * uv_repeats:.7f}")
            nx, ny, nz = normalize(-wave_derivative_x(x, z), 1.0,
                                   -wave_derivative_z(x, z))
            normals.append(f"vn {nx:.7f} {ny:.7f} {nz:.7f}")

    faces: list[str] = []
    stride = subdivisions + 1
    for row in range(subdivisions):
        for column in range(subdivisions):
            a = row * stride + column + 1
            b = a + 1
            d = (row + 1) * stride + column + 1
            c = d + 1
            faces.append(f"f {a}/{a}/{a} {d}/{d}/{d} {b}/{b}/{b}")
            faces.append(f"f {b}/{b}/{b} {d}/{d}/{d} {c}/{c}/{c}")

    contents = [
        "# Xochimilco ocean-like water surface generated procedurally",
        "# Single subdivided surface: no vertical box walls can occlude islands",
        "mtllib agua.mtl",
        "o agua",
        *vertices,
        *texcoords,
        *normals,
        "usemtl material0",
        "s 1",
        *faces,
        "",
    ]
    path.write_text("\n".join(contents), encoding="utf-8", newline="\n")
    print(f"generated {path} ({len(vertices)} vertices, {len(faces)} triangles)")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--subdivisions", type=int, default=64)
    parser.add_argument("--uv-repeats", type=float, default=8.0)
    args = parser.parse_args()
    generate(args.output, args.subdivisions, args.uv_repeats)


if __name__ == "__main__":
    main()
