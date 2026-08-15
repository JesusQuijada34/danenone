#!/usr/bin/env python3
"""Generate the Danenone Syslinux background from the desktop wallpaper."""
from __future__ import annotations

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
WALLPAPER = ROOT / "archiso-profile/airootfs/usr/share/backgrounds/influent/danenone-river-wallpaper.jpg"
if not WALLPAPER.exists():
    WALLPAPER = ROOT / "branding-v2/danenone-river-wallpaper.jpg"
LOGO = ROOT / "native-shell/assets/danenone-cube/danenone-cube-logo.png"
OUTPUT = ROOT / "archiso-profile/syslinux/splash.png"
WIDTH, HEIGHT = 640, 480

source = Image.open(WALLPAPER).convert("RGB")
scale = max(WIDTH / source.width, HEIGHT / source.height)
resized = source.resize((round(source.width * scale), round(source.height * scale)), Image.Resampling.LANCZOS)
left = (resized.width - WIDTH) // 2
top = (resized.height - HEIGHT) // 2
image = resized.crop((left, top, left + WIDTH, top + HEIGHT)).convert("RGBA")

# Preserve the exact desktop image while adding only a readability veil for the boot menu.
overlay = Image.new("RGBA", (WIDTH, HEIGHT), (4, 10, 18, 100))
image = Image.alpha_composite(image, overlay)

logo = Image.open(LOGO).convert("RGBA")
logo.thumbnail((170, 170), Image.Resampling.LANCZOS)
logo_layer = Image.new("RGBA", image.size, (0, 0, 0, 0))
glow = Image.new("RGBA", image.size, (0, 0, 0, 0))
glow_draw = ImageDraw.Draw(glow)
glow_draw.ellipse((35, 29, 220, 214), fill=(32, 144, 255, 72))
glow = glow.filter(ImageFilter.GaussianBlur(28))
logo_layer.alpha_composite(glow)
logo_layer.alpha_composite(logo, (58, 45))
image = Image.alpha_composite(image, logo_layer)
draw = ImageDraw.Draw(image)
font_dir = Path("/usr/share/fonts/truetype/dejavu")
title_font = ImageFont.truetype(font_dir / "DejaVuSans-Bold.ttf", 29)
subtitle_font = ImageFont.truetype(font_dir / "DejaVuSans.ttf", 14)
small_font = ImageFont.truetype(font_dir / "DejaVuSans.ttf", 12)

draw.rounded_rectangle((267, 116, 602, 292), radius=20, fill=(5, 12, 24, 178), outline=(224, 240, 255, 76), width=1)
draw.text((292, 140), "Influent Danenone", font=title_font, fill=(248, 251, 255, 255))
draw.text((294, 183), "El mismo paisaje te acompaña desde el arranque", font=subtitle_font, fill=(212, 227, 245, 255))
draw.text((294, 220), "Preparando el entorno...", font=small_font, fill=(180, 201, 226, 255))
bar_x, bar_y, bar_w, bar_h = 294, 251, 257, 7
draw.rounded_rectangle((bar_x, bar_y, bar_x + bar_w, bar_y + bar_h), radius=4, fill=(25, 42, 67, 225))
draw.rounded_rectangle((bar_x, bar_y, bar_x + 124, bar_y + bar_h), radius=4, fill=(105, 190, 255, 255))
draw.text((294, 271), "Danenone boot environment", font=small_font, fill=(190, 211, 238, 255))

OUTPUT.parent.mkdir(parents=True, exist_ok=True)
image.convert("RGB").save(OUTPUT, format="PNG", optimize=True)
print(f"generated {OUTPUT} from {WALLPAPER} and {LOGO}")
