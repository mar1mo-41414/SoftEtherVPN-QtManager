#!/usr/bin/env bash
# Linux向けAppImageを作る。linuxdeploy + linuxdeploy-plugin-qt を使う。
#
# 注意: このプロジェクトは現状macOS環境でのみ開発・検証しているため、このスクリプト自体は
# Linux上で未実行・未検証 (ベストエフォート)。Linux環境で実行して問題があれば調整すること。
#
# 使い方 (Linux上で):
#   packaging/linux/build-appimage.sh
#
# 成果物: dist/linux/SoftEtherVPN-QtManager-x86_64.AppImage
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="$ROOT/build-release"
DIST_DIR="$ROOT/dist/linux"
APP_NAME="SoftEtherVPN-QtManager"
TOOLS_DIR="$ROOT/.packaging-tools"

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

LINUXDEPLOY=$(fetch_tool linuxdeploy-x86_64.AppImage \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage")
LINUXDEPLOY_PLUGIN_QT=$(fetch_tool linuxdeploy-plugin-qt-x86_64.AppImage \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage")

cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release -j

APPDIR="$BUILD_DIR/AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/applications"

cp "$BUILD_DIR/$APP_NAME" "$APPDIR/usr/bin/"
cp "$ROOT/packaging/linux/softethervpn-qtmanager.desktop" "$APPDIR/usr/share/applications/"

export QML_SOURCES_PATHS="$ROOT/src"
"$LINUXDEPLOY" --appdir "$APPDIR" \
    --desktop-file "$ROOT/packaging/linux/softethervpn-qtmanager.desktop" \
    --plugin qt \
    --output appimage

mv "$APP_NAME"*.AppImage "$DIST_DIR/" 2>/dev/null || mv ./*.AppImage "$DIST_DIR/"

echo "done: see $DIST_DIR/"
