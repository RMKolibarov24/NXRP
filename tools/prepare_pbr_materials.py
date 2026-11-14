"""Download/checksum and convert the two attributed CC0 Poly Haven materials.

Requires Pillow. Run with --download to obtain the original maps; without that
flag it converts the existing files under build/asset-sources/pbr. Runtime never
contacts Poly Haven. API credit: Powered by Poly Haven, https://polyhaven.com/.
"""
import argparse
import hashlib
import json
from pathlib import Path
from urllib.request import Request, urlopen
from PIL import Image


def fetch(url):
    request = Request(url, headers={'User-Agent': 'NXRP-asset-import/1.0'})
    with urlopen(request, timeout=60) as response:
        return response.read()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--download', action='store_true')
    parser.add_argument('--source', type=Path, default=Path('build/asset-sources/pbr'))
    parser.add_argument('--output', type=Path, default=Path('assets/textures/pbr'))
    args = parser.parse_args()
    args.source.mkdir(parents=True, exist_ok=True)
    args.output.mkdir(parents=True, exist_ok=True)
    receipt = {'license': 'CC0-1.0', 'license_url': 'https://polyhaven.com/license',
               'credit': 'Powered by Poly Haven', 'assets': []}
    for label, asset in [('ground', 'brown_mud_leaves_01'), ('wood', 'weathered_brown_planks')]:
        files = json.loads(fetch('https://api.polyhaven.com/files/'+asset)) if args.download else None
        asset_receipt = {'asset': asset, 'source': 'https://polyhaven.com/a/'+asset, 'files': []}
        for kind, extension in [('diffuse', 'jpg'), ('nor_gl', 'png'), ('arm', 'jpg')]:
            source = args.source/(label+'-'+kind+'.'+extension)
            if files is not None:
                key = next(k for k in files if k.lower() == kind)
                entry = files[key]['1k'][extension]
                asset_receipt['files'].append({'map': kind, **entry})
                if not source.exists() or hashlib.md5(source.read_bytes()).hexdigest() != entry['md5']:
                    data = fetch(entry['url'])
                    if len(data) != entry['size'] or hashlib.md5(data).hexdigest() != entry['md5']:
                        raise ValueError('Poly Haven asset checksum/size mismatch: '+entry['url'])
                    source.write_bytes(data)
            image = Image.open(source).convert('RGB')
            if image.size != (1024, 1024):
                image = image.resize((1024, 1024), Image.Resampling.LANCZOS)
            image.save(args.output/(label+'-'+kind+'.png'), optimize=True)
            print(label, kind, image.size)
        receipt['assets'].append(asset_receipt)
    if args.download:
        (args.output/'SOURCES.json').write_text(json.dumps(receipt, indent=2)+'\n')


if __name__ == '__main__':
    main()
