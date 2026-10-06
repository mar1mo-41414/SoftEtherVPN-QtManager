#!/usr/bin/env bash
# macOS向け配布用 .app を作る。Releaseビルド + macdeployqtでQtフレームワークを
# バンドルに同梱し、Homebrew Qt未導入の環境でも動く自己完結アプリにする。
#
# 使い方:
#   packaging/macos/build-app.sh
#   # HomebrewでQtを入れていない環境 (例: aqtinstallで導入した場合) は明示指定:
#   CMAKE_PREFIX_PATH=~/Qt/6.7.3/macos packaging/macos/build-app.sh
#
# 成果物: dist/macos/SoftEtherVPN-QtManager.app, dist/macos/SoftEtherVPN-QtManager-macos.zip
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="$ROOT/build-release"
DIST_DIR="$ROOT/dist/macos"
APP_NAME="SoftEtherVPN-QtManager"

CMAKE_PREFIX_PATH_ARGS=()
if [ -n "${CMAKE_PREFIX_PATH:-}" ]; then
    CMAKE_PREFIX_PATH_ARGS=(-DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH")
fi

cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release "${CMAKE_PREFIX_PATH_ARGS[@]}"
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

# macdeployqtがフレームワーク内dylibのrpathをinstall_name_toolで書き換える際、
# 元の(リンク時に自動付与された)署名が無効化されてしまう。再署名しないまま配布すると
# 「このアプリケーションは、必要なコードシグネチャを持っていません」的なSIGKILLで
# 起動時に即クラッシュする(実機で確認: EXC_BAD_ACCESS/SIGKILL Code Signature Invalid)。
# Developer ID等は無いためad-hoc(`-s -`)で再署名するだけで起動できるようになる。
codesign --force --deep -s - "$DIST_DIR/$APP_NAME.app"
codesign --verify --deep --strict "$DIST_DIR/$APP_NAME.app"

(cd "$DIST_DIR" && ditto -c -k --sequesterRsrc --keepParent "$APP_NAME.app" "$APP_NAME-macos.zip")

echo "done: $DIST_DIR/$APP_NAME.app"
echo "done: $DIST_DIR/$APP_NAME-macos.zip"
