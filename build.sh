#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 [--clean]"
    echo "  Builds lib/libuilo.a"
    echo "  --clean   Clean before building"
    exit 1
}

CLEAN=0
for arg in "$@"; do
    case "$arg" in
        --clean) CLEAN=1 ;;
        -h|--help) usage ;;
        *) echo "Unknown argument: $arg"; usage ;;
    esac
done

cd "$(dirname "$0")"

[[ $CLEAN -eq 1 ]] && make clean
make lib
