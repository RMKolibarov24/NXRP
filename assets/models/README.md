# Stalker model

`stalker.nxm` and `stalker.jpg` are derived from the light version of
[Horror Monster by Grab3D](https://grab3d.com/models/creatures/horror-monster/),
released under [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/).
The original model was AI-assisted; Grab3D's package says its base mesh and
textures were generated with an image-to-3D tool and then cleaned and textured
in Blender. The source model has no rig. The game reduces its geometry and adds
procedural vertex motion; it does not include skeletal animation.

The source light model has 400,000 triangles. The included mesh has 60,316
triangles and a height of 1.65 game units. `stalker.nxm` stores a four-byte
`NXM1` signature, little-endian 32-bit vertex and index counts, interleaved
position/normal/UV float32 vertices, then uint32 triangle indices.

To regenerate the reduced files from the source GLB, install `numpy`,
`pygltflib`, and `fast-simplification` in a separate Python environment and run
`python tools/prepare_stalker.py path/to/horror-monster-light.glb` from the
repository root. The conversion tools are not required to build or run the game.

# Survivor model

`survivor.nxa` and `survivor.png` are derived from [Jog - Maya 3D Person, Animated](https://www.innerscene.com/tools/library/3d-parts/jog-maya-3d-person-animated-0e72686c),
an Innerscene character adapted from MakeHuman assets and released under
[CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). The source has
a skeletal jog animation; NXRP decimates the geometry and bakes 24 poses into
a shared animated mesh for all co-op players, plus a blended stationary pose.
It removes the source phone,
teeth and fine hairline geometry and combines materials into one texture atlas.

To regenerate the two files from the source GLB, install `numpy`, `Pillow`,
`pygltflib` and `fast-simplification` in a separate Python environment, then
run `python tools/prepare_survivor.py path/to/jog-maya-3d-person-animated.glb`.
The converter is not needed to build or run the game.

# First-person hands

`hands-l.nxm` and `hands-r.nxm` are extracted from the same downloaded CC0
Innerscene/MakeHuman Jog Maya source listed above, using its original hand and
forearm triangles, UVs and `survivor.png` skin texture. No hand geometry is
procedurally authored. The converter selects lowerarm/hand descendant bone
weights, recentres the meshes, and runtime transforms animate camera sway,
drinking and reaching for the slot handle. Regenerate with prepare_survivor.py.

`hands-l-grip.nxm` and `hands-r-grip.nxm` retain the exact same source topology
and UVs as the open hands. They bake additional rotations of the source finger
joints using the original skin weights. Runtime interpolates open and closed
positions/normals for basketball, vessel and lever contact, with no new geometry.
