"""Regenerate the five articulated mariachi asset sets.

The exported filenames and local pivots are kept compatible with
SceneManager/Renderer.  Every component receives explicit material slots and
relative MTL texture maps, so the face, hands, charro suit, shirt, bow tie,
hat and instrument no longer share the old orange stripe texture.

Run from Blender:
    blender --background --python generate_mariachi_assets.py -- --root <ConfigInicial>
"""

from __future__ import annotations

import math
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Vector


TEX_SIZE = 128
OUTPUT_ROOT = Path("Models") / "Mariachis"

CHARACTERS = (
    ("mariachi_1_violin", "violin", True),
    ("mariachi_2_violin", "violin", True),
    ("mariachi_3_trumpet", "trumpet", False),
    ("mariachi_4_violin", "violin", True),
    ("mariachi_5_guitar", "guitar", False),
)

MATERIALS = {
    "piel_rostro": ((0.88, 0.68, 0.52), (0.98, 0.80, 0.62)),
    "piel_manos": ((0.80, 0.48, 0.30), (0.96, 0.68, 0.44)),
    "charro_negro": ((0.92, 0.90, 0.85), (1.0, 0.97, 0.88)),
    "camisa_blanca": ((0.98, 0.97, 0.92), (1.0, 0.99, 0.96)),
    "mono_rojo": ((0.90, 0.12, 0.15), (1.0, 0.35, 0.20)),
    "sombrero_negro": ((0.92, 0.90, 0.85), (1.0, 0.96, 0.86)),
    "oro_botonadura": ((0.95, 0.62, 0.04), (1.0, 0.86, 0.20)),
    "madera_violin": ((0.30, 0.065, 0.018), (0.78, 0.30, 0.06)),
    "madera_guitarra": ((0.20, 0.035, 0.012), (0.64, 0.20, 0.035)),
    "metal_trompeta": ((0.72, 0.28, 0.015), (1.0, 0.82, 0.18)),
    "ebano": ((0.025, 0.012, 0.008), (0.13, 0.06, 0.02)),
}


def clamp(value: float) -> float:
    return max(0.0, min(1.0, value))


def texture_color(material: str, x: int, y: int):
    base, accent = MATERIALS[material]
    u = x / float(TEX_SIZE - 1)
    v = y / float(TEX_SIZE - 1)
    shimmer = 0.92 + 0.06 * math.sin(u * 19.0 + v * 11.0)
    color = [clamp(channel * shimmer) for channel in base]

    if material == "piel_rostro":
        # The geometric eyes/moustache are the primary detail; this map adds
        # a warm painted skin base and subtle cheek/face-paint accents.
        if ((u - 0.30) / 0.10) ** 2 + ((v - 0.54) / 0.13) ** 2 < 1.0:
            color = [0.025, 0.012, 0.010]
        if ((u - 0.70) / 0.10) ** 2 + ((v - 0.54) / 0.13) ** 2 < 1.0:
            color = [0.025, 0.012, 0.010]
        if 0.28 < v < 0.37 and (0.18 < u < 0.42 or 0.58 < u < 0.82):
            color = [0.08, 0.018, 0.012]
        if 0.42 < v < 0.47 and (0.18 < u < 0.34 or 0.66 < u < 0.82):
            color = list(accent)
    elif material in {"camisa_blanca", "charro_negro", "sombrero_negro"}:
        if abs(math.sin(u * 28.0 + v * 3.0)) > 0.985:
            color = list(accent)
    elif material == "oro_botonadura":
        color = [clamp(channel * (0.78 + 0.24 * u)) for channel in accent]
    elif material in {"madera_violin", "madera_guitarra", "ebano"}:
        grain = 0.76 + 0.24 * abs(math.sin(u * 40.0 + v * 5.0))
        color = [clamp(channel * grain) for channel in base]
    elif material == "metal_trompeta":
        color = [clamp(channel * (0.76 + 0.28 * u)) for channel in accent]
    elif material == "mono_rojo":
        if abs(math.sin(u * 22.0 + v * 8.0)) > 0.93:
            color = list(accent)
    elif material == "piel_manos":
        if abs(math.sin(u * 15.0 + v * 9.0)) > 0.96:
            color = list(accent)
    return tuple(clamp(channel) for channel in color)


def create_texture(path: Path, material: str) -> None:
    image = bpy.data.images.new(path.stem, width=TEX_SIZE, height=TEX_SIZE, alpha=True)
    pixels = []
    for y in range(TEX_SIZE):
        for x in range(TEX_SIZE):
            pixels.extend((*texture_color(material, x, y), 1.0))
    image.pixels = pixels
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()
    bpy.data.images.remove(image)


def material(name: str, texture: Path):
    result = bpy.data.materials.new(name)
    result.use_nodes = True
    nodes = result.node_tree.nodes
    links = result.node_tree.links
    bsdf = next((node for node in nodes if node.type == "BSDF_PRINCIPLED"), None)
    if bsdf is None:
        bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    image = bpy.data.images.load(str(texture), check_existing=True)
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = image
    links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.42
    return result


def assign(obj, mat):
    obj.data.materials.append(mat)
    return obj


def sphere(location, scale, mat):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=20, ring_count=12, location=location)
    obj = bpy.context.object
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return assign(obj, mat)


def cube(location, scale, mat, rotation=(0.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return assign(obj, mat)


def cylinder_between(start, end, radius, mat, vertices=18):
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
    return assign(obj, mat)


def torus(location, major, minor, mat):
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major, minor_radius=minor, major_segments=28,
        minor_segments=8, location=location)
    return assign(bpy.context.object, mat)


def cone(location, radius1, radius2, depth, mat, rotation=(0.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_cone_add(
        vertices=24, radius1=radius1, radius2=radius2, depth=depth,
        location=location, rotation=rotation)
    return assign(bpy.context.object, mat)


def body(materials):
    black = materials["charro_negro"]
    hat = materials["sombrero_negro"]
    shirt = materials["camisa_blanca"]
    red = materials["mono_rojo"]
    gold = materials["oro_botonadura"]
    skin = materials["piel_rostro"]
    ebano = materials["ebano"]
    objects = []
    # Blender cones are born along local Z. Rotate this torso 90 degrees
    # around X so its long axis is Y-up before the OBJ axis conversion.
    objects.append(cone((0.0, 1.02, 0.0), 0.43, 0.52, 1.28, black,
        rotation=(math.radians(90.0), 0.0, 0.0)))
    objects.extend([
        cube((-0.20, 0.28, 0.0), (0.16, 0.28, 0.20), black),
        cube((0.20, 0.28, 0.0), (0.16, 0.28, 0.20), black),
        cube((-0.20, 0.08, 0.12), (0.19, 0.08, 0.28), ebano),
        cube((0.20, 0.08, 0.12), (0.19, 0.08, 0.28), ebano),
        cube((0.0, 1.18, 0.43), (0.20, 0.43, 0.045), shirt),
        sphere((-0.11, 1.61, 0.43), (0.13, 0.08, 0.035), red),
        sphere((0.11, 1.61, 0.43), (0.13, 0.08, 0.035), red),
        sphere((0.0, 1.61, 0.45), (0.055, 0.055, 0.035), gold),
    ])
    for side in (-1.0, 1.0):
        for y in (1.05, 1.25, 1.45):
            objects.append(sphere((side * 0.29, y, 0.43), (0.035, 0.035, 0.025), gold))

    # Warm face, eyes, brows and the characteristic dark charro moustache.
    objects.append(sphere((0.0, 2.08, 0.0), (0.42, 0.46, 0.36), skin))
    for x in (-0.15, 0.15):
        objects.append(sphere((x, 2.16, 0.335), (0.07, 0.085, 0.035), ebano))
        objects.append(cube((x, 2.28, 0.34), (0.095, 0.025, 0.025), ebano,
            rotation=(0.0, 0.0, math.radians(-8.0 * x))))
    objects.extend([
        sphere((-0.105, 1.98, 0.345), (0.12, 0.045, 0.028), ebano),
        sphere((0.105, 1.98, 0.345), (0.12, 0.045, 0.028), ebano),
        cube((0.0, 1.90, 0.348), (0.07, 0.018, 0.018), ebano),
    ])
    # Clean sombrero with a thin gold embroidered edge.
    objects.extend([
        torus((0.0, 2.51, 0.0), 0.50, 0.075, hat),
        cylinder_between((0.0, 2.51, 0.0), (0.0, 2.82, 0.0), 0.29, hat),
        torus((0.0, 2.52, 0.0), 0.49, 0.018, gold),
        torus((0.0, 2.56, 0.0), 0.46, 0.012, red),
        torus((0.0, 2.75, 0.0), 0.27, 0.014, gold),
    ])
    return objects


def arm(side: float, materials):
    black = materials["charro_negro"]
    skin = materials["piel_manos"]
    shoulder = (side * 0.36, 1.55, 0.12)
    elbow = (side * 0.56, 1.34, 0.27)
    wrist = (side * 0.64, 1.18, 0.43)
    return [
        cylinder_between(shoulder, elbow, 0.105, black),
        cylinder_between(elbow, wrist, 0.085, black),
        sphere(wrist, (0.115, 0.115, 0.105), skin),
    ]


def instrument(kind: str, materials):
    if kind == "trumpet":
        gold = materials["metal_trompeta"]
        dark = materials["ebano"]
        objects = [
            cylinder_between((0.0, 1.83, 0.32), (0.0, 1.83, 0.88), 0.045, gold),
            cone((0.0, 1.83, 0.98), 0.18, 0.055, 0.24, gold),
            cylinder_between((-0.08, 1.83, 0.54), (0.08, 1.83, 0.54), 0.035, gold),
            sphere((0.0, 1.83, 0.30), (0.07, 0.07, 0.07), dark),
        ]
        return objects

    wood = materials["madera_guitarra" if kind == "guitar" else "madera_violin"]
    dark = materials["ebano"]
    if kind == "guitar":
        objects = [sphere((0.0, 1.14, 0.48), (0.32, 0.40, 0.13), wood)]
        objects.append(torus((0.0, 1.18, 0.615), 0.09, 0.018, dark))
        objects.append(cylinder_between((0.0, 1.42, 0.48), (0.0, 2.32, 0.48), 0.055, dark))
        objects.append(sphere((0.0, 2.36, 0.48), (0.10, 0.08, 0.06), wood))
    else:
        objects = [sphere((0.0, 1.30, 0.48), (0.25, 0.34, 0.11), wood)]
        objects.append(cylinder_between((0.0, 1.46, 0.48), (0.0, 2.12, 0.48), 0.045, dark))
        objects.append(sphere((0.0, 2.15, 0.48), (0.08, 0.06, 0.05), wood))
    return objects


def bow(materials):
    wood = materials["ebano"]
    return [cylinder_between((-0.36, 1.18, 0.62), (0.36, 1.86, 0.62), 0.018, wood)]


def bow_tie(materials):
    red = materials["mono_rojo"]
    return [
        sphere((-0.12, 1.61, 0.45), (0.13, 0.08, 0.035), red),
        sphere((0.12, 1.61, 0.45), (0.13, 0.08, 0.035), red),
        sphere((0.0, 1.61, 0.46), (0.055, 0.055, 0.035), red),
    ]


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.cameras, bpy.data.lights):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def join_prepare(objects, name):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    joined = bpy.context.object
    joined.name = name
    # Blender's OBJ exporter maps Blender Y-up into its raw forward convention
    # unless the authored object is pre-rotated.  Apply the inverse conversion
    # around the global origin so the file itself is OpenGL-ready: +Y up and
    # +Z toward the face/instrument.
    joined.matrix_world = Matrix.Rotation(math.pi * 0.5, 4, "X") @ joined.matrix_world
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.uv.smart_project(angle_limit=1.15192, island_margin=0.02)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    return joined


def write_mtl(path: Path, used_materials, texture_names):
    lines = []
    for material_name in sorted(used_materials):
        key = material_name.split("__", 1)[-1]
        kd = MATERIALS[key][0]
        lines += [
            f"newmtl {material_name}", "Ka 0.10 0.10 0.10",
            f"Kd {kd[0]:.4f} {kd[1]:.4f} {kd[2]:.4f}",
            "Ks 0.30 0.30 0.30", "Ns 32",
            f"map_Kd {texture_names[key]}", "",
        ]
    path.write_text("\n".join(lines), encoding="utf-8")


def export_part(objects, obj_path: Path, texture_names):
    joined = join_prepare(objects, obj_path.stem)
    used = {mat.name for mat in joined.data.materials if mat}
    bpy.ops.object.select_all(action="DESELECT")
    joined.select_set(True)
    bpy.context.view_layer.objects.active = joined
    bpy.ops.wm.obj_export(
        filepath=str(obj_path), export_selected_objects=True,
        export_materials=True, export_uv=True, export_normals=True)
    write_mtl(obj_path.with_suffix(".mtl"), used, texture_names)
    contents = obj_path.read_text(encoding="utf-8", errors="replace")
    if "\nvt " not in contents or "\nvn " not in contents:
        raise RuntimeError(f"OBJ without UV/normals: {obj_path}")
    bpy.data.objects.remove(joined, do_unlink=True)


def generate_character(root: Path, folder: str, kind: str, has_bow: bool):
    clear_scene()
    output = root / OUTPUT_ROOT / folder
    output.mkdir(parents=True, exist_ok=True)
    texture_names = {}
    materials = {}
    for key in MATERIALS:
        filename = f"{key}.png"
        create_texture(output / filename, key)
        texture_names[key] = filename
        materials[key] = material(f"{folder}__{key}", output / filename)

    export_part(body(materials), output / "cuerpo_sombrero.obj", texture_names)
    export_part(arm(1.0, materials), output / "brazo_derecho.obj", texture_names)
    export_part(arm(-1.0, materials), output / "brazo_izquierdo.obj", texture_names)
    export_part(instrument(kind, materials), output / "instrumento.obj", texture_names)
    export_part(bow_tie(materials), output / "mono_rojo.obj", texture_names)
    if has_bow:
        export_part(bow(materials), output / "arco.obj", texture_names)


def parse_root() -> Path:
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if "--root" not in args:
        raise SystemExit("Usage: blender --background --python generate_mariachi_assets.py -- --root <ConfigInicial>")
    return Path(args[args.index("--root") + 1]).resolve()


if __name__ == "__main__":
    project_root = parse_root()
    for character in CHARACTERS:
        generate_character(project_root, *character)
    print(f"Generated mariachi assets in {project_root / OUTPUT_ROOT}")
