#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QPushButton;
class QTableWidget;

// 公式Manager「ユーザーの管理」(D_SM_USER) 相当。
class UserListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    // groupFilter が空でなければ、そのグループに所属するユーザーのみを表示する (「メンバ一覧」)。
    UserListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr, QString groupFilter = QString());

private slots:
    void onCreate();
    void onEdit();
    void onDelete();
    void onStatus();
    void updateButtons();

private:
    void reload();
    QString selectedUserName() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QString m_groupFilter;
    QTableWidget *m_table;
    QPushButton *m_editButton;
    QPushButton *m_statusButton;
    QPushButton *m_deleteButton;
};
