#!/usr/bin/env python3
"""Remove a connected near-white background while preserving internal highlights."""
from __future__ import annotations

import argparse
from collections import deque
from pathlib import Path

from PIL import Image


def near_white(pixel: tuple[int, int, int], threshold: int) -> bool:
    return min(pixel) >= threshold and max(pixel) - min(pixel) <= 18


def remove_background(source: Path, target: Path, threshold: int) -> None:
    image = Image.open(source).convert("RGBA")
    width, height = image.size
    pixels = image.load()
    background = bytearray(width * height)
    queue: deque[tuple[int, int]] = deque()

    for x in range(width):
        queue.append((x, 0))
        queue.append((x, height - 1))
    for y in range(height):
        queue.append((0, y))
        queue.append((width - 1, y))

    while queue:
        x, y = queue.popleft()
        index = y * width + x
        if background[index]:
            continue
        r, g, b, _ = pixels[x, y]
        if not near_white((r, g, b), threshold):
            continue
        background[index] = 1
        for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1), (x - 1, y - 1), (x + 1, y - 1), (x - 1, y + 1), (x + 1, y + 1)):
            if 0 <= nx < width and 0 <= ny < height:
                queue.append((nx, ny))

    for y in range(height):
        for x in range(width):
            index = y * width + x
            if background[index]:
                pixels[x, y] = (pixels[x, y][0], pixels[x, y][1], pixels[x, y][2], 0)

    bbox = image.getchannel("A").getbbox()
    if bbox:
        image = image.crop(bbox)
    target.parent.mkdir(parents=True, exist_ok=True)
    image.save(target, "PNG", optimize=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("target", type=Path)
    parser.add_argument("--threshold", type=int, default=238)
    args = parser.parse_args()
    remove_background(args.source, args.target, args.threshold)
    print(f"wrote={args.target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
