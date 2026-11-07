"""Bake the CC0 Innerscene/MakeHuman jog character into NXRP's animated mesh.

Install numpy, Pillow, pygltflib and fast-simplification separately, then run:
  python tools/prepare_survivor.py path/to/jog-maya-3d-person-animated.glb
The source, license and conversion choices are documented in assets/models/README.md.
"""
import argparse
import io
import struct
from pathlib import Path

import fast_simplification
import numpy as np
from PIL import Image
from pygltflib import GLTF2


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--output', type=Path, default=Path('assets/models'))
    parser.add_argument('--frames', type=int, default=24)
    args = parser.parse_args()
    if not 4 <= args.frames <= 60:
        raise ValueError('Frame count must be between 4 and 60')
    model = GLTF2().load(str(args.source))
    blob = model.binary_blob()
    component = {5121: '<u1', 5123: '<u2', 5125: '<u4', 5126: '<f4'}
    width = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}

    def accessor(index):
        entry = model.accessors[index]
        if entry.sparse is not None or entry.bufferView is None:
            raise ValueError('Sparse and bufferless accessors are not supported')
        view = model.bufferViews[entry.bufferView]
        dtype = np.dtype(component[entry.componentType])
        count = width[entry.type]
        offset = (view.byteOffset or 0) + (entry.byteOffset or 0)
        stride = view.byteStride or count * dtype.itemsize
        result = np.ndarray((entry.count, count), dtype=dtype, buffer=blob,
                            offset=offset, strides=(stride, dtype.itemsize)).copy()
        if entry.type == 'MAT4':
            return result.reshape(-1, 4, 4).transpose(0, 2, 1)
        return result

    def matrix(node, translation, rotation, scale):
        if node.matrix is not None:
            return np.asarray(node.matrix, dtype=np.float64).reshape(4, 4).T
        x, y, z, w = rotation
        r = np.array([
            [1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
            [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
            [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]], dtype=np.float64)
        result = np.eye(4)
        result[:3, :3] = r @ np.diag(scale)
        result[:3, 3] = translation
        return result

    # A single atlas keeps each animated player to one texture and one draw list.
    tile, grid, pad = 256, 4, 3
    atlas = Image.new('RGBA', (tile * grid, tile * grid), (30, 28, 25, 255))
    for index, material in enumerate(model.materials):
        if index >= grid * grid:
            raise ValueError('Too many materials for the atlas')
        pbr = material.pbrMetallicRoughness
        factor = np.asarray(pbr.baseColorFactor if pbr and pbr.baseColorFactor else [1, 1, 1, 1])
        texture = pbr.baseColorTexture if pbr else None
        if texture is None:
            image = Image.new('RGBA', (tile - 2 * pad, tile - 2 * pad), tuple((factor * 255).astype(int)))
        else:
            image_def = model.images[model.textures[texture.index].source]
            view = model.bufferViews[image_def.bufferView]
            image = Image.open(io.BytesIO(blob[view.byteOffset:view.byteOffset + view.byteLength])).convert('RGBA')
            image = image.resize((tile - 2 * pad, tile - 2 * pad), Image.Resampling.LANCZOS)
            rgba = np.asarray(image, dtype=np.float32) * factor.reshape(1, 1, 4)
            if index == 2:  # Give the bright plum shirt a subdued field-jacket palette.
                luminance = rgba[:, :, :3] @ np.array([.2126, .7152, .0722])
                rgba[:, :, :3] = luminance[:, :, None] * np.array([.68, .76, .62])
            image = Image.fromarray(np.clip(rgba, 0, 255).astype(np.uint8), 'RGBA')
        atlas.paste(image, ((index % grid) * tile + pad, (index // grid) * tile + pad))

    positions, normals, uvs, joints, weights, triangles = [], [], [], [], [], []
    vertex_offset = 0
    for node in model.nodes:
        if node.mesh is None or node.name in {'Hairline | fine scalp roots', 'Phone', 'teeth_base'}:
            continue
        for primitive in model.meshes[node.mesh].primitives:
            attrs = primitive.attributes
            p, n = accessor(attrs.POSITION), accessor(attrs.NORMAL)
            uv = accessor(attrs.TEXCOORD_0) if attrs.TEXCOORD_0 is not None else np.full((len(p), 2), .5)
            joint, weight = accessor(attrs.JOINTS_0), accessor(attrs.WEIGHTS_0)
            faces = accessor(primitive.indices).reshape(-1, 3).astype(np.int32)
            budget = 12000 if node.name == 'Maya | anatomical body' else 2000 if node.name in {'ponytail01', 'shoes03'} else len(faces)
            if len(faces) > budget:
                reduced, faces, collapses = fast_simplification.simplify(
                    p, faces, target_count=budget, preserve_border=False, return_collapses=True)
                replayed, replayed_faces, mapping = fast_simplification.replay_simplification(p, accessor(primitive.indices).reshape(-1, 3).astype(np.int32), collapses)
                if not np.allclose(reduced, replayed) or not np.array_equal(faces, replayed_faces):
                    raise ValueError('Mesh simplification replay did not match')
                valid = np.flatnonzero((mapping >= 0) & (mapping < len(reduced)))
                distance = np.sum((p[valid] - reduced[mapping[valid]]) ** 2, axis=1)
                sorted_ids = valid[np.lexsort((distance, mapping[valid]))]
                _, first = np.unique(mapping[sorted_ids], return_index=True)
                chosen = sorted_ids[first]
                if len(chosen) != len(reduced):
                    raise ValueError('A reduced vertex has no original attribute source')
                selection = np.empty(len(reduced), dtype=int); selection[mapping[chosen]] = chosen
                p, n, uv, joint, weight = reduced, n[selection], uv[selection], joint[selection], weight[selection]
            material = primitive.material
            uv = np.clip(uv, 0, 1).astype(np.float32)
            uv[:, 0] = ((material % grid) * tile + pad + uv[:, 0] * (tile - 2 * pad - 1)) / (tile * grid)
            uv[:, 1] = ((material // grid) * tile + pad + uv[:, 1] * (tile - 2 * pad - 1)) / (tile * grid)
            positions.append(p); normals.append(n); uvs.append(uv); joints.append(joint); weights.append(weight)
            triangles.append(faces.astype(np.uint32) + vertex_offset); vertex_offset += len(p)

    positions = np.concatenate(positions).astype(np.float64)
    normals = np.concatenate(normals).astype(np.float64)
    uvs = np.concatenate(uvs).astype('<f4')
    joints = np.concatenate(joints).astype(np.int32)
    weights = np.concatenate(weights).astype(np.float64)
    weights /= np.maximum(weights.sum(axis=1, keepdims=True), 1e-12)
    triangles = np.concatenate(triangles).reshape(-1).astype('<u4')
    inverse_bind = accessor(model.skins[0].inverseBindMatrices).astype(np.float64)
    animation = model.animations[0]
    tracks = []
    for channel in animation.channels:
        sampler = animation.samplers[channel.sampler]
        if sampler.interpolation != 'CUBICSPLINE':
            raise ValueError('Expected cubic animation channels')
        times = accessor(sampler.input).reshape(-1)
        values = accessor(sampler.output).reshape(len(times), 3, -1)
        tracks.append((channel.target.node, channel.target.path, times, values))
    duration = max(float(times[-1]) for _, _, times, _ in tracks)
    parents = [-1] * len(model.nodes)
    for index, node in enumerate(model.nodes):
        for child in node.children or []:
            parents[child] = index

    def pose_at(time):
        trs = [[np.array(n.translation or [0, 0, 0], dtype=float),
                np.array(n.rotation or [0, 0, 0, 1], dtype=float),
                np.array(n.scale or [1, 1, 1], dtype=float)] for n in model.nodes]
        for node, path, times, values in tracks:
            index = min(max(int(np.searchsorted(times, time, side='right')) - 1, 0), len(times) - 2)
            span = float(times[index + 1] - times[index])
            t = float(np.clip((time - times[index]) / span, 0, 1)) if span > 0 else 0
            v0, tangent0 = values[index, 1], values[index, 2] * span
            v1, tangent1 = values[index + 1, 1], values[index + 1, 0] * span
            value = ((2*t**3-3*t**2+1)*v0 + (t**3-2*t**2+t)*tangent0
                     + (-2*t**3+3*t**2)*v1 + (t**3-t**2)*tangent1)
            part = {'translation': 0, 'rotation': 1, 'scale': 2}[path]
            trs[node][part] = value / max(np.linalg.norm(value), 1e-12) if part == 1 else value
        local = [matrix(node, *trs[i]) for i, node in enumerate(model.nodes)]
        world = [None] * len(local)
        def global_matrix(i):
            if world[i] is None:
                world[i] = global_matrix(parents[i]) @ local[i] if parents[i] >= 0 else local[i]
            return world[i]
        return np.stack([global_matrix(j) @ inverse_bind[i] for i, j in enumerate(model.skins[0].joints)])

    homogeneous = np.column_stack((positions, np.ones(len(positions))))
    frames = []
    for frame in range(args.frames):
        bones = pose_at(frame * duration / args.frames)
        transformed = np.zeros((len(positions), 3)); transformed_normal = np.zeros_like(transformed)
        for influence in range(4):
            matrices = bones[joints[:, influence]]
            weighted = weights[:, influence, None]
            transformed += np.einsum('nij,nj->ni', matrices[:, :3, :], homogeneous) * weighted
            transformed_normal += np.einsum('nij,nj->ni', matrices[:, :3, :3], normals) * weighted
        transformed_normal /= np.maximum(np.linalg.norm(transformed_normal, axis=1, keepdims=True), 1e-12)
        frames.append((transformed, transformed_normal))
    # Keep all poses at the same scale and floor height, preserving the gait.
    floor = min(frame[0][:, 1].min() for frame in frames)
    height = max(frame[0][:, 1].max() for frame in frames) - floor
    if not np.isfinite(height) or height <= 0:
        raise ValueError('Invalid character height')
    # Blend opposite strides into a grounded neutral pose for stationary players.
    idle_position = (frames[0][0] + frames[len(frames)//4][0]) * .5
    idle_normal = (frames[0][1] + frames[len(frames)//4][1]) * .5
    idle_normal /= np.maximum(np.linalg.norm(idle_normal, axis=1, keepdims=True), 1e-12)
    idle_position[:, 1] -= idle_position[:, 1].min() - floor
    frames.append((idle_position, idle_normal))
    args.output.mkdir(parents=True, exist_ok=True)
    atlas.save(args.output / 'survivor.png', optimize=True)
    with (args.output / 'survivor.nxa').open('wb') as out:
        out.write(b'NXA1')
        out.write(struct.pack('<IIIf', len(positions), len(triangles), len(frames), duration))
        out.write(uvs.tobytes()); out.write(triangles.tobytes())
        for p, n in frames:
            p[:, 1] -= floor; p *= 1.18 / height
            out.write(np.concatenate((p, n), axis=1).astype('<f4').tobytes())
    print(f'Prepared {len(positions):,} vertices, {len(triangles)//3:,} triangles and {len(frames)} animated frames')


if __name__ == '__main__':
    main()
