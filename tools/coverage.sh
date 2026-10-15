#!/usr/bin/env bash
# Copyright 2024 EASy68K Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
# Line coverage of the core libraries (Task 12.2) with Clang source-based
# coverage: builds <build-dir> with ENABLE_COVERAGE, runs the three test
# suites and reports src/sim and src/asm (the ported original assembler
# core src/asm/core is reported separately). Targets: easym68k-sim >= 90%,
# easym68k-asm >= 85% (lines).
#
# Usage: tools/coverage.sh [build-dir] [--html out-dir]
#   LLVM_COV / LLVM_PROFDATA select the llvm tools (default: llvm-cov,
#   llvm-profdata; they must match the clang version).
set -eu
LLVM_COV="${LLVM_COV:-llvm-cov}"
LLVM_PROFDATA="${LLVM_PROFDATA:-llvm-profdata}"
root="$(cd "$(dirname "$0")/.." && pwd)"
build="${1:-$root/build-cov}"
html=""
[[ "${2:-}" == "--html" ]] && html="${3:-$build/coverage-html}"

CC=clang CXX=clang++ cmake -S "$root" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_GUI=OFF -DBUILD_TESTING=ON -DENABLE_COVERAGE=ON >/dev/null
cmake --build "$build" >/dev/null
rm -f "$build"/*.profraw
for t in sim asm integration; do
  (cd "$root" && LLVM_PROFILE_FILE="$build/$t.profraw" "$build/tests/easym68k-$t-tests" \
    --gtest_brief=1 >/dev/null)
done
"$LLVM_PROFDATA" merge -sparse "$build"/*.profraw -o "$build/coverage.profdata"
objs=(-object "$build/tests/easym68k-sim-tests" -object "$build/tests/easym68k-asm-tests"
      -object "$build/tests/easym68k-integration-tests")
report() {  # report <label> <regex for sources to include>
  "$LLVM_COV" report "${objs[@]:1}" -instr-profile="$build/coverage.profdata" \
    "$root/$2" 2>/dev/null | tail -1 | awk -v l="$1" '{print l": lines " $(NF-3) " (" $(NF-5) " lines)"}'
}
pct() {  # pct <source path>: line coverage percentage of that path
  "$LLVM_COV" report "${objs[@]:1}" -instr-profile="$build/coverage.profdata" "$root/$1" \
    2>/dev/null | tail -1 | awk '{v=$(NF-3); sub("%","",v); print v}'
}
report "easym68k-sim" "src/sim"
report "easym68k-asm (port code)" "src/asm/assembler.cc"
sim="$(pct src/sim)"
asm="$(pct src/asm)"
echo "easym68k-asm (all, incl. core/): lines ${asm}%"
status=0
awk -v v="$sim" 'BEGIN{exit !(v >= 90)}' || { echo "FAIL: easym68k-sim below 90%"; status=1; }
awk -v v="$asm" 'BEGIN{exit !(v >= 85)}' || { echo "FAIL: easym68k-asm below 85%"; status=1; }
if [[ -n "$html" ]]; then
  "$LLVM_COV" show "${objs[@]:1}" -instr-profile="$build/coverage.profdata" -format=html \
    -output-dir="$html" "$root/src" >/dev/null
  echo "HTML report: $html/index.html"
fi
exit $status
