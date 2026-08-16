#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageEnhance, ImageFilter, ImageOps

ROOT = Path(__file__).resolve().parents[1]
source = ROOT / "branding-v2" / "danenone-river-wallpaper.jpg"
targets = [
    ROOT / "native-shell" / "assets" / "oobe-river-blurred.jpg",
    ROOT / "archiso-profile" / "airootfs" / "usr" / "share" / "backgrounds" / "influent" / "oobe-river-blurred.jpg",
]

image = Image.open(source).convert("RGB")
canvas = ImageOps.fit(image, (1600, 1000), method=Image.Resampling.LANCZOS, centering=(0.5, 0.48))
blurred = canvas.filter(ImageFilter.GaussianBlur(radius=17))
blurred = ImageEnhance.Color(blurred).enhance(0.92)
blue_wash = Image.new("RGB", blurred.size, (16, 34, 58))
blurred = Image.blend(blurred, blue_wash, 0.18)
for target in targets:
    target.parent.mkdir(parents=True, exist_ok=True)
    blurred.save(target, "JPEG", quality=92, optimize=True, progressive=True)
print("created", ", ".join(str(target) for target in targets))
