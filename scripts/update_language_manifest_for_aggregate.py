from pathlib import Path

ROOT = Path('/home/ubuntu/danenone')
SOURCE = ROOT / 'native-shell/languages/manifest.tsv'
ROOTFS = ROOT / 'archiso-profile/airootfs/usr/share/influent/languages/manifest.tsv'
PACKAGE = 'influent-language-pack-all_0.2.1_all.deb'
URL = 'https://github.com/JesusQuijada34/danenone/releases/download/language-packs-v0.2.1'
SHA256 = '30f1045bb9d0d8755d6553d4cad5a2ede3ef0b12e25aa9fa53d973dee1c0c00c'

rows = []
for line in SOURCE.read_text(encoding='utf-8').splitlines():
    if not line.strip() or line.startswith('#'):
        continue
    fields = line.split('|')
    if len(fields) >= 5:
        rows.append((fields[0], fields[4]))
content = '# code|package|url|sha256|display_name\n' + '\n'.join(f'{code}|{PACKAGE}|{URL}|{SHA256}|{display}' for code, display in rows) + '\n'
SOURCE.write_text(content, encoding='utf-8')
ROOTFS.parent.mkdir(parents=True, exist_ok=True)
ROOTFS.write_text(content, encoding='utf-8')
print(f'LANGUAGE_MANIFEST_ROWS={len(rows)}')
print(f'LANGUAGE_MANIFEST_PACKAGE={PACKAGE}')
print(f'LANGUAGE_MANIFEST_SHA256={SHA256}')
