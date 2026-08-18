from pathlib import Path
import csv
import shutil
import textwrap

ROOT = Path('/home/ubuntu/danenone')
PROFILE = ROOT / 'native-shell/emojis/profiles.tsv'
OUT = ROOT / 'build/emoji-fluthin-projects'
ICON = Path('/home/ubuntu/settingspanel/app/app-icon.ico')
VERSION = 'v0.1-26.08-22.31'
PUBLISHER = 'Influent'
AUTHOR = 'JesusQuijada34'
PLATFORM = 'Danenone'


def xml_escape(value: str) -> str:
    return value.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;')


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    rows = []
    with PROFILE.open(encoding='utf-8') as handle:
        for row in csv.reader(handle, delimiter='|'):
            if not row or row[0].startswith('#') or len(row) < 6:
                continue
            rows.append(row[:6])
    for code, display, source, first_four, license_text, asset_status in rows:
        app_id = f'emoji-{code}'
        project = OUT / app_id
        if project.exists():
            shutil.rmtree(project)
        for directory in ('app', 'assets', 'config', 'docs', 'lib', 'source'):
            (project / directory).mkdir(parents=True, exist_ok=True)
            (project / directory / '.*-container').write_text('', encoding='utf-8')
            (project / directory / f'.{directory}-container').write_text('', encoding='utf-8')
        if ICON.exists():
            shutil.copy2(ICON, project / 'app/app-icon.ico')
        details = f'''<app>
  <publisher>{PUBLISHER}</publisher>
  <app>{app_id}</app>
  <name>{xml_escape(display)}</name>
  <version>{VERSION}</version>
  <correlationid>{app_id}-{VERSION}</correlationid>
  <rate>Todas las edades</rate>
  <author>{AUTHOR}</author>
  <platform>{PLATFORM}</platform>
  <description>Perfil de emojis {xml_escape(display)} para Influent Danenone. Solo instala metadatos y configuración; no redistribuye fuentes propietarias.</description>
  <year>2026</year>
</app>
'''
        (project / 'details.xml').write_text(details, encoding='utf-8')
        (project / 'version.res').write_text(VERSION + '\n', encoding='utf-8')
        (project / '.storedetail').write_text(f'{PUBLISHER}|{app_id}|{VERSION}|{PLATFORM}\n', encoding='utf-8')
        profile = f'''profile={code}\ndisplay_name={display}\nsource_url={source}\nfirst_four_codepoints={first_four}\nlicense={license_text}\nasset_status={asset_status}\nproprietary_windows_font=false\n'''
        (project / 'config/profile.conf').write_text(profile, encoding='utf-8')
        (project / 'assets/profile.tsv').write_text('|'.join((code, display, source, first_four, license_text, asset_status)) + '\n', encoding='utf-8')
        (project / 'LICENSE').write_text(f'Profile metadata license notice. Upstream: {source}.\n{license_text}\nNo proprietary Windows font is included.\n', encoding='utf-8')
        (project / 'README.md').write_text(textwrap.dedent(f'''\
            # {display}

            This Fluthin package installs the `{code}` emoji profile metadata for Influent Danenone.

            The package is **profile-only**: it does not contain or redistribute Segoe UI Emoji or another proprietary Windows font. Upstream reference: {source}. First four preview code points: `{first_four}`. License notice: {license_text}.
        '''), encoding='utf-8')
        (project / 'lib/requirements.txt').write_text('', encoding='utf-8')
        (project / 'requirements.txt').write_text('', encoding='utf-8')
        (project / 'autorun').write_text(f'python3 {app_id}.py\n', encoding='utf-8')
        (project / 'autorun.bat').write_text(f'{app_id}.exe\n', encoding='utf-8')
        main_script = project / f'{app_id}.py'
        main_script.write_text(textwrap.dedent(f'''\
            #!/usr/bin/env python3
            from pathlib import Path

            PROFILE = {code!r}
            DISPLAY_NAME = {display!r}
            SOURCE = {source!r}

            def main() -> int:
                print(f"{{DISPLAY_NAME}} ({{PROFILE}}) — {{SOURCE}}")
                print("Perfil de metadatos instalado; no se incluye ninguna fuente propietaria.")
                return 0

            if __name__ == "__main__":
                raise SystemExit(main())
        '''), encoding='utf-8')
        main_script.chmod(0o755)
    print(f'EMOJI_PROJECT_COUNT={len(rows)}')
    print(f'EMOJI_PROJECT_ROOT={OUT}')


if __name__ == '__main__':
    main()
