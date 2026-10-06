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
linuxdeploy-plugin-qt を自動ダウンロードして使い、`dist/linux/*.AppImage`を生成する
想定のスクリプト。

**未検証:** こちらもmacdeployqt同様にLinux上での実行確認ができていない
(ベストエフォート)。専用アプリアイコンも未作成のため、デスクトップエントリの
`Icon=network-vpn`はfreedesktopの標準アイコン名へのフォールバックになっている
(linuxdeployがアイコンを見つけられない旨の警告を出す可能性があるが、動作自体には
影響しない見込み)。

## バージョン番号

現状`CPACK_PACKAGE_VERSION`は`CMakeLists.txt`内に`0.1.0`固定で書いてある。
リリースを切るようになったらGitタグ等と連動させることを検討する。
