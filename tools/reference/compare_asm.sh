#!/usr/bin/env bash
# Copyright 2024 EASy68K Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
# Compare the EASy68K-Qt assembler against the original EASy68K assembler
# (reference oracle) on a set of sources.
#
# Usage: tools/reference/compare_asm.sh <build-dir> <source.x68>... [-- <flags>]
#   <flags> (e.g. --mex --sex --cex) are passed to both assemblers.
#   <build-dir> must contain tools/easy68k-asm/easy68k-asm and (configured
#   with -DBUILD_REFERENCE_TOOLS=ON) tools/reference/edit68k/easy68k-ref-asm.
# Prints one line per source: SAME / DIFF-S68 / DIFF-L68 / REF-ERROR /
# PORT-ERROR; with -v prints the diffs. Exit status 1 if any file differs.
set -u
verbose=0
if [[ "${1:-}" == "-v" ]]; then verbose=1; shift; fi
build="$1"; shift
port="$build/tools/easy68k-asm/easy68k-asm"
ref="$build/tools/reference/edit68k/easy68k-ref-asm"
[[ -x "$port" && -x "$ref" ]] || { echo "missing $port or $ref" >&2; exit 2; }

sources=(); ref_flags=()
while [[ $# -gt 0 ]]; do
  if [[ "$1" == "--" ]]; then shift; ref_flags=("$@"); break; fi
  sources+=("$1"); shift
done

# Files are the same if equal or both absent (e.g. --no-list).
same() { [[ ! -e "$1" && ! -e "$2" ]] || cmp -s "$1" "$2"; }

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
status=0
port="$(cd "$(dirname "$port")" && pwd)/$(basename "$port")"
ref="$(cd "$(dirname "$ref")" && pwd)/$(basename "$ref")"
for src in "${sources[@]}"; do
  name="$(basename "${src%.*}")"
  # Run both in the source's directory: the original opens INCLUDE/INCBIN
  # files relative to the current directory.
  dir="$(cd "$(dirname "$src")" && pwd)"
  file="$(basename "$src")"
  (cd "$dir" && "$ref" "${ref_flags[@]}" "$file" -o "$work/$name.ref") >"$work/$name.ref.log" 2>&1
  ref_rc=$?
  (cd "$dir" && "$port" "${ref_flags[@]}" "$file" -o "$work/$name.port") >"$work/$name.port.log" 2>&1
  port_rc=$?
  # Normalise line endings for comparison.
  for f in "$work/$name".{ref,port}.{S68,L68}; do [[ -f "$f" ]] && sed -i 's/\r$//' "$f"; done
  verdict="SAME"
  if [[ $ref_rc -ne 0 && $port_rc -eq 0 ]]; then verdict="REF-ERROR"
  elif [[ $port_rc -ne 0 && $ref_rc -eq 0 ]]; then verdict="PORT-ERROR"
  elif ! same "$work/$name.ref.S68" "$work/$name.port.S68" &&
       ! { grep -qi "SIMHALT" "$src" && python3 "$(dirname "$0")/srec_equal.py" \
             "$work/$name.ref.S68" "$work/$name.port.S68"; }; then
    # (SIMHALT: the port writes both $FFFF words, the original drops one;
    # accepted deviation when the loaded images are equal.)
    verdict="DIFF-S68"
  elif ! same "$work/$name.ref.L68" "$work/$name.port.L68"; then verdict="DIFF-L68"
  fi
  printf '%-12s %s\n' "$verdict" "$src"
  if [[ "$verdict" != "SAME" ]]; then
    status=1
    if [[ $verbose -eq 1 ]]; then
      case "$verdict" in
        DIFF-S68) diff "$work/$name.ref.S68" "$work/$name.port.S68" | head -20 ;;
        DIFF-L68) diff "$work/$name.ref.L68" "$work/$name.port.L68" | head -40 ;;
        *) cat "$work/$name.ref.log" "$work/$name.port.log" | head -20 ;;
      esac
    fi
  fi
done
exit $status
