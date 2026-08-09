#!/usr/bin/env bash
# Builds and runs tools/layout_probe.cpp, which prints the resolved bounds of
# every element in a set of layout cases. Its value is as a regression check:
# take a baseline before a layout change, compare after, and the diff should be
# empty for any change that is supposed to be behaviour-preserving.
#
#   tools/layout_probe.sh                 build and print
#   tools/layout_probe.sh --baseline      save the current output as the baseline
#   tools/layout_probe.sh --check         compare against the baseline
#
# The compile and link flags are lifted out of the CMake build, so run ./build.sh
# first. Everything lands in build/layout-probe/, which is disposable.
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/layout-probe"
BASELINE="$OUT/baseline.txt"
mkdir -p "$OUT"

DB="$(find "$ROOT/build" -name compile_commands.json -print -quit 2>/dev/null)"
NINJA="$(find "$ROOT/build" -name build.ninja -print -quit 2>/dev/null)"
if [[ -z "$DB" || -z "$NINJA" ]]; then
    echo "no CMake build found under build/ -- run ./build.sh first" >&2
    exit 2
fi

# Include flags: whatever the library itself is compiled with.
INCS="$(python3 - "$DB" <<'PY'
import json, re, sys
db = json.load(open(sys.argv[1]))
ent = next((e for e in db if 'Element.cpp' in e['file']), db[0])
cmd = ent.get('command') or ' '.join(ent['arguments'])
found = re.findall(r'-I\s*(\S+)|-isystem\s*(\S+)', cmd)
print(' '.join(f'-I{a or b}' for a, b in found))
PY
)"

# Link line: whatever an example links against.
LIBS="$(grep -m1 'LINK_LIBRARIES = ' "$NINJA" | sed 's/.*LINK_LIBRARIES = //')"

clang++ -std=c++20 -O1 $INCS "$ROOT/tools/layout_probe.cpp" $LIBS -o "$OUT/probe" || exit 1

case "${1:-}" in
    --baseline)
        "$OUT/probe" > "$BASELINE" 2>&1
        echo "baseline saved: $BASELINE  ($(tail -1 "$BASELINE"))"
        ;;
    --check)
        if [[ ! -f "$BASELINE" ]]; then
            echo "no baseline yet -- run: tools/layout_probe.sh --baseline" >&2
            exit 2
        fi
        "$OUT/probe" > "$OUT/current.txt" 2>&1
        if diff -q "$BASELINE" "$OUT/current.txt" >/dev/null; then
            echo "layout IDENTICAL to baseline  ($(tail -1 "$OUT/current.txt"))"
        else
            echo "layout DIFFERS from baseline:"
            diff "$BASELINE" "$OUT/current.txt"
            exit 1
        fi
        ;;
    *)
        "$OUT/probe"
        ;;
esac
