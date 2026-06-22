#!/usr/bin/env bash
set -euo pipefail

ARCH="${1:-arm64}"
if [[ "$ARCH" != "arm64" && "$ARCH" != "x86_64" ]]; then
  echo "Usage: $0 arm64|x86_64"
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-macos-$ARCH"
PKG_ROOT="$ROOT_DIR/dist/macos-$ARCH/pkgroot"
DMG_ROOT="$ROOT_DIR/dist/macos-$ARCH/dmgroot"
OUT_DIR="$ROOT_DIR/dist/macos-$ARCH"
PKG_PATH="$OUT_DIR/trukendoos-mac-$ARCH.pkg"
DMG_PATH="$OUT_DIR/trukendoos-mac-$ARCH.dmg"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Xcode -DCMAKE_OSX_ARCHITECTURES="$ARCH"
cmake --build "$BUILD_DIR" --config Release

rm -rf "$PKG_ROOT"
mkdir -p "$PKG_ROOT/Library/Audio/Plug-Ins/VST3"
mkdir -p "$PKG_ROOT/Applications"
mkdir -p "$OUT_DIR"

cp -R "$BUILD_DIR/SpectralRot_artefacts/Release/VST3/trukendoos.vst3" "$PKG_ROOT/Library/Audio/Plug-Ins/VST3/"

if [[ -d "$BUILD_DIR/SpectralRot_artefacts/Release/Standalone/trukendoos.app" ]]; then
  cp -R "$BUILD_DIR/SpectralRot_artefacts/Release/Standalone/trukendoos.app" "$PKG_ROOT/Applications/"
fi

mkdir -p "$PKG_ROOT/Users/Shared/trukendoos/Presets"

pkgbuild \
  --root "$PKG_ROOT" \
  --identifier "com.tremmaudio.trukendoos.$ARCH" \
  --version "0.1.5" \
  --install-location "/" \
  "$PKG_PATH"

rm -rf "$DMG_ROOT"
mkdir -p "$DMG_ROOT"
cp "$PKG_PATH" "$DMG_ROOT/"
cat > "$DMG_ROOT/README.txt" <<EOF
trukendoos macOS installer / updater

Run trukendoos-mac-$ARCH.pkg to install or update:
- VST3 plugin: /Library/Audio/Plug-Ins/VST3/trukendoos.vst3
- Standalone app, if built: /Applications/trukendoos.app

Existing trukendoos files at those paths are replaced in place. User presets are not removed.

This build is unsigned. If macOS blocks it, right-click the pkg and choose Open.
EOF

rm -f "$DMG_PATH"
hdiutil create \
  -volname "trukendoos $ARCH" \
  -srcfolder "$DMG_ROOT" \
  -ov \
  -format UDZO \
  "$DMG_PATH"

echo "Built $PKG_PATH"
echo "Built $DMG_PATH"
