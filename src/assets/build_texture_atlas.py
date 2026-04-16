"""Build glyph_atlas.png and glyph_atlas.plist from render/*.png."""

from __future__ import annotations

import plistlib
import re
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

SYMBOLS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_{|}~`"
FRAME_SIZE = 256
COLUMNS = 8
MAX_ATLAS_WIDTH = FRAME_SIZE * COLUMNS
PACK_GUTTER_X = 1
PACK_GUTTER_Y = 0
KEY_NAME_OVERRIDES = {
    "\\": "backslash",
    '"': "double_quote",
    "'": "single_quote",
    "`": "backtick",
}


@dataclass
class GlyphPackInfo:
    symbol: str
    frame_key: str
    source_image: Image.Image
    left_trim: int
    right_trim: int
    trim_width: int
    source_width: int
    source_height: int
    pack_x: int = 0
    pack_y: int = 0


def make_frame_key(index: int, symbol: str) -> str:
    symbol_label = KEY_NAME_OVERRIDES.get(symbol, symbol)
    cleaned = re.sub(r"[^0-9A-Za-z_]+", "", symbol_label) or f"u{ord(symbol):04X}"
    return f"glyph_{index:03d}_{cleaned}.png"


def format_offset(value: float) -> str:
    rounded = round(value)
    if abs(value - rounded) < 1e-9:
        return str(int(rounded))
    return f"{value:.2f}".rstrip("0").rstrip(".")


def compute_horizontal_trim(image: Image.Image) -> tuple[int, int, int]:
    alpha = image.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        print("Warning: fully transparent glyph, keeping 1px placeholder width.")
        return 0, image.width - 1, 1

    left, _, right_exclusive, _ = bbox
    trim_width = max(1, right_exclusive - left)
    right = left + trim_width - 1
    return left, right, trim_width


def collect_png_paths(input_dir: Path) -> list[Path]:
    indexed_paths: list[tuple[int, Path]] = []
    for path in input_dir.iterdir():
        if path.suffix.lower() != ".png":
            continue
        try:
            indexed_paths.append((int(path.stem), path))
        except ValueError:
            print(f"Skipping non-numeric glyph file: {path.name}")

    return [path for _, path in sorted(indexed_paths, key=lambda item: item[0])]


def pack_tight_rows(glyphs: list[GlyphPackInfo], max_width: int) -> tuple[int, int]:
    cursor_x = 0
    cursor_y = 0
    row_height = 0
    packed_width = 0
    packed_height = 0

    for glyph in glyphs:
        if glyph.trim_width > max_width:
            raise ValueError(
                f"Trimmed glyph width {glyph.trim_width} exceeds max atlas width {max_width} for {glyph.frame_key}."
            )
        if cursor_x > 0 and cursor_x + glyph.trim_width > max_width:
            cursor_x = 0
            cursor_y += row_height + PACK_GUTTER_Y
            row_height = 0

        glyph.pack_x = cursor_x
        glyph.pack_y = cursor_y
        cursor_x += glyph.trim_width + PACK_GUTTER_X
        row_height = max(row_height, glyph.source_height)
        packed_width = max(packed_width, glyph.pack_x + glyph.trim_width)
        packed_height = max(packed_height, glyph.pack_y + glyph.source_height)

    return max(1, packed_width), max(1, packed_height)


def main() -> None:
    script_dir = Path(__file__).resolve().parent
    input_dir = script_dir / "render"
    atlas_path = script_dir / "glyph_atlas.png"
    plist_path = script_dir / "glyph_atlas.plist"

    png_paths = collect_png_paths(input_dir)
    if len(png_paths) != len(SYMBOLS):
        raise ValueError(
            f"Glyph count mismatch: found {len(png_paths)} PNGs but expected {len(SYMBOLS)} for symbol map."
        )
    expected_indices = list(range(1, len(SYMBOLS) + 1))
    actual_indices = [int(path.stem) for path in png_paths]
    if actual_indices != expected_indices:
        raise ValueError(
            "Glyph filenames must map one-to-one with symbols using numeric stems 1..N "
            f"(expected {expected_indices[:3]}...{expected_indices[-3:]}, got "
            f"{actual_indices[:3]}...{actual_indices[-3:]})."
        )

    glyphs: list[GlyphPackInfo] = []
    for index, (png_path, symbol) in enumerate(zip(png_paths, SYMBOLS), start=1):
        with Image.open(png_path) as tile:
            source = tile.convert("RGBA")
        left_trim, right_bound, trim_width = compute_horizontal_trim(source)
        right_trim = source.width - (right_bound + 1)
        cropped = source.crop((left_trim, 0, left_trim + trim_width, source.height))
        glyphs.append(
            GlyphPackInfo(
                symbol=symbol,
                frame_key=make_frame_key(index, symbol),
                source_image=cropped,
                left_trim=left_trim,
                right_trim=right_trim,
                trim_width=trim_width,
                source_width=source.width,
                source_height=source.height,
            )
        )

    atlas_width, atlas_height = pack_tight_rows(glyphs, MAX_ATLAS_WIDTH)
    atlas_image = Image.new("RGBA", (atlas_width, atlas_height), (0, 0, 0, 0))
    for glyph in glyphs:
        atlas_image.paste(glyph.source_image, (glyph.pack_x, glyph.pack_y))

    atlas_image.save(atlas_path, format="PNG")

    frames: dict[str, dict[str, object]] = {}
    symbol_to_frame: dict[str, str] = {}
    for glyph in glyphs:
        offset_x = (glyph.left_trim - glyph.right_trim) / 2
        frames[glyph.frame_key] = {
            "aliases": [],
            "spriteOffset": f"{{{format_offset(offset_x)},0}}",
            "spriteSize": f"{{{glyph.trim_width},{glyph.source_height}}}",
            "spriteSourceSize": f"{{{glyph.source_width},{glyph.source_height}}}",
            "textureRect": f"{{{{{glyph.pack_x},{glyph.pack_y}}},{{{glyph.trim_width},{glyph.source_height}}}}}",
            "textureRotated": False,
        }
        symbol_to_frame[glyph.symbol] = glyph.frame_key

    plist_data = {
        "frames": frames,
        "metadata": {
            "format": 3,
            "pixelFormat": "RGBA8888",
            "premultiplyAlpha": False,
            "realTextureFileName": atlas_path.name,
            "textureFileName": atlas_path.name,
            "size": f"{{{atlas_width},{atlas_height}}}",
            "symbolFrameKeys": symbol_to_frame,
        },
    }
    with plist_path.open("wb") as plist_file:
        plistlib.dump(plist_data, plist_file, fmt=plistlib.FMT_XML, sort_keys=False)

    print(f"Atlas written: {atlas_path}")
    print(f"Plist written: {plist_path}")
    print(f"Atlas size: {atlas_width}x{atlas_height}")
    print(f"Frames packed: {len(glyphs)}")


if __name__ == "__main__":
    main()
