#!/usr/bin/env bash
# Copyright 2024 EASy68K Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
# clang-tidy on the core libraries and CLI tools (Task 12.3) with the profile in
# ci/clang-tidy.cfg: src/asm/*.cc, src/sim/*.cc, tools/easy68k-*/*.cc and
# include/easym68k.
# src/asm/core and tools/reference are not checked (original layout).
# Configures <build-dir> with compile_commands.json (Clang, no GUI), builds it
# (generated sources must exist) and fails on any warning.
#
# Usage: tools/clang-tidy.sh [build-dir]
#   CLANG_TIDY selects the binary (default: clang-tidy).
set -eu
CLANG_TIDY="${CLANG_TIDY:-clang-tidy}"
root="$(cd "$(dirname "$0")/.." && pwd)"
build="${1:-$root/build-tidy}"

CC=clang CXX=clang++ cmake -S "$root" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_GUI=OFF -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null
if ! out="$(cmake --build "$build" 2>&1)"; then
  echo "$out" >&2
  exit 1
fi

cd "$root"
jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
log="$build/clang-tidy.log"
status=0
files=(src/asm/*.cc src/sim/*.cc tools/easy68k-asm/*.cc tools/easy68k-sim/*.cc tools/easy68k-run/*.cc)
printf '%s\n' "${files[@]}" |
  xargs -P "$jobs" -n 1 "$CLANG_TIDY" -p "$build" --quiet \
    --config-file="$root/ci/clang-tidy.cfg" >"$log" 2>/dev/null || status=$?
count="$(grep -c 'error:\|warning:' "$log" || true)"
if [[ "$status" -ne 0 || "$count" -ne 0 ]]; then
  cat "$log"
  echo "clang-tidy: $count finding(s)" >&2
  exit 1
fi
echo "clang-tidy: clean (${#files[@]} files)"
