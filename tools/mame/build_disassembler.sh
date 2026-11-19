#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 MAME_SOURCE_DIR" >&2
  exit 2
fi

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
mame_source="$(cd "$1" && pwd)"
actual_commit="$(git -C "$mame_source" rev-parse HEAD)"
expected_commit="d19b5702bdbd1e6c14ecd522e9236154468fc4ef"
if [[ "$actual_commit" != "$expected_commit" ]]; then
  echo "expected MAME $expected_commit, got $actual_commit" >&2
  exit 1
fi

mkdir -p "$repo_root/research_local"
g++ -std=c++20 -O2 \
  -I"$repo_root/tools/mame/shim" \
  -I"$mame_source/src/devices" -I"$mame_source/src/emu" -I"$mame_source/src/osd" \
  -I"$mame_source/src/lib" -I"$mame_source/src/lib/util" -I"$mame_source/src" -I"$mame_source/3rdparty" \
  "$repo_root/tools/mame/adsp_disassemble.cpp" \
  "$mame_source/src/devices/cpu/adsp2100/2100dasm.cpp" \
  "$mame_source/src/lib/util/disasmintf.cpp" "$mame_source/src/lib/util/strformat.cpp" \
  -o "$repo_root/research_local/mame-adsp-disassemble"
