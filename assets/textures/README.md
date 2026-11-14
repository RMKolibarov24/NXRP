# Game textures

## Sourced PBR materials

`pbr/ground-*.png` use [Brown Mud Leaves 01 by Rob Tuytel](https://polyhaven.com/a/brown_mud_leaves_01).
`pbr/wood-*.png` use [Weathered Brown Planks by Dimitrios Savva and Rico Cilliers](https://polyhaven.com/a/weathered_brown_planks).
Both sets are [CC0](https://polyhaven.com/license). Powered by Poly Haven:
https://polyhaven.com/. The public API is used only by the optional import tool;
the game has no dependency on their service.

The diffuse, OpenGL tangent-space normal and AO/roughness/metal maps were
downloaded at 1K, verified against the API MD5 and size, and converted to RGB PNG.
ARM channels remain R=occlusion, G=roughness, B=metalness (these are dielectrics).
Normal and material values are sampled as data, without a colour gamma transform.
The renderer uses mipmapped filtering. Water collects in source occlusion/roughness
recesses, darkens the mud and reduces its roughness. Timber floors remain dry.
Normals are transformed through the UV derivative frame; roughness controls
highlight width and the strength/blur of wet ground reflections.

Reproduce with `python tools/prepare_pbr_materials.py --download` (requires Pillow).
Converted source maps ship with the game; Python is not required to play.

## Earlier generated assets

The earlier assets below were generated with the built-in image generation tool.
The sourced PBR maps now replace the primary ground and shelter-wood textures;
the earlier albedo images remain fallback assets.

## shelter-wood.png

Use case: photorealistic-natural. Asset type: seamless albedo texture for 3D game shelter walls. Straight-on close view of old rough vertical timber planks from a small abandoned forest cabin, dark weathered brown wood with subtle age cracks, grain and sparse moss in seams, consistent scale and evenly distributed detail. Neutral flat diffuse lighting, no cast shadows, no perspective, no highlights, fills the entire square frame edge to edge and tiles seamlessly on all sides. No windows, doors, nails, text, symbols, people or background.

## hedge.png

Use case: photorealistic-natural. Asset type: repeatable albedo texture for a 3D haunted hedge maze game. A close, straight-on surface of dense, small, dark green yew and boxwood leaves interwoven with a few fine brown twigs; richly detailed natural foliage. Fill the entire square frame with evenly distributed leaves at consistent scale and depth, with no background or horizon. Neutral diffuse illumination, no directional light, no shadows cast across the whole image. Tileable on all four edges. No text, flowers, fruit, people, or objects.

## ground.png

Use case: photorealistic-natural. Asset type: repeatable albedo ground texture for a first-person haunted hedge maze game. Straight-down close-up of damp packed dark soil, scattered fine gravel, sparse small dead leaves and a little patchy moss, even consistent density across the image. Neutral diffuse light and natural muted earth colors; show fine realistic surface detail. Fill the entire square frame edge to edge. Tileable on all four sides, with no strong repeated landmarks. No text, grass blades sticking up, objects, or background.

## creature.png

Use case: stylized-concept. Asset type: transparent full-body enemy sprite for a first-person haunted hedge maze game. An original eerie gaunt humanoid creature, elongated limbs, hunched shoulders, a ragged dark organic silhouette with subtle tangled roots and dead leaves, two small dim amber eyes; unsettling but no gore. Realistic tactile details, dark muted bark and cloth tones, cinematic horror. Front view with the entire figure visible from head to feet, centered and occupying most of a square canvas with comfortable transparent margins, arms clear of the body, no ground plane or shadow. Isolated on a genuinely transparent background. No text, no frame, no extra characters.

## foliage.png

Use case: photorealistic-natural. Asset type: transparent foliage card for a first-person haunted hedge maze game. A single compact, irregular sprig of small dark green yew and boxwood leaves on thin brown twigs, viewed straight-on, naturally overlapping dense foliage, fine individual leaf edges. The sprig fills most of the square frame with a small margin and has a wide roughly horizontal silhouette suitable for repeated use as 3D hedge foliage. Soft neutral diffuse lighting, no cast shadow. Isolated on a genuinely transparent background, no ground, no sky, no pot, no text, no border.
