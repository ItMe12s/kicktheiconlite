"""Generate mod.json settings block from src/ModTuning.h constexpr values."""

import argparse
import json
import os
import pathlib
import re
import sys

# Key naming


def symbol_to_key(name: str, ns_prefix: str = "") -> str:
    """Convert C++ symbol like kPlayerFriction → player-friction."""
    # Strip leading k if followed by uppercase
    if name.startswith("k") and len(name) > 1 and name[1].isupper():
        name = name[1:]
    # Split uppercase-run boundaries: ABCDef → ABC-Def
    result = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1-\2", name)
    # Split camelCase boundaries: aB → a-B
    result = re.sub(r"([a-z\d])([A-Z])", r"\1-\2", result)
    # Only split digit→alpha (e.g. 2Max → 2-Max); keep alpha→digit (B2 stays b2)
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
    """
    Return True if rhs contains only numeric/bool literals and arithmetic operators.
    Any identifier (other than true/false) disqualifies it.
    """
    s = rhs.strip()
    if s in ("true", "false"):
        return True
    # Strip C++ numeric type suffixes after digits/hex digits
    s_norm = re.sub(r"(?<=[0-9a-fA-F])([fFlLuU]+)(?=[^a-zA-Z0-9_]|$)", "", s)
    # Erase numeric literals so their letter components (x in 0x, e in 1e-6) don't
    # get matched as identifiers. Order: hex first, then scientific, then plain float/int
    s_erased = re.sub(r"0[xX][0-9a-fA-F]+", " ", s_norm)
    s_erased = re.sub(r"(?<![a-zA-Z0-9_])\d+\.?\d*[eE][+-]?\d+", " ", s_erased)
    s_erased = re.sub(r"(?<![a-zA-Z0-9_])\.\d+", " ", s_erased)
    s_erased = re.sub(r"(?<![a-zA-Z0-9_])\d+\.?\d*", " ", s_erased)
    # Any remaining letter sequences must be bool keywords only
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
    # Strip all C++ numeric suffixes so Python eval can handle it
    s = re.sub(r"(?<=[\d.eE])([fFlLuU]+)(?=[^a-zA-Z]|$)", "", s)
    try:
        # eval is safe here: is_pure_literal already confirmed no identifiers
        val = eval(s)  # noqa: S307
        if cpp_type in ("float", "double"):
            return float(val)
        return int(val)
    except Exception:
        return None


# Header parser

CONSTEXPR_RE = re.compile(
    r"^\s*constexpr\s+(bool|int|float|double)\s+(\w+)\s*=\s*(.+?)\s*;"
)
SECTION_RE = re.compile(r"^//\s+([A-Z][^/\n]{0,100}?)\s*$")
NAMESPACE_OPEN_RE = re.compile(r"^\s*namespace\s+(\w+)\s*\{")
NAMESPACE_CLOSE_RE = re.compile(r"^\s*\}\s*(?://|$)")
STATIC_ASSERT_RE = re.compile(r"^\s*static_assert\b")


def parse_header(path: pathlib.Path):
    """
    Yield entries in source order:
      {'kind': 'title',   'name': str}
      {'kind': 'setting', 'key': str, 'type': str, 'default': value, 'description': str|None}
      {'kind': 'skip',    'symbol': str, 'reason': str}
    Raises ValueError on key collision.
    """
    namespace_stack: list[str] = []
    current_section: str | None = None
    section_emitted = False
    seen_keys: dict[str, str] = {}
    prev_blank = True  # treat file start as blank

    lines = path.read_text(encoding="utf-8").splitlines()
    for line in lines:
        is_blank = not line.strip()

        # Namespace open
        m = NAMESPACE_OPEN_RE.match(line)
        if m:
            namespace_stack.append(m.group(1))
            prev_blank = False
            continue

        # Namespace close (best-effort: only pops if stack non-empty)
        if NAMESPACE_CLOSE_RE.match(line) and namespace_stack:
            if "namespace" in line:
                namespace_stack.pop()
            prev_blank = False
            continue

        # Skip static_assert
        if STATIC_ASSERT_RE.match(line):
            prev_blank = False
            continue

        if is_blank:
            prev_blank = True
            continue

        # Section comment: standalone // line preceded by a blank line
        m = SECTION_RE.match(line)
        if m and not CONSTEXPR_RE.match(line) and prev_blank:
            current_section = m.group(1).strip()
            section_emitted = False
            prev_blank = False
            continue

        prev_blank = False

        # Constexpr
        m = CONSTEXPR_RE.match(line)
        if not m:
            continue

        cpp_type, symbol, rhs_raw = m.group(1), m.group(2), m.group(3)

        # Separate RHS from inline comment; capture comment as description
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

        # Build key with namespace prefix
        ns_prefix = namespace_stack[-1].replace("_", "-") if namespace_stack else ""
        key = symbol_to_key(symbol, ns_prefix)

        if key in seen_keys:
            raise ValueError(
                f"Key collision: '{key}' from '{symbol}' and '{seen_keys[key]}'"
            )
        seen_keys[key] = symbol

        # Emit pending section title on first entry of the section
        if current_section and not section_emitted:
            yield {"kind": "title", "name": current_section}
            section_emitted = True

        entry: dict = {
            "kind": "setting",
            "key": key,
            "type": "float" if cpp_type == "double" else cpp_type,
            "default": value,
        }
        if description:
            entry["description"] = description
        yield entry


# Build settings dict


def build_settings(header: pathlib.Path) -> tuple[dict, list]:
    settings: dict = {}
    skipped: list = []
    title_counter: dict[str, int] = {}

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

        elif kind == "setting":
            s: dict = {
                "type": entry["type"],
                "name": key_to_name(entry["key"]),
                "default": entry["default"],
            }
            if entry.get("description"):
                s["description"] = entry["description"]
            settings[entry["key"]] = s

    return settings, skipped


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Generate mod.json settings from ModTuning.h constexpr values."
    )
    ap.add_argument("--header", default="src/ModTuning.h", type=pathlib.Path)
    ap.add_argument("--mod-json", default="mod.json", type=pathlib.Path)
    ap.add_argument(
        "--dry-run",
        action="store_true",
        help="Print generated settings to stdout; do not modify mod.json",
    )
    ap.add_argument(
        "--print-skipped",
        action="store_true",
        help="List computed constants that were skipped",
    )
    args = ap.parse_args()

    if not args.header.exists():
        print(f"ERROR: header not found: {args.header}", file=sys.stderr)
        sys.exit(1)

    try:
        settings, skipped = build_settings(args.header)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)

    if args.print_skipped:
        print(f"\nSkipped ({len(skipped)} computed constants):")
        for s in skipped:
            print(f"  {s['symbol']}: {s['reason']}")
        print()

    if args.dry_run:
        print(json.dumps(settings, indent="\t", ensure_ascii=False))
        return

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

    print(f"Wrote {len(settings)} settings entries to {args.mod_json}")
    if skipped:
        print(
            f"  ({len(skipped)} computed constants skipped"
            f" — run --print-skipped to list)"
        )


if __name__ == "__main__":
    main()
