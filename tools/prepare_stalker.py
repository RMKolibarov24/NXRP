"""Prepare the CC0 Grab3D light Horror Monster for NXRP.

Install numpy, pygltflib and fast-simplification in a separate Python environment,
then run: python tools/prepare_stalker.py path/to/horror-monster-light.glb
The source and license are recorded in assets/models/README.md.
"""
import argparse
import struct
from pathlib import Path

import fast_simplification
import numpy as np
from pygltflib import GLTF2


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--output", type=Path, default=Path("assets/models"))
    args = parser.parse_args()
    gltf = GLTF2().load(str(args.source))
    blob = gltf.binary_blob()
    if len(gltf.meshes) != 1 or len(gltf.meshes[0].primitives) != 1:
        raise ValueError("Expected the single mesh in Grab3D's light model")

    def accessor(index, components, dtype):
        item = gltf.accessors[index]
        view = gltf.bufferViews[item.bufferView]
        if item.sparse is not None:
            raise ValueError("Sparse accessors are not supported")
        dtype = np.dtype(dtype)
        stride = view.byteStride or components * dtype.itemsize
        offset = (view.byteOffset or 0) + (item.byteOffset or 0)
        return np.ndarray((item.count, components), dtype=dtype, buffer=blob,
                          offset=offset, strides=(stride, dtype.itemsize)).copy()

    primitive = gltf.meshes[0].primitives[0]
    positions = accessor(primitive.attributes.POSITION, 3, "<f4")
    uv = accessor(primitive.attributes.TEXCOORD_0, 2, "<f4")
    triangles = accessor(primitive.indices, 1, "<u4").reshape(-1, 3).astype(np.int32)
    reduced, faces, collapses = fast_simplification.simplify(
        positions, triangles, target_count=28000, preserve_border=True,
        return_collapses=True)
    replayed, replayed_faces, mapping = fast_simplification.replay_simplification(
        positions, triangles, collapses)
    if reduced.shape != replayed.shape or not np.allclose(reduced, replayed):
        raise ValueError("Decimation mapping does not match the reduced mesh")
    if faces.shape != replayed_faces.shape or not np.array_equal(faces, replayed_faces):
        raise ValueError("Decimation triangle mapping does not match")
    reduced = reduced.astype(np.float32)
    faces = faces.astype(np.uint32)

    # Preserve the UV of the nearest original vertex in each collapsed group.
    valid = np.flatnonzero((mapping >= 0) & (mapping < len(reduced)))
    distance = np.sum((positions[valid] - reduced[mapping[valid]]) ** 2, axis=1)
    sorted_ids = valid[np.lexsort((distance, mapping[valid]))]
    _, first = np.unique(mapping[sorted_ids], return_index=True)
    chosen = sorted_ids[first]
    if len(chosen) != len(reduced):
        raise ValueError("Some reduced vertices have no UV source")
    reduced_uv = np.zeros((len(reduced), 2), dtype=np.float32)
    reduced_uv[mapping[chosen]] = uv[chosen]

    a, b, c = (reduced[faces[:, i]] for i in range(3))
    face_normals = np.cross(b - a, c - a)
    normals = np.zeros_like(reduced)
    for i in range(3):
        np.add.at(normals, faces[:, i], face_normals)
    normals /= np.maximum(np.linalg.norm(normals, axis=1)[:, None], 1e-9)
    reduced[:, 1] -= reduced[:, 1].min()
    reduced *= 1.65 / reduced[:, 1].max()
    vertices = np.concatenate((reduced, normals, reduced_uv), axis=1).astype("<f4")

    args.output.mkdir(parents=True, exist_ok=True)
    with (args.output / "stalker.nxm").open("wb") as out:
        out.write(b"NXM1")
        out.write(struct.pack("<II", len(vertices), faces.size))
        out.write(vertices.tobytes())
        out.write(faces.astype("<u4").tobytes())
    material = gltf.materials[primitive.material].pbrMetallicRoughness
    image = gltf.images[gltf.textures[material.baseColorTexture.index].source]
    if image.mimeType != "image/jpeg" or image.bufferView is None:
        raise ValueError("Expected an embedded JPEG base-color texture")
    view = gltf.bufferViews[image.bufferView]
    (args.output / "stalker.jpg").write_bytes(blob[view.byteOffset:view.byteOffset + view.byteLength])
    print(f"Prepared {len(vertices):,} vertices and {len(faces):,} triangles")


if __name__ == "__main__":
    main()
