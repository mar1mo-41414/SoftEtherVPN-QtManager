# HISTORY

## 2026-09-21 プロジェクト開始・技術選定

### 経緯
SoftEther VPNの公式管理GUI「VPN Server Manager」はWindows専用。ソースコードは
[SoftEtherVPN/SoftEtherVPN](https://github.com/SoftEtherVPN/SoftEtherVPN) で公開されているが、
実体である `src/Cedar/SM.c`（約44万バイト）は冒頭から末尾まで丸ごと `#ifdef OS_WIN32` で
囲われており、ダイアログは生のWin32 API（HWND / CreateWindow / .rcリソース）で直接実装されている。
Qt・GTK・wxWidgetsのようなクロスプラットフォームツールキットは一切使われていないため、
「移植」は事実上不可能で、GUI層を丸ごと新規に作り直す以外の選択肢がないと判断した。

### 検討して却下した代替案
- **`vpncmd`（公式CLI）をそのまま使う** — GUIが欲しいという要望の核心と矛盾するため却下。
- **Windows機からリモート管理** — 運用上は可能だが、Mac側で日本語入力（半角/全角キーが無い）
  ができず実用に耐えない。
- **Wine上でWindows版Managerを動かす** — 動作はするが、UIが英語表記に固定され日本語指定が
  効かない。
- **Web UIとして自作** — クロスプラットフォームだが「公式GUIそのまま」の操作感が得られない
  ため、できれば避けたい。

### 採用した方針
- **通信層:** SoftEther VPN Serverには公式のJSON-RPC 2.0 API
  （`https://<host>:<port>/api/` へPOST、認証は `X-VPNADMIN-HUBNAME` /
  `X-VPNADMIN-PASSWORD` ヘッダーのみ）が用意されている
  （`developer_tools/vpnserver-jsonrpc-clients/README.md` 参照）。`vpncmd` や
  Windows版Managerが内部で使う独自バイナリプロトコル（PACK形式）を自前で解析する必要が
  なく、この公式APIをそのまま使う。API一覧は
  [docs/upstream-reference/jsonrpc-api-reference.md](docs/upstream-reference/jsonrpc-api-reference.md)
  に保存済み（Apache-2.0ライセンスの上流ドキュメントをそのまま複製、著作権表示は保持）。
- **GUIツールキット:** Qt 6（C++）を採用。Mac/Linuxを1つのコードベースでカバーでき、
  日本語IME・フォント描画は標準で問題なく動作する（Wineで発生した「日本語固定不可」問題を
  そもそも回避できる）。
- **用語の一致:** SoftEther本体の文字列テーブル `strtable_ja.stb` / `strtable_en.stb`
  （Apache-2.0、`src/bin/hamcore/`）には公式GUIで使われている正式な日本語/英語の用語が
  ほぼ全て収録されている（例: `SM_HUB_COLUMN_1` = "仮想 HUB 名" 等、`SM_` 接頭辞のキーだけで
  499件）。これを `docs/upstream-reference/` に保存し、UI文言を書く際の正式な訳語リファレンス
  として参照する。ダイアログのコントロール文言自体はこのテーブルには含まれない
  （テーブル冒頭コメント参照）ため、そちらは `src/PenCore/PenCore.rc` の実装を読みながら
  意味を汲み取って独自に文言を起こす。
- **スコープ:** 公式GUIの全画面を最初から目指す方針（v1から段階的機能限定はしない）。
  ただし現実的に一度には作り切れないため、[docs/ROADMAP.md](docs/ROADMAP.md) でフェーズ分けし、
  フェーズごとに動くものを積み上げていく。

### 開発環境（確認済み）
- macOS, Xcode Command Line Tools あり
- Homebrew経由で Qt 6.11.1, CMake 4.4.3 導入済み

## 2026-09-21 Phase 1〜2 実機確認

実機テスト用サーバーは nux2 (Gitea稼働機) 上のSoftEther VPN Server。
**ポート443は使えない** (Pangolinがリバースプロキシとして掌握しているため)。
テストには **SE-Port の 5555** を使うこと。

Phase 1 (JSON-RPCクライアント疎通、EnumHubによるHUB一覧表示) 、Phase 2 (接続設定の
作成・編集・削除、パスワード入力プロンプト、実際の接続) とも nux2:5555 に対して
動作確認済み。
