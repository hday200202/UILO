#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
EXAMPLES_DIR="$(cd "$(dirname "$0")" && pwd)"

# Discover all examples (any .cpp at the examples root level)
ALL_EXAMPLES=()
for f in "$EXAMPLES_DIR"/*.cpp; do
    [[ -f "$f" ]] && ALL_EXAMPLES+=("$(basename "$f" .cpp)")
done

usage() {
    local examples_list
    examples_list=$(IFS=', '; echo "${ALL_EXAMPLES[*]:-<none>}")
    echo "Usage: $0 [options] [target...]"
    echo ""
    echo "Targets:"
    echo "  uilo               Build libuilo.a only"
    echo "  all                Build libuilo.a + all examples"
    echo "  <name>             Build a specific example (implies uilo)"
    echo "  (no target)        Same as 'all'"
    echo ""
    echo "Available examples: $examples_list"
    echo ""
    echo "Options:"
    echo "  --clean            Clean before building"
    echo "  -h, --help         Show this help"
    exit 1
}

CLEAN=0
TARGETS=()

for arg in "$@"; do
    case "$arg" in
        --clean) CLEAN=1 ;;
        -h|--help) usage ;;
        -*) echo "Unknown option: $arg"; usage ;;
        *) TARGETS+=("$arg") ;;
    esac
done

# Default: build all
[[ ${#TARGETS[@]} -eq 0 ]] && TARGETS=("all")

build_uilo() {
    echo "==> Building libuilo.a"
    [[ $CLEAN -eq 1 ]] && make -C "$ROOT_DIR" clean
    make -C "$ROOT_DIR" lib
}

build_example() {
    local name="$1"
    # Verify source exists
    if [[ ! -f "$EXAMPLES_DIR/$name.cpp" ]]; then
        echo "Error: No example source found for '$name' ($EXAMPLES_DIR/$name.cpp)"
        exit 1
    fi
    # Add target to examples Makefile if not already present
    echo "==> Building example: $name"
    make -C "$EXAMPLES_DIR" "$name"
}

# Add a Makefile target for examples that don't yet have one
ensure_makefile_target() {
    local name="$1"
    local mf="$EXAMPLES_DIR/Makefile"
    # Check if the target already exists
    if ! grep -q "^$name:" "$mf" 2>/dev/null; then
        printf '\n%s: bin/%s\n\nbin/%s: %s.cpp | bin/\n\t$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)\n' \
            "$name" "$name" "$name" "$name" >> "$mf"
        echo "    (added Makefile target for '$name')"
    fi
}

BUILT_UILO=0

for target in "${TARGETS[@]}"; do
    case "$target" in
        uilo)
            build_uilo
            BUILT_UILO=1
            ;;
        all)
            build_uilo
            BUILT_UILO=1
            [[ $CLEAN -eq 1 ]] && make -C "$EXAMPLES_DIR" clean
            for ex in "${ALL_EXAMPLES[@]}"; do
                ensure_makefile_target "$ex"
                build_example "$ex"
            done
            ;;
        *)
            if [[ $BUILT_UILO -eq 0 ]]; then
                build_uilo
                BUILT_UILO=1
            fi
            ensure_makefile_target "$target"
            build_example "$target"
            ;;
    esac
done

echo "==> Done."
