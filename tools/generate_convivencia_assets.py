"""Generate stylized convivencia assets and repair the chinampa skirt.

Run with Blender in background mode from the project root.  The exported OBJ
files intentionally use MTL diffuse colors instead of image textures so the
existing Assimp/Model loader can render them without additional texture
dependencies.
"""

import math
import os
import sys

import bpy
from mathutils import Vector


PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODELS_DIR = os.path.join(PROJECT_ROOT, "ConfigInicial", "Models")


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves,
                       bpy.data.cameras, bpy.data.lights):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def make_material(name, color, roughness=0.72, metallic=0.0):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    if principled:
        principled.inputs["Base Color"].default_value = (*color, 1.0)
        principled.inputs["Roughness"].default_value = roughness
        principled.inputs["Metallic"].default_value = metallic
    # Blender's OBJ exporter uses diffuse_color for Kd in the MTL. Set it
    # after enabling nodes because use_nodes initializes it to neutral gray.
    material.diffuse_color = (*color, 1.0)
    return material


MATS = {
    "black": make_material("Charro_Negro", (0.035, 0.045, 0.065), 0.60),
    "white": make_material("Sombrero_Blanco", (0.86, 0.82, 0.70), 0.75),
    "gold": make_material("Detalles_Dorados", (0.95, 0.52, 0.08), 0.38, 0.35),
    "red": make_material("Moño_Rojo", (0.72, 0.025, 0.035), 0.62),
    "wood": make_material("Madera_Instrum", (0.34, 0.10, 0.035), 0.68),
    "wood_light": make_material("Madera_Clara", (0.72, 0.28, 0.065), 0.66),
    "skin": make_material("Piel", (0.56, 0.25, 0.12), 0.82),
    "hair": make_material("Cabello", (0.035, 0.018, 0.012), 0.86),
    "teal": make_material("Ropa_Turquesa", (0.02, 0.40, 0.42), 0.78),
    "magenta": make_material("Ropa_Magenta", (0.68, 0.06, 0.30), 0.76),
    "yellow": make_material("Ropa_Amarilla", (0.95, 0.50, 0.04), 0.75),
    "blue": make_material("Pantalon_Azul", (0.03, 0.12, 0.38), 0.82),
    "green": make_material("Falda_Verde", (0.05, 0.34, 0.15), 0.80),
    "cream": make_material("Mesa_Ceramica", (0.91, 0.78, 0.53), 0.72),
    "wood_dark": make_material("Madera_Cabana", (0.12, 0.045, 0.018), 0.88),
    "wood_plank": make_material("Tablon_Desgastado", (0.27, 0.095, 0.035), 0.84),
    "roof": make_material("Techo_Lamina_Envejecida", (0.18, 0.20, 0.19), 0.93),
    "roof_edge": make_material("Borde_Techo", (0.36, 0.19, 0.06), 0.80),
    "rope": make_material("Soga", (0.25, 0.12, 0.035), 0.96),
    "dress_beige": make_material("Vestido_Beige_Viejo", (0.55, 0.47, 0.35), 0.92),
    "dress_pink": make_material("Vestido_Rosa_Deslavado", (0.52, 0.23, 0.28), 0.91),
    "dress_blue": make_material("Vestido_Azul_Deslavado", (0.22, 0.31, 0.36), 0.90),
    "eye_white": make_material("Ojos_Claros", (0.82, 0.78, 0.62), 0.85),
    "eye_dark": make_material("Cuencas_Oscuras", (0.012, 0.008, 0.006), 0.95),
    "window_dark": make_material("Ventana_Oscura", (0.018, 0.025, 0.026), 0.96),
}

MATERIAL_COLORS = {
    "Charro_Negro": (0.035, 0.045, 0.065),
    "Sombrero_Blanco": (0.86, 0.82, 0.70),
    "Detalles_Dorados": (0.95, 0.52, 0.08),
    "Moño_Rojo": (0.72, 0.025, 0.035),
    "Madera_Instrum": (0.34, 0.10, 0.035),
    "Madera_Clara": (0.72, 0.28, 0.065),
    "Piel": (0.56, 0.25, 0.12),
    "Cabello": (0.035, 0.018, 0.012),
    "Ropa_Turquesa": (0.02, 0.40, 0.42),
    "Ropa_Magenta": (0.68, 0.06, 0.30),
    "Ropa_Amarilla": (0.95, 0.50, 0.04),
    "Pantalon_Azul": (0.03, 0.12, 0.38),
    "Falda_Verde": (0.05, 0.34, 0.15),
    "Madera_Cabana": (0.12, 0.045, 0.018),
    "Tablon_Desgastado": (0.27, 0.095, 0.035),
    "Techo_Lamina_Envejecida": (0.18, 0.20, 0.19),
    "Borde_Techo": (0.36, 0.19, 0.06),
    "Soga": (0.25, 0.12, 0.035),
    "Vestido_Beige_Viejo": (0.55, 0.47, 0.35),
    "Vestido_Rosa_Deslavado": (0.52, 0.23, 0.28),
    "Vestido_Azul_Deslavado": (0.22, 0.31, 0.36),
    "Ojos_Claros": (0.82, 0.78, 0.62),
    "Cuencas_Oscuras": (0.012, 0.008, 0.006),
    "Ventana_Oscura": (0.018, 0.025, 0.026),
}


def to_blender(project_position):
    """Convert project coordinates (Y-up) to Blender coordinates (Z-up)."""
    return Vector((project_position[0], -project_position[2], project_position[1]))


def to_project(blender_position):
    return Vector((blender_position.x, blender_position.z, -blender_position.y))


def set_material(obj, material):
    obj.data.materials.append(material)
    return obj


def add_uv_sphere(name, location, scale, material, segments=12, rings=8):
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=segments, ring_count=rings, location=to_blender(location))
    obj = bpy.context.object
    obj.name = name
    obj.scale = (scale[0], scale[2], scale[1])
    return set_material(obj, material)


def add_cylinder(name, location, radius, depth, material, vertices=10):
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices, radius=radius, depth=depth,
        location=to_blender(location))
    obj = bpy.context.object
    obj.name = name
    return set_material(obj, material)


def add_cone(name, location, radius1, radius2, depth, material, vertices=10):
    bpy.ops.mesh.primitive_cone_add(
        vertices=vertices, radius1=radius1, radius2=radius2, depth=depth,
        location=to_blender(location))
    obj = bpy.context.object
    obj.name = name
    return set_material(obj, material)


def add_cube(name, location, scale, material):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=to_blender(location))
    obj = bpy.context.object
    obj.name = name
    obj.scale = (scale[0], scale[2], scale[1])
    return set_material(obj, material)


def add_cube_rotated(name, location, scale, material, rotation_x=0.0):
    obj = add_cube(name, location, scale, material)
    obj.rotation_mode = "XYZ"
    obj.rotation_euler[0] = math.radians(rotation_x)
    return obj


def add_torus(name, location, major_radius, minor_radius, material):
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major_radius, minor_radius=minor_radius,
        major_segments=16, minor_segments=6,
        location=to_blender(location))
    obj = bpy.context.object
    obj.name = name
    return set_material(obj, material)


def add_cylinder_between(name, start, end, radius, material, vertices=8):
    start = to_blender(start)
    end = to_blender(end)
    direction = end - start
    obj = add_cylinder(name, to_project((start + end) * 0.5), radius, direction.length,
                       material, vertices)
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = Vector((0.0, 0.0, 1.0)).rotation_difference(direction)
    return obj


def add_cone_between(name, start, end, radius1, radius2, material, vertices=8):
    start = to_blender(start)
    end = to_blender(end)
    direction = end - start
    obj = add_cone(name, to_project((start + end) * 0.5), radius1, radius2,
                   direction.length, material, vertices)
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = Vector((0.0, 0.0, 1.0)).rotation_difference(direction)
    return obj


def patch_mtl_colors(filename):
    mtl_path = os.path.join(MODELS_DIR, os.path.splitext(filename)[0] + ".mtl")
    if not os.path.exists(mtl_path):
        return
    output = []
    current_material = None
    with open(mtl_path, "r", encoding="utf-8") as source:
        for line in source:
            if line.startswith("newmtl "):
                current_material = line.split(None, 1)[1].strip()
            if line.startswith("Kd ") and current_material in MATERIAL_COLORS:
                r, g, b = MATERIAL_COLORS[current_material]
                line = f"Kd {r:.6f} {g:.6f} {b:.6f}\n"
            output.append(line)
    with open(mtl_path, "w", encoding="utf-8", newline="\n") as target:
        target.writelines(output)


def join_and_export(objects, filename):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    objects[0].name = os.path.splitext(filename)[0]
    filepath = os.path.join(MODELS_DIR, filename)
    bpy.ops.wm.obj_export(
        filepath=filepath,
        export_selected_objects=True,
        export_materials=True,
        export_normals=True,
        export_uv=True,
        forward_axis="NEGATIVE_Z",
        up_axis="Y",
        path_mode="AUTO")
    patch_mtl_colors(filename)
    bpy.ops.object.select_all(action="DESELECT")


def add_sombrero(parts, prefix, material):
    parts.append(add_cylinder(prefix + "_brim", (0.0, 1.98, 0.0),
                               0.46, 0.075, material, 16))
    parts.append(add_cone(prefix + "_crown", (0.0, 2.12, 0.0),
                           0.27, 0.19, 0.27, material, 12))
    parts.append(add_torus(prefix + "_band", (0.0, 2.08, 0.0),
                            0.235, 0.025, MATS["gold"]))


def add_mariachi_body(parts, prefix, body_material, hat_material):
    parts.append(add_cylinder(prefix + "_torso", (0.0, 1.10, 0.0),
                               0.30, 0.70, body_material, 10))
    parts.append(add_uv_sphere(prefix + "_head", (0.0, 1.72, 0.0),
                               (0.24, 0.26, 0.24), MATS["skin"]))
    parts.append(add_uv_sphere(prefix + "_hair", (0.0, 1.80, -0.02),
                               (0.245, 0.10, 0.245), MATS["hair"]))
    add_sombrero(parts, prefix, hat_material)
    parts.append(add_uv_sphere(prefix + "_tie_left", (-0.09, 1.48, 0.27),
                               (0.10, 0.045, 0.045), MATS["red"]))
    parts.append(add_uv_sphere(prefix + "_tie_right", (0.09, 1.48, 0.27),
                               (0.10, 0.045, 0.045), MATS["red"]))
    for index, y in enumerate((1.28, 1.13, 0.98)):
        parts.append(add_uv_sphere(prefix + "_button_" + str(index),
                                   (0.0, y, 0.285), (0.035, 0.035, 0.025),
                                   MATS["gold"], 8, 6))
    parts.append(add_cylinder_between(prefix + "_leg_l", (-0.12, 0.76, 0.0),
                                      (-0.14, 0.18, 0.0), 0.09, MATS["black"]))
    parts.append(add_cylinder_between(prefix + "_leg_r", (0.12, 0.76, 0.0),
                                      (0.14, 0.18, 0.0), 0.09, MATS["black"]))
    parts.append(add_cylinder_between(prefix + "_arm_l", (-0.25, 1.35, 0.0),
                                      (-0.32, 1.05, 0.20), 0.075, body_material))
    parts.append(add_cylinder_between(prefix + "_arm_r", (0.25, 1.35, 0.0),
                                      (0.32, 1.08, 0.20), 0.075, body_material))


def create_mariachi(filename, instrument):
    parts = []
    prefix = os.path.splitext(filename)[0]
    body_material = MATS["white"] if instrument == "trumpet" else MATS["black"]
    add_mariachi_body(parts, prefix, body_material, MATS["white"])

    if instrument == "trumpet":
        parts.append(add_cylinder_between(prefix + "_trumpet_tube",
                                          (0.10, 1.23, 0.23),
                                          (0.68, 1.28, 0.54), 0.045, MATS["gold"]))
        parts.append(add_cone_between(prefix + "_trumpet_bell",
                                      (0.62, 1.27, 0.51),
                                      (0.88, 1.30, 0.65), 0.12, 0.025,
                                      MATS["gold"]))
        parts.append(add_cylinder(prefix + "_trumpet_valve",
                                  (0.35, 1.28, 0.36), 0.07, 0.18,
                                  MATS["gold"], 8))
    elif instrument == "guitar":
        parts.append(add_uv_sphere(prefix + "_guitar_body", (0.36, 1.04, 0.30),
                                   (0.30, 0.14, 0.40), MATS["wood_light"]))
        parts.append(add_uv_sphere(prefix + "_soundhole", (0.36, 1.05, 0.58),
                                   (0.075, 0.025, 0.075), MATS["black"], 10, 6))
        parts.append(add_cylinder_between(prefix + "_guitar_neck",
                                          (0.36, 1.12, 0.51),
                                          (0.36, 1.17, 0.98), 0.055, MATS["wood"]))
        parts.append(add_cylinder_between(prefix + "_guitar_head",
                                          (0.36, 1.17, 0.94),
                                          (0.36, 1.18, 1.08), 0.085, MATS["wood"]))
    else:
        parts.append(add_uv_sphere(prefix + "_violin_body", (-0.31, 1.10, 0.28),
                                   (0.18, 0.10, 0.25), MATS["wood_light"]))
        parts.append(add_uv_sphere(prefix + "_violin_body2", (-0.31, 0.91, 0.28),
                                   (0.15, 0.08, 0.19), MATS["wood_light"]))
        parts.append(add_cylinder_between(prefix + "_violin_neck",
                                          (-0.31, 1.20, 0.28),
                                          (-0.31, 1.60, 0.28), 0.045, MATS["wood"]))
        parts.append(add_cylinder_between(prefix + "_violin_bow",
                                          (-0.56, 1.02, 0.52),
                                          (-0.05, 1.38, 0.52), 0.018, MATS["gold"]))

    join_and_export(parts, filename)


def create_person(filename, shirt, lower, seated=False, skirt=False):
    parts = []
    prefix = os.path.splitext(filename)[0]
    base_y = 0.70 if seated else 1.05
    if skirt:
        parts.append(add_cone(prefix + "_skirt", (0.0, base_y, 0.0),
                              0.34, 0.23, 0.58, MATS[lower], 10))
    else:
        parts.append(add_cylinder(prefix + "_torso", (0.0, base_y, 0.0),
                                  0.27, 0.62, MATS[shirt], 10))
    parts.append(add_uv_sphere(prefix + "_head", (0.0, base_y + 0.55, 0.0),
                               (0.22, 0.24, 0.22), MATS["skin"]))
    parts.append(add_uv_sphere(prefix + "_hair", (0.0, base_y + 0.63, -0.02),
                               (0.23, 0.13, 0.22), MATS["hair"]))
    parts.append(add_cylinder_between(prefix + "_arm_l", (-0.23, base_y + 0.17, 0.0),
                                      (-0.34, base_y - 0.08, 0.18), 0.07, MATS[shirt]))
    parts.append(add_cylinder_between(prefix + "_arm_r", (0.23, base_y + 0.17, 0.0),
                                      (0.34, base_y - 0.08, 0.18), 0.07, MATS[shirt]))
    if seated:
        parts.append(add_cylinder_between(prefix + "_leg_l", (-0.12, 0.48, 0.0),
                                          (-0.30, 0.25, 0.32), 0.09, MATS[lower]))
        parts.append(add_cylinder_between(prefix + "_leg_r", (0.12, 0.48, 0.0),
                                          (0.30, 0.25, 0.32), 0.09, MATS[lower]))
    else:
        parts.append(add_cylinder_between(prefix + "_leg_l", (-0.11, 0.70, 0.0),
                                          (-0.13, 0.12, 0.0), 0.085, MATS[lower]))
        parts.append(add_cylinder_between(prefix + "_leg_r", (0.11, 0.70, 0.0),
                                          (0.13, 0.12, 0.0), 0.085, MATS[lower]))
    parts.append(add_uv_sphere(prefix + "_scarf", (0.0, base_y + 0.29, 0.26),
                               (0.18, 0.045, 0.04), MATS["red"], 8, 6))
    join_and_export(parts, filename)


def create_munecas_cabin():
    parts = []
    prefix = "cabana_munecas"
    # Rustic plank volume with an intentionally visible front facade.
    parts.append(add_cube(prefix + "_walls", (0.0, 0.82, 0.0),
                          (2.30, 1.55, 1.35), MATS["wood_dark"]))
    for index, y in enumerate((0.30, 0.58, 0.86, 1.14, 1.42)):
        parts.append(add_cube(prefix + "_front_plank_" + str(index),
                              (0.0, y, 0.70), (2.22, 0.07, 0.055),
                              MATS["wood_plank"]))
    for x in (-1.06, 1.06):
        parts.append(add_cube(prefix + "_corner_" + str(x),
                              (x, 0.88, 0.73), (0.10, 1.60, 0.10),
                              MATS["wood_plank"]))

    # Gable roof: two weathered slabs with an overhanging eave.
    parts.append(add_cube_rotated(prefix + "_roof_left", (0.0, 1.78, -0.42),
                                  (2.65, 0.12, 1.05), MATS["roof"], 32.0))
    parts.append(add_cube_rotated(prefix + "_roof_right", (0.0, 1.78, 0.42),
                                  (2.65, 0.12, 1.05), MATS["roof"], -32.0))
    parts.append(add_cube(prefix + "_ridge", (0.0, 2.17, 0.0),
                          (2.72, 0.12, 0.12), MATS["roof_edge"]))
    for x in (-1.22, 1.22):
        parts.append(add_cube(prefix + "_eave_" + str(x),
                              (x, 1.56, 0.0), (0.10, 0.16, 1.05),
                              MATS["roof_edge"]))

    # Door between posts and a barred/crossed front window.
    parts.append(add_cube(prefix + "_door", (0.0, 0.78, 0.775),
                          (0.62, 1.20, 0.07), MATS["window_dark"]))
    for x in (-0.42, 0.42):
        parts.append(add_cube(prefix + "_door_frame_" + str(x),
                              (x, 0.80, 0.86), (0.09, 1.28, 0.10),
                              MATS["wood_plank"]))
    parts.append(add_cube(prefix + "_door_header", (0.0, 1.45, 0.86),
                          (0.53, 0.09, 0.10), MATS["wood_plank"]))
    parts.append(add_cube(prefix + "_window", (0.78, 1.05, 0.78),
                          (0.42, 0.48, 0.06), MATS["window_dark"]))
    parts.append(add_cube_rotated(prefix + "_window_cross_a", (0.78, 1.05, 0.84),
                                  (0.06, 0.55, 0.07), MATS["wood_plank"], 45.0))
    parts.append(add_cube_rotated(prefix + "_window_cross_b", (0.78, 1.05, 0.85),
                                  (0.06, 0.55, 0.07), MATS["wood_plank"], -45.0))

    # Small porch and front hanging beam for the dolls.
    parts.append(add_cube(prefix + "_porch", (0.0, 0.12, 0.98),
                          (1.42, 0.16, 0.62), MATS["wood_plank"]))
    parts.append(add_cube(prefix + "_hanging_beam", (0.0, 1.55, 1.17),
                          (2.20, 0.12, 0.12), MATS["wood_plank"]))
    for x in (-0.98, 0.98):
        parts.append(add_cube(prefix + "_porch_post_" + str(x),
                              (x, 0.88, 1.10), (0.10, 1.42, 0.10),
                              MATS["wood_plank"]))
    join_and_export(parts, "cabana_munecas.obj")


def create_hanging_doll():
    parts = []
    prefix = "muneca_colgada"
    # The model origin is the attachment point. The rope exits from y=0 and
    # the doll body hangs below it, making every renderer position physical.
    parts.append(add_cylinder(prefix + "_rope", (0.0, -0.18, 0.0),
                              0.028, 0.36, MATS["rope"], 8))
    parts.append(add_uv_sphere(prefix + "_knot", (0.0, -0.03, 0.0),
                               (0.07, 0.06, 0.07), MATS["rope"], 8, 6))
    parts.append(add_uv_sphere(prefix + "_head", (0.0, -0.43, 0.0),
                               (0.20, 0.22, 0.18), MATS["skin"], 12, 8))
    parts.append(add_uv_sphere(prefix + "_hair", (0.0, -0.44, -0.10),
                               (0.23, 0.25, 0.10), MATS["hair"], 12, 8))
    # Large eyes and dark pupils make the face readable from the bridges.
    for x in (-0.065, 0.065):
        parts.append(add_uv_sphere(prefix + "_eye_white_" + str(x),
                                   (x, -0.43, 0.17), (0.045, 0.055, 0.025),
                                   MATS["eye_white"], 8, 6))
        parts.append(add_uv_sphere(prefix + "_eye_dark_" + str(x),
                                   (x, -0.43, 0.195), (0.021, 0.028, 0.012),
                                   MATS["eye_dark"], 8, 6))
    dress_material = MATS["dress_beige"]
    parts.append(add_cone(prefix + "_dress", (0.0, -0.84, 0.0),
                          0.29, 0.15, 0.58, dress_material, 10))
    parts.append(add_cylinder(prefix + "_collar", (0.0, -0.58, 0.0),
                               0.13, 0.08, MATS["dress_pink"], 10))
    parts.append(add_cylinder_between(prefix + "_arm_l", (-0.17, -0.68, 0.0),
                                      (-0.32, -0.92, 0.02), 0.055, dress_material))
    parts.append(add_cylinder_between(prefix + "_arm_r", (0.17, -0.68, 0.0),
                                      (0.32, -0.92, 0.02), 0.055, dress_material))
    parts.append(add_cylinder_between(prefix + "_leg_l", (-0.10, -1.12, 0.0),
                                      (-0.12, -1.40, 0.02), 0.05, MATS["dress_blue"]))
    parts.append(add_cylinder_between(prefix + "_leg_r", (0.10, -1.12, 0.0),
                                      (0.12, -1.40, 0.02), 0.05, MATS["dress_blue"]))
    join_and_export(parts, "muneca_colgada.obj")


def repair_island_mesh():
    clear_scene()
    source = os.path.join(MODELS_DIR, "isla.obj")
    bpy.ops.wm.obj_import(filepath=source, forward_axis="NEGATIVE_Z", up_axis="Y")
    imported = list(bpy.context.selected_objects)
    for obj in imported:
        if obj.type != "MESH":
            continue
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
        for vertex in obj.data.vertices:
            # The original lower soil skirt flares to radius 3.64 while the
            # grass perimeter is about 3.20. Taper only the submerged part so
            # it stays inside the shoreline posts instead of reading as a disk.
            if vertex.co.z < -0.12:
                amount = min(1.0, max(0.0, (-vertex.co.z - 0.12) / 0.56))
                factor = 1.0 - 0.24 * amount
                vertex.co.x *= factor
                vertex.co.y *= factor
    bpy.ops.object.select_all(action="DESELECT")
    for obj in imported:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = imported[0]
    filepath = os.path.join(MODELS_DIR, "isla_chinampa_corregida.obj")
    bpy.ops.wm.obj_export(
        filepath=filepath,
        export_selected_objects=True,
        export_materials=True,
        export_normals=True,
        export_uv=True,
        forward_axis="NEGATIVE_Z",
        up_axis="Y",
        path_mode="AUTO")


def main():
    os.makedirs(MODELS_DIR, exist_ok=True)
    repair_island_mesh()
    clear_scene()
    create_mariachi("mariachi_trompeta.obj", "trumpet")
    clear_scene()
    create_mariachi("mariachi_guitarra.obj", "guitar")
    clear_scene()
    create_mariachi("mariachi_violin.obj", "violin")
    clear_scene()
    create_person("persona_convivencia_1.obj", "teal", "blue")
    clear_scene()
    create_person("persona_convivencia_2.obj", "magenta", "green", skirt=True)
    clear_scene()
    create_person("persona_convivencia_3.obj", "yellow", "blue", seated=True)
    clear_scene()
    create_munecas_cabin()
    clear_scene()
    create_hanging_doll()
    print("Generated convivencia assets in", MODELS_DIR)


if __name__ == "__main__":
    main()
