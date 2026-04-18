"""Build {folder}_atlas.png and {folder}_atlas.plist from render/*.png (numeric stems).

Place this script in any folder that contains a sibling ``render/`` directory with
sequential frame PNGs (e.g. ``0001.png``). Frames are packed at full size (no trim).

Tunables: ``MAX_ATLAS_WIDTH`` (default 2048) if single frames exceed atlas row width.
"""

from __future__ import annotations

import plistlib
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

MAX_ATLAS_WIDTH = 2048
PACK_GUTTER_X = 1
PACK_GUTTER_Y = 0


@dataclass
class FramePackInfo:
    frame_key: str
    source_image: Image.Image
    trim_width: int
    source_width: int
    source_height: int
    pack_x: int = 0
    pack_y: int = 0


def collect_png_paths(input_dir: Path) -> list[Path]:
    indexed_paths: list[tuple[int, Path]] = []
    for path in input_dir.iterdir():
        if path.suffix.lower() != ".png":
            continue
        try:
            indexed_paths.append((int(path.stem), path))
        except ValueError:
            print(f"Skipping non-numeric frame file: {path.name}")

    return [path for _, path in sorted(indexed_paths, key=lambda item: item[0])]


def pack_tight_rows(frames: list[FramePackInfo], max_width: int) -> tuple[int, int]:
    cursor_x = 0
    cursor_y = 0
    row_height = 0
    packed_width = 0
    packed_height = 0

    for frame in frames:
        if frame.trim_width > max_width:
            raise ValueError(
                f"Frame width {frame.trim_width} exceeds max atlas width {max_width} for {frame.frame_key}."
            )
        if cursor_x > 0 and cursor_x + frame.trim_width > max_width:
            cursor_x = 0
            cursor_y += row_height + PACK_GUTTER_Y
            row_height = 0

        frame.pack_x = cursor_x
        frame.pack_y = cursor_y
        cursor_x += frame.trim_width + PACK_GUTTER_X
        row_height = max(row_height, frame.source_height)
        packed_width = max(packed_width, frame.pack_x + frame.trim_width)
        packed_height = max(packed_height, frame.pack_y + frame.source_height)

    return max(1, packed_width), max(1, packed_height)


def main() -> None:
    script_dir = Path(__file__).resolve().parent
    input_dir = script_dir / "render"
    base_name = f"{script_dir.name}_atlas"
    atlas_path = script_dir / f"{base_name}.png"
    plist_path = script_dir / f"{base_name}.plist"

    if not input_dir.is_dir():
        raise FileNotFoundError(f"Missing render directory: {input_dir}")

    png_paths = collect_png_paths(input_dir)
    if not png_paths:
        raise ValueError(f"No numeric-stem PNG files found in {input_dir}")

    frames: list[FramePackInfo] = []
    for index, png_path in enumerate(png_paths, start=1):
        frame_key = f"frame_{index:03d}.png"
        with Image.open(png_path) as tile:
            source = tile.convert("RGBA")
        w, h = source.width, source.height
        frames.append(
            FramePackInfo(
                frame_key=frame_key,
                source_image=source,
                trim_width=w,
                source_width=w,
                source_height=h,
            )
        )

    atlas_width, atlas_height = pack_tight_rows(frames, MAX_ATLAS_WIDTH)
    atlas_image = Image.new("RGBA", (atlas_width, atlas_height), (0, 0, 0, 0))
    for frame in frames:
        atlas_image.paste(frame.source_image, (frame.pack_x, frame.pack_y))

    atlas_image.save(atlas_path, format="PNG")

    plist_frames: dict[str, dict[str, object]] = {}
    frame_sequence: list[str] = []
    for frame in frames:
        plist_frames[frame.frame_key] = {
            "aliases": [],
            "spriteOffset": "{0,0}",
            "spriteSize": f"{{{frame.trim_width},{frame.source_height}}}",
            "spriteSourceSize": f"{{{frame.source_width},{frame.source_height}}}",
            "textureRect": f"{{{{{frame.pack_x},{frame.pack_y}}},{{{frame.trim_width},{frame.source_height}}}}}",
            "textureRotated": False,
        }
        frame_sequence.append(frame.frame_key)

    plist_data = {
        "frames": plist_frames,
        "metadata": {
            "format": 3,
            "pixelFormat": "RGBA8888",
            "premultiplyAlpha": False,
            "realTextureFileName": atlas_path.name,
            "textureFileName": atlas_path.name,
            "size": f"{{{atlas_width},{atlas_height}}}",
            "frameSequence": frame_sequence,
        },
    }
    with plist_path.open("wb") as plist_file:
        plistlib.dump(plist_data, plist_file, fmt=plistlib.FMT_XML, sort_keys=False)

    print(f"Atlas written: {atlas_path}")
    print(f"Plist written: {plist_path}")
    print(f"Atlas size: {atlas_width}x{atlas_height}")
    print(f"Frames packed: {len(frames)}")


if __name__ == "__main__":
    main()
