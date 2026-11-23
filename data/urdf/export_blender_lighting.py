"""Regenerate the material-preview environment and AgX LUT with Blender.

Run: blender -b --python data/urdf/export_blender_lighting.py
The files use a little-endian uint32 width/height followed by float32 RGBA.
"""

import array
import math
import struct
import sys
from pathlib import Path

import bpy
import PyOpenColorIO as ocio


def save_texture(path, width, height, values):
    if sys.byteorder != "little":
        values.byteswap()
    with path.open("wb") as output:
        output.write(struct.pack("<II", width, height))
        values.tofile(output)


directory = Path(__file__).resolve().parents[2] / "ImGui/ImGui/resources"
forest = Path(bpy.utils.resource_path("LOCAL")) / "datafiles/studiolights/world/forest.exr"
image = bpy.data.images.load(str(forest), check_existing=False)
width, height = image.size
pixels = array.array("f", [0.0]) * (width * height * 4)
image.pixels.foreach_get(pixels)
save_texture(directory / "forest_env.rgba32f", width, height, pixels)

transform = ocio.DisplayViewTransform()
transform.setSrc("Linear Rec.709")
transform.setDisplay("sRGB")
transform.setView("AgX")
processor = ocio.GetCurrentConfig().getProcessor(transform).getDefaultCPUProcessor()
size = 48
lut = array.array("f")
for g in range(size):
    for b in range(size):
        for r in range(size):
            rgb = [0.0 if v == 0 else 2 ** (v / (size - 1) * 18 - 12)
                   for v in (r, g, b)]
            lut.extend((*processor.applyRGB(rgb), 1.0))
save_texture(directory / "agx_view.rgba32f", size * size, size, lut)
