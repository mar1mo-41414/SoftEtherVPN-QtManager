#!/usr/bin/env bash
# Linux向けAppImageを作る。linuxdeploy + linuxdeploy-plugin-qt を使う。
# x86_64 / aarch64 両対応 (uname -mで自動判定)。
#
# 使い方 (Linux上で):
#   packaging/linux/build-appimage.sh
#
# 成果物: dist/linux/SoftEtherVPN-QtManager-<arch>.AppImage
#
# 実機検証: marnux (Linux Mint 22, x86_64) で動作確認済み(2026-10-06)。aarch64は
# GitHub Actions (ubuntu-*-arm) でのビルドのみ、実機起動確認はまだ。
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="$ROOT/build-release"
DIST_DIR="$ROOT/dist/linux"
APP_NAME="SoftEtherVPN-QtManager"
TOOLS_DIR="$ROOT/.packaging-tools"
ARCH="$(uname -m)"

mkdir -p "$TOOLS_DIR" "$DIST_DIR"

fetch_tool() {
    local name="$1" url="$2"
    local path="$TOOLS_DIR/$name"
    if [ ! -x "$path" ]; then
        curl -fL -o "$path" "$url"
        chmod +x "$path"
    fi
    echo "$path"
}

LINUXDEPLOY=$(fetch_tool "linuxdeploy-$ARCH.AppImage" \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage")
LINUXDEPLOY_PLUGIN_QT=$(fetch_tool "linuxdeploy-plugin-qt-$ARCH.AppImage" \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage")

cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release -j

APPDIR="$BUILD_DIR/AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/applications"

cp "$BUILD_DIR/$APP_NAME" "$APPDIR/usr/bin/"
cp "$ROOT/packaging/linux/softethervpn-qtmanager.desktop" "$APPDIR/usr/share/applications/"

export QML_SOURCES_PATHS="$ROOT/src"
# qmakeがqtchooser経由でQt5を指している環境があるため、Qt6のqmakeを明示指定する。
export QMAKE="$(command -v qmake6 || command -v qmake)"
"$LINUXDEPLOY" --appdir "$APPDIR" \
    --desktop-file "$ROOT/packaging/linux/softethervpn-qtmanager.desktop" \
    --icon-file "$ROOT/packaging/icon/hicolor/256x256/apps/softethervpn-qtmanager.png" \
    --plugin qt \
    --output appimage

mv "$APP_NAME"*.AppImage "$DIST_DIR/" 2>/dev/null || mv ./*.AppImage "$DIST_DIR/"

echo "done: see $DIST_DIR/"
