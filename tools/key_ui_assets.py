from __future__ import annotations

import shutil
from collections import deque
from datetime import datetime
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
ASSET_DIR = ROOT / "Assets" / "ui"
BACKUP_DIR = ASSET_DIR / "source_chroma"

TARGETS = [
    "header_melt.png",
    "header_rust.png",
    "header_glass.png",
    "pedal_rust.png",
    "pedal_melt.png",
    "pedal_glass.png",
    "pedal_harmonizer.png",
    "knob_melt.png",
    "knob_rust.png",
    "knob_glass.png",
    "preset_plate.png",
    "global_bus_plate.png",
    "input_badge.png",
    "output_badge.png",
    "knob_global.png",
    "knob_harmonizer.png",
]

EDGE_BLACK_TARGETS = {
    "pedal_rust.png",
    "pedal_melt.png",
    "pedal_glass.png",
}

OPAQUE_TARGETS = {
    "knob_melt.png",
    "knob_rust.png",
    "knob_glass.png",
    "knob_global.png",
    "knob_harmonizer.png",
}


def is_green_screen(r: int, g: int, b: int) -> bool:
    return g > 135 and g > r * 1.28 and g > b * 1.28


def is_edge_black(r: int, g: int, b: int) -> bool:
    return r < 34 and g < 34 and b < 34


def key_alpha(path: Path) -> None:
    image = Image.open(path).convert("RGBA")
    pixels = image.load()
    width, height = image.size

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            max_rb = max(r, b)

            if is_green_screen(r, g, b):
                # Strong green gets removed; edge pixels receive a soft alpha.
                alpha = 0 if g - max_rb > 80 else max(0, min(180, int((g - max_rb) * 2.0)))
                pixels[x, y] = (r, min(max_rb, g), b, 255 - alpha if alpha else 0)

            if g > 190 and r < 120 and b < 120:
                pixels[x, y] = (r, g, b, 0)

    image.save(path)


def key_edge_black(path: Path) -> None:
    image = Image.open(path).convert("RGBA")
    pixels = image.load()
    width, height = image.size

    queue: deque[tuple[int, int]] = deque()
    visited = set()

    for x in range(width):
        queue.append((x, 0))
        queue.append((x, height - 1))
    for y in range(height):
        queue.append((0, y))
        queue.append((width - 1, y))

    while queue:
        x, y = queue.popleft()
        if (x, y) in visited or x < 0 or y < 0 or x >= width or y >= height:
            continue

        visited.add((x, y))
        r, g, b, a = pixels[x, y]
        if a == 0 or is_edge_black(r, g, b):
            pixels[x, y] = (r, g, b, 0)
            queue.append((x + 1, y))
            queue.append((x - 1, y))
            queue.append((x, y + 1))
            queue.append((x, y - 1))

    image.save(path)


def make_visible_pixels_opaque(path: Path) -> None:
    image = Image.open(path).convert("RGBA")
    pixels = image.load()
    width, height = image.size

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            if a > 12:
                pixels[x, y] = (r, g, b, 255)

    image.save(path)


def main() -> int:
    run_backup_dir = BACKUP_DIR / datetime.now().strftime("%Y%m%d-%H%M%S")
    run_backup_dir.mkdir(parents=True, exist_ok=True)

    for name in TARGETS:
        path = ASSET_DIR / name
        if not path.exists():
            print(f"skip missing {name}")
            continue

        backup = run_backup_dir / name
        shutil.copy2(path, backup)

        key_alpha(path)
        if name in EDGE_BLACK_TARGETS:
            key_edge_black(path)
        if name in OPAQUE_TARGETS:
            make_visible_pixels_opaque(path)

        print(path)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
