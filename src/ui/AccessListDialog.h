#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QTableWidget;

// 公式Manager「アクセスリスト」(D_SM_ACCESS_LIST) 相当。IPv4のみ対応。
class AccessListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    AccessListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onAdd();
    void onEdit();
    void onDelete();

private:
    void reload();

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QTableWidget *m_table;
};
