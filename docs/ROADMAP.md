# ロードマップ

公式 VPN Server Manager の全画面を目指すが、一度には作れないためフェーズ分けする。
各フェーズは「動くもの」を積み上げる単位。JSON-RPC APIメソッド名は
[jsonrpc-api-reference.md](upstream-reference/jsonrpc-api-reference.md) 参照。

- [x] **Phase 0: 土台** — CMake + Qt6 プロジェクト雛形、Git、ドキュメント構成
- [ ] **Phase 1: 通信層** — 汎用JSON-RPC 2.0クライアント（自己署名証明書許容、認証ヘッダー、
      エラーコード変換）。疎通確認用に `Test` / `GetServerInfo` / `GetServerStatus` を実装
- [ ] **Phase 2: 接続管理** — 「新しい接続設定の作成」ダイアログ相当（接続設定の保存・一覧・
      編集・削除）、ログイン、メイン画面の骨格（サーバー全体管理 / 仮想HUB管理モードの分岐）
- [ ] **Phase 3: 仮想HUB一覧・基本操作** — `EnumHub` / `CreateHub` / `SetHub` / `GetHub` /
      `DeleteHub` / `SetHubOnline` / `GetHubStatus`。公式の仮想HUB一覧グリッド
      （HUB名/状態/種類/ユーザー数/グループ数/セッション数...）を再現
- [ ] **Phase 4: HUB管理ダイアログ（ユーザー/グループ）** — `CreateUser` / `SetUser` /
      `GetUser` / `DeleteUser` / `EnumUser`、`CreateGroup` / `SetGroup` / `GetGroup` /
      `DeleteGroup` / `EnumGroup`
- [ ] **Phase 5: セッション・テーブル管理** — `EnumSession` / `GetSessionStatus` /
      `DeleteSession`、`EnumMacTable` / `DeleteMacTable`、`EnumIpTable` / `DeleteIpTable`
- [ ] **Phase 6: アクセスリスト・カスケード接続** — `AddAccess` / `DeleteAccess` /
      `EnumAccess` / `SetAccessList`、`CreateLink` / `SetLink` / `GetLink` / `EnumLink` /
      `SetLinkOnline` / `SetLinkOffline` / `DeleteLink` / `RenameLink` / `GetLinkStatus`
- [ ] **Phase 7: SecureNAT・ローカルブリッジ** — `EnableSecureNAT` / `DisableSecureNAT` /
      `SetSecureNATOption` / `GetSecureNATOption` / `EnumNAT` / `EnumDHCP`、
      `EnumEthernet` / `AddLocalBridge` / `DeleteLocalBridge` / `EnumLocalBridge`
- [ ] **Phase 8: サーバー全体設定** — リスナー管理（`CreateListener` 等）、証明書
      （`SetServerCert` / `GetServerCert` / `RegenerateServerCert`）、暗号設定
      （`GetServerCipher` / `SetServerCipher`）、ログ閲覧（`EnumLogFile` / `ReadLogFile`）、
      syslog（`SetSysLog` / `GetSysLog`）
- [ ] **Phase 9: 拡張機能** — IPsec/L2TP（`SetIPsecServices` 等）、EtherIP/L2TPv3
      （`*EtherIpId`）、OpenVPN/SSTPクローン設定（`SetOpenVpnSstpConfig`）、
      DDNS（`GetDDnsClientStatus` 等）、VPN Azure、クラスタ/Farm設定、L3スイッチ
- [ ] **Phase 10: 仕上げ** — 日本語/英語ローカライズ（Qt Linguist、`strtable_ja/en.stb`を
      訳語リファレンスに）、アイコン/レイアウトの公式GUIへの見た目寄せ、
      macOS `.app` / Linux AppImage・deb パッケージング

## 現在地
Phase 0 完了。次はPhase 1（JSON-RPCクライアント）。
