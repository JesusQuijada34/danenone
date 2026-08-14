from __future__ import annotations

import shutil
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PACKAGEMAKER = Path("/home/ubuntu/fluthin_compatible/packagemaker/packagemaker.py")
WORK = ROOT / "build" / "system-fluthin-projects"
OUT = ROOT / "build" / "system-fluthin-artifacts"

APPS = {
    "danenone-shell": "Aplicación principal del escritorio Influent Danenone.",
    "influent-updater": "Actualizador seguro de paquetes Fluthin y Debian.",
    "influent-notifications": "Servicio de notificaciones del sistema Influent.",
}


def write_project(app_id: str, description: str) -> Path:
    project = WORK / app_id
    if project.exists():
        shutil.rmtree(project)
    (project / "app").mkdir(parents=True)
    (project / "assets").mkdir()
    (project / "docs").mkdir()
    root = ET.Element("app")
    fields = {
        "publisher": "Influent",
        "app": app_id,
        "name": app_id.replace("-", " ").title(),
        "version": "0.3-26.08-21.56",
        "correlationid": f"influent-{app_id}",
        "rate": "free",
        "author": "JesusQuijada34",
        "platform": "Danenone",
        "description": description,
    }
    for key, value in fields.items():
        ET.SubElement(root, key).text = value
    ET.ElementTree(root).write(project / "details.xml", encoding="utf-8", xml_declaration=True)
    (project / "README.md").write_text(
        f"# {fields['name']}\\n\\n{description}\\n\\nEste paquete se ejecuta con Python sobre Linux dentro de Influent Danenone.\\n",
        encoding="utf-8",
    )
    launcher = f'''from pathlib import Path\nimport subprocess\nimport sys\n\nROOT = Path("/opt/influent-danenone")\nMODULES = {{\n    "danenone-shell": [sys.executable, "-m", "danenone_shell.app"],\n    "influent-updater": [sys.executable, "-m", "danenone_shell.updater"],\n    "influent-notifications": [sys.executable, "-m", "danenone_shell.notifications"],\n}}\n\nif __name__ == "__main__":\n    raise SystemExit(subprocess.call(MODULES["{app_id}"], cwd=ROOT, env={{**__import__("os").environ, "PYTHONPATH": str(ROOT / "src")}}))\n'''
    (project / "app.py").write_text(launcher, encoding="utf-8")
    (project / "app" / f"{app_id}-icon.svg").write_text(
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48"><path d="M8 8h32v32H8zM16 16h16v16H16z" fill="none" stroke="#FFFFFF" stroke-width="3"/></svg>\n',
        encoding="utf-8",
    )
    return project


def compile_project(project: Path) -> Path:
    result = subprocess.run(
        ["python3", str(PACKAGEMAKER), "--buildthis", str(project)],
        cwd=PACKAGEMAKER.parent,
        text=True,
        capture_output=True,
        timeout=900,
        check=False,
    )
    (OUT / f"{project.name}.log").write_text(result.stdout + "\n" + result.stderr, encoding="utf-8")
    if result.returncode != 0:
        raise RuntimeError(f"PackageMaker falló para {project.name}: {result.returncode}")
    candidates = sorted(Path.home().glob("Documents/Packagemaker Projects/Compiled/*.iflapp"), key=lambda p: p.stat().st_mtime, reverse=True)
    if not candidates:
        raise FileNotFoundError(f"No se produjo .iflapp para {project.name}")
    artifact = candidates[0]
    destination = OUT / artifact.name
    shutil.copy2(artifact, destination)
    return destination


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    results = []
    for app_id, description in APPS.items():
        project = write_project(app_id, description)
        artifact = compile_project(project)
        results.append({"app": app_id, "project": str(project), "artifact": str(artifact)})
    (OUT / "manifest.json").write_text(__import__("json").dumps(results, indent=2), encoding="utf-8")
    print(__import__("json").dumps(results, indent=2))


if __name__ == "__main__":
    main()
