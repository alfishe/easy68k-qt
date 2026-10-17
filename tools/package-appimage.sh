#!/usr/bin/env bash
# Assemble the easy68k-qt AppImage from the package_suite_linux staging tree
# (build/packages/easy68k-qt-Suite). linuxdeploy and linuxdeploy-plugin-qt
# (both AppImages, pinned by SHA256 in the CI workflow) must be on PATH and
# must resolve the same Qt (qmake first on PATH) the build used.
#
# Usage: EASY68K_PACKAGE_VERSION=YYYYMMDD.HHMMSS tools/package-appimage.sh [build-dir]

set -euo pipefail

build_dir=$(realpath "${1:-build}")
repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
suite="$build_dir/packages/easy68k-qt-Suite"
appdir="$build_dir/packages/easy68k.AppDir"
output="$build_dir/packages/easy68k-qt-Suite-Linux-x86_64.AppImage"

apps=(easy68k-simulator easy68k-editor easybin easy68k-asm easy68k-sim easy68k-run)
for app in "${apps[@]}"; do
    test -x "$suite/bin/$app" || { echo "missing staged executable: $suite/bin/$app" >&2; exit 1; }
done

rm -rf "$appdir"
mkdir -p "$appdir/usr/share/doc/easy68k-qt"
cp -a "$suite/." "$appdir/usr/"
cp "$repo_dir/LICENSE" "$appdir/usr/share/doc/easy68k-qt/"

executables=()
for app in "${apps[@]}"; do
    executables+=(--executable "$appdir/usr/bin/$app")
done

# The simulator is the suite's main entry; the editor/easybin desktop files
# already staged under usr/share/applications integrate them into menus.
export EXTRA_PLATFORM_PLUGINS=libqoffscreen.so
export APPIMAGE_EXTRACT_AND_RUN=1
export LDAI_OUTPUT="$output"
export LINUXDEPLOY_OUTPUT_VERSION="${EASY68K_PACKAGE_VERSION:?Set EASY68K_PACKAGE_VERSION to YYYYMMDD.HHMMSS}"

linuxdeploy-x86_64.AppImage \
    --appdir "$appdir" \
    "${executables[@]}" \
    --desktop-file "$repo_dir/install/linux/easy68k-simulator.desktop" \
    --icon-file "$repo_dir/install/linux/icons/hicolor/256x256/apps/easy68k-simulator.png" \
    --plugin qt \
    --output appimage
