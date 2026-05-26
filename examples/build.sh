#!/usr/bin/env bash
# Thin wrapper — delegates to the root build.sh from within the examples dir.
# All arguments are forwarded as-is, so the interface is identical:
#
#   ./build.sh                       release build of uilo + all examples
#   ./build.sh clean containers      clean + rebuild containers only
#   ./build.sh debug uilo containers debug build of library + containers
exec "$(cd "$(dirname "$0")/.." && pwd)/build.sh" "$@"
