#!/usr/bin/env python3
"""Build Influent Danenone language data packages from Unicode CLDR names.

The packages are intentionally data-only: they install a locale metadata file and
an initial UI catalog. Languages without a complete Danenone translation carry an
explicit English fallback marker instead of pretending to be fully translated.
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

TRANSLATIONS = {
    "en": {"start": "Start", "settings": "Settings", "continue": "Continue", "cancel": "Cancel", "language": "Language"},
    "es": {"start": "Inicio", "settings": "Configuración", "continue": "Continuar", "cancel": "Cancelar", "language": "Idioma"},
    "fr": {"start": "Démarrer", "settings": "Réglages", "continue": "Continuer", "cancel": "Annuler", "language": "Langue"},
    "de": {"start": "Start", "settings": "Einstellungen", "continue": "Weiter", "cancel": "Abbrechen", "language": "Sprache"},
    "pt": {"start": "Iniciar", "settings": "Definições", "continue": "Continuar", "cancel": "Cancelar", "language": "Idioma"},
    "ru": {"start": "Пуск", "settings": "Настройки", "continue": "Продолжить", "cancel": "Отмена", "language": "Язык"},
    "pl": {"start": "Start", "settings": "Ustawienia", "continue": "Dalej", "cancel": "Anuluj", "language": "Język"},
    "sl": {"start": "Začetek", "settings": "Nastavitve", "continue": "Nadaljuj", "cancel": "Prekliči", "language": "Jezik"},
    "zh": {"start": "开始", "settings": "设置", "continue": "继续", "cancel": "取消", "language": "语言"},
    "ja": {"start": "スタート", "settings": "設定", "continue": "続行", "cancel": "キャンセル", "language": "言語"},
    "ko": {"start": "시작", "settings": "설정", "continue": "계속", "cancel": "취소", "language": "언어"},
    "ar": {"start": "ابدأ", "settings": "الإعدادات", "continue": "متابعة", "cancel": "إلغاء", "language": "اللغة"},
    "hi": {"start": "प्रारंभ", "settings": "सेटिंग्स", "continue": "जारी रखें", "cancel": "रद्द करें", "language": "भाषा"},
}

FORCED = {"en", "es", "fr", "de", "zh", "pl", "sl", "ru", "pt", "ja", "ko", "ar", "hi"}
EXCLUDED = {"root", "und", "mul", "zxx", "mis", "qaa", "qab"}


def load_languages(path: Path, minimum: int) -> list[tuple[str, str]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    names = data["main"]["en"]["localeDisplayNames"]["languages"]
    rows: dict[str, str] = {}
    for code, name in names.items():
        code = code.lower()
        if code in EXCLUDED or not re.fullmatch(r"[a-z]{2,3}", code):
            continue
        if re.search(r"\b(?:variant|private use|unknown)\b", name, re.I):
            continue
        rows[code] = name
    for code in FORCED:
        rows.setdefault(code, {"zh": "Chinese", "pl": "Polish", "sl": "Slovenian", "ru": "Russian", "pt": "Portuguese"}.get(code, code.upper()))
    ordered = sorted(rows.items(), key=lambda item: (item[1].casefold(), item[0]))
    if len(ordered) < minimum:
        raise RuntimeError(f"CLDR catalog contains only {len(ordered)} usable languages; expected at least {minimum}")
    return ordered


def deb_control(code: str, name: str, version: str) -> str:
    return (f"Package: influent-language-{code}\n"
            f"Version: {version}\n"
            "Section: localization\n"
            "Priority: optional\n"
            "Architecture: all\n"
            "Maintainer: Influent Danenone <danenone@users.noreply.github.com>\n"
            f"Description: Influent Danenone language data ({name})\n"
            " Data-only locale metadata and the Danenone UI catalog.\n"
            " Languages without a complete catalog explicitly fall back to English.\n")


def build_one(out_dir: Path, code: str, name: str, version: str) -> Path:
    with tempfile.TemporaryDirectory(prefix=f"danenone-{code}-") as tmp_name:
        root = Path(tmp_name)
        control_dir = root / "DEBIAN"
        data_dir = root / "usr/share/influent-danenone/locales"
        control_dir.mkdir(parents=True)
        data_dir.mkdir(parents=True)
        (control_dir / "control").write_text(deb_control(code, name, version), encoding="utf-8")
        catalog = {
            "locale": code,
            "display_name": name,
            "translation_status": "initial-catalog" if code in TRANSLATIONS else "english-fallback",
            "fallback": "en" if code not in TRANSLATIONS else None,
            "strings": TRANSLATIONS.get(code, {}),
        }
        (data_dir / f"{code}.json").write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        package_path = out_dir / f"influent-language-{code}_{version}_all.deb"
        subprocess.run(["dpkg-deb", "--build", "--root-owner-group", str(root), str(package_path)], check=True, stdout=subprocess.DEVNULL)
    return package_path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cldr", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--version", default="0.1.0")
    parser.add_argument("--minimum", type=int, default=200)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    for old in args.out.glob("influent-language-*.deb"):
        old.unlink()
    languages = load_languages(args.cldr, args.minimum)
    manifest = ["# code|package|url|sha256|display_name"]
    for code, name in languages:
        package_path = build_one(args.out, code, name, args.version)
        digest = subprocess.check_output(["sha256sum", str(package_path)], text=True).split()[0]
        manifest.append(f"{code}|{package_path.name}|ASSET_URL|{digest}|{name}")
    (args.out / "languages.manifest.template.tsv").write_text("\n".join(manifest) + "\n", encoding="utf-8")
    (args.out / "catalog-summary.json").write_text(json.dumps({"version": args.version, "count": len(languages), "languages": [{"code": c, "name": n} for c, n in languages]}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"built={len(languages)}")
    print(f"translated_initial_catalogs={sum(code in TRANSLATIONS for code, _ in languages)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
