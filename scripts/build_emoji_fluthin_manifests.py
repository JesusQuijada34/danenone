from pathlib import Path
import csv
import hashlib

ROOT = Path('/home/ubuntu/danenone')
PROFILE = ROOT / 'native-shell/emojis/profiles.tsv'
ARTIFACTS = ROOT / 'build/emoji-fluthin-artifacts'
VALIDATION = ROOT / 'build/emoji-fluthin-validation.txt'
OUT_SOURCE = ROOT / 'build/emoji-fluthin-manifests'
OUT_ROOTFS = ROOT / 'archiso-profile/airootfs/usr/share/influent-danenone/fluthin'
RELEASE_URL = 'https://github.com/JesusQuijada34/danenone/releases/download/emoji-profiles-v0.1.0'
VERSION = 'v0.1-26.08-22.31'


def main() -> None:
    digests = {}
    for line in VALIDATION.read_text(encoding='utf-8').splitlines():
        fields = line.split('|')
        if len(fields) >= 3 and fields[0].endswith('.iflapp'):
            digests[fields[0]] = fields[2]
    profiles = {}
    with PROFILE.open(encoding='utf-8') as handle:
        for row in csv.reader(handle, delimiter='|'):
            if row and not row[0].startswith('#') and len(row) >= 6:
                profiles[row[0]] = row[:6]
    rows = []
    for asset in sorted(digests):
        app_id = asset.split('.v0.1-', 1)[0].split('.', 1)[1]
        code = app_id.removeprefix('emoji-')
        code, display, source, first_four, license_text, asset_status = profiles[code]
        rows.append((app_id, display, asset, digests[asset], source, license_text, first_four, asset_status))
    OUT_SOURCE.mkdir(parents=True, exist_ok=True)
    OUT_ROOTFS.mkdir(parents=True, exist_ok=True)
    repo_lines = ['# app|display_name|publisher|version|platform|asset|release_url|sha256|source|license|asset_status']
    trust_lines = ['# asset|sha256|publisher|version|platform|source|license']
    cert_lines = ['# source|license|policy|note']
    for app_id, display, asset, digest, source, license_text, first_four, asset_status in rows:
        repo_lines.append('|'.join((app_id, display, 'Influent', VERSION, 'Danenone', asset, RELEASE_URL, digest, source, license_text, asset_status)))
        trust_lines.append('|'.join((asset, digest, 'Influent', VERSION, 'Danenone', source, license_text)))
        cert_lines.append('|'.join((source, license_text, 'sha256-pinned', 'Profile-only metadata; no proprietary Windows font included')))
    (OUT_SOURCE / 'emoji-repository.list').write_text('\n'.join(repo_lines) + '\n', encoding='utf-8')
    (OUT_SOURCE / 'emoji-trusted-sha256.tsv').write_text('\n'.join(trust_lines) + '\n', encoding='utf-8')
    (OUT_SOURCE / 'emoji-license-notices.tsv').write_text('\n'.join(cert_lines) + '\n', encoding='utf-8')
    for name in ('emoji-repository.list', 'emoji-trusted-sha256.tsv', 'emoji-license-notices.tsv'):
        (OUT_ROOTFS / name).write_bytes((OUT_SOURCE / name).read_bytes())
    print(f'EMOJI_MANIFEST_ROWS={len(rows)}')
    print(f'EMOJI_MANIFEST_SHA256={hashlib.sha256((OUT_SOURCE / "emoji-repository.list").read_bytes()).hexdigest()}')


if __name__ == '__main__':
    main()
