# ロードマップ

公式 VPN Server Manager の全画面を目指すが、一度には作れないためフェーズ分けする。
各フェーズは「動くもの」を積み上げる単位。JSON-RPC APIメソッド名は
[jsonrpc-api-reference.md](upstream-reference/jsonrpc-api-reference.md) 参照。

- [x] **Phase 0: 土台** — CMake + Qt6 プロジェクト雛形、Git、ドキュメント構成
- [x] **Phase 1: 通信層** — 汎用JSON-RPC 2.0クライアント（自己署名証明書許容、認証ヘッダー、
      エラーコード変換）。疎通確認用に `Test` / `GetServerInfo` / `GetServerStatus` を実装。
      実サーバー(nux2)への接続・`EnumHub`によるHUB一覧表示まで実機確認済み(2026-09-21)
- [x] **Phase 2: 接続管理** — 「新しい接続設定の作成」ダイアログ相当（接続設定の保存・一覧・
      編集・削除）、ログイン、メイン画面の骨格（サーバー全体管理 / 仮想HUB管理モードの分岐）。
      D_SM_MAIN / D_SM_EDIT_SETTING の文言を再現。実機(nux2:5555)で作成・編集・削除・
      パスワード入力接続まで確認済み(2026-09-21)。プロキシ経由接続(D_SM_EDIT_SETTINGの
      STATIC8以降)は未対応、必要になったらPhase 2.1として追加
- [x] **Phase 3: 仮想HUB一覧・基本操作** — `EnumHub` / `CreateHub` / `SetHub` / `GetHub` /
      `DeleteHub` / `SetHubOnline` / `GetHubStatus`。D_SM_SERVER/D_SM_EDIT_HUB/D_SM_STATUS
      の文言を再現し、仮想HUBの作成・プロパティ編集・オンライン/オフライン切替・状態表示・
      削除を実装。実機(nux2:5555)でテストHUBを作成して一通り確認済み、Windows版公式Manager
      と同時に見比べても差異なし(2026-09-21)。仮想HUB管理オプション/接続元IP制限リスト/
      拡張オプション/メッセージ設定/クラスタリング(スタティック・ダイナミック)は
      D_SM_EDIT_HUBの範囲外として後続フェーズへ
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
Phase 3 完了・実機確認済み（nux2:5555、Windows版公式Managerとの比較確認済み）。次はPhase 4（HUB管理ダイアログのユーザー/グループ）。
