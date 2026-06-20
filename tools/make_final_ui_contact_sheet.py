from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
ASSET_DIR = ROOT / "Assets" / "ui"
OUT = ASSET_DIR / "final_asset_contact_sheet.png"


def main() -> int:
    files = sorted(
        p for p in ASSET_DIR.glob("*.png")
        if p.name not in {"header_all.png", OUT.name} and "source" not in p.name
    )

    columns = 4
    cell_w = 300
    cell_h = 230
    rows = (len(files) + columns - 1) // columns

    sheet = Image.new("RGBA", (columns * cell_w, rows * cell_h), (245, 245, 235, 255))
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.load_default()

    for index, path in enumerate(files):
        image = Image.open(path).convert("RGBA")
        image.thumbnail((260, 175))

        x = (index % columns) * cell_w + 20
        y = (index // columns) * cell_h + 20

        preview_bg = Image.new("RGBA", image.size, (40, 40, 36, 255))
        preview_bg.alpha_composite(image)
        sheet.alpha_composite(preview_bg, (x, y))
        draw.text((x, y + 182), path.name, fill=(0, 0, 0, 255), font=font)

    sheet.convert("RGB").save(OUT)
    print(OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

