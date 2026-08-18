from pathlib import Path
import hashlib
import re
import zipfile
import xml.etree.ElementTree as ET

ROOT = Path('/home/ubuntu/danenone/build/emoji-fluthin-artifacts')
EXPECTED_VERSION = 'v0.1-26.08-22.31'
NAME_RE = re.compile(r'^Influent\.emoji-[a-z0-9-]+\.v0\.1-26\.08-22\.31-Danenone\.iflapp$')


def safe(name: str) -> bool:
    p = Path(name)
    return not p.is_absolute() and '..' not in p.parts


def main() -> int:
    artifacts = sorted(ROOT.glob('*/*.iflapp'))
    failures = []
    rows = []
    for artifact in artifacts:
        try:
            with zipfile.ZipFile(artifact) as zf:
                names = zf.namelist()
                if any(not safe(n) for n in names):
                    failures.append(f'{artifact.name}: path traversal')
                xml = ET.fromstring(zf.read('details.xml'))
                fields = {child.tag: (child.text or '') for child in xml}
                required = ('publisher', 'app', 'version', 'author', 'platform')
                for key in required:
                    if not fields.get(key):
                        failures.append(f'{artifact.name}: missing {key}')
                if fields.get('publisher') != 'Influent' or fields.get('platform') != 'Danenone' or fields.get('version') != EXPECTED_VERSION:
                    failures.append(f'{artifact.name}: metadata mismatch {fields}')
                if not NAME_RE.match(artifact.name):
                    failures.append(f'{artifact.name}: public name mismatch')
                app_id = fields.get('app', '')
                binary_candidates = [n for n in names if n in (app_id, f'bin/{app_id}', f'{app_id}.exe') or n.endswith(f'/bin/{app_id}')]
                if not binary_candidates:
                    failures.append(f'{artifact.name}: main binary absent')
                required_assets = [f'app/app-icon.ico', 'assets/profile.tsv', 'config/profile.conf', 'LICENSE']
                for required_asset in required_assets:
                    if required_asset not in names:
                        failures.append(f'{artifact.name}: missing {required_asset}')
                digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
                rows.append((artifact.name, artifact.stat().st_size, digest, app_id, binary_candidates[0] if binary_candidates else ''))
        except Exception as exc:
            failures.append(f'{artifact.name}: {exc}')
    print(f'EMOJI_ARTIFACT_COUNT={len(artifacts)}')
    for name, size, digest, app_id, binary in rows:
        print(f'{name}|{size}|{digest}|{app_id}|{binary}')
    print(f'EMOJI_VALIDATION_FAILURES={len(failures)}')
    for failure in failures:
        print(f'FAIL={failure}')
    return 1 if failures or len(artifacts) != 10 else 0


if __name__ == '__main__':
    raise SystemExit(main())
