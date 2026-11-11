"""Generate the Finado/Moribunda sugar-skull assets for Xochimilco.

The script intentionally produces simple, readable procedural geometry rather
than depending on an external Blender file.  Each exported OBJ has UVs and
normals, its MTL uses only relative PNG names, and all output stays below the
project's ConfigInicial tree.

Run from Blender:
    blender --background --python generate_finando_moribunda.py -- --root <ConfigInicial>
"""

from __future__ import annotations

import math
import re
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Vector


TEX_SIZE = 128
OUTPUT_FOLDER = Path("Models") / "FinadoMoribunda"

MATERIAL_DEFS = {
    "calaverita_blanco": ((0.92, 0.88, 0.96), (0.55, 0.26, 0.78)),
    "calaverita_gris": ((0.38, 0.40, 0.46), (0.70, 0.72, 0.80)),
    "finado_face": ((0.95, 0.86, 0.66), (0.08, 0.42, 0.92)),
    "moribunda_face": ((0.91, 0.82, 0.94), (0.86, 0.14, 0.62)),
    "moribunda_lila": ((0.72, 0.22, 0.58), (0.95, 0.53, 0.82)),
    "moribunda_morado": ((0.25, 0.05, 0.36), (0.62, 0.18, 0.90)),
    "moribunda_oro": ((0.95, 0.48, 0.06), (1.0, 0.84, 0.24)),
    "moribunda_madera": ((0.25, 0.08, 0.025), (0.58, 0.25, 0.08)),
    "finado_marfil": ((0.96, 0.88, 0.68), (0.28, 0.55, 0.92)),
    "finado_verde": ((0.08, 0.38, 0.17), (0.20, 0.78, 0.35)),
    "finado_azul": ((0.025, 0.08, 0.30), (0.10, 0.38, 0.92)),
    "finado_rojo": ((0.64, 0.04, 0.025), (1.0, 0.28, 0.05)),
    "finado_naranja": ((0.92, 0.22, 0.025), (1.0, 0.70, 0.08)),
    "finado_oro": ((0.86, 0.40, 0.025), (1.0, 0.86, 0.20)),
    "finado_blanco": ((0.93, 0.93, 0.86), (1.0, 0.75, 0.28)),
    "negro": ((0.005, 0.004, 0.012), (0.08, 0.015, 0.16)),
}


def clamp(value: float) -> float:
    return max(0.0, min(1.0, value))


def texture_color(material_name: str, x: int, y: int) -> tuple[float, float, float]:
    base, accent = MATERIAL_DEFS[material_name]
    u = x / float(TEX_SIZE - 1)
    v = y / float(TEX_SIZE - 1)
    pulse = 0.90 + 0.08 * math.sin(u * 23.0 + v * 15.0)
    color = [clamp(channel * pulse) for channel in base]
    accent_line = abs(math.sin((u * 8.0 + v * 3.0) * math.pi)) > 0.94

    if material_name in {"finado_face", "moribunda_face"}:
        # A painted front-facing face map. The 3D eye sockets and ornaments
        # remain on top of it, so the design stays readable from any angle.
        if 0.21 < u < 0.39 or 0.61 < u < 0.79:
            eye = ((u - 0.30) / 0.10) ** 2 + ((v - 0.48) / 0.15) ** 2 < 1.0
            if eye:
                color = [0.015, 0.008, 0.025]
                spiral = abs(math.sin((u * 110.0 + v * 90.0) * math.pi)) > 0.72
                if spiral:
                    color = list(accent)
        if 0.35 < u < 0.65 and 0.70 < v < 0.90:
            color = list(accent)
        if abs(u - 0.5) < 0.025 and 0.39 < v < 0.62:
            color = [0.01, 0.005, 0.015]
        if 0.32 < v < 0.37 and (abs(u - 0.30) < 0.14 or abs(u - 0.70) < 0.14):
            color = list(accent)
    elif "calaverita" in material_name or "marfil" in material_name:
        if accent_line or abs(u - 0.5) < 0.012:
            color = list(accent)
        if ((x - 32) ** 2 + (y - 38) ** 2) < 22:
            color = [0.15, 0.04, 0.20]
    elif material_name == "negro":
        color = [clamp(channel * (0.8 + 0.18 * math.sin(v * 30.0))) for channel in base]
    elif "oro" in material_name:
        color = [clamp(channel * (0.75 + 0.32 * u)) for channel in accent]
        if accent_line:
            color = list(base)
    elif "madera" in material_name:
        grain = 0.70 + 0.30 * abs(math.sin(u * 44.0 + v * 4.0))
        color = [clamp(channel * grain) for channel in base]
        if abs(u - 0.5) < 0.02:
            color = list(accent)
    elif "azul" in material_name or "verde" in material_name:
        if accent_line:
            color = list(accent)
    elif material_name in {"moribunda_lila", "moribunda_morado", "finado_rojo", "finado_naranja"}:
        if accent_line:
            color = list(accent)
    return tuple(clamp(channel) for channel in color)


def create_texture(output: Path, material_name: str) -> None:
    image = bpy.data.images.new(output.stem, width=TEX_SIZE, height=TEX_SIZE, alpha=True)
    pixels = []
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            red, green, blue = texture_color(material_name, x, y)
            pixels.extend((red, green, blue, 1.0))
    image.pixels = pixels
    image.filepath_raw = str(output)
    image.file_format = "PNG"
    image.save()
    bpy.data.images.remove(image)


def make_material(name: str, texture_path: Path):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    bsdf = next((node for node in nodes if node.type == "BSDF_PRINCIPLED"), None)
    if bsdf is None:
        bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    image = bpy.data.images.load(str(texture_path), check_existing=True)
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    links.new(texture.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.48
    return material


def assign(obj, material) -> object:
    obj.data.materials.append(material)
    return obj


def uv_sphere(location, scale, material):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=16, location=location)
    obj = bpy.context.object
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return assign(obj, material)


def cube(location, scale, material, rotation=(0.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return assign(obj, material)


def cylinder_between(start, end, radius, material, vertices=16):
    start_vec = Vector(start)
    end_vec = Vector(end)
    direction = end_vec - start_vec
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices, radius=radius, depth=max(direction.length, 0.001),
        location=(start_vec + end_vec) * 0.5)
    obj = bpy.context.object
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = direction.to_track_quat("Z", "Y")
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    return assign(obj, material)


def torus(location, major_radius, minor_radius, material, rotation=(0.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major_radius, minor_radius=minor_radius,
        major_segments=32, minor_segments=10, location=location, rotation=rotation)
    return assign(bpy.context.object, material)


def text_mesh(body, location, size, material):
    bpy.ops.object.text_add(location=location)
    obj = bpy.context.object
    obj.data.body = body
    obj.data.align_x = "CENTER"
    obj.data.align_y = "CENTER"
    obj.data.size = size
    obj.data.extrude = 0.018
    obj.data.bevel_depth = 0.006
    obj.data.materials.append(material)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    return obj


def skull_face(materials, character: str, head_objects: list):
    skull = materials["moribunda_face" if character == "moribunda" else "finado_face"]
    white = materials["calaverita_blanco"]
    gray = materials["calaverita_gris"]
    dark = materials["negro"]
    purple = materials["moribunda_morado"]
    blue = materials["finado_azul"]
    gold = materials["moribunda_oro" if character == "moribunda" else "finado_oro"]

    # The skull is deliberately oversized: it is about 55% of the full
    # character height, giving the requested friendly chibi silhouette.
    head_objects.append(uv_sphere((0.0, 2.08, 0.0), (0.72, 0.83, 0.62), skull))
    for x in (-0.23, 0.23):
        head_objects.append(uv_sphere((x, 2.18, 0.58), (0.20, 0.23, 0.065), dark))
        for ring_radius in (0.11, 0.055):
            head_objects.append(torus((x, 2.18, 0.655), ring_radius, 0.020,
                purple if character == "moribunda" else blue))
    bpy.ops.mesh.primitive_cone_add(vertices=3, radius1=0.10, radius2=0.025,
        depth=0.18, location=(0.0, 2.00, 0.60), rotation=(math.pi * 0.5, 0.0, 0.0))
    head_objects.append(assign(bpy.context.object, dark))
    head_objects.append(torus((0.0, 1.83, 0.60), 0.25, 0.023, dark,
        rotation=(math.pi * 0.5, 0.0, 0.0)))
    # Three rounded teeth/waves form a soft cartoon jaw instead of a square
    # detached mandible.
    for x in (-0.14, 0.0, 0.14):
        head_objects.append(uv_sphere((x, 1.69, 0.62), (0.095, 0.10, 0.055), white))

    letter = "M" if character == "moribunda" else "F"
    head_objects.append(text_mesh(letter, (0.0, 2.66, 0.62), 0.24, gold))
    for x in (-0.22, 0.22):
        head_objects.append(torus((x, 2.60, 0.61), 0.10, 0.014, gold))

    if character == "moribunda":
        bow = materials["moribunda_morado"]
        wood = materials["moribunda_madera"]
        head_objects.extend([
            uv_sphere((-0.18, 2.92, 0.0), (0.20, 0.14, 0.07), bow),
            uv_sphere((0.18, 2.92, 0.0), (0.20, 0.14, 0.07), bow),
            uv_sphere((0.0, 2.92, 0.0), (0.08, 0.10, 0.08), gold),
        ])
        for side in (-1.0, 1.0):
            for index in range(6):
                y = 1.72 - index * 0.19
                x = side * (0.62 + 0.035 * math.sin(index * 1.4))
                head_objects.append(uv_sphere((x, y, -0.03), (0.13, 0.16, 0.12), wood))
        head_objects.extend([
            uv_sphere((-0.58, 1.03, -0.03), (0.12, 0.08, 0.06), bow),
            uv_sphere((-0.72, 1.03, -0.03), (0.12, 0.08, 0.06), bow),
            uv_sphere((0.58, 1.03, -0.03), (0.12, 0.08, 0.06), bow),
            uv_sphere((0.72, 1.03, -0.03), (0.12, 0.08, 0.06), bow),
        ])
    else:
        for index, color in enumerate(("finado_azul", "finado_naranja", "finado_verde")):
            head_objects.append(torus((-0.23 + index * 0.23, 2.78, 0.50),
                0.085, 0.016, materials[color]))


def build_head(character: str, materials):
    objects = []
    skull_face(materials, character, objects)
    return objects


def build_body(character: str, materials):
    objects = []
    if character == "moribunda":
        dress = materials["moribunda_lila"]
        purple = materials["moribunda_morado"]
        gold = materials["moribunda_oro"]
        white = materials["calaverita_blanco"]
        gray = materials["calaverita_gris"]
        wood = materials["moribunda_madera"]
        bpy.ops.mesh.primitive_cone_add(vertices=32, radius1=0.68, radius2=0.38,
            depth=1.25, location=(0.0, 0.72, 0.0))
        objects.append(assign(bpy.context.object, dress))
        bpy.ops.mesh.primitive_cylinder_add(vertices=20, radius=0.16, depth=0.22,
            location=(0.0, 1.48, 0.0), rotation=(math.pi * 0.5, 0.0, 0.0))
        objects.append(assign(bpy.context.object, white))
        for y in (1.42, 1.51):
            objects.append(torus((0.0, y, 0.0), 0.16, 0.012, gray,
                rotation=(math.pi * 0.5, 0.0, 0.0)))
        objects.extend([
            cube((0.0, 1.25, 0.40), (0.24, 0.32, 0.06), purple),
            cube((-0.16, 1.40, 0.43), (0.045, 0.24, 0.025), gold,
                rotation=(0.0, 0.0, math.radians(-28.0))),
            cube((0.16, 1.40, 0.43), (0.045, 0.24, 0.025), gold,
                rotation=(0.0, 0.0, math.radians(28.0))),
        ])
        objects.append(cylinder_between((-0.34, 1.13, 0.0), (-0.67, 1.26, 0.18), 0.09, dress))
        objects.append(cylinder_between((0.34, 1.13, 0.0), (0.67, 1.18, 0.18), 0.09, dress))
        objects.extend([
            uv_sphere((-0.70, 1.28, 0.22), (0.12, 0.12, 0.12), white),
            uv_sphere((0.70, 1.20, 0.20), (0.12, 0.12, 0.12), white),
        ])
        # White flower with a yellow center in the raised left hand.
        for index in range(5):
            angle = index * math.tau / 5.0
            objects.append(uv_sphere((-0.78 + math.cos(angle) * 0.09,
                1.40 + math.sin(angle) * 0.09, 0.28), (0.07, 0.07, 0.035), white))
        objects.append(uv_sphere((-0.78, 1.40, 0.30), (0.055, 0.055, 0.04), gold))
        for x in (-0.25, 0.25):
            objects.append(uv_sphere((x, 0.10, 0.08), (0.23, 0.12, 0.28), white))
            objects.append(cube((x, 0.10, 0.28), (0.025, 0.045, 0.16), purple))
    else:
        green = materials["finado_verde"]
        blue = materials["finado_azul"]
        red = materials["finado_rojo"]
        orange = materials["finado_naranja"]
        gold = materials["finado_oro"]
        white = materials["finado_blanco"]
        gray = materials["calaverita_gris"]
        bpy.ops.mesh.primitive_cone_add(vertices=32, radius1=0.68, radius2=0.40,
            depth=1.25, location=(0.0, 0.72, 0.0))
        objects.append(assign(bpy.context.object, green))
        bpy.ops.mesh.primitive_cylinder_add(vertices=20, radius=0.16, depth=0.22,
            location=(0.0, 1.48, 0.0), rotation=(math.pi * 0.5, 0.0, 0.0))
        objects.append(assign(bpy.context.object, white))
        for y in (1.42, 1.51):
            objects.append(torus((0.0, y, 0.0), 0.16, 0.012, gray,
                rotation=(math.pi * 0.5, 0.0, 0.0)))
        objects.extend([
            cube((0.0, 1.18, 0.43), (0.27, 0.40, 0.06), blue),
            cube((0.0, 0.58, 0.44), (0.52, 0.045, 0.05), red),
            cube((0.0, 0.68, 0.44), (0.52, 0.045, 0.05), orange),
            cube((0.0, 0.78, 0.44), (0.52, 0.045, 0.05), gold),
        ])
        objects.append(cylinder_between((-0.34, 1.15, 0.0), (-0.70, 1.25, 0.10), 0.095, green))
        objects.append(cylinder_between((0.34, 1.15, 0.0), (0.70, 1.25, 0.10), 0.095, green))
        objects.extend([
            uv_sphere((-0.72, 1.27, 0.12), (0.13, 0.13, 0.13), white),
            uv_sphere((0.72, 1.27, 0.12), (0.13, 0.13, 0.13), white),
        ])
        for x in (-0.25, 0.25):
            objects.append(uv_sphere((x, 0.10, 0.08), (0.23, 0.12, 0.28), white))
            objects.append(cube((x, 0.10, 0.28), (0.025, 0.045, 0.16), blue))
    return objects


def join_and_prepare(objects, name):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    joined = bpy.context.object
    joined.name = name
    # Blender's OBJ exporter uses Blender's forward convention for its raw
    # vertex stream.  Pre-rotate the authored Y-up character by +90 degrees
    # so the exported coordinates are again OpenGL-compatible: +Y up, +Z
    # toward the face, and X left/right.
    joined.matrix_world = Matrix.Rotation(math.pi * 0.5, 4, "X") @ joined.matrix_world
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.15192, island_margin=0.02)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    return joined


def write_mtl(path: Path, material_names: set[str], texture_names: dict[str, str]) -> None:
    lines = []
    for material_name in sorted(material_names):
        kd = MATERIAL_DEFS.get(material_name, ((0.8, 0.8, 0.8), (1.0, 1.0, 1.0)))[0]
        lines.extend([
            f"newmtl {material_name}",
            "Ka 0.08 0.08 0.08",
            f"Kd {kd[0]:.4f} {kd[1]:.4f} {kd[2]:.4f}",
            "Ks 0.35 0.35 0.35",
            "Ns 32",
            f"map_Kd {texture_names[material_name]}",
            "",
        ])
    path.write_text("\n".join(lines), encoding="utf-8")


def export_part(objects, obj_path: Path, output_folder: Path, texture_names: dict[str, str]) -> None:
    joined = join_and_prepare(objects, obj_path.stem)
    material_names = {material.name for material in joined.data.materials if material}
    bpy.ops.object.select_all(action="DESELECT")
    joined.select_set(True)
    bpy.context.view_layer.objects.active = joined
    bpy.ops.wm.obj_export(
        filepath=str(obj_path), export_selected_objects=True,
        export_materials=True, export_uv=True, export_normals=True)
    mtl_path = obj_path.with_suffix(".mtl")
    write_mtl(mtl_path, material_names, texture_names)
    contents = obj_path.read_text(encoding="utf-8", errors="replace")
    if not re.search(r"^vt\s", contents, flags=re.MULTILINE):
        raise RuntimeError(f"No UV coordinates exported in {obj_path}")
    if not re.search(r"^vn\s", contents, flags=re.MULTILINE):
        raise RuntimeError(f"No normals exported in {obj_path}")
    bpy.data.objects.remove(joined, do_unlink=True)


def clear_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.cameras, bpy.data.lights):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def main(config_root: Path) -> None:
    clear_scene()
    output_folder = (config_root / OUTPUT_FOLDER).resolve()
    output_folder.mkdir(parents=True, exist_ok=True)

    texture_names = {}
    for material_name in MATERIAL_DEFS:
        filename = f"{material_name}.png"
        create_texture(output_folder / filename, material_name)
        texture_names[material_name] = filename
    materials = {
        name: make_material(name, output_folder / texture_name)
        for name, texture_name in texture_names.items()
    }

    exports = [
        ("moribunda", "cabeza", build_head("moribunda", materials)),
        ("moribunda", "cuerpo", build_body("moribunda", materials)),
        ("finado", "cabeza", build_head("finado", materials)),
        ("finado", "cuerpo", build_body("finado", materials)),
    ]
    for character, part, objects in exports:
        export_part(objects, output_folder / f"{character}_{part}.obj", output_folder, texture_names)
    print(f"Generated Finado/Moribunda assets in {output_folder}")


def parse_root() -> Path:
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if "--root" not in args:
        raise SystemExit("Usage: blender --background --python generate_finando_moribunda.py -- --root <ConfigInicial>")
    return Path(args[args.index("--root") + 1]).resolve()


if __name__ == "__main__":
    main(parse_root())
