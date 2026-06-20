from __future__ import annotations

from collections import deque
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
ASSET_DIR = ROOT / "Assets" / "ui"
SOURCE = ASSET_DIR / "header_all.png"
OUTPUTS = [
    ASSET_DIR / "header_melt.png",
    ASSET_DIR / "header_rust.png",
    ASSET_DIR / "header_glass.png",
]


def is_key(pixel: tuple[int, int, int, int]) -> bool:
    r, g, b, _ = pixel
    return g > 170 and r < 90 and b < 90


def find_components(mask: list[list[bool]]) -> list[tuple[int, int, int, int, int]]:
    height = len(mask)
    width = len(mask[0])
    seen = [[False] * width for _ in range(height)]
    components: list[tuple[int, int, int, int, int]] = []

    for y in range(height):
        for x in range(width):
            if seen[y][x] or not mask[y][x]:
                continue

            queue = deque([(x, y)])
            seen[y][x] = True
            min_x = max_x = x
            min_y = max_y = y
            count = 0

            while queue:
                cx, cy = queue.popleft()
                count += 1
                min_x = min(min_x, cx)
                max_x = max(max_x, cx)
                min_y = min(min_y, cy)
                max_y = max(max_y, cy)

                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < width and 0 <= ny < height and not seen[ny][nx] and mask[ny][nx]:
                        seen[ny][nx] = True
                        queue.append((nx, ny))

            if count > 10_000:
                components.append((min_x, min_y, max_x + 1, max_y + 1, count))

    return components


def remove_green_key(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    pixels = rgba.load()
    width, height = rgba.size

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            if g > 150 and r < 120 and b < 120:
                pixels[x, y] = (r, g, b, 0)
            elif g > r * 1.5 and g > b * 1.5 and g > 120:
                # Soft despill for antialiasing around the metal edge.
                alpha = max(0, min(255, int((max(r, b) / max(g, 1)) * 255)))
                pixels[x, y] = (r, min(g, max(r, b)), b, alpha)

    return rgba


def main() -> int:
    source = Image.open(SOURCE).convert("RGBA")
    width, height = source.size
    data = source.load()

    mask = [[not is_key(data[x, y]) for x in range(width)] for y in range(height)]
    components = sorted(find_components(mask), key=lambda item: item[1])

    if len(components) < 3:
        print(f"Expected at least 3 components, found {len(components)}")
        return 1

    # Keep the three largest vertically sorted plates.
    plates = sorted(components, key=lambda item: item[4], reverse=True)[:3]
    plates = sorted(plates, key=lambda item: item[1])

    for bounds, output in zip(plates, OUTPUTS):
        x0, y0, x1, y1, _ = bounds
        pad = 8
        crop = source.crop((
            max(0, x0 - pad),
            max(0, y0 - pad),
            min(width, x1 + pad),
            min(height, y1 + pad),
        ))
        remove_green_key(crop).save(output)
        print(output)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

