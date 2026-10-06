#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QTableWidget;
class QPushButton;

// 公式Manager「%S 上のカスケード接続」(D_SM_LINK) 相当。
class CascadeLinkListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    CascadeLinkListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onCreate();
    void onEdit();
    void onDelete();
    void onRename();
    void onSetOnline();
    void onSetOffline();
    void onShowStatus();
    void onSelectionChanged();

private:
    void reload(bool silent = false);
    QString selectedAccountName() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    bool m_reloading = false;
    QTableWidget *m_table;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QPushButton *m_renameButton;
    QPushButton *m_onlineButton;
    QPushButton *m_offlineButton;
    QPushButton *m_statusButton;
};
