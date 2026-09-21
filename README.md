# SoftEtherVPN-QtManager

[English README →](README-EN.md)

SoftEther VPN の公式管理GUI「VPN Server Manager」はWindows専用ですが、SoftEther VPN
Server自体が公開している JSON-RPC API を使って、Mac / Linux で動く同等の管理GUIを
Qt (C++) でゼロから作り直すプロジェクトです。

## 状態

🚧 開発中（Phase 0: プロジェクト雛形構築が完了）。まだ実用段階ではありません。
進捗は [docs/ROADMAP.md](docs/ROADMAP.md) を参照してください。
技術的な背景・採用理由は [HISTORY.md](HISTORY.md) を参照してください。

## 想定する使い方

SoftEther VPN Server（Linux/Mac/Windows問わず、JSON-RPC APIが有効なもの）に対して、
ホスト名・ポート・管理パスワードを指定して接続し、Windows版 VPN Server Manager と
同様の操作感で仮想HUB・ユーザー・グループ・セッションなどを管理できるようにする予定です。

## ビルド方法

```bash
brew install qt cmake   # 未導入の場合
cmake -S . -B build
cmake --build build
```

(実行可能なアプリケーションが揃うのはPhase 2以降の予定)

## 必要環境

- Qt 6
- CMake 3.20 以上
- C++20 対応コンパイラ（macOS: Xcode Command Line Tools / Linux: gcc or clang）
