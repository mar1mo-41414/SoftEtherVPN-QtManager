# SoftEtherVPN-QtManager

[English README →](README-EN.md)

SoftEther VPN の公式管理GUI「VPN Server Manager」はWindows専用ですが、SoftEther VPN
Server自体が公開している JSON-RPC API を使って、Mac / Linux で動く同等の管理GUIを
Qt (C++) でゼロから作り直すプロジェクトです。文言・画面レイアウトは公式GUIにできる限り
合わせています。

## 状態

🚧 開発中。主要画面は一通り動作しますが、まだ実用段階ではありません。
進捗は [docs/ROADMAP.md](docs/ROADMAP.md) を参照してください。
アーキテクチャや実装上の注意点は [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)、
技術的な経緯は [HISTORY.md](HISTORY.md) を参照してください。

## できること

SoftEther VPN Server（Linux/Mac/Windows問わず、JSON-RPC APIが有効なもの）に対して、
ホスト名・ポート・管理パスワードを指定して接続し、Windows版 VPN Server Manager と
同様の操作感で管理できます。

- 接続設定の管理、サーバー全体の管理（リスナー、暗号化設定、クラスタリング、
  DDNS、VPN Azure、IPsec/L2TP、EtherIP/L2TPv3、OpenVPN/SSTP 等）
- 仮想HUBの管理（ユーザー・グループ・セキュリティポリシー・アクセスリスト・
  カスケード接続・SecureNAT・ローカルブリッジ・レイヤ3スイッチ 等）
- セッション・MAC/IPテーブルの表示、ログファイルのダウンロード 等

## ダウンロード

ビルド済みバイナリは [Releases](../../releases) から取得できます！

## ビルド方法

```bash
brew install qt cmake   # 未導入の場合 (Linuxはディストリのパッケージマネージャで同等のものを)
cmake -S . -B build
cmake --build build
```

ビルドしたアプリは `build/SoftEtherVPN-QtManager`（macOSは `.app` バンドル）です。

配布用パッケージ（macOS `.app`/zip、Linux `.deb`・AppImage）の作り方は
[docs/PACKAGING.md](docs/PACKAGING.md) を参照してください。

画面確認だけしたい場合は、実サーバーの代わりに付属のモックサーバー
（[tools/README.md](tools/README.md)）を使えます。

## 必要環境

- Qt 6
- CMake 3.20 以上
- C++20 対応コンパイラ（macOS: Xcode Command Line Tools / Linux: gcc or clang）

## ライセンス

[Apache License 2.0](LICENSE)。本プロジェクトは SoftEther VPN 本体の著作権者とは
無関係の独立した再実装であり、公式プロジェクトの支援や承認を受けたものではありません。
`docs/upstream-reference/` 以下の資料は SoftEther VPN プロジェクト (Apache License 2.0)
由来のもので、詳細は [NOTICE](NOTICE) を参照してください。
