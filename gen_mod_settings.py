"""Generate mod.json settings block and src/ModSettings.cpp from src/ModTuning.h constexpr values."""

import argparse
import json
import os
import pathlib
import re
import sys

# Compile-time-required constants: stay constexpr, excluded from mod.json and binder
SKIP_SYMBOLS = {
    "kFixedPhysicsDt",  # static_assert + kPhysicsAccumulatorCap
    "kMaxPhysicsSubsteps",  # static_assert + kPhysicsAccumulatorCap
    "kMaxSimulationFrameDt",  # static_assert
    "kImpactFlashTotalSeconds",  # kStarBurstMaxPhaseIndex
    "kImpactFlashPhaseSeconds",  # kStarBurstMaxPhaseIndex
    "kStarBurstSpriteSlots",  # fixed star burst slot count
    "kDebugLabelZOrder",  # kDebugLabelBackgroundZOrder
    "kDebugLabelUpdateHz",  # kDebugLabelUpdateInterval
    "kMenuShardRows",  # kMenuShardTrianglesTotal (std::array size)
    "kMenuShardCols",  # kMenuShardTrianglesTotal (std::array size)
    "kClickWindowSec",  # kDoubleClickScheduledDelaySec
    "kDoubleTapMinCommitDelaySec",  # kDoubleClickScheduledDelaySec
    "kB2MaxPolygonVertices",  # Body struct layout
}

# Key naming


def symbol_to_key(name: str, ns_prefix: str = "") -> str:
    """Convert C++ symbol like kPlayerFriction → player-friction."""
    if name.startswith("k") and len(name) > 1 and name[1].isupper():
        name = name[1:]
    result = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1-\2", name)
    result = re.sub(r"([a-z\d])([A-Z])", r"\1-\2", result)
    result = re.sub(r"(\d)([A-Za-z])", r"\1-\2", result)
    result = result.lower().replace("_", "-")
    result = re.sub(r"-+", "-", result).strip("-")
    if ns_prefix:
        result = ns_prefix + "-" + result
    return result


def key_to_name(key: str) -> str:
    """player-friction → Player Friction"""
    return " ".join(w.capitalize() for w in key.split("-"))


# Literal purity detection


def is_pure_literal(rhs: str) -> bool:
    s = rhs.strip()
    if s in ("true", "false"):
        return True
    s_norm = re.sub(r"(?<=[0-9a-fA-F])([fFlLuU]+)(?=[^a-zA-Z0-9_]|$)", "", s)
    s_erased = re.sub(r"0[xX][0-9a-fA-F]+", " ", s_norm)
    s_erased = re.sub(r"(?<![a-zA-Z0-9_])\d+\.?\d*[eE][+-]?\d+", " ", s_erased)
    s_erased = re.sub(r"(?<![a-zA-Z0-9_])\.\d+", " ", s_erased)
    s_erased = re.sub(r"(?<![a-zA-Z0-9_])\d+\.?\d*", " ", s_erased)
    for tok in re.findall(r"[a-zA-Z_][a-zA-Z0-9_]*", s_erased):
        if tok not in ("true", "false"):
            return False
    return True


def parse_literal(rhs: str, cpp_type: str):
    """Parse pure-literal RHS to Python value."""
    s = rhs.strip()
    if s == "true":
        return True
    if s == "false":
        return False
    s = re.sub(r"(?<=[\d.eE])([fFlLuU]+)(?=[^a-zA-Z]|$)", "", s)
    try:
        val = eval(s)  # noqa: S307
        if cpp_type in ("float", "double"):
            return float(val)
        return int(val)
    except Exception:
        return None


# Header parser

CONSTEXPR_RE = re.compile(
    r"^\s*(?:constexpr|inline)\s+(bool|int|float|double)\s+(\w+)\s*=\s*(.+?)\s*;"
)
SECTION_RE = re.compile(r"^//\s+([A-Z][^/\n]{0,100}?)\s*$")
NAMESPACE_OPEN_RE = re.compile(r"^\s*namespace\s+(\w+)\s*\{")
NAMESPACE_CLOSE_RE = re.compile(r"^\s*\}\s*(?://|$)")
STATIC_ASSERT_RE = re.compile(r"^\s*static_assert\b")


def parse_header(path: pathlib.Path):
    """
    Yield entries in source order:
      {'kind': 'title',   'name': str}
      {'kind': 'setting', 'key': str, 'type': str, 'cpp_type': str,
       'default': value, 'symbol': str, 'namespace': str|None}
      {'kind': 'skip',    'symbol': str, 'reason': str}
    Raises ValueError on key collision.
    """
    namespace_stack: list[str] = []
    current_section: str | None = None
    section_emitted = False
    seen_keys: dict[str, str] = {}
    prev_blank = True

    lines = path.read_text(encoding="utf-8").splitlines()
    for line in lines:
        is_blank = not line.strip()

        m = NAMESPACE_OPEN_RE.match(line)
        if m:
            namespace_stack.append(m.group(1))
            prev_blank = False
            continue

        if NAMESPACE_CLOSE_RE.match(line) and namespace_stack:
            if "namespace" in line:
                namespace_stack.pop()
            prev_blank = False
            continue

        if STATIC_ASSERT_RE.match(line):
            prev_blank = False
            continue

        if is_blank:
            prev_blank = True
            continue

        m = SECTION_RE.match(line)
        if m and not CONSTEXPR_RE.match(line) and prev_blank:
            current_section = m.group(1).strip()
            section_emitted = False
            prev_blank = False
            continue

        prev_blank = False

        m = CONSTEXPR_RE.match(line)
        if not m:
            continue

        cpp_type, symbol, rhs_raw = m.group(1), m.group(2), m.group(3)

        if symbol in SKIP_SYMBOLS:
            yield {
                "kind": "skip",
                "symbol": symbol,
                "reason": "pinned compile-time constant",
            }
            continue

        description: str | None = None
        rhs = rhs_raw
        ci = rhs.find("//")
        if ci >= 0:
            description = rhs[ci + 2 :].strip() or None
            rhs = rhs[:ci].strip()

        if not is_pure_literal(rhs):
            yield {"kind": "skip", "symbol": symbol, "reason": f"computed: {rhs}"}
            continue

        value = parse_literal(rhs, cpp_type)
        if value is None:
            yield {"kind": "skip", "symbol": symbol, "reason": f"parse failed: {rhs}"}
            continue

        ns_prefix = namespace_stack[-1].replace("_", "-") if namespace_stack else ""
        key = symbol_to_key(symbol, ns_prefix)

        if key in seen_keys:
            raise ValueError(
                f"Key collision: '{key}' from '{symbol}' and '{seen_keys[key]}'"
            )
        seen_keys[key] = symbol

        if current_section and not section_emitted:
            yield {"kind": "title", "name": current_section}
            section_emitted = True

        entry: dict = {
            "kind": "setting",
            "key": key,
            "type": "float" if cpp_type == "double" else cpp_type,
            "cpp_type": cpp_type,
            "default": value,
            "symbol": symbol,
            "namespace": namespace_stack[-1] if namespace_stack else None,
        }
        if description:
            entry["description"] = description
        yield entry


# Build settings dict and binder entries


def build_settings(header: pathlib.Path) -> tuple[dict, list, list]:
    """Returns (settings_dict, skipped_list, binder_entries)."""
    settings: dict = {}
    skipped: list = []
    binder_entries: list = []
    title_counter: dict[str, int] = {}
    pending_title: dict | None = None

    for entry in parse_header(header):
        kind = entry["kind"]

        if kind == "skip":
            skipped.append(entry)

        elif kind == "title":
            slug = re.sub(r"[^a-z0-9]+", "-", entry["name"].lower()).strip("-")
            n = title_counter.get(slug, 0)
            title_counter[slug] = n + 1
            tkey = f"title-{slug}-{n}"
            settings[tkey] = {"type": "title", "name": entry["name"]}
            pending_title = {"kind": "title", "name": entry["name"]}

        elif kind == "setting":
            s: dict = {
                "type": entry["type"],
                "name": key_to_name(entry["key"]),
                "default": entry["default"],
            }
            if entry.get("description"):
                s["description"] = entry["description"]
            settings[entry["key"]] = s

            if pending_title is not None:
                binder_entries.append(pending_title)
                pending_title = None
            binder_entries.append(
                {
                    "kind": "setting",
                    "key": entry["key"],
                    "type": entry["type"],
                    "cpp_type": entry["cpp_type"],
                    "symbol": entry["symbol"],
                    "namespace": entry.get("namespace"),
                }
            )

    return settings, skipped, binder_entries


# C++ binder emitter


def build_binder(header_path: pathlib.Path, binder_entries: list) -> str:
    """Generate ModSettings.cpp content from binder entries."""
    lines = [
        "// @@GENERATED do not edit! Regenerate: python gen_mod_settings.py --bind-cpp src/ModSettings.cpp",
        f"// Source: {header_path}",
        "",
        "#include <Geode/loader/Mod.hpp>",
        "#include <Geode/loader/SettingV3.hpp>",
        '#include "ModSettings.h"',
        '#include "ModTuning.h"',
        "",
        "using namespace geode;",
        "",
        "namespace mod_settings {",
        "",
        "void bindAll() {",
        "    auto* mod = Mod::get();",
    ]

    for entry in binder_entries:
        if entry["kind"] == "title":
            lines.append("")
            lines.append(f"    // {entry['name']}")
            continue

        key = entry["key"]
        symbol = entry["symbol"]
        ns = entry.get("namespace")
        qual = f"{ns}::{symbol}" if ns else symbol
        cpp_type = entry["cpp_type"]

        if cpp_type == "bool":
            seed = f'mod->getSettingValue<bool>("{key}")'
            setting_type = "bool"
            cb_sig = "bool v"
            assign = "v"
        elif cpp_type == "int":
            seed = f'static_cast<int>(mod->getSettingValue<int64_t>("{key}"))'
            setting_type = "int64_t"
            cb_sig = "int64_t v"
            assign = "static_cast<int>(v)"
        else:  # float or double
            seed = f'static_cast<{cpp_type}>(mod->getSettingValue<double>("{key}"))'
            setting_type = "double"
            cb_sig = "double v"
            assign = f"static_cast<{cpp_type}>(v)"

        lines.append(f"    {qual} = {seed};")
        lines.append(
            f'    listenForSettingChanges<{setting_type}>("{key}", []({cb_sig}) {{ {qual} = {assign}; }});'
        )

    lines.extend(
        [
            "}",
            "",
            "} // namespace mod_settings",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Generate mod.json settings and optional ModSettings.cpp binder from ModTuning.h."
    )
    ap.add_argument("--header", default="src/ModTuning.h", type=pathlib.Path)
    ap.add_argument("--mod-json", default="mod.json", type=pathlib.Path)
    ap.add_argument(
        "--bind-cpp",
        type=pathlib.Path,
        metavar="PATH",
        help="Also emit a ModSettings.cpp binder at this path",
    )
    ap.add_argument(
        "--dry-run",
        action="store_true",
        help="Print generated settings to stdout; do not modify files",
    )
    ap.add_argument(
        "--print-skipped",
        action="store_true",
        help="List skipped constants (computed or pinned compile-time)",
    )
    args = ap.parse_args()

    if not args.header.exists():
        print(f"ERROR: header not found: {args.header}", file=sys.stderr)
        sys.exit(1)

    try:
        settings, skipped, binder_entries = build_settings(args.header)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)

    if args.print_skipped:
        print(f"\nSkipped ({len(skipped)} constants):")
        for s in skipped:
            print(f"  {s['symbol']}: {s['reason']}")
        print()

    if args.dry_run:
        print(json.dumps(settings, indent="\t", ensure_ascii=False))
        return

    # Write mod.json
    if not args.mod_json.exists():
        print(f"ERROR: mod.json not found: {args.mod_json}", file=sys.stderr)
        sys.exit(1)

    mod = json.loads(args.mod_json.read_text(encoding="utf-8"))
    mod["settings"] = settings

    out = json.dumps(mod, indent="\t", ensure_ascii=False) + "\n"
    tmp = args.mod_json.with_suffix(".tmp")
    try:
        tmp.write_text(out, encoding="utf-8")
        os.replace(tmp, args.mod_json)
    finally:
        if tmp.exists():
            tmp.unlink(missing_ok=True)

    setting_count = sum(1 for v in settings.values() if v.get("type") != "title")
    print(f"Wrote {setting_count} settings entries to {args.mod_json}")
    if skipped:
        print(f"  ({len(skipped)} constants skipped — run --print-skipped to list)")

    # Write binder
    if args.bind_cpp:
        cpp_content = build_binder(args.header, binder_entries)
        tmp_cpp = args.bind_cpp.with_suffix(".tmp")
        try:
            tmp_cpp.write_text(cpp_content, encoding="utf-8")
            os.replace(tmp_cpp, args.bind_cpp)
        finally:
            if tmp_cpp.exists():
                tmp_cpp.unlink(missing_ok=True)
        print(f"Wrote binder to {args.bind_cpp}")


if __name__ == "__main__":
    main()
