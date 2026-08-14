from pathlib import Path
import gzip
from PIL import Image

source = Path(__file__).resolve().parents[1] / "branding" / "influent-danenone-boot.png"
out = Path(__file__).resolve().parents[1] / "build" / "influent-boot.xpm.gz"
out.parent.mkdir(parents=True, exist_ok=True)
image = Image.open(source).convert("RGB").resize((640, 480), Image.Resampling.LANCZOS).quantize(colors=32)
palette = image.getpalette()[: 32 * 3]
colors = []
for index in range(32):
    red, green, blue = palette[index * 3:index * 3 + 3]
    colors.append(f"c{index:02x} c \\#{red:02x}{green:02x}{blue:02x}")
rows = []
for y in range(image.height):
    row = "".join(f"{pixel:02x}" for pixel in image.crop((0, y, image.width, y + 1)).getdata())
    rows.append(f'"{row}"')
xpm = [
    "/* XPM */",
    "static char * influent_danenone_xpm[] = {",
    f'"{image.width} {image.height} 32 2",',
    *[f'"{line}",' for line in colors],
    *[f"{row}," if index < len(rows) - 1 else f"{row}" for index, row in enumerate(rows)],
    "};",
]
raw = "\n".join(xpm).encode("ascii")
with gzip.open(out, "wb", compresslevel=9) as dst:
    dst.write(raw)
