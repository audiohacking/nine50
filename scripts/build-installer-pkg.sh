#!/usr/bin/env bash
# Build a macOS .pkg installer for NINE50 (AU + VST3) from the current build tree.
# Run from repo root after: cmake --build build --target NINE50
#
# Usage:
#   ./scripts/build-installer-pkg.sh [--sign-plugins] [--version 1.0.0]
#
# Output: release-artefacts/NINE50-macOS-Installer.pkg
# Install location:
#   /Library/Audio/Plug-Ins/Components/NINE50.component
#   /Library/Audio/Plug-Ins/VST3/NINE50.vst3

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

PLUGIN_AU_NAME="NINE50.component"
PLUGIN_VST3_NAME="NINE50.vst3"
PKG_IDENTIFIER="com.audiohacking.nine50"

SIGN_PLUGINS=false
PKG_VERSION="1.0.0"

while [ $# -gt 0 ]; do
  case "$1" in
    --sign-plugins) SIGN_PLUGINS=true; shift ;;
    --version)
      PKG_VERSION="${2#v}"
      shift 2
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

# JUCE places plugin bundles under build/Source/NINE50_artefacts/
ARTEFACTS_DIR="${REPO_ROOT}/build/Source/NINE50_artefacts"
AU_PATH="${ARTEFACTS_DIR}/AU/${PLUGIN_AU_NAME}"
VST3_PATH="${ARTEFACTS_DIR}/VST3/${PLUGIN_VST3_NAME}"

if [ ! -d "$AU_PATH" ] || [ ! -d "$VST3_PATH" ]; then
  echo "Error: Plugin bundles not found. Build first:" >&2
  echo "  cmake -B build -DCMAKE_BUILD_TYPE=Release" >&2
  echo "  cmake --build build --target NINE50 -j8" >&2
  echo "" >&2
  echo "Looking in: ${ARTEFACTS_DIR}" >&2
  find "${ARTEFACTS_DIR}" -maxdepth 3 -type d \( \
    -name "*.component" -o -name "*.vst3" \
    \) 2>/dev/null || echo "(no bundles found)"
  exit 1
fi

echo "Using AU:   ${AU_PATH}"
echo "Using VST3: ${VST3_PATH}"

mkdir -p release-artefacts
rm -rf \
  "release-artefacts/${PLUGIN_AU_NAME}" \
  "release-artefacts/${PLUGIN_VST3_NAME}"
cp -R "$AU_PATH" "release-artefacts/${PLUGIN_AU_NAME}"
cp -R "$VST3_PATH" "release-artefacts/${PLUGIN_VST3_NAME}"

if [ "$SIGN_PLUGINS" = true ]; then
  echo "Ad-hoc signing plugin bundles..."
  xcrun codesign --force --sign - --deep "release-artefacts/${PLUGIN_AU_NAME}"
  xcrun codesign --force --sign - --deep "release-artefacts/${PLUGIN_VST3_NAME}"
fi

rm -rf payload
mkdir -p payload/Library/Audio/Plug-Ins/Components
mkdir -p payload/Library/Audio/Plug-Ins/VST3
cp -R "release-artefacts/${PLUGIN_AU_NAME}" "payload/Library/Audio/Plug-Ins/Components/"
cp -R "release-artefacts/${PLUGIN_VST3_NAME}" "payload/Library/Audio/Plug-Ins/VST3/"

pkgbuild \
  --root payload \
  --identifier "$PKG_IDENTIFIER" \
  --version "$PKG_VERSION" \
  --install-location / \
  "release-artefacts/NINE50-macOS-Installer.pkg"

rm -rf payload

echo ""
echo "Created release-artefacts/NINE50-macOS-Installer.pkg (version ${PKG_VERSION})"
echo ""
echo "Install:"
echo "  sudo installer -pkg release-artefacts/NINE50-macOS-Installer.pkg -target /"
echo "Or open the .pkg in Finder for a GUI install."
