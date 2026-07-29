#!/usr/bin/env python3
"""Regenerate include/assets/EmbeddedIcons.hpp from assets/icons/*.svg.

The built-in icon set is embedded as string literals rather than read from disk
so that consumers only have to link UILO -- no asset folder to ship, no working
directory to get right. The whole set is ~110 KB of markup, which is cheap to
carry; the expensive part (parse + rasterize) stays lazy at runtime.

Run this after adding, removing, or editing anything in assets/icons/:

    python3 tools/gen_embedded_icons.py

The generated header is committed, so a normal build never needs Python.
"""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent.parent
ICON_DIR = ROOT / "assets" / "icons"
OUT = ROOT / "include" / "assets" / "EmbeddedIcons.hpp"

# C++ identifiers can't contain '-' and can't start with a digit.
RESERVED = {
    "alignas", "alignof", "and", "asm", "auto", "bool", "break", "case", "catch",
    "char", "class", "const", "continue", "default", "delete", "do", "double",
    "else", "enum", "explicit", "export", "extern", "false", "float", "for",
    "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace",
    "new", "not", "nullptr", "operator", "or", "private", "protected", "public",
    "register", "return", "short", "signed", "sizeof", "static", "struct",
    "switch", "template", "this", "throw", "true", "try", "typedef", "typeid",
    "typename", "union", "unsigned", "using", "virtual", "void", "volatile",
    "while", "xor",
}


def identifier(stem: str) -> str:
    ident = re.sub(r"[^0-9a-zA-Z]+", "_", stem)
    if not ident or ident[0].isdigit():
        ident = "_" + ident
    if ident in RESERVED:
        ident += "_"
    return ident


def raw_literal(markup: str) -> str:
    """Wrap markup in a raw string literal, picking a delimiter it can't contain."""
    for delim in ("", "svg", "uilo", "x1", "x2"):
        if f'){delim}"' not in markup:
            return f'R"{delim}({markup}){delim}"'
    raise ValueError("could not find a safe raw-string delimiter")


def main() -> int:
    if not ICON_DIR.is_dir():
        print(f"error: {ICON_DIR} not found", file=sys.stderr)
        return 1

    # Sort by STEM, not by file name. Resources binary-searches the table by icon
    # name, and the two orders are not the same: "arrow-down-circle.svg" sorts
    # before "arrow-down.svg" ('-' < '.'), while the stem "arrow-down" sorts
    # before "arrow-down-circle". Sorting by file name here silently breaks
    # lookups for a chunk of the set.
    files = sorted(ICON_DIR.glob("*.svg"), key=lambda p: p.stem)
    if not files:
        print(f"error: no .svg files in {ICON_DIR}", file=sys.stderr)
        return 1

    entries = []
    seen: dict[str, str] = {}
    for path in files:
        name = path.stem                      # "arrow-left"
        ident = identifier(name)              # "arrow_left"
        if ident in seen:
            print(
                f"error: '{name}' and '{seen[ident]}' both map to identifier "
                f"'{ident}'; rename one of them",
                file=sys.stderr,
            )
            return 1
        seen[ident] = name
        markup = path.read_text(encoding="utf-8").strip()
        entries.append((name, ident, markup))

    total = sum(len(m) for _, _, m in entries)

    out = []
    w = out.append
    w("#pragma once")
    w("")
    w("// GENERATED FILE -- do not edit by hand.")
    w("// Regenerate with: python3 tools/gen_embedded_icons.py")
    w(f"// Source: assets/icons/  ({len(entries)} icons, {total} bytes of markup)")
    w("//")
    w("// Included at the bottom of utils/Resources.hpp, which declares the")
    w("// Resources::icons nested struct that the name constants below define.")
    w("")
    w("#include <string_view>")
    w("")
    w("namespace uilo {")
    w("namespace detail {")
    w("")
    w("// name -> markup, sorted by name so Resources can binary-search it.")
    w("struct EmbeddedIconEntry {")
    w("    std::string_view name;")
    w("    std::string_view markup;")
    w("};")
    w("")
    w("inline constexpr EmbeddedIconEntry kEmbeddedIcons[] = {")
    for name, _, markup in entries:
        w(f'    {{ "{name}",')
        w(f"      {raw_literal(markup)} }},")
    w("};")
    w("")
    w(f"inline constexpr std::size_t kEmbeddedIconCount = {len(entries)};")
    w("")
    w("} // namespace detail")
    w("")
    w("// Canonical names for every built-in icon. These are the icon *names*, not")
    w("// the markup: the markup lives once in the table above and is reached")
    w("// through the registry, so a built-in and a user-registered icon take the")
    w("// same path. Hyphens in the file name become underscores here")
    w("// (arrow-left.svg -> Resources::icons::arrow_left).")
    w("struct Resources::icons {")
    for name, ident, _ in entries:
        w(f'    static constexpr std::string_view {ident} = "{name}";')
    w("};")
    w("")
    w("} // namespace uilo")
    w("")

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(out), encoding="utf-8")
    print(f"wrote {OUT.relative_to(ROOT)}: {len(entries)} icons, {total} bytes of markup")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
