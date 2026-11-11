"""Generate a tileable, non-grid water albedo for the Xochimilco channels."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path

import numpy as np

try:
    from PIL import Image
except ImportError:
    Image = None


def chunk(kind: bytes, payload: bytes) -> bytes:
    return (struct.pack(">I", len(payload)) + kind + payload
            + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))


def save_png(path: Path, rgba: np.ndarray) -> None:
    rgba = np.ascontiguousarray(rgba.astype(np.uint8))
    if Image is not None:
        Image.fromarray(rgba, "RGBA").save(path)
        return
    height, width, _ = rgba.shape
    rows = [b"\x00" + rgba[y].tobytes() for y in range(height)]
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    data = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header)
    data += chunk(b"IDAT", zlib.compress(b"".join(rows), 6))
    data += chunk(b"IEND", b"")
    path.write_bytes(data)


def generate(size: int) -> np.ndarray:
    rng = np.random.default_rng(20261110)
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32)
    u = xx / size
    v = yy / size

    # Long, directionally coherent ripples; no orthogonal texture grid.
    ripple_a = 0.5 + 0.5 * np.sin(v * 155.0 + np.sin(u * 13.0) * 8.0)
    ripple_b = 0.5 + 0.5 * np.sin(v * 87.0 + u * 19.0 + 1.7)
    ripple_c = 0.5 + 0.5 * np.sin((u + v * 0.32) * 230.0 + 0.6)

    # Tileable-looking low-frequency variation from wrapped sine fields plus
    # a small smoothed random component.
    low = rng.random((32, 32), dtype=np.float32)
    low = (low + np.roll(low, 1, 0) + np.roll(low, -1, 0)
           + np.roll(low, 1, 1) + np.roll(low, -1, 1)) / 5.0
    noise = low[(yy.astype(np.int32) * 32 // size) % 32,
                (xx.astype(np.int32) * 32 // size) % 32]
    pattern = 0.48 * ripple_a + 0.28 * ripple_b + 0.16 * ripple_c + 0.08 * noise

    # Dark marine teal base with restrained silver-blue glints.
    red = 5.0 + 7.0 * pattern
    green = 38.0 + 46.0 * pattern + 7.0 * ripple_b
    blue = 58.0 + 66.0 * pattern + 10.0 * ripple_a
    rgb = np.stack((red, green, blue), axis=-1)
    rgba = np.concatenate((np.clip(rgb, 0, 255),
                           np.full((size, size, 1), 255.0)), axis=-1)
    return rgba.astype(np.uint8)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--size", type=int, default=512)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    save_png(args.output, generate(args.size))
    print(f"generated {args.output} {args.size}x{args.size}")


if __name__ == "__main__":
    main()
