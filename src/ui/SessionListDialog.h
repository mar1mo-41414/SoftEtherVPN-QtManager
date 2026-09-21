#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QTableWidget;
class QPushButton;

// 公式Manager「セッションの管理」(D_SM_SESSION) 相当。
class SessionListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    SessionListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onShowStatus();
    void onDisconnect();
    void onSessionMacTable();
    void onSessionIpTable();
    void onMacTable();
    void onIpTable();
    void onSelectionChanged();

private:
    void reload();
    QString selectedSessionName() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QTableWidget *m_table;
    QPushButton *m_statusButton;
    QPushButton *m_disconnectButton;
    QPushButton *m_sessionMacButton;
    QPushButton *m_sessionIpButton;
};
