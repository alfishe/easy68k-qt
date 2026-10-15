#!/usr/bin/env bash
# Copyright 2024 EASy68K Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
# Compare the EASy68K-Qt simulator core against the original Sim68K core
# (reference oracle) by running programs in both and diffing per-instruction
# register traces, final memory hash and Trap #15 text output.
#
# Usage: tools/reference/compare_sim.sh [-v] <build-dir> <prog.x68|prog.S68>... [-- <flags>]
#   <flags> (e.g. -e -b -n 5000) are passed to both simulators.
#   .x68 sources are assembled first with the reference assembler.
#   If <prog>.in exists next to the program it is fed to stdin of both;
#   <prog>.flags adds per-program flags.
#   <build-dir> must contain tools/easy68k-sim/easy68k-sim and (configured
#   with -DBUILD_REFERENCE_TOOLS=ON) tools/reference/sim68k/easy68k-ref-sim
#   and tools/reference/edit68k/easy68k-ref-asm.
# Prints one line per program: SAME / DIFF-TRACE / DIFF-OUTPUT / ASM-ERROR;
# with -v prints the first differing lines. Exit status 1 if any differs.
# Simulator message-pane lines (MSG ...) are not compared.
set -u
verbose=0
if [[ "${1:-}" == "-v" ]]; then verbose=1; shift; fi
build="$1"; shift
port="$build/tools/easy68k-sim/easy68k-sim"
ref="$build/tools/reference/sim68k/easy68k-ref-sim"
asm="$build/tools/reference/edit68k/easy68k-ref-asm"
[[ -x "$port" && -x "$ref" && -x "$asm" ]] || { echo "missing $port, $ref or $asm" >&2; exit 2; }
port="$(cd "$(dirname "$port")" && pwd)/$(basename "$port")"
ref="$(cd "$(dirname "$ref")" && pwd)/$(basename "$ref")"
asm="$(cd "$(dirname "$asm")" && pwd)/$(basename "$asm")"

progs=(); flags=()
while [[ $# -gt 0 ]]; do
  if [[ "$1" == "--" ]]; then shift; flags=("$@"); break; fi
  progs+=("$1"); shift
done

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
status=0
for prog in "${progs[@]}"; do
  name="$(basename "${prog%.*}")"
  dir="$(cd "$(dirname "$prog")" && pwd)"
  input=/dev/null
  extra=()
  [[ -f "$dir/$name.flags" ]] && read -r -a extra <"$dir/$name.flags"
  [[ -f "$dir/$name.in" ]] && input="$dir/$name.in"
  if [[ "$prog" == *.x68 || "$prog" == *.X68 ]]; then
    if ! (cd "$dir" && "$asm" "$(basename "$prog")" -o "$work/$name") >"$work/$name.asm.log" 2>&1 \
        || [[ ! -f "$work/$name.S68" ]]; then
      printf '%-12s %s\n' "ASM-ERROR" "$prog"; status=1
      [[ $verbose -eq 1 ]] && head -20 "$work/$name.asm.log"
      continue
    fi
    s68="$work/$name.S68"
  else
    s68="$prog"
  fi
  # Each simulator runs in its own empty directory (Trap #15 file tasks).
  s68="$(cd "$(dirname "$s68")" && pwd)/$(basename "$s68")"
  rm -rf "$work/ref.d" "$work/port.d"
  mkdir -p "$work/ref.d" "$work/port.d"
  (cd "$work/ref.d" && "$ref" -t "${flags[@]}" "${extra[@]}" -o "$work/$name.ref.trace" "$s68" \
     <"$input" >"$work/$name.ref.out" 2>/dev/null)
  (cd "$work/port.d" && "$port" -t "${flags[@]}" "${extra[@]}" -o "$work/$name.port.trace" "$s68" \
     <"$input" >"$work/$name.port.out" 2>/dev/null)
  # files the program left behind must match too
  diff -r "$work/ref.d" "$work/port.d" >/dev/null || echo "(files differ)" >>"$work/$name.port.out"
  for f in "$work/$name".{ref,port}.trace; do sed -i '/^MSG /d' "$f"; done
  verdict="SAME"
  if ! cmp -s "$work/$name.ref.trace" "$work/$name.port.trace"; then verdict="DIFF-TRACE"
  elif ! cmp -s "$work/$name.ref.out" "$work/$name.port.out"; then verdict="DIFF-OUTPUT"
  fi
  printf '%-12s %s\n' "$verdict" "$prog"
  if [[ "$verdict" != "SAME" ]]; then
    status=1
    if [[ $verbose -eq 1 ]]; then
      case "$verdict" in
        DIFF-TRACE) diff "$work/$name.ref.trace" "$work/$name.port.trace" | head -12 ;;
        DIFF-OUTPUT) diff "$work/$name.ref.out" "$work/$name.port.out" | head -12 ;;
      esac
    fi
  fi
done
exit $status
