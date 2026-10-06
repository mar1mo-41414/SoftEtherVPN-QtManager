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

実機テストには、自宅LAN内で動作しているSoftEther VPN Server (v4.42) を使った。
ポート443は別用途で使われていたため、同サーバーの別の待ち受けポート (5555) に接続した。

Phase 1 (JSON-RPCクライアント疎通、EnumHubによるHUB一覧表示) 、Phase 2 (接続設定の
作成・編集・削除、パスワード入力プロンプト、実際の接続) とも テスト用サーバー(v4.42) に対して
動作確認済み。

## 2026-10-06 Phase 10: 公式UIを見ながらのレイアウト再現

Windows 11 の仮想マシン (RDP接続) で公式の「SoftEther VPN サーバー管理マネージャ」を開いて見比べながら、
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
- **画面キャプチャの注意:** computer-useのバックグラウンド操作 (`app_*`) はリモートデスクトップのウィンドウへ
  のクリックが届かないことがあり、その場合はフルスクリーン操作 (`computer_batch`) に切り替える。
  Qtアプリ側のテーブル行クリックは、ダイアログ(別ウィンドウ)上だと「別プロセスの要素」として
  拒否されることがあった (親ウィンドウ上のテーブルは問題なし)。
- **未実装として残したもの:** 証明書作成ツール、スマートカード、更新通知設定、
  「信頼する証明機関」から接続するカスケード接続の証明機関管理ボタン (カスケード編集内)。
- **モックサーバー:** 画面確認に使ったモックを `tools/mock_vpnserver.py` として同梱
  (値は架空、TLS証明書は起動時に openssl で自動生成)。使い方は `tools/README.md`。
- **中断メモ:** UI作業を一旦終了。次回は Phase 9 のダイアログ (IPsec/EtherIP/OpenVPN/DDNS/
  Azure/クラスタ/リスナー作成/ログファイル一覧) を公式Managerと見比べるところから。
  カスケード接続の実機確認は実験環境がないため後回し。

## 2026-10-06 カスケード接続の実機確認で判明した不具合 (パスワードハッシュの連結順)

別回線の実機サーバー (v5.02) から自宅側 (v4.42) へのカスケード接続が「ユーザー認証に失敗しました
(コード 9)」になった。接続側ログでは認証方法は「パスワード認証」で通信自体は問題なし。
原因は `HashedPassword_bin` の計算順: JSON-RPC の API ドキュメントは
`SHA0(UpperCase(username) + password)` と記載しているが、本体ソース
(`src/Cedar/Account.c` の `HashPassword`) は **パスワード → 大文字ユーザー名** の順で連結する。
ドキュメントの記述に従って逆順で実装していたため、ハッシュが常に不一致になっていた。修正済み。
(ドキュメントの記述は信用せず、挙動に関わる箇所は本体ソースで確認すること。)

## 2026-10-06 Phase 9 見比べ (別回線のテスト用サーバー (v5.02) で公式Managerと比較) とクラッシュ修正

- DDNS (2カラム化・無効化ボタンは設定ファイル編集の案内・プロキシ表示は `b_support_ddns_proxy` 連動)、
  Azure (ラジオ即時反映・OKのみ)、IPsec、OpenVPN/SSTP、ログファイル一覧 (場所列)、
  Config編集 (公式同様に編集可能) を公式に合わせた。リスナー作成・TCP/IPコネクション一覧・
  ローカルブリッジは既に公式と一致していた。
- **クラッシュ:** 定期更新(Azure 1秒 / DDNS・カスケード一覧 2秒)を入れた画面を閉じると、
  応答待ちだったRPCのコールバックが破棄済みダイアログを触ってクラッシュした。
  コールバックで `QPointer` ガードする形に修正。**他のダイアログにも同じ潜在リスク**
  (非同期ラムダで `this` を捕捉するだけ) があるため、`VpnServerRpc::call` に
  コンテキストオブジェクトを渡して寿命管理する形への一括リファクタを検討すること。

## 2026-10-06 EtherIP 実機テストとキー照合

- 別回線のテスト用サーバーで EtherIP 定義のダミーID追加 (`AddEtherIpId`) → 一覧反映 → 編集ダイアログ表示
  (`GetEtherIpId`) → 削除 (`DeleteEtherIpId`) を実機で確認。いずれも成功。
- **キー照合:** アプリが使う JSON-RPC キー364個を本体ソース (Pack の Add/Get 呼び出し) と突き合わせた。
  不一致は4件のみ: `NumConnectionsEatablished` (ドキュメントの誤記、修正済み) /
  `NoTls1` (RPCのPackにキーが無い=サーバーは無視する。カスケード編集の「SSL 3.0 を使用する」は
  APIでは効かない) / `NumPort` (配列の件数で実質無関係) / `OpenVPNPortList` (master の
  `OpenVpnSstpConfig` には EnableOpenVPN / EnableSSTP のみ。ポート一覧は4.x系サーバーのみの項目の
  ため、GetOpenVpnSstpConfig の結果に含まれる場合だけ欄を表示するようにした)。
  ポリシーのキー (`policy:CheckMac` 等) は本体の `PACK_GET_POLICY_*` と一致を確認
  (ドキュメントの `SecPol_*` は誤記)。
- ダイアログ内のテーブル行へのバックグラウンドクリックは拒否されるため、行選択が必要な
  操作確認は `computer_batch` (フルスクリーン操作) で行う。

## 2026-10-06 L3スイッチ/サーバー管理系の見比べ、公開準備

- レイヤ3スイッチの編集画面・インターフェイス追加・ルート追加ダイアログを公式Managerと
  見比べて説明文・ボタン配置を合わせた。管理者パスワード設定・VPN over ICMP/DNS設定の
  見出し/文言も合わせた。
- `NoTls1` (カスケード接続の「SSL 3.0 を使用する」) は、RPCのPackにキーが無くサーバーが
  無視する項目だったため画面から削除した。
- **テスト専用サーバー (raspi5-2相当) での実機書き込み検証:** 公式Managerと見比べながら、
  以下を実際に書き込んで確認した。
  - レイヤ3スイッチの新規作成・インターフェイス追加・ルート追加・動作開始/停止・削除
    (`AddL3Switch`/`AddL3If`/`AddL3Table`/`StartL3Switch`/`StopL3Switch`/`DelL3Switch`等)。
  - SecureNATの有効化、NAT/DHCPセッションテーブルと動作状況の表示 (公式と同じ列構成に
    修正: NATは10列、DHCPは6列、動作状況は項目/値のテーブル)。
  - SecureNAT設定の保存 (`SetSecureNATOption`)、特にプッシュする静的ルーティングテーブルの
    保存→公式Managerで再表示して値が一致することを確認。
  - EtherIP定義の追加・編集画面表示・削除 (前回セッションで確認済み)。
  いずれも問題なし。
- **公開準備:** GitHub公開を前提に、`LICENSE` (Apache-2.0)・`NOTICE` (upstream由来物の
  著作権表示)・`docs/ARCHITECTURE.md` (アーキテクチャ・実装メモ・APIドキュメントの
  落とし穴一覧) を追加。README/README-ENを簡潔な内容に書き直し、技術的な話は
  ARCHITECTURE.mdへ切り出した。環境固有の情報 (ホスト名・IPアドレス等) がコード/ドキュメント
  中に残っていないか確認済み。

## 2026-10-06 ローカライズ基盤の構築 (日本語→英語)

- Qt Linguist (lupdate/lrelease) を導入。`i18n/SoftEtherVPN-QtManager_en.ts` が英語訳、
  `CMakeLists.txt` の `qt_add_translations()` で `.qm` にコンパイルしリソース埋め込み、
  `src/main.cpp` でOSロケールに応じて自動読み込み (`SEQTM_LANG` 環境変数で強制指定も可能、
  テスト用)。
- UI文言は `docs/upstream-reference/strtable_ja.stb` の文言をそのまま使っている箇所が
  多いため、`tools/gen_translations.py` で `strtable_ja.stb`/`strtable_en.stb` の対応する
  日本語文字列から公式の英訳を自動的に流し込んだ (PREFIXブロック単位でキー対応、
  Windowsニーモニック表記 "(&X)" の有無のゆらぎ、Qtの `%1`/`%2` ↔ stbの `%S` の
  プレースホルダ変換も吸収)。1634件中1296件 (約79%) を自動翻訳、残りはアプリ独自の
  文言や複数行の説明文など (このスクリプトの `EXTRA_TRANSLATIONS` に主要なものを手動補完)。
  未訳の文字列は実行時に日本語のまま表示される (Qtの通常のフォールバック)。
  実機で英語ロケール表示を確認済み (接続一覧・サーバー管理画面はほぼ全訳)。
- 残りの未訳文字列は `i18n/SoftEtherVPN-QtManager_en.ts` をQt Linguistで直接編集するか、
  `tools/gen_translations.py` に追記して埋めていく想定 (継続作業)。

## 2026-10-06 パッケージング (macOS .app / Linux .deb・AppImage)

- macOS: `packaging/macos/build-app.sh` でReleaseビルド+`macdeployqt`によるQt
  フレームワーク同梱を自動化。開発機(Apple Silicon, Qt 6.11.1 Homebrew版)で実際に
  生成した`.app`が自己完結して起動することを確認済み(macdeployqtが無関係な
  QtPdf/QtSvg/QtVirtualKeyboard等のプラグインについて出す`ERROR: Cannot resolve rpath`
  は実害のない警告と判明、最終的な`.app`はotoolで全依存が`@executable_path/../Frameworks`
  配下に解決されていることを確認)。未署名のためGatekeeper警告が出る点はdocsに明記。
- Linux: CMakeのCPack (DEBジェネレータ) を`UNIX AND NOT APPLE`限定で`CMakeLists.txt`に
  追加 (`cmake --build build-release --target package`)。デスクトップエントリ
  `packaging/linux/softethervpn-qtmanager.desktop`を追加 (専用アイコン未作成のため
  `Icon=network-vpn`のfreedesktop標準アイコン名にフォールバック)。
  AppImageは`packaging/linux/build-appimage.sh`でlinuxdeploy+linuxdeploy-plugin-qtを
  自動取得して生成する想定のスクリプトを用意。
- 開発機がmacOSのみのため、Linux側(.deb/AppImageとも)は実行確認ができておらず
  ベストエフォート・未検証である旨を`docs/PACKAGING.md`に明記した。
- `build-release/`・`dist/`・`.packaging-tools/`・`*.deb`・`*.AppImage`を`.gitignore`に追加。

## 2026-10-06 GitHub公開、AppImageをLinux arm64含めCI化

- GitHub (`mar1mo-41414/SoftEtherVPN-QtManager`) にpublicリポジトリとして公開。
  `origin`はGitea (git.markun.f5.si、開発時のメイン)のまま、`github`リモートを追加。
- `.github/workflows/release.yml`: `vX.Y.Z`タグpushで以下を自動ビルドしGitHub Release化。
  - macOS arm64 (`.app`のzip、`macos-14`ランナー)
  - Linux x86_64 / arm64 (AppImage、`ubuntu-24.04` / `ubuntu-24.04-arm`)
  macOS x86_64 (Intel)はGitHub Actionsのホスト型ランナーが既にIntel Mac提供を終了して
  いるため含めず、手動ビルドでの対応とする方針。
- `packaging/linux/build-appimage.sh`を`uname -m`でx86_64/aarch64自動判定するよう一般化
  (linuxdeploy/linuxdeploy-plugin-qtはどちらのアーキテクチャの継続ビルドも提供している)。
- Linux用の実アイコンを作成 (`packaging/icon/`、ImageMagickで生成した盾+Vモチーフ、
  macOS `.icns`とLinux hicolorテーマ各サイズを用意)したことで、AppImageビルド時に
  以前の`Icon=network-vpn`フォールバックで出ていた`ERROR: Could not find icon executable`
  が解消した。
- marnux (Linux Mint 22, x86_64) で生成したAppImageを実機起動確認のうえMacの
  `~/Downloads/`に配置。

## 2026-10-06 GitHub Actions: macOS arm64ビルドのハングを確認、手動ビルド運用に変更

- `.github/workflows/release.yml`のmacOS arm64ジョブ(`macos-14`ランナー)で、
  `cmake --build`が全ソースファイルのコンパイルを終えた直後(最終リンク開始前、
  具体的には`qrc_SoftEtherVPN-QtManager_translations.cpp.o`のコンパイル完了直後)で
  完全に応答しなくなる現象を2回連続で確認(1回目: 単一ステップ構成で約28分放置後に
  手動キャンセル、2回目: 原因切り分けのためConfigure/Build/Deployにステップ分割し
  timeout-minutes追加のうえ再実行したが、同じ箇所で7分経過時点でも無進捗だったため
  同様にキャンセル)。
- ローカル(Apple Silicon実機のHomebrew Qt 6.11.1)では同一コードが1〜2分で問題なく
  完走しビルド成果物も正常動作するため、再現しない。Qt・CMakeいずれのインストール済み
  ファイルにもcodesignを明示的に呼ぶ記述は無く(Apple Siliconではリンカ自体が自動で
  ad-hoc署名するため別プロセス呼び出しは発生しない)、ローカルで問題なく動く以上
  codesign起因と断定はできず。GitHub Actions側も当時「macOS arm64ランナーは容量不足で
  キューイングが長引く場合がある」という注記を出しており、原因はビルド手順側か
  CI環境(ランナーの状態・ネットワーク等)側か切り分けできていない。
- 無理に自動化を追わず、macOS (arm64・Intelとも) は`packaging/macos/build-app.sh`による
  手動ビルド運用とし、`.github/workflows/release.yml`からはmacOSジョブを一旦削除。
  Linux x64/arm64のAppImageビルドはCIのまま自動化を維持(両方とも数分で正常完走する
  ことを確認済み)。原因の見当がつけば再度自動化を検討する。

## 2026-10-06 v0.1.0公開、Intel Macビルド環境を`p_mac`に確立

- v0.1.0としてGitHub Releaseを作成。Linux x86_64/arm64はCI自動ビルド、
  macOS arm64(Apple Silicon)・macOS x86_64(Intel)はいずれも手動ビルドで添付。
- Intel Mac版のビルド環境を`p_mac`(proxMac、24/365稼働)に確立。今後のIntelビルドは
  継続してこの機を使う。
  - Homebrewが2026-10-06時点で既にIntel Macのサポートを終了しておりインストール不可
    ("Homebrew on macOS is only supported on Apple Silicon processors!")と判明。
    代わりに`pip3 install --user aqtinstall cmake`でQt/CMakeを導入(sudo不要)。
  - Qt 6.11.1はホストツール(lrelease等)がmacOS 13以降必須でp_mac(macOS 12.7.6)では
    動作せず。Qt 6.5.3はホストツールは動くがmoc(Meta-Object Compiler)がXcodeの
    SDK(MacOSX13.1.sdk)のlibc++ `<concepts>`ヘッダーをパースできずビルド自体が失敗
    ("Parse error at ::"、`__cplusplus`の値を揃えても解消せず)。Qt 6.7.3で両方の
    問題が解消することを確認し、これをp_macでの標準バージョンとした。
  - `packaging/macos/build-app.sh`に`CMAKE_PREFIX_PATH`環境変数での上書きに対応させ、
    Homebrew以外の場所に入れたQtも指定できるようにした。
  - 実機(p_mac, x86_64)でビルド・起動確認済み、Releaseにx86_64版zipを追加。

## 2026-10-06 macOSビルドの起動時クラッシュ(SIGKILL Code Signature Invalid)を修正

- v0.1.0のReleaseに上げたmacOS arm64版`.app`が実機で起動直後にクラッシュする不具合を
  ユーザー報告で発見。クラッシュレポート(`~/Library/Logs/DiagnosticReports/*.ips`)を
  解析したところ `EXC_BAD_ACCESS` / `SIGKILL (Code Signature Invalid)` で、
  `codesign --verify --deep --strict` でも
  `invalid signature (code or signature have been modified) In subcomponent:
  Contents/Frameworks/libbrotlicommon.1.dylib` と再現した。
- 原因: `macdeployqt`がバンドルしたフレームワーク内dylibのrpathを`install_name_tool`で
  書き換える際、リンク時にXcodeツールチェーンが自動付与したad-hoc署名が無効化される。
  従来は`macdeployqt`実行後に再署名していなかったため、署名検証を厳格化した
  macOS(このクラッシュはmacOS 27.2で確認)では起動時にカーネルがSIGKILLで止める。
- 修正: `packaging/macos/build-app.sh`で`macdeployqt`実行後に
  `codesign --force --deep -s - "$APP"` でad-hoc再署名し、
  `codesign --verify --deep --strict` で検証するステップを追加。Developer ID証明書は
  無いため引き続き未署名(ad-hoc)だが、これで起動時のクラッシュは解消する。
- 修正後のビルドで実機起動確認(Apple Silicon・p_mac Intelとも)のうえ、
  v0.1.0 Releaseの`SoftEtherVPN-QtManager-macos-arm64.zip`/`-x86_64.zip`を差し替え。
