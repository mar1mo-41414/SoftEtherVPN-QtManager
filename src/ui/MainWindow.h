#pragma once

#include "rpc/VpnServerRpc.h"

#include <QMainWindow>

class QTableWidget;
class QLabel;

// Phase 3で本格的な仮想HUB管理画面に置き換えるまでの、疎通確認用の最小メイン画面。
// 接続直後に EnumHub を呼び、公式Managerの仮想HUB一覧グリッドと同じ列構成で表示する。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void setConnection(VpnServerRpc *rpc, const QJsonObject &serverInfo);

private:
    void showConnectDialog();
    void refreshHubList();
    void applyServerInfo(const QJsonObject &info);

    VpnServerRpc *m_rpc = nullptr;
    QTableWidget *m_hubTable;
    QLabel *m_serverInfoLabel;
};
