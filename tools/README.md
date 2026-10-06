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
