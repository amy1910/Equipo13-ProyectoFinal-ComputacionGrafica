"""Generate a seamless procedural Xochimilco Day-of-the-Dead skybox.

The image is evaluated from a 3D direction for every pixel instead of painting
faces independently. The six cube faces therefore share the same horizon,
stars, nebula, mountains and moon exactly at their edges. Pillow is used when
available; a small standard-library PNG writer keeps the script dependency-safe
inside Blender's Python runtime, where NumPy is available but Pillow may not be.
"""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path

import numpy as np

try:
    from PIL import Image
except ImportError:  # Blender's bundled Python can use the fallback below.
    Image = None


SIZE = 1024
FACE_NAMES = ("right", "left", "top", "bottom", "front", "back")


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return (struct.pack(">I", len(payload)) + kind + payload
            + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))


def write_png(path: Path, image: np.ndarray) -> None:
    image = np.ascontiguousarray(image.astype(np.uint8))
    if Image is not None:
        Image.fromarray(image, "RGBA").save(path)
        return
    height, width, _ = image.shape
    rows = [b"\x00" + image[y].tobytes() for y in range(height)]
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    data = b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", header)
    data += png_chunk(b"IDAT", zlib.compress(b"".join(rows), 6))
    data += png_chunk(b"IEND", b"")
    path.write_bytes(data)


def cube_direction(face: str, s: np.ndarray, t: np.ndarray) -> np.ndarray:
    if face == "right":
        xyz = np.stack((np.ones_like(s), -t, -s), axis=-1)
    elif face == "left":
        xyz = np.stack((-np.ones_like(s), -t, s), axis=-1)
    elif face == "top":
        xyz = np.stack((s, np.ones_like(s), t), axis=-1)
    elif face == "bottom":
        xyz = np.stack((s, -np.ones_like(s), -t), axis=-1)
    elif face == "front":
        xyz = np.stack((s, -t, np.ones_like(s)), axis=-1)
    else:  # back
        xyz = np.stack((-s, -t, -np.ones_like(s)), axis=-1)
    return xyz / np.linalg.norm(xyz, axis=-1, keepdims=True)


def make_star_field(seed: int = 20261110):
    rng = np.random.default_rng(seed)
    longitude = rng.uniform(-np.pi, np.pi, 36)
    altitude = rng.uniform(0.12, 0.92, 36)
    latitude = np.arcsin(altitude)
    directions = np.stack((np.cos(latitude) * np.cos(longitude),
                           np.sin(latitude),
                           np.cos(latitude) * np.sin(longitude)), axis=1)
    brightness = rng.uniform(0.45, 1.0, 36)
    radius = rng.uniform(0.0009, 0.0032, 36)
    return directions.astype(np.float32), brightness.astype(np.float32), radius.astype(np.float32)


STAR_DIRECTIONS, STAR_BRIGHTNESS, STAR_RADIUS = make_star_field()


def make_face(face: str) -> np.ndarray:
    axis = (np.arange(SIZE, dtype=np.float32) + 0.5) / SIZE * 2.0 - 1.0
    s, t = np.meshgrid(axis, axis)
    directions = cube_direction(face, s, t)
    x, y, z = directions[..., 0], directions[..., 1], directions[..., 2]
    azimuth = np.arctan2(z, x)

    # A deep indigo sky, a violet zenith and a warm low horizon.
    upper = np.clip((y + 0.04) / 1.04, 0.0, 1.0)[..., None]
    midnight = np.array([0.012, 0.018, 0.085], dtype=np.float32)
    indigo = np.array([0.045, 0.020, 0.145], dtype=np.float32)
    violet = np.array([0.16, 0.035, 0.25], dtype=np.float32)
    color = midnight + (indigo - midnight) * upper
    color += (violet - indigo) * np.clip((y - 0.28) / 0.72, 0.0, 1.0)[..., None]

    # A shared nebula band gives the whole cube one continuous Milky Way.
    nebula_center = 0.22 + 0.055 * np.sin(azimuth * 1.7 + 0.6)
    nebula = np.exp(-((y - nebula_center) / 0.10) ** 2)
    nebula *= 0.30 + 0.70 * (0.5 + 0.5 * np.sin(azimuth * 5.0 + y * 12.0))
    color += nebula[..., None] * np.array([0.035, 0.010, 0.095], dtype=np.float32)

    # Warm candle haze at the distant horizon and dark navy water below it.
    haze = np.exp(-((y + 0.015) / 0.16) ** 2)
    color += haze[..., None] * np.array([0.24, 0.022, 0.075], dtype=np.float32)
    water_mix = np.clip((-y + 0.01) / 0.58, 0.0, 1.0)
    water = np.broadcast_to(np.array([0.004, 0.012, 0.060], dtype=np.float32),
                            y.shape + (3,)).copy()
    water += (0.5 + 0.5 * np.sin(azimuth * 7.0 + y * 22.0))[..., None] \
        * np.array([0.005, 0.018, 0.030], dtype=np.float32)
    color = color * (1.0 - water_mix[..., None]) + water * water_mix[..., None]

    # Distant volcanoes, low hills and ahuejote crowns, continuous around the horizon.
    mountain_height = (0.045 + 0.080 * (0.5 + 0.5 * np.sin(azimuth * 2.4 + 0.7))
                       + 0.045 * (0.5 + 0.5 * np.sin(azimuth * 5.1 - 0.9)))
    mountain_mask = np.clip((mountain_height - y) / 0.020, 0.0, 1.0)
    mountain_mask *= (y > -0.075).astype(np.float32)
    silhouette = np.array([0.012, 0.010, 0.035], dtype=np.float32)
    color = color * (1.0 - 0.78 * mountain_mask[..., None]) \
        + silhouette * (0.78 * mountain_mask[..., None])

    crown_wave = np.sin(azimuth * 24.0 + 0.8 * np.sin(azimuth * 3.0))
    crown_height = 0.055 + 0.035 * (0.5 + 0.5 * crown_wave)
    crown_mask = np.clip((crown_height - y) / 0.012, 0.0, 1.0)
    crown_mask *= (y > -0.005).astype(np.float32) * (y < 0.09).astype(np.float32)
    color = color * (1.0 - 0.56 * crown_mask[..., None]) \
        + silhouette * (0.56 * crown_mask[..., None])

    # Nítidas estrellas with varying sizes, evaluated from direction so cube edges match.
    star_glow = np.zeros_like(y, dtype=np.float32)
    for star, brightness, radius in zip(STAR_DIRECTIONS, STAR_BRIGHTNESS, STAR_RADIUS):
        dot = (directions[..., 0] * star[0] + directions[..., 1] * star[1]
               + directions[..., 2] * star[2])
        star_glow += np.clip((dot - (1.0 - radius)) / radius, 0.0, 1.0) ** 2 * brightness
    star_glow *= (y > 0.055).astype(np.float32)
    color += star_glow[..., None] * np.array([0.68, 0.72, 0.95], dtype=np.float32)

    # Full moon and halo, placed toward the front (+Z) view.
    moon_direction = np.array([0.14, 0.43, 0.90], dtype=np.float32)
    moon_direction /= np.linalg.norm(moon_direction)
    moon_dot = (directions[..., 0] * moon_direction[0]
                + directions[..., 1] * moon_direction[1]
                + directions[..., 2] * moon_direction[2])
    halo = np.exp((moon_dot - 1.0) / 0.022) * 0.40
    disc = np.clip((moon_dot - 0.9972) / 0.0023, 0.0, 1.0)
    color += halo[..., None] * np.array([0.20, 0.20, 0.32], dtype=np.float32)
    color = color * (1.0 - disc[..., None]) + np.array([0.93, 0.90, 0.78], dtype=np.float32) * disc[..., None]

    return np.concatenate((np.clip(color, 0.0, 1.0) * 255.0,
                           np.full((SIZE, SIZE, 1), 255.0, dtype=np.float32)), axis=-1).astype(np.uint8)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-root", type=Path, required=True)
    args = parser.parse_args()
    roots = (args.project_root / "ConfigInicial" / "Models" / "skybox",
             args.project_root / "bin" / "Win32" / "Debug" / "Models" / "skybox")
    for root in roots:
        root.mkdir(parents=True, exist_ok=True)
    for face in FACE_NAMES:
        image = make_face(face)
        for root in roots:
            write_png(root / f"{face}.png", image)
        print(f"generated {face}.png")


if __name__ == "__main__":
    main()
