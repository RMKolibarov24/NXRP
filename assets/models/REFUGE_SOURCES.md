# Downloaded refuge props

## Pillar Slots

Author: local.yany. License: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
Original: https://sketchfab.com/3d-models/pillar-slots-91e255e5a95745f4857607b388421ee1
Downloaded GLB mirror:
https://raw.githubusercontent.com/bedchem/fruit-fly-hub/main/public/models/slot-machine.glb
Mirror license record: https://github.com/bedchem/fruit-fly-hub/blob/main/LICENSE
The embedded glTF asset metadata independently records the same author, original
source and CC BY 4.0 license.
Source GLB SHA256: `CAC46A07E4D2AA61446E48F0DE46661A8C6C485F6B24861871EBF7FCC7C7B340`.

Derived files: slots-cabinet.nxm, slots-lever.nxm, slots-stool.nxm, slots.png,
slots.json and slots-reel-*-*.nxm. The downloaded curved reel strips are separated
by source column and reuse four original texture variations for spinning and
settled outcomes. Changes: decoded Draco geometry, transparent glass omitted for the
legacy renderer, materials baked to atlas, scaled and recentered. The original
lever pivot is preserved and animated at runtime. No mesh was authored here.

## Bar counter module

Source: https://3dassets.dev/assets/restaurant-kitchen-and-dining-bar-counter-module-488ecf7d
Download: https://cdn.3dassets.dev/assets/16519/v1/model.glb
License: [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/).
Source GLB SHA256: `BDBBA9C1939B0609EF1BF6B7739B245D8E873D7EA30EE9A3E1D8B396DCCFDA8C`.
Derived files: bar-counter.nxm, bar.png, bar.json. Changes: node transforms and
quantized coordinates decoded, materials baked to atlas, scaled and recentered.
No mesh was authored here.

Reproduce with tools/prepare_refuge_props.py using source GLBs placed under
build/asset-sources. Runtime does not require Python or the conversion libraries.
