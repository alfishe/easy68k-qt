#!/usr/bin/env bash
# Regenerate data/golden/asm/*.l68/*.s68 with the ORIGINAL EASy68K assembler
# (reference oracle), using its default options. The port's golden test then
# checks the EASy68K-Qt assembler against these files.
#
# Usage: tools/reference/regenerate_golden.sh <build-dir>
#   <build-dir> configured with -DBUILD_REFERENCE_TOOLS=ON.
set -euo pipefail
build="$1"
ref="$build/tools/reference/edit68k/easy68k-ref-asm"
[[ -x "$ref" ]] || { echo "missing $ref (configure with -DBUILD_REFERENCE_TOOLS=ON)" >&2; exit 2; }
root="$(cd "$(dirname "$0")/../.." && pwd)"
golden="$root/data/golden/asm"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
for src in "$golden"/*.x68; do
  name="$(basename "${src%.x68}")"
  if ! EASY68K_REF_DATE="" "$ref" "$src" -o "$work/$name" >"$work/$name.log" 2>&1; then
    echo "reference assembler reported errors for $name:" >&2
    cat "$work/$name.log" >&2
    exit 1
  fi
  cp "$work/$name.L68" "$golden/$name.l68"
  cp "$work/$name.S68" "$golden/$name.s68"
  echo "regenerated $name"
done
