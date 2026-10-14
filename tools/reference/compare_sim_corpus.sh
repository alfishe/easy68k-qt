#!/usr/bin/env bash
# Copyright 2024 EASy68K Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
# Run the simulator instruction corpus in the original Sim68K core (oracle)
# and the port, and compare case by case (compare_sim_blocks.py) plus the
# final memory hash, honouring the accepted deviations.
#
# Usage: tools/reference/compare_sim_corpus.sh [-v] <build-dir> [<corpus-dir>]
#   <corpus-dir> defaults to a fresh one generated from data/golden/asm/matrix.
#   Deviations: data/golden/sim/deviations.tsv (case rules; "HASH <regex>"
#   lines accept a memory hash difference for matching programs).
# Prints one line per program with differences; exit status 1 if any
# difference is not covered by a deviation.
set -u
verbose=0
if [[ "${1:-}" == "-v" ]]; then verbose=1; shift; fi
build="$(cd "$1" && pwd)"
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
devs="$root/data/golden/sim/deviations.tsv"
port="$build/tools/easy68k-sim/easy68k-sim"
ref="$build/tools/reference/sim68k/easy68k-ref-sim"
asm="$build/tools/reference/edit68k/easy68k-ref-asm"
[[ -x "$port" && -x "$ref" && -x "$asm" ]] || { echo "missing $port, $ref or $asm" >&2; exit 2; }

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
corpus="${2:-}"
if [[ -z "$corpus" ]]; then
  corpus="$work/corpus"
  python3 "$here/gen_sim_corpus.py" "$root/data/golden/asm/matrix" "$corpus" >/dev/null
fi
corpus="$(cd "$corpus" && pwd)"

status=0
total=0
for src in "$corpus"/*.x68; do
  name="$(basename "${src%.x68}")"
  flags=(-e -n 200000)
  [[ -f "$corpus/$name.flags" ]] && read -r -a extra <"$corpus/$name.flags" && flags+=("${extra[@]}")
  (cd "$work" && "$asm" "$src" -o "$work/$name") >/dev/null 2>&1
  "$ref" -t "${flags[@]}" -o "$work/$name.ref" "$work/$name.S68" </dev/null >/dev/null 2>&1
  "$port" -t "${flags[@]}" -o "$work/$name.port" "$work/$name.S68" </dev/null >/dev/null 2>&1
  out="$(python3 "$here/compare_sim_blocks.py" "$work/$name.L68" "$work/$name.ref" "$work/$name.port" "$devs")"
  rc=$?
  total=$((total + 1))
  if [[ $rc -ne 0 ]]; then
    status=1
    echo "$out" | tail -1
    [[ $verbose -eq 1 ]] && echo "$out" | head -n -1 | head -30
  fi
  if [[ "$(grep '^HASH' "$work/$name.ref")" != "$(grep '^HASH' "$work/$name.port")" ]]; then
    if ! grep '^HASH ' "$devs" | cut -f1 | cut -c6- | while read -r re; do
         [[ "$name" =~ $re ]] && exit 1; done; then
      :  # accepted
    else
      status=1
      echo "$name: memory hash differs"
    fi
  fi
done
echo "$total programs compared"
exit $status
