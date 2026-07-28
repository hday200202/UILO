#!/usr/bin/env bash
# Thin wrapper — delegates to the root build.sh from within the examples dir.
# All arguments are forwarded as-is, so the interface is identical:
#
#   ./build.sh                       release build of uilo + all examples
#   ./build.sh clean containers      clean + rebuild containers only
#   ./build.sh debug uilo containers debug build of library + containers
#
# Extra convenience: pass `run <example>` (e.g. `./build.sh run containers`
# or `./build.sh release run containers`) to build and then execute the
# resulting binary. The example is run from this directory so relative
# asset paths (assets/fonts/...) resolve correctly.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"

RUN_TARGET=""
FORWARD=()

i=0
args=("$@")
while [[ $i -lt ${#args[@]} ]]; do
    a="${args[$i]}"
    case "$a" in
        run)
            i=$((i+1))
            if [[ $i -ge ${#args[@]} ]]; then
                echo "build.sh: 'run' requires an example name" >&2
                exit 1
            fi
            RUN_TARGET="${args[$i]}"
            FORWARD+=("$RUN_TARGET")
            ;;
        *) FORWARD+=("$a") ;;
    esac
    i=$((i+1))
done

"$ROOT/build.sh" "${FORWARD[@]}"

if [[ -n "$RUN_TARGET" ]]; then
    # Find the built binary across generator layouts: single-config puts it at
    # bin/<name>, multi-config (Visual Studio) at bin/<Config>/<name>, and it
    # carries a .exe on Windows.
    BIN="$(ls "$HERE/bin/$RUN_TARGET" "$HERE"/bin/*/"$RUN_TARGET" \
              "$HERE/bin/$RUN_TARGET.exe" "$HERE"/bin/*/"$RUN_TARGET.exe" \
              2>/dev/null | head -1 || true)"
    if [[ -z "$BIN" || ! -x "$BIN" ]]; then
        echo "build.sh: binary for '$RUN_TARGET' not found under $HERE/bin" >&2
        exit 1
    fi
    cd "$HERE"
    exec "$BIN"
fi
