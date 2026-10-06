#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QPushButton;
class QTableWidget;

// 公式Manager「コネクション一覧」(D_SM_CONNECTION) 相当。サーバーに張られているTCPコネクションの一覧。
class TcpConnectionListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    TcpConnectionListDialog(VpnServerRpc *rpc, QString serverName, QWidget *parent = nullptr);

private slots:
    void onShowInfo();
    void onDisconnect();
    void onSelectionChanged();

private:
    void reload();
    QString selectedName() const;

    VpnServerRpc *m_rpc;
    QTableWidget *m_table;
    QPushButton *m_infoButton;
    QPushButton *m_disconnectButton;
};
