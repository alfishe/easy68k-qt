#!/usr/bin/env bash
# Copyright 2024 EASy68K Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
# Regenerate the expected listings and S-records of the assembler corpus
# (data/golden/asm/corpus) with the ORIGINAL EASy68K assembler (reference
# oracle), for each option variant used by tests/asm/corpus_test.cc.
#
# Usage: tools/reference/regenerate_corpus.sh <build-dir>
#   <build-dir> configured with -DBUILD_REFERENCE_TOOLS=ON.
set -eu
build="$(cd "$1" && pwd)"
ref="$build/tools/reference/edit68k/easy68k-ref-asm"
[[ -x "$ref" ]] || { echo "missing $ref" >&2; exit 2; }
root="$(cd "$(dirname "$0")/../.." && pwd)"
corpus="$root/data/golden/asm/corpus"

declare -A variants=(
  [default]=""
  [expand]="--mex --sex --cex"
  [nocre]="--no-cre --no-warnings --no-bitfield"
)
cd "$corpus"   # INCLUDE/INCBIN files are opened relative to the current directory
for v in "${!variants[@]}"; do
  out="$corpus/expected/$v"
  mkdir -p "$out"
  rm -f "$out"/*.L68 "$out"/*.S68
  for src in *.x68; do
    case "$src" in inc_*) continue ;; esac
    name="${src%.x68}"
    # shellcheck disable=SC2086
    "$ref" ${variants[$v]} "$src" -o "$out/$name" >/dev/null 2>&1 || true
  done
done
echo "regenerated $(ls "$corpus"/expected/*/*.L68 | wc -l) listings"
