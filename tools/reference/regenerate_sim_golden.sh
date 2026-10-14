#!/usr/bin/env bash
# Copyright 2024 EASy68K Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
# Regenerate the golden simulation traces (data/golden/sim/programs) from the
# ORIGINAL Sim68K core (oracle). Never generate them from the port.
#
# Usage: tools/reference/regenerate_sim_golden.sh <build-dir>
#   <build-dir> configured with -DBUILD_REFERENCE_TOOLS=ON.
#
# Programs: every data/golden/sim/src/*.x68 (hand-written; optional .in
# keyboard input and .flags simulator flags) plus the instruction corpus
# programs named in data/golden/sim/corpus.txt (gen_sim_corpus.py, run with
# -e; bit field programs with -b). Only programs whose oracle trace the port
# reproduces exactly belong here (no accepted deviations).
#
# For each program <name>: <name>.S68 (assembled by the reference assembler),
# <name>.expected (FNV-1a 64 digest of the INIT/STEP trace lines, then the
# END, FINAL and HASH lines), <name>.out (Trap #15 output) and, if any,
# <name>.in / <name>.flags. tests/sim/golden_trace_test.cc replays them.
set -eu
build="$(cd "$1" && pwd)"
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
ref="$build/tools/reference/sim68k/easy68k-ref-sim"
asm="$build/tools/reference/edit68k/easy68k-ref-asm"
out="$root/data/golden/sim/programs"
[[ -x "$ref" && -x "$asm" ]] || { echo "missing $ref or $asm" >&2; exit 2; }

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
python3 "$here/gen_sim_corpus.py" "$root/data/golden/asm/matrix" "$work/corpus" >/dev/null

rm -rf "$out"
mkdir -p "$out"

run() {  # run <name> <src.x68> <flags...>
  local name="$1" src="$2"
  shift 2
  (cd "$(dirname "$src")" && "$asm" "$(basename "$src")" -o "$work/$name") >/dev/null 2>&1 \
    || { echo "$name: assembly failed" >&2; exit 1; }
  cp "$work/$name.S68" "$out/$name.S68"
  local input=/dev/null
  if [[ -f "${src%.x68}.in" ]]; then
    cp "${src%.x68}.in" "$out/$name.in"
    input="$out/$name.in"
  fi
  if [[ $# -gt 0 ]]; then printf "%s\n" "$*" >"$out/$name.flags"; fi
  "$ref" -t "$@" -n 400000 -o "$work/$name.trace" "$out/$name.S68" <"$input" >"$out/$name.out"
  python3 - "$work/$name.trace" >"$out/$name.expected" <<'EOF'
import sys
h = 0xcbf29ce484222325
tail = []
for line in open(sys.argv[1]):
    if line.startswith(("INIT", "STEP")):
        for b in line.encode():
            h = ((h ^ b) * 0x100000001b3) & 0xFFFFFFFFFFFFFFFF
    elif line.startswith(("END", "FINAL", "HASH")):
        tail.append(line)
print(f"TRACE {h:016X}")
sys.stdout.write("".join(tail))
EOF
  echo "$name"
}

for src in "$root"/data/golden/sim/src/*.x68; do
  name="$(basename "${src%.x68}")"
  flags=()
  [[ -f "${src%.x68}.flags" ]] && read -r -a flags <"${src%.x68}.flags"
  run "$name" "$src" "${flags[@]}"
done
while read -r name; do
  [[ -z "$name" || "$name" == \#* ]] && continue
  flags=(-e)
  [[ -f "$work/corpus/$name.flags" ]] && flags+=($(cat "$work/corpus/$name.flags"))
  run "$name" "$work/corpus/$name.x68" "${flags[@]}"
done <"$root/data/golden/sim/corpus.txt"
