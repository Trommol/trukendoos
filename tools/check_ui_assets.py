from __future__ import annotations

import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ASSET_DIR = ROOT / "Assets" / "ui"

EXPECTED = {
    "backplate.png": (1480, 940),
}

REQUIRED = [
    "backplate.png",
    "preset_plate.png",
    "global_bus_plate.png",
    "input_badge.png",
    "output_badge.png",
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
    "knob_global.png",
    "knob_harmonizer.png",
]


def png_size(path: Path) -> tuple[int, int]:
    with path.open("rb") as handle:
        header = handle.read(24)

    if len(header) < 24 or header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
        raise ValueError("not a PNG file")

    return struct.unpack(">II", header[16:24])


def main() -> int:
    missing = []
    failures = []

    for name in REQUIRED:
        path = ASSET_DIR / name

        if not path.exists():
            missing.append(name)
            continue

        try:
            size = png_size(path)
        except ValueError as error:
            failures.append(f"{name}: {error}")
            continue

        expected = EXPECTED.get(name)
        if expected is not None and size != expected:
            failures.append(f"{name}: expected {expected[0]}x{expected[1]}, got {size[0]}x{size[1]}")
        else:
            print(f"ok  {name}  {size[0]}x{size[1]}")

    if missing:
        print("\nMissing:")
        for name in missing:
            print(f" - {name}")

    if failures:
        print("\nProblems:")
        for failure in failures:
            print(f" - {failure}")

    return 1 if missing or failures else 0


if __name__ == "__main__":
    raise SystemExit(main())

