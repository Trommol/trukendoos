from __future__ import annotations

import math
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont


ROOT = Path(__file__).resolve().parents[1]
ASSET_DIR = ROOT / "Assets" / "ui"

INK = (18, 15, 12, 255)
CREAM = (245, 229, 181, 255)
CORAL = (228, 82, 64, 255)
TEAL = (42, 150, 145, 255)
YELLOW = (242, 232, 45, 255)
BRASS = (176, 143, 72, 255)
PURPLE = (86, 54, 130, 255)


def font(size: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    candidates = [
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/arial.ttf",
    ]

    for candidate in candidates:
        if Path(candidate).exists():
            return ImageFont.truetype(candidate, size)

    return ImageFont.load_default()


def rounded_mask(size: tuple[int, int], radius: int) -> Image.Image:
    mask = Image.new("L", size, 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle((0, 0, size[0] - 1, size[1] - 1), radius=radius, fill=255)
    return mask


def grunge(draw: ImageDraw.ImageDraw, bounds: tuple[int, int, int, int], seed: int, colour: tuple[int, int, int, int], count: int) -> None:
    rng = random.Random(seed)
    x0, y0, x1, y1 = bounds
    for _ in range(count):
        x = rng.randint(x0, x1)
        y = rng.randint(y0, y1)
        length = rng.randint(4, 34)
        angle = rng.random() * math.tau
        x2 = x + int(math.cos(angle) * length)
        y2 = y + int(math.sin(angle) * length)
        draw.line((x, y, x2, y2), fill=colour, width=rng.choice([1, 1, 2]))


def screw(draw: ImageDraw.ImageDraw, cx: int, cy: int, r: int) -> None:
    draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(34, 31, 28, 255), outline=(7, 6, 5, 255), width=3)
    draw.ellipse((cx - r + 4, cy - r + 4, cx + r - 4, cy + r - 4), outline=(180, 170, 150, 180), width=2)
    draw.line((cx - r + 6, cy, cx + r - 6, cy), fill=(7, 6, 5, 255), width=4)
    draw.line((cx, cy - r + 6, cx, cy + r - 6), fill=(7, 6, 5, 255), width=4)


def save_plate(name: str, size: tuple[int, int], fill: tuple[int, int, int, int], outline: tuple[int, int, int, int], seed: int) -> None:
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    plate = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(plate)
    bounds = (8, 8, size[0] - 9, size[1] - 9)
    d.rounded_rectangle(bounds, radius=22, fill=fill, outline=INK, width=5)
    d.rounded_rectangle((18, 18, size[0] - 19, size[1] - 19), radius=16, outline=outline, width=3)
    grunge(d, bounds, seed, (20, 14, 10, 68), 160)
    grunge(d, bounds, seed + 1, (255, 245, 205, 55), 90)
    screw(d, 32, 32, 11)
    screw(d, size[0] - 32, 32, 11)
    screw(d, 32, size[1] - 32, 11)
    screw(d, size[0] - 32, size[1] - 32, 11)
    img.alpha_composite(plate)
    img.save(ASSET_DIR / name)


def preset_plate() -> None:
    size = (460, 82)
    save_plate("preset_plate.png", size, YELLOW, CORAL, 21)
    img = Image.open(ASSET_DIR / "preset_plate.png").convert("RGBA")
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((82, 22, 326, 60), radius=8, fill=(17, 15, 12, 210), outline=INK, width=2)
    d.rounded_rectangle((338, 22, 418, 60), radius=8, fill=(17, 15, 12, 210), outline=INK, width=2)
    d.polygon([(42, 41), (62, 25), (62, 57)], fill=INK)
    d.polygon([(72, 41), (52, 25), (52, 57)], outline=INK)
    img.save(ASSET_DIR / "preset_plate.png")


def global_bus_plate() -> None:
    size = (1380, 120)
    save_plate("global_bus_plate.png", size, (13, 32, 30, 255), TEAL, 34)
    img = Image.open(ASSET_DIR / "global_bus_plate.png").convert("RGBA")
    d = ImageDraw.Draw(img)
    for x in (180, 420, 680, 920, 1160):
        d.ellipse((x - 28, 48 - 28, x + 28, 48 + 28), outline=(245, 229, 181, 120), width=2)
    for x in (60, 280, 760, 1040):
        d.line((x, 84, x + 180, 84), fill=(245, 229, 181, 90), width=4)
        d.ellipse((x - 6, 78, x + 6, 90), fill=CREAM)
    img.save(ASSET_DIR / "global_bus_plate.png")


def badge(name: str, text: str, accent: tuple[int, int, int, int], seed: int) -> None:
    size = (150, 150)
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.ellipse((8, 8, 142, 142), fill=YELLOW, outline=INK, width=5)
    d.ellipse((23, 23, 127, 127), outline=accent, width=5)
    d.ellipse((48, 48, 102, 102), fill=(20, 16, 13, 130), outline=INK, width=2)
    grunge(d, (15, 15, 135, 135), seed, (20, 14, 10, 75), 55)
    label_font = font(21)
    box = d.textbbox((0, 0), text, font=label_font)
    d.text(((size[0] - (box[2] - box[0])) / 2, 18), text, fill=INK, font=label_font)
    img.save(ASSET_DIR / name)


def knob(name: str, fill: tuple[int, int, int, int], rim: tuple[int, int, int, int], seed: int) -> None:
    size = (220, 220)
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx = cy = size[0] // 2
    d.ellipse((25, 27, 195, 197), fill=(0, 0, 0, 90))
    d.ellipse((20, 20, 200, 200), fill=rim, outline=INK, width=6)
    d.ellipse((43, 43, 177, 177), fill=fill, outline=(255, 245, 210, 120), width=2)
    for i in range(18):
        a = i / 18 * math.tau
        x0 = cx + math.cos(a) * 74
        y0 = cy + math.sin(a) * 74
        x1 = cx + math.cos(a) * 88
        y1 = cy + math.sin(a) * 88
        d.line((x0, y0, x1, y1), fill=INK, width=2)
    d.line((cx, 44, cx, 82), fill=CREAM, width=7)
    grunge(d, (32, 32, 188, 188), seed, (20, 14, 10, 80), 70)
    grunge(d, (40, 40, 180, 180), seed + 1, (255, 245, 210, 50), 35)
    img = img.filter(ImageFilter.UnsharpMask(radius=1.2, percent=120, threshold=3))
    img.save(ASSET_DIR / name)


def main() -> int:
    ASSET_DIR.mkdir(parents=True, exist_ok=True)
    preset_plate()
    global_bus_plate()
    badge("input_badge.png", "INPUT", TEAL, 45)
    badge("output_badge.png", "OUTPUT", PURPLE, 46)
    knob("knob_global.png", (226, 197, 145, 255), (36, 32, 28, 255), 57)
    knob("knob_harmonizer.png", (186, 184, 181, 255), (38, 32, 78, 255), 58)

    for name in (
        "preset_plate.png",
        "global_bus_plate.png",
        "input_badge.png",
        "output_badge.png",
        "knob_global.png",
        "knob_harmonizer.png",
    ):
        print(ASSET_DIR / name)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

