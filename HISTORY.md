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

## 2026-10-06 Phase 10: 公式UIを見ながらのレイアウト再現

Win11 VM (RDP) で公式の「SoftEther VPN サーバー管理マネージャ」を開いて見比べながら、
ダイアログの配置を公式に寄せた。設定変更はせず、開く→確認→キャンセルのみ。

- **作業の進め方:** 公式ダイアログを開いてスクリーンショット → `strtable_ja.stb` の対応
  `PREFIX D_SM_*` ブロックで文言を確認 → Qt側を書き換え。Qt側の動作確認は、実サーバーの
  パスワードを使わずに済むよう、スクラッチパッドに置いたモックJSON-RPCサーバー
  (Python、自己署名TLS) に対して行った (リポジトリには含めていない)。
- **DDNSキー:** 公式は `GetConfig` で取得したコンフィグ本文の `DDnsClient` ブロックから
  `Key` を読み出して表示していた (推測どおり設定ファイルの直読み)。
- **アクセスリスト:** 公式は一覧への追加・編集・削除をメモリ上で行い、[保存] で
  `SetAccessList` にまとめて反映する方式。Qt側もこれに合わせた (従来は項目ごとにAPI呼び出し)。
- **カスケード接続のパスワード認証:** 公式の「標準パスワード認証」は `AuthType=1`
  (SHA-0ハッシュ、`SHA0(パスワード + 大文字ユーザー名)`) 。従来の実装は `AuthType=2`
  (平文、RADIUS/NT用) を送っていたため、SoftEther同士のカスケードでは認証に失敗しうる。
  Qtには SHA-0 がないため `util/Sha0.h` に自前実装 (SHA-0("abc") の既知値で検証済み)。
- **SecureNAT設定:** 従来は `ApplyDhcpPushRoutes_bool=false` / `DhcpPushRoutes_str=""` を
  固定で送っており、設定を保存すると既存の静的ルート (スプリットトンネリング設定) が消えて
  いた。プッシュルートの取得・編集・保存に対応して修正。
- **ユーザー/グループ/ポリシー:** `util/PolicyTable.h` (POL_n / POL_EX_n から生成) を元に
  セキュリティポリシー編集ダイアログを共通部品化 (ユーザー・グループ・カスケードで共用。
  カスケード用はユーザー専用ポリシーを除外)。ユーザー編集は認証6種・有効期限・グループ参照
  (グループ管理画面を「選択モード」で開く公式と同じ作り)に対応。
- **macOSのQFormLayout:** 既定では入力欄が広がらず公式の見た目にならないため、
  `QProxyStyle` で `SH_FormLayoutFieldGrowthPolicy` を差し替えた (main.cpp)。
- **画面キャプチャの注意:** computer-useのバックグラウンド操作 (`app_*`) はRDPウィンドウへ
  のクリックが届かないことがあり、その場合はフルスクリーン操作 (`computer_batch`) に切り替える。
  Qtアプリ側のテーブル行クリックは、ダイアログ(別ウィンドウ)上だと「別プロセスの要素」として
  拒否されることがあった (親ウィンドウ上のテーブルは問題なし)。
- **未実装として残したもの:** 証明書作成ツール、スマートカード、更新通知設定、
  「信頼する証明機関」から接続するカスケード接続の証明機関管理ボタン (カスケード編集内)。
- **モックサーバー:** 画面確認に使ったモックを `tools/mock_vpnserver.py` として同梱
  (値は架空、TLS証明書は起動時に openssl で自動生成)。使い方は `tools/README.md`。
- **中断メモ:** UI作業を一旦終了。次回は Phase 9 のダイアログ (IPsec/EtherIP/OpenVPN/DDNS/
  Azure/クラスタ/リスナー作成/ログファイル一覧) を公式VMと見比べるところから。
  カスケード接続の実機確認は実験環境がないため後回し。

## 2026-10-06 カスケード接続の実機確認で判明した不具合 (パスワードハッシュの連結順)

別回線の実機サーバー (v5.02) から自宅側 (v4.42) へのカスケード接続が「ユーザー認証に失敗しました
(コード 9)」になった。接続側ログでは認証方法は「パスワード認証」で通信自体は問題なし。
原因は `HashedPassword_bin` の計算順: JSON-RPC の API ドキュメントは
`SHA0(UpperCase(username) + password)` と記載しているが、本体ソース
(`src/Cedar/Account.c` の `HashPassword`) は **パスワード → 大文字ユーザー名** の順で連結する。
ドキュメントの記述に従って逆順で実装していたため、ハッシュが常に不一致になっていた。修正済み。
(ドキュメントの記述は信用せず、挙動に関わる箇所は本体ソースで確認すること。)
