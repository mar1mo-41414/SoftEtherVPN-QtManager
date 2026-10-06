# アーキテクチャと実装メモ

## 全体像

- **通信:** SoftEther VPN Server 公式の JSON-RPC 2.0 API (`https://<host>:<port>/api/`) をそのまま使う。
  認証は `X-VPNADMIN-HUBNAME` / `X-VPNADMIN-PASSWORD` ヘッダー。自己署名証明書を許容する。
  - `src/rpc/JsonRpcClient` : 汎用クライアント (プロキシ対応)
  - `src/rpc/VpnServerRpc` : 型付きラッパー + 任意メソッドを呼べる `call(method, params, ...)`
- **GUI:** Qt 6 Widgets。1つの `MainWindow` が3ページ (接続一覧 / サーバー管理 / 仮想HUB管理) を
  `QStackedWidget` で切り替え、各機能はダイアログとして開く。
  - 文言とレイアウトは公式 VPN Server Manager に合わせる (ダイアログ名 `D_SM_*` などは
    `docs/upstream-reference/strtable_ja.stb` の `PREFIX` ブロックに対応)。
- **画面の流れ:** 接続一覧 → (サーバー全体管理者) サーバー管理画面 → 仮想HUB管理画面。
  仮想HUB管理者として接続した場合はサーバー管理画面を飛ばし、サーバー全体の機能を無効にする。

## 実装上の方針

- **非同期RPCと寿命:** コールバックは `RpcUi::guarded(this, ...)` で包み、画面が破棄された後に
  応答が返っても何もしないようにする (定期更新のある画面で実際にクラッシュしたため)。
- **公式の挙動に合わせる箇所:** アクセスリスト / 接続元IP制限リストは、メモリ上で編集して
  「保存」で一括反映 (`SetAccessList` / `SetAcList`)。Azure はラジオ切り替えで即時反映。
  接続状況などは定期更新する。
- **ポリシー:** `src/util/PolicyTable.h` (文字列テーブルから生成) を共通部品
  (`PolicyDialog`) が使う。ユーザー・グループ・カスケード接続で共用。
- **生成ファイル:** `CapsLabels.h` / `ErrorStrings.h` / `HubOptionTexts.h` / `PolicyTable.h` は
  `strtable_ja.stb` からの生成物 (手編集しない)。

## APIドキュメントの落とし穴

`docs/upstream-reference/jsonrpc-api-reference.md` は本体の実装と食い違うことがある。
挙動に関わる箇所は本体ソース (`src/Cedar/*.c`) で確認すること。判明している例:

| 項目 | ドキュメント | 実際 |
| --- | --- | --- |
| 標準パスワード認証のハッシュ | `SHA0(UPPER(ユーザー名) + パスワード)` | `SHA0(パスワード + UPPER(ユーザー名))` (`HashPassword()`) |
| リンク状態の確立回数キー | `NumConnectionsEatablished_u32` | `NumConnectionsEstablished_u32` |
| ポリシーのキー (一部) | `SecPol_CheckMac_bool` 等 | `policy:CheckMac_bool` 等 |
| リンクの `NoTls1_bool` | 記載あり | RPC の Pack にキーが無くサーバーは無視する |
| OpenVPN のUDPポート一覧 | `OpenVPNPortList_str` | 最新ソースの `OpenVpnSstpConfig` には無い (4.x 系のみ) |

## テスト方法

- `tools/mock_vpnserver.py` : 架空データのモックサーバー (レイアウト確認用、書き込みは何もしない)。
- 実サーバーでの確認は、副作用が小さい操作から行うこと (テスト専用サーバー推奨)。
- GUI の見た目は、Windows 版公式 Manager を開いて同じ画面と見比べて合わせている。
