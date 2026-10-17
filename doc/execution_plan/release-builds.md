# Phase 14: Release Packaging

Multi-platform distributable builds, modeled on the unreal-ng release pipeline
(`.github/workflows/release.yml` + CMake packaging targets), activated after
Phase 13 completed the GUI.

### Task 14.1 — Runtime probes and bundle-ready apps

- `WIN32 MACOSX_BUNDLE` on the three GUI executables + shared
  `easy68k_configure_gui_app()` (`cmake/GuiAppBundle.cmake`): bundle
  properties, custom `install/macos/Info.plist`, per-app `.icns`, Windows
  `.rc` exe icon/version info, compiled-in window icon (`install/qrc-icons`).
- Icon set generated programmatically (chip motif, one accent per app):
  `install/icons` (1024px sources), `install/macos/*.icns`,
  `install/windows/*.ico`, `install/linux/icons/hicolor/*`.
- Packaged-layout runtime probes: `MainWindow::DefaultTemplatePath()`
  (env → app-relative → compiled fallback, mirroring `DefaultHelpDir`),
  `../Resources/data/...` candidates in the help/examples probes,
  `.exe` + sibling-bundle candidates in `LaunchSimulator` (fixes packaged
  Windows, which never probed `easy68k-simulator.exe`) and
  `ResolveEasyBinExecutable` (+ build-tree bundle candidate for macOS dev
  runs).
- AppleClang: `-Wno-error=deprecated-declarations` (SDK deprecates `sprintf`
  in C++; ported assembler core uses it pervasively — the MSVC build already
  silences the equivalent via `_CRT_SECURE_NO_WARNINGS`).

**Quality Gate 14.1:** full test suite green; DMG apps launch and find
help/examples/template via the new probes; dev-tree behavior unchanged.

### Task 14.2 — Packaging targets and DEB

- `cmake/SuitePackaging.cmake`: `package_suite_macos` (per-bundle data +
  macdeployqt + ad-hoc codesign + `hdiutil` UDZO DMG), `package_suite_windows`
  (flat-root staging, windeployqt ×3, MSVC CRT DLLs, zip),
  `package_suite_linux` (bin/+share staging for AppImage/DEB; headers and
  static libs excluded).
- `tools/package-appimage.sh`: AppDir from the staging tree, pinned
  linuxdeploy + linuxdeploy-plugin-qt, `qoffscreen` plugin included.
- CPack DEB on Linux: distro-Qt dependencies via `dpkg-shlibdeps`, runtime
  files in the `Suite` component (desktop files + hicolor icons installed),
  `Devel` component (headers/static libs) not shipped.
- `cmake/MacOSDeploymentTarget.cmake`: `CMAKE_OSX_DEPLOYMENT_TARGET` 12.0
  floor (Qt 6.9 minimum).

**Quality Gate 14.2:** all three package targets build; DMG verified locally
(self-contained Mach-O, `LSMinimumSystemVersion`, codesign, launch smoke).

### Task 14.3 — Release workflow

- `.github/workflows/release.yml`: Linux (AppImage + DEB + clean-container
  verify), Windows MSVC x64 zip, macOS x86_64 + arm64 DMGs; rolling
  "continuous" pre-release via workflow_dispatch, permanent releases on `v*`
  tags, PRs build/verify without publishing; linuxdeploy pinned by SHA256.
- Task 12.1 done alongside: `ci/github/*.yml` copied to `.github/workflows/`.

**Quality Gate 12.1/14.3:** all CI pipelines and a first snapshot release
green on GitHub.
