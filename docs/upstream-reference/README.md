# upstream-reference

SoftEther VPN (Apache License 2.0) 由来の参照資料です。UI の文言・API 仕様を確認するために
そのまま保存しています。著作権表示は各ファイルに保持しています。

| ファイル | 出典 |
| --- | --- |
| `strtable_ja.stb` / `strtable_en.stb` | `src/bin/hamcore/` (SoftEtherVPN/SoftEtherVPN) |
| `jsonrpc-api-reference.md` | `developer_tools/vpnserver-jsonrpc-clients/` (SoftEtherVPN/SoftEtherVPN) |

注意: `jsonrpc-api-reference.md` には本体の実装と食い違う記述があります
(例: キー名の誤記、パスワードハッシュの連結順)。挙動に関わる箇所は本体ソースで確認してください。
詳しくは [../ARCHITECTURE.md](../ARCHITECTURE.md) の「APIドキュメントの落とし穴」を参照。
