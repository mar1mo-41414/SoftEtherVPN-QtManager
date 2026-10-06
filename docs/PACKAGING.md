# パッケージング

配布用バイナリの作り方。いずれもRelease設定でビルドし、`build/`とは別の
`build-release/`ディレクトリを使う(通常の開発用ビルドと混ざらないように)。

## macOS: `.app` / zip

```bash
packaging/macos/build-app.sh
```

`cmake --build` でReleaseビルドした後、`macdeployqt`でQtのフレームワーク一式を
`.app`バンドルに同梱し、Homebrew等でQtを別途入れていない環境でもそのまま動く
自己完結した`.app`にする。`dist/macos/SoftEtherVPN-QtManager.app`と、配布しやすい
ように固めた`dist/macos/SoftEtherVPN-QtManager-macos.zip`を生成する。

- 開発機 (Apple Silicon, Qt 6.11.1 Homebrew版) で実行・動作確認済み(2026-10-06)。
- Apple Developer ID での署名・公証(notarization)は行っていない。未署名のため、
  他のMacで初回起動時にGatekeeperの警告が出る(右クリック→開く、または
  システム設定の「セキュリティとプライバシー」から許可する必要がある)。
  署名が必要になったら`macdeployqt -codesign=<identity>`相当の対応を追加する。

### Intel Mac (`p_mac`) でのビルド

Intel Mac版は `p_mac` (proxMac、常時稼働) で手動ビルドする運用。この機は macOS 12.7.6
(Monterey) で、**Homebrewが既にIntel Macのサポートを終了しておりインストールできない**
(2026-10-06時点)。代わりに以下の手順でビルドする:

```bash
# 1. Homebrewの代わりにpipでQt/CMakeを導入 (sudo不要)
pip3 install --user aqtinstall cmake
export PATH="$HOME/Library/Python/3.9/bin:$PATH"   # Pythonバージョンは環境に合わせる

# 2. Qt本体を導入。6.11系はホストツール(lrelease等)がmacOS 13以降必須でこの機では動かず、
#    6.5.3はmoc(Meta-Object Compiler)がXcodeのSDK(libc++の<concepts>ヘッダー)を
#    パースできずビルド自体が失敗する。6.7.3は両方の問題を回避できることを確認済み。
aqt install-qt mac desktop 6.7.3 clang_64 -O ~/Qt -m qt5compat

# 3. ビルド (CMAKE_PREFIX_PATHでaqt版Qtを明示)
CMAKE_PREFIX_PATH=~/Qt/6.7.3/macos packaging/macos/build-app.sh
```

実機(p_mac, x86_64)でビルド・起動確認済み(2026-10-06)。将来Qtをバージョンアップする際は
上記2点(ホストツールのmacOSバージョン要件、mocのconcepts構文パース可否)を再度確認すること。

## Linux: `.deb`

CMakeのCPack (DEBジェネレータ) を使う。`CMakeLists.txt`に`UNIX AND NOT APPLE`限定で
設定済み。

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j
cmake --build build-release --target package
# または: (cd build-release && cpack)
```

`build-release/softethervpn-qtmanager_0.1.0_<arch>.deb` が生成される。
実行ファイルは`/usr/bin/`に、デスクトップエントリ
(`packaging/linux/softethervpn-qtmanager.desktop`)は`/usr/share/applications/`に
インストールされる。Qtランタイム(`libqt6widgets6`等)は`CPACK_DEBIAN_PACKAGE_SHLIBDEPS`
により自動的に依存関係として検出される想定。

**未検証:** この項目は開発機がmacOSのみのため、実際にLinux上でビルド・インストール
確認はできていない。Linux環境で試す際は依存解決やパス周りで調整が要るかもしれない。

## Linux: AppImage

```bash
packaging/linux/build-appimage.sh
```

[linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) と
linuxdeploy-plugin-qt を自動ダウンロードして使い、`dist/linux/SoftEtherVPN-QtManager-<arch>.AppImage`
を生成する(`uname -m`でx86_64/aarch64を自動判定)。

- marnux (Linux Mint 22, x86_64) で実機ビルド・起動確認済み(2026-10-06)。
  `qt6-base-dev` `qt6-base-dev-tools` `qt6-l10n-tools` `qt6-tools-dev` が必要
  (`qt6-l10n-tools`だけだとQt6LinguistToolsのCMake Configが無く`cmake`の段階で失敗する)。
- `qmake`が`qtchooser`経由でQt5を指す環境があるため、スクリプト内で`QMAKE`環境変数に
  `qmake6`を明示指定している。
- aarch64 (Linux arm64) はGitHub Actions (`ubuntu-24.04-arm`) でのビルドのみ確認、
  実機での起動確認はまだ。

## GitHub Actions: 自動リリース

`.github/workflows/release.yml` が `vX.Y.Z` 形式のタグをpushすると自動的に以下を
ビルドしてGitHub Releaseに添付する:

- Linux x86_64 / arm64 の AppImage (`ubuntu-24.04` / `ubuntu-24.04-arm`)

**macOS (arm64 / x86_64 とも) は現状CIに含めず、手動ビルドで対応している。**
`macos-14`ランナーで`cmake --build`を回すと、全ソースのコンパイルが終わった直後
(最終リンクの直前)で毎回ハングする現象を2026-10-06に2回連続で確認した
(ローカルのApple Silicon実機では同じコードが1〜2分で完走しており、再現しない)。
GitHub Actions側も当時「macOS arm64ランナーは容量不足でキューイングが長引く場合がある」
と案内しており、こちらのビルド手順の問題かCI環境固有の問題か切り分けできていない。
無理に自動化を追わず、`packaging/macos/build-app.sh`で実機ビルドしたものを
Releaseページに手動でアップロードする運用とした。原因の見当がついたら自動化に戻す。

## バージョン番号

`CMakeLists.txt`の`project(SoftEtherVPN-QtManager VERSION x.y.z ...)`が単一の情報源。
ここから`src/Version.h.in`経由でビルド時に`Version.h`が生成され(`SOFTETHERVPN_QTMANAGER_VERSION`マクロ)、
アプリ内のバージョン情報ダイアログ・`CPACK_PACKAGE_VERSION`・macOSバンドルの
`CFBundleShortVersionString`/`CFBundleVersion`すべてがここを参照する。リリースの際は
このバージョンをGitタグ(`vX.Y.Z`)と合わせて更新すること。
