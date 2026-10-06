# ロードマップ

公式 VPN Server Manager の全画面を目指すが、一度には作れないためフェーズ分けする。
各フェーズは「動くもの」を積み上げる単位。JSON-RPC APIメソッド名は
[jsonrpc-api-reference.md](upstream-reference/jsonrpc-api-reference.md) 参照。

- [x] **Phase 0: 土台** — CMake + Qt6 プロジェクト雛形、Git、ドキュメント構成
- [x] **Phase 1: 通信層** — 汎用JSON-RPC 2.0クライアント（自己署名証明書許容、認証ヘッダー、
      エラーコード変換）。疎通確認用に `Test` / `GetServerInfo` / `GetServerStatus` を実装。
      実サーバーへの接続・`EnumHub`によるHUB一覧表示まで実機確認済み(2026-09-21)
- [x] **Phase 2: 接続管理** — 「新しい接続設定の作成」ダイアログ相当（接続設定の保存・一覧・
      編集・削除）、ログイン、メイン画面の骨格（サーバー全体管理 / 仮想HUB管理モードの分岐）。
      D_SM_MAIN / D_SM_EDIT_SETTING の文言を再現。実機(テスト用サーバー v4.42)で作成・編集・削除・
      パスワード入力接続まで確認済み(2026-09-21)。プロキシ経由接続(D_SM_EDIT_SETTINGの
      STATIC8以降)は未対応、必要になったらPhase 2.1として追加
- [x] **Phase 3: 仮想HUB一覧・基本操作** — `EnumHub` / `CreateHub` / `SetHub` / `GetHub` /
      `DeleteHub` / `SetHubOnline` / `GetHubStatus`。D_SM_SERVER/D_SM_EDIT_HUB/D_SM_STATUS
      の文言を再現し、仮想HUBの作成・プロパティ編集・オンライン/オフライン切替・状態表示・
      削除を実装。実機(テスト用サーバー v4.42)でテストHUBを作成して一通り確認済み、Windows版公式Manager
      と同時に見比べても差異なし(2026-09-21)。仮想HUB管理オプション/接続元IP制限リスト/
      拡張オプション/メッセージ設定/クラスタリング(スタティック・ダイナミック)は
      D_SM_EDIT_HUBの範囲外として後続フェーズへ
- [x] **Phase 4: HUB管理ダイアログ（ユーザー/グループ）** — `CreateUser` / `SetUser` /
      `GetUser` / `DeleteUser` / `EnumUser`、`CreateGroup` / `SetGroup` / `GetGroup` /
      `DeleteGroup` / `EnumGroup`。D_SM_HUB(仮想HUB管理の入口)・D_SM_USER/D_SM_EDIT_USER・
      D_SM_GROUP/D_SM_EDIT_GROUPの文言を再現。仮想HUB一覧から「仮想 HUB の管理」で
      D_SM_HUB画面に入る導線を追加、仮想HUB管理モード接続時はここに直接入る。
      認証方法はパスワード認証のみ対応(証明書/RADIUS/NT認証・セキュリティポリシー・
      有効期限は未対応、後続フェーズへ)。実機でユーザー/グループの作成・編集・削除、
      実機とのデータ整合性まで確認済み(2026-09-21)
- [x] **Phase 5: セッション・テーブル管理** — `EnumSession` / `GetSessionStatus` /
      `DeleteSession`、`EnumMacTable` / `DeleteMacTable`、`EnumIpTable` / `DeleteIpTable`。
      D_SM_SESSION/D_SM_MAC/D_SM_IPの文言・列構成を再現。HUB管理画面の「セッションの管理」
      から遷移、セッション単位でのMAC/IPテーブル絞り込み表示にも対応。実機でセッション
      一覧・情報表示・切断・MAC/IPテーブル表示/削除まで確認済み(2026-09-21)。転送バイト数は
      ポーリングタイミングの差でWindows版と完全一致はしないが正常範囲
- [x] **Phase 6: アクセスリスト・カスケード接続** — `AddAccess` / `DeleteAccess` /
      `EnumAccess`、`CreateLink` / `SetLink` / `GetLink` / `EnumLink` /
      `SetLinkOnline` / `SetLinkOffline` / `DeleteLink` / `RenameLink` / `GetLinkStatus`。
      D_SM_ACCESS_LIST/D_SM_EDIT_ACCESS・D_SM_LINKの文言を再現。アクセスリストは
      IPv4のみ対応(IPv6・MACヘッダフィルタ・TCP状態検査・リダイレクト・
      遅延/パケットロスシミュレーションは未対応)、編集はDeleteAccess+AddAccessで実現。
      カスケード接続は匿名/パスワード認証のみ対応(証明書認証・プロキシ経由・
      セキュリティポリシーは未対応)。`SetAccessList`(一括置換)は未使用。
      実機でアクセスリスト・カスケード接続とも一通り確認済み(2026-09-21)
- [x] **Phase 7: SecureNAT・ローカルブリッジ** — `EnableSecureNAT` / `DisableSecureNAT` /
      `SetSecureNATOption` / `GetSecureNATOption` / `GetSecureNATStatus` / `EnumNAT` /
      `EnumDHCP`、`EnumEthernet` / `AddLocalBridge` / `DeleteLocalBridge` / `EnumLocalBridge`。
      D_SM_SNAT/D_NM_OPTION/D_NM_NAT/D_NM_DHCP(HUB管理画面から)・D_SM_BRIDGE(仮想HUB
      一覧画面から、サーバー全体機能)の文言を再現。SecureNAT無効時は設定/状況表示系
      ボタンをグレーアウトする公式Managerの挙動も再現(ユーザー指摘により追加)。
      静的ルーティングテーブルのプッシュ・タグVLANパケット透過設定ツールは未対応。
      実機でSecureNAT・ローカルブリッジとも動作確認済み(2026-09-21)
- [x] **Phase 8: サーバー全体設定** — リスナー管理（`CreateListener` / `EnumListener` /
      `DeleteListener` / `EnableListener`）、証明書（`SetServerCert` / `GetServerCert` /
      `RegenerateServerCert`）、暗号設定（`GetServerCipher` / `SetServerCipher`）、
      ログ閲覧（`EnumLogFile` / `ReadLogFile`）、syslog（`SetSysLog` / `GetSysLog`）。
      D_SM_SSL(暗号化と通信関係の設定に証明書+暗号+syslogを統合)・D_SM_CREATE_LISTENER・
      D_SM_LOG_FILE・D_CERTの文言を再現。証明書インポートはSoftEther独自のDER形式
      (.cer/.key)のみ対応(PEM形式は未対応)。インターネット接続維持機能・管理者
      パスワード変更・VPN over ICMP/DNS設定は未対応(後続フェーズへ)。
      実機で一通り確認済み(2026-09-21)
- [x] **Phase 9: 拡張機能** — IPsec/L2TP（`SetIPsecServices` 等）、EtherIP/L2TPv3
      （`*EtherIpId`）、OpenVPN/SSTPクローン設定（`SetOpenVpnSstpConfig`）、
      DDNS（`GetDDnsClientStatus` 等）、VPN Azure、クラスタ/Farm設定、L3スイッチ。
      D_SM_IPSEC/D_SM_ETHERIP(_ID)/D_SM_OPENVPN/D_SM_DDNS/D_SM_PROXY/D_SM_AZURE/D_SM_FARM
      (_MEMBER)/D_SM_L3(_ADD/_SW/_SW_IF/_SW_TABLE)/D_SM_SPECIALLISTENER/D_SM_CHANGE_PASSWORDを再現。
      Phase 8で見送ったインターネット接続維持機能・管理者パスワード変更・VPN over ICMP/DNS設定も
      D_SM_SSLに統合。仮想HUB一覧画面のボタンを「サーバー情報の参照および設定」グループへ再編。
      EtherIP定義の編集は削除+再追加。SOCKS4/5はAPI上区別されないため単一選択肢。
      実機で一通り確認済み(2026-10-06)
- [ ] **Phase 10: 仕上げ** — 公式UIとのレイアウト合わせ込み (Windows版公式Managerを見ながら)、
      日本語/英語ローカライズ（Qt Linguist、`strtable_ja/en.stb`を訳語リファレンスに）、
      macOS `.app` / Linux AppImage・deb パッケージング
  - [x] ローカライズ基盤: Qt Linguist (lupdate/lrelease) 導入、`tools/gen_translations.py`で
        strtable_ja/en.stbから自動翻訳流し込み (1634件中1296件=約79%)、OSロケール連動の自動切替。
        残りはアプリ独自文言・長文説明文などで継続作業中
  - [x] パッケージング: macOS `.app`/zip (`packaging/macos/build-app.sh`、実機確認済み)、
        Linux `.deb` (CPack) / AppImage (`packaging/linux/build-appimage.sh`、
        marnuxのLinux Mint 22 x86_64で実機ビルド・起動確認済み、arm64はCIのみ)。
        専用アプリアイコン作成済み(`packaging/icon/`)。詳細は[docs/PACKAGING.md](PACKAGING.md)
  - [x] GitHub公開 (`mar1mo-41414/SoftEtherVPN-QtManager`)、`.github/workflows/release.yml`で
        タグpush時にLinux x64・arm64のビルド成果物をGitHub Release化。macOS (arm64/Intelとも)は
        `macos-14`ランナーでのビルドが最終リンク直前で毎回ハングする現象を確認したため、
        いったん手動ビルド対応の方針 (詳細は[docs/PACKAGING.md](PACKAGING.md))
  - [x] v0.1.0リリース。Linux x64・arm64 (AppImage, CI) / macOS arm64・x86_64 (zip, 手動)の
        4種類をReleaseに添付。Intel Mac版は`p_mac`(常時稼働)で継続ビルドする運用を確立
  - [x] レイアウト再現(進行中): 接続一覧/接続編集(+プロキシ)/サーバー管理画面(D_SM_SERVER)/
        仮想HUB管理画面(D_SM_HUB)/ユーザー(6認証方式・有効期限・ポリシー)/グループ/
        セキュリティポリシー/アクセスリスト(IPv4/IPv6・MAC・TCP状態・リダイレクト・遅延/ロス、
        メモリ編集+保存方式)/カスケード接続(認証4種・プロキシ・サーバー証明書・ポリシー・高度な通信設定)/
        仮想HUBプロパティ(管理オプション・拡張オプション・接続元IP制限・メッセージ)/
        ログ保存設定/認証サーバー(RADIUS)/信頼する証明機関/無効な証明書/SecureNAT(静的ルート
        プッシュ対応)/暗号化と通信関係の設定/ローカルブリッジ/L3スイッチ/セッション・MAC/IPテーブル
  - [x] IPsec/EtherIP/OpenVPN/DDNS/Azure/リスナー作成/ログファイル一覧/レイヤ3スイッチ/
        管理者パスワード変更/VPN over ICMP/DNS設定/SecureNAT(NAT・DHCPテーブル・動作状況)
  - [x] クラスタ構成(D_SM_FARM: 現在の動作モード表示・性能基準比・クラスタメンバー設定項目)・
        クラスタ状態(D_SM_FARM_MEMBER/クラスタメンバサーバーの状態/クラスタコントローラへの
        接続状態) — 文言・レイアウトとも公式Managerと一致を確認
  - [ ] 未確認・未再現: ユーザー情報・セッション情報の細部、証明書表示ダイアログ(D_CERT)の
        詳細表示
  - [x] 実機書き込み検証(テスト専用サーバー): アクセスリスト保存・HUBメッセージ・
        接続元IP制限リスト・管理/拡張オプション・ログ保存設定・RADIUS設定・信頼するCA・CRL・
        カスケード接続(パスワード認証)・レイヤ3スイッチ全操作・SecureNAT有効化と設定保存
        (静的ルートのプッシュを含む)・EtherIP定義の追加/編集/削除
  - **実装しない方針:** 証明書作成ツール、スマートカード対応、更新通知設定
    (対応するJSON-RPC APIが公開されていない、または独立したツールの領分のため)

## 現在地
Phase 10 進行中。レイアウト再現・実機書き込み検証はクラスタ構成/状態まで含めてほぼ完了。
次は **ローカライズ**（日本語/英語）と **パッケージング**（macOS `.app` / Linux AppImage・deb）
に取り組む。

優先度低 (後回し。必要になったときに着手する):
- カスケード接続の証明書認証・RADIUS/NT認証・プロキシ経由接続・高度な通信設定の実機確認
  (実験用のネットワーク環境が別途必要)。匿名/パスワード認証(SHA-0)のみ実機確認済み。
- 証明書作成ツール・スマートカード対応・更新通知設定 — 実装しない方針。

画面確認用に `tools/mock_vpnserver.py` (架空データのモックサーバー) を同梱している。
