#pragma once

#include "InfoTableDialog.h"
#include "rpc/VpnServerRpc.h"

// 公式Manager「サーバー状態」「接続先 VPN Server バージョン情報」「仮想 HUB の状態」を、
// InfoTableDialog として組み立てるファクトリ。rpcは借用のみ (呼び出し側が生存管理する)。
namespace ServerInfoDialogs {

InfoTableDialog *createServerStatusDialog(VpnServerRpc *rpc, QWidget *parent);
InfoTableDialog *createServerInfoDialog(VpnServerRpc *rpc, QWidget *parent);
InfoTableDialog *createHubStatusDialog(VpnServerRpc *rpc, const QString &hubName, QWidget *parent);

// D_SM_HUB 右側の「この仮想 HUB の現在の状況」にも使う行データ。
InfoTable::Rows hubStatusRows(const QJsonObject &status);

} // namespace ServerInfoDialogs
