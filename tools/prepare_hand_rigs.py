"""Extract unsimplified CC0 Innerscene/MakeHuman arms, skin and original rigs.

Uses the same downloaded jog-maya.glb attributed in assets/models/README.md.
No geometry is generated, subdivided or retopologized. Source triangles,
normals, UVs, finger joints and vertex weights are retained.
"""
import argparse
import io
import struct
from pathlib import Path
import numpy as np
from PIL import Image
from pygltflib import GLTF2


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--output', type=Path, default=Path('assets/models'))
    args = parser.parse_args()
    model = GLTF2().load(str(args.source))
    blob = model.binary_blob()
    def read(index):
        a = model.accessors[index]
        v = model.bufferViews[a.bufferView]
        dtype = np.dtype({5121: '<u1', 5123: '<u2', 5125: '<u4', 5126: '<f4'}[a.componentType])
        width = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}[a.type]
        out = np.ndarray((a.count, width), dtype=dtype, buffer=blob,
                         offset=(v.byteOffset or 0)+(a.byteOffset or 0),
                         strides=(v.byteStride or width*dtype.itemsize, dtype.itemsize)).copy()
        if a.normalized:
            out = out.astype(float)/np.iinfo(dtype).max
        return out.reshape(-1,4,4).transpose(0,2,1) if a.type == 'MAT4' else out

    skin = model.skins[0]
    bind = np.linalg.inv(read(skin.inverseBindMatrices).astype(float))
    # Original bind matrices have a uniform armature scale. Keep actual joint
    # positions and mesh dimensions; remove only scale from rotation bases.
    rotation = bind[:,:3,:3].copy()
    rotation /= np.linalg.norm(rotation,axis=1)[:,None,:]
    assert np.allclose(rotation.transpose(0,2,1)@rotation, np.eye(3), atol=2e-5)
    body = next(n for n in model.nodes if n.name == 'Maya | anatomical body')
    primitive = model.meshes[body.mesh].primitives[0]
    a = primitive.attributes
    position, normal, uv = read(a.POSITION), read(a.NORMAL), read(a.TEXCOORD_0)
    joints, weights = read(a.JOINTS_0).astype(int), read(a.WEIGHTS_0).astype(float)
    faces = read(primitive.indices).reshape(-1,3)
    texture = model.materials[primitive.material].pbrMetallicRoughness.baseColorTexture
    image = model.images[model.textures[texture.index].source]
    view = model.bufferViews[image.bufferView]
    args.output.mkdir(parents=True, exist_ok=True)
    Image.open(io.BytesIO(blob[view.byteOffset:view.byteOffset+view.byteLength])).convert('RGB').save(args.output/'hands-skin.png')
    parent = {child:i for i,n in enumerate(model.nodes) for child in n.children or []}
    by_name = {n.name:i for i,n in enumerate(model.nodes)}
    for side in ('l','r'):
        nodes = []
        def include(node):
            if node in skin.joints:
                nodes.append(node)
            for child in model.nodes[node].children or []:
                include(child)
        include(by_name['upperarm_'+side])
        node_map = {node:i for i,node in enumerate(nodes)}
        source_bones = [skin.joints.index(n) for n in nodes]
        bone_map = {source:i for i,source in enumerate(source_bones)}
        influence = np.sum(weights*np.isin(joints,source_bones),axis=1)
        selected = faces[np.all(influence[faces]>.60,axis=1)]
        used, indices = np.unique(selected.reshape(-1),return_inverse=True)
        weight = weights[used]*np.isin(joints[used],source_bones)
        weight /= weight.sum(axis=1,keepdims=True)
        joint = np.array([[bone_map.get(int(j),0) for j in row] for row in joints[used]],dtype='<u4')
        hand = skin.joints.index(by_name['hand_'+side])
        middle = skin.joints.index(by_name['middle_01_'+side])
        index = skin.joints.index(by_name['index_01_'+side])
        pinky = skin.joints.index(by_name['pinky_01_'+side])
        forward = bind[middle,:3,3]-bind[hand,:3,3]
        palm_length = np.linalg.norm(forward)
        forward /= palm_length
        across = bind[index,:3,3]-bind[pinky,:3,3]
        across -= forward*np.dot(across,forward)
        across /= np.linalg.norm(across)
        palm_basis = np.column_stack((across,forward,np.cross(across,forward)))
        with (args.output/f'arms-{side}.nxr').open('wb') as out:
            out.write(b'NXR1'+struct.pack('<III',len(used),len(indices),len(nodes)))
            out.write(struct.pack('<f',palm_length))
            out.write(palm_basis.astype('<f4').tobytes())
            for i,node in enumerate(nodes):
                name = model.nodes[node].name
                role = 1 if name.startswith('upperarm_') else 2 if name=='lowerarm_'+side else 3 if name=='hand_'+side else 4 if name.startswith('thumb_') else 5 if name.startswith(('index_','middle_','ring_','pinky_')) else 0
                bend = .45 if role==4 else .65 if '_01_' in name else .85
                out.write(struct.pack('<iIf',node_map.get(parent.get(node),-1),role,bend))
                source = source_bones[i]
                out.write(bind[source,:3,3].astype('<f4').tobytes())
                out.write(rotation[source].astype('<f4').tobytes())
            vertices = np.concatenate((position[used],normal[used],uv[used],weight),axis=1).astype('<f4')
            for vertex, ids in zip(vertices,joint):
                out.write(vertex.tobytes());out.write(ids.tobytes())
            out.write(indices.astype('<u4').tobytes())
        print(f'{side}: {len(used)} source vertices, {len(indices)//3} source triangles, {len(nodes)} original bones')


if __name__ == '__main__':
    main()
