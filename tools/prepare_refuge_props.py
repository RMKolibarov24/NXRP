"""Convert downloaded glTF props, preserving materials and the slot lever pivot.

Requires numpy, Pillow, pygltflib and DracoPy. No geometry is generated.
Sources and licenses are recorded in assets/models/REFUGE_SOURCES.md.
"""
import io
import json
import struct
from pathlib import Path

import DracoPy
import numpy as np
from PIL import Image
from pygltflib import GLTF2


def convert(source, prefix, groups, height):
    gltf = GLTF2().load(str(source))
    blob = gltf.binary_blob()
    def accessor(index):
        a = gltf.accessors[index]
        v = gltf.bufferViews[a.bufferView]
        dtype = np.dtype({5120:'i1',5121:'u1',5122:'<i2',5123:'<u2',5125:'<u4',5126:'<f4'}[a.componentType])
        width = {'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a.type]
        out = np.ndarray((a.count,width), dtype=dtype, buffer=blob,
            offset=(v.byteOffset or 0)+(a.byteOffset or 0),strides=(v.byteStride or width*dtype.itemsize,dtype.itemsize)).copy()
        if a.normalized:
            limit = np.iinfo(dtype).max
            out = np.maximum(out.astype(float)/limit,-1)
        return out
    tile, grid = 256, 5
    atlas = Image.new('RGB',(tile*grid,tile*grid),'white')
    for i,m in enumerate(gltf.materials):
        p = m.pbrMetallicRoughness
        factor = np.array((p.baseColorFactor or [1,1,1,1])[:3])
        # glTF factors are linear; atlas is sampled by legacy fixed-function GL.
        factor = np.maximum(factor,0)**(1/2.2)
        image = Image.new('RGB',(tile,tile),'white')
        if p.baseColorTexture is not None:
            texture = gltf.textures[p.baseColorTexture.index]
            imageDef = gltf.images[texture.source if texture.source is not None else texture.extensions['EXT_texture_webp']['source']]
            view = gltf.bufferViews[imageDef.bufferView]
            image = Image.open(io.BytesIO(blob[view.byteOffset:view.byteOffset+view.byteLength])).convert('RGB').resize((tile,tile))
        image = Image.fromarray(np.clip(np.asarray(image)*factor,0,255).astype('uint8'))
        atlas.paste(image,((i%grid)*tile,(i//grid)*tile))
    data = []
    for node in gltf.nodes:
        if node.mesh is None: continue
        if node.rotation or node.matrix: raise ValueError('Unexpected rotation; review source transforms')
        scale = np.array(node.scale or [1,1,1]); translation = np.array(node.translation or [0,0,0])
        for primitive in gltf.meshes[node.mesh].primitives:
            m = primitive.material
            # Do not bake transparent glass as an opaque cover over the reels.
            factor = gltf.materials[m].pbrMetallicRoughness.baseColorFactor or [1,1,1,1]
            if factor[3] < .5: continue
            extension = primitive.extensions.get('KHR_draco_mesh_compression')
            if extension:
                view = gltf.bufferViews[extension['bufferView']]
                decoded = DracoPy.decode(blob[view.byteOffset:view.byteOffset+view.byteLength])
                p,n,uv,faces = decoded.points,decoded.normals,decoded.tex_coord,decoded.faces
            else:
                p = accessor(primitive.attributes.POSITION)
                n = accessor(primitive.attributes.NORMAL)
                uv = accessor(primitive.attributes.TEXCOORD_0) if primitive.attributes.TEXCOORD_0 is not None else None
                faces = accessor(primitive.indices).reshape(-1,3) if primitive.indices is not None else np.arange(len(p)).reshape(-1,3)
            p = p*scale+translation
            n = n/scale; n /= np.maximum(np.linalg.norm(n,axis=1,keepdims=True),1e-9)
            uv = np.zeros((len(p),2))+.5 if uv is None else np.clip(uv,0,1)
            uv[:,0] = (m%grid+(2+uv[:,0]*251)/tile)/grid
            uv[:,1] = (m//grid+(2+(1-uv[:,1])*251)/tile)/grid
            data.append((node.name,p,n,uv,faces,m))
    # Use cabinet bounds for common scaling; lever stays in the same space.
    main = np.concatenate([p for name,p,_,_,_,_ in data if name in groups[0][1]])
    low,high = main.min(0),main.max(0)
    origin = np.array([(low[0]+high[0])/2,low[1],(low[2]+high[2])/2])
    scale = height/(high[1]-low[1])
    output = Path('assets/models'); output.mkdir(exist_ok=True)
    atlas.save(output/(prefix+'.png'))
    metadata = {'origin':origin.tolist(),'scale':scale,'bounds':((high-low)*scale).tolist()}
    for label,names in groups:
        vertices,indices = [],[]; offset=0
        for name,p,n,uv,faces,material in data:
            if name not in names: continue
            if prefix == "slots" and label == "cabinet" and material in (7,8,10,11): continue
            vertices.append(np.concatenate(((p-origin)*scale,n,uv),axis=1))
            indices.append(faces+offset); offset+=len(p)
        vertices=np.concatenate(vertices).astype('<f4');indices=np.concatenate(indices).astype('<u4').reshape(-1)
        with (output/(prefix+'-'+label+'.nxm')).open('wb') as file:
            file.write(b'NXM1'+struct.pack('<II',len(vertices),len(indices)))
            file.write(vertices.tobytes());file.write(indices.tobytes())
        print(label,len(vertices),len(indices)//3)
    if prefix == 'slots':
        # Select the downloaded curved reel strips by source material and column.
        # The four states reuse the original artwork; no replacement geometry.
        for column in range(3):
            for symbol,remap in enumerate((7,8,10,11)):
                verts,inds=[],[];offset=0
                for name,p,n,uv,faces,material in data:
                    if name != 'Cabinet' or material not in (7,8,10,11):continue
                    centres=p[faces].mean(axis=1)
                    lateral=np.where(np.abs(centres[:,2])>np.abs(centres[:,0]),centres[:,0],-centres[:,2])
                    cols=np.argmin(np.abs(lateral[:,None]-np.array([-.112,0,.112])[None,:]),axis=1)
                    chosen=faces[cols==column]
                    if not len(chosen):continue
                    used,inverse=np.unique(chosen.reshape(-1),return_inverse=True)
                    mapped=uv[used].copy()
                    mapped[:,0]+=(remap%grid-material%grid)/grid
                    mapped[:,1]+=(remap//grid-material//grid)/grid
                    verts.append(np.concatenate(((p[used]-origin)*scale,n[used],mapped),axis=1))
                    inds.append(inverse+offset);offset+=len(used)
                verts=np.concatenate(verts).astype('<f4');inds=np.concatenate(inds).astype('<u4')
                with (output/f'slots-reel-{column}-{symbol}.nxm').open('wb') as file:
                    file.write(b'NXM1'+struct.pack('<II',len(verts),len(inds)))
                    file.write(verts.tobytes());file.write(inds.tobytes())
    for node in gltf.nodes:
        if node.name=='Lever':metadata['leverPivot']=((np.array(node.translation)-origin)*scale).tolist()
    (output/(prefix+'.json')).write_text(json.dumps(metadata,indent=2))
    print(prefix,metadata)


if __name__ == '__main__':
    convert(Path('build/asset-sources/slot-machine.glb'),'slots',
        [('cabinet',['Cabinet']),('lever',['Lever']),('stool',['Stool'])],1.20)
    convert(Path('build/asset-sources/bar-counter.glb'),'bar',
        [('counter',['bar-counter-module_'+str(i) for i in range(5)])],.90)

