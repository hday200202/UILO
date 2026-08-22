#!/usr/bin/env bash
# Builds and runs one of the headless probes in tools/, which drive UILO's
# element tree with no window and no GPU.
#
#   tools/probe.sh layout              print every element's resolved bounds
#   tools/probe.sh layout --baseline   save that output as the baseline
#   tools/probe.sh layout --check      compare against the baseline
#   tools/probe.sh pointer             run the pointer-event assertions
#   tools/probe.sh ctxmenu             run the context-menu assertions
#
# layout and pointer need no window; ctxmenu opens one, because a menu sizes
# itself from measured text and flips against the window edges.
#
# The layout probe's value is as a regression check: take a baseline before a
# layout change, compare after, and the diff should be empty for any change that
# is supposed to be behaviour-preserving. The pointer probe checks itself and
# reports its own exit status.
#
# The compile and link flags are lifted out of the CMake build, so run ./build.sh
# first. Everything lands in build/probe/, which is disposable.
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NAME="${1:-layout}"
SRC="$ROOT/tools/${NAME}_probe.cpp"
OUT="$ROOT/build/probe"
BASELINE="$OUT/${NAME}-baseline.txt"

if [[ ! -f "$SRC" ]]; then
    echo "no such probe: tools/${NAME}_probe.cpp" >&2
    echo "available: $(cd "$ROOT/tools" && ls *_probe.cpp | sed 's/_probe.cpp//' | tr '\n' ' ')" >&2
    exit 2
fi
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

clang++ -std=c++20 -O1 $INCS "$SRC" $LIBS -o "$OUT/${NAME}_probe" || exit 1

case "${2:-}" in
    --baseline)
        "$OUT/${NAME}_probe" > "$BASELINE" 2>&1
        echo "baseline saved: $BASELINE  ($(tail -1 "$BASELINE"))"
        ;;
    --check)
        if [[ ! -f "$BASELINE" ]]; then
            echo "no baseline yet -- run: tools/probe.sh $NAME --baseline" >&2
            exit 2
        fi
        "$OUT/${NAME}_probe" > "$OUT/${NAME}-current.txt" 2>&1
        if diff -q "$BASELINE" "$OUT/${NAME}-current.txt" >/dev/null; then
            echo "$NAME IDENTICAL to baseline  ($(tail -1 "$OUT/${NAME}-current.txt"))"
        else
            echo "$NAME DIFFERS from baseline:"
            diff "$BASELINE" "$OUT/${NAME}-current.txt"
            exit 1
        fi
        ;;
    *)
        "$OUT/${NAME}_probe"
        ;;
esac
