#!/usr/bin/env bash
# macOS向け配布用 .app を作る。Releaseビルド + macdeployqtでQtフレームワークを
# バンドルに同梱し、Homebrew Qt未導入の環境でも動く自己完結アプリにする。
#
# 使い方:
#   packaging/macos/build-app.sh
#
# 成果物: dist/macos/SoftEtherVPN-QtManager.app, dist/macos/SoftEtherVPN-QtManager-macos.zip
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="$ROOT/build-release"
DIST_DIR="$ROOT/dist/macos"
APP_NAME="SoftEtherVPN-QtManager"

cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release -j

APP_BUNDLE="$BUILD_DIR/$APP_NAME.app"
if [ ! -d "$APP_BUNDLE" ]; then
    echo "error: $APP_BUNDLE が見つかりません" >&2
    exit 1
fi

rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"
cp -R "$APP_BUNDLE" "$DIST_DIR/"

macdeployqt "$DIST_DIR/$APP_NAME.app"

(cd "$DIST_DIR" && ditto -c -k --sequesterRsrc --keepParent "$APP_NAME.app" "$APP_NAME-macos.zip")

echo "done: $DIST_DIR/$APP_NAME.app"
echo "done: $DIST_DIR/$APP_NAME-macos.zip"
