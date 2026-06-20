from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / "Assets" / "ui" / "generated"
OUT = SOURCE_DIR / "contact_sheet.png"


def main() -> int:
    files = sorted(
        (p for p in SOURCE_DIR.glob("*.png") if p.name != OUT.name),
        key=lambda p: p.stat().st_mtime,
        reverse=True,
    )

    if not files:
        print("No generated PNGs found.")
        return 1

    thumb_size = (220, 160)
    columns = 5
    rows = (len(files) + columns - 1) // columns
    cell_w = 260
    cell_h = 205

    sheet = Image.new("RGB", (columns * cell_w, rows * cell_h), "white")
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.load_default()

    for index, path in enumerate(files, start=1):
        image = Image.open(path).convert("RGB")
        image.thumbnail(thumb_size)

        x = ((index - 1) % columns) * cell_w + 20
        y = ((index - 1) // columns) * cell_h + 20

        sheet.paste(image, (x, y))
        draw.text((x, y + 164), f"{index:02d}", fill=(0, 0, 0), font=font)
        draw.text((x + 28, y + 164), path.name[:28], fill=(0, 0, 0), font=font)

    sheet.save(OUT)
    print(OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

