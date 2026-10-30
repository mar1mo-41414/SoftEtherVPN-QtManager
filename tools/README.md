# tools

## mock_vpnserver.py

実サーバーのパスワードを使わずに GUI のレイアウトを確認するための、SoftEther VPN Server の
JSON-RPC (`https://host:port/api/`) を真似る最小のモックサーバーです。値はすべて架空の固定データで、
書き込み系の API は何もせず空の結果を返します。

```bash
python3 tools/mock_vpnserver.py
```

アプリで接続設定を作ります (ホスト `127.0.0.1`、ポート `5556`、管理モード = サーバー全体、
パスワードは何でも可)。要 `openssl` (自己署名証明書を一時ディレクトリに自動生成)。

## win-settings-import

公式 VPN Server Manager (Windows専用) に登録済みの接続設定一覧を、このアプリの設定ストアへ
取り込むCUIミニツールです。GUI本体 (`SoftEtherVPN-QtManager`) とは別のバイナリとしてビルドされ、
GUIは一切起動しません。

公式Managerは接続設定一覧をWindowsレジストリ
`HKEY_CURRENT_USER\Software\SoftEther VPN Developer Edition\SoftEther VPN\Server Manager\Settings`
に、1件ごと固定長(13420バイト)の生C構造体として保存しています。このツールはそれを
`reg export` した `.reg` ファイルを読み込んで変換するので、**Windows機で動かす必要はなく**、
このアプリと同じ(Mac/Linuxの)環境で完結します。

```bash
# 1. Windows側で対象キーをエクスポート (PowerShellまたはコマンドプロンプト)
reg export "HKCU\Software\SoftEther VPN Developer Edition\SoftEther VPN\Server Manager\Settings" settings.reg

# 2. settings.reg をこちらの環境へコピー

# 3. 取り込み (まず --dry-run で内容を確認するのがおすすめ)
build/win-settings-import settings.reg --dry-run
build/win-settings-import settings.reg
```

**管理パスワードは移行できません。** 公式Managerは管理パスワードを平文でも復号可能な形でも
保存しておらず、SHA-0の一方向ハッシュとしてのみ保存しています(公式Manager自身もこれを
復号して表示することはできません)。取り込んだ接続設定はすべて「パスワードを保存しない」
扱いになるため、初回接続時に再入力してください。一方、プロキシ認証パスワードは平文で
保存されているため、こちらは正しく移行されます。

ホスト名・HUB名などの一部フィールド(`char[]`)は公式Manager側がWindowsのANSI(ローカル
コードページ)で読み書きしているため、UTF-8として解釈できない場合はLatin-1にフォール
バックします。ASCII範囲外の文字(日本語など)が含まれる場合は文字化けする可能性があるので、
取り込み後に確認してください。接続設定名 (Title) はUnicodeで正しく保存されているため、
この問題はありません。
