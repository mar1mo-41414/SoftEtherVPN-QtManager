#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QTableWidget;

// 公式Manager「ユーザーの管理」(D_SM_USER) 相当。
class UserListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    UserListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onCreate();
    void onEdit();
    void onDelete();

private:
    void reload();
    QString selectedUserName() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QTableWidget *m_table;
};
