#pragma once

#include "rpc/VpnServerRpc.h"

#include <QJsonObject>
#include <QWidget>

class QTableWidget;
class QLabel;
class QPushButton;

// Phase 3で本格的な仮想HUB管理画面 (D_SM_SERVER) に置き換えるまでの、
// 接続直後に EnumHub を呼んで一覧表示するだけの最小画面。
class HubListPage : public QWidget
{
    Q_OBJECT

public:
    explicit HubListPage(QWidget *parent = nullptr);

    void setConnection(VpnServerRpc *rpc, const QJsonObject &serverInfo);

signals:
    void disconnectRequested();

private:
    void refreshHubList();
    void applyServerInfo(const QJsonObject &info);

    VpnServerRpc *m_rpc = nullptr;
    QTableWidget *m_hubTable;
    QLabel *m_serverInfoLabel;
    QPushButton *m_refreshButton;
    QPushButton *m_disconnectButton;
};
