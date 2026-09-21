#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QTableWidget;

// 公式Manager「仮想 DHCP サーバー上の IP リーステーブル」(D_NM_DHCP) 相当。読み取り専用。
class DhcpTableDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    DhcpTableDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private:
    void reload();

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QTableWidget *m_table;
};
