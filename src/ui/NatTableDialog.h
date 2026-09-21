#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QTableWidget;

// 公式Manager「仮想 NAT ルータ上の NAT セッションテーブル」(D_NM_NAT) 相当。読み取り専用。
class NatTableDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    NatTableDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private:
    void reload();

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QTableWidget *m_table;
};
