#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QPushButton;
class QTableWidget;

// 公式Manager「グループの管理」(D_SM_GROUP) 相当。
class GroupListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    GroupListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onCreate();
    void onEdit();
    void onDelete();
    void onMembers();
    void updateButtons();

private:
    void reload();
    QString selectedGroupName() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QTableWidget *m_table;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QPushButton *m_memberButton;
};
