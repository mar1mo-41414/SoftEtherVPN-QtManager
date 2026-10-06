#pragma once

#include <QMainWindow>

#include "model/ConnectionProfile.h"

class QStackedWidget;
class ConnectionListPage;
class HubListPage;
class HubManagementPage;
class VpnServerRpc;
class QJsonObject;

// アプリのトップレベルウィンドウ。接続設定一覧・仮想HUB一覧・仮想HUB管理の3画面を
// QStackedWidgetで切り替える (公式Managerが1つのウィンドウの中身を差し替えるのと同じ構成)。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnected(VpnServerRpc *rpc, const QJsonObject &serverInfo, const ConnectionProfile &profile);
    void onDisconnectRequested();
    void onManageHubRequested(VpnServerRpc *rpc, const QString &hubName);
    void onHubManagementBackRequested();

private:
    void fitToCurrentPage();
    void updateTitle();

    QStackedWidget *m_stack;
    ConnectionListPage *m_connectionListPage;
    HubListPage *m_hubListPage;
    HubManagementPage *m_hubManagementPage;

    // 仮想HUB管理モードでの接続時は仮想HUB一覧をスキップして直接この画面に入るため、
    // 「閉じる」で戻る先を憶えておく必要がある。
    bool m_hubManagementEnteredDirectly = false;

    QString m_settingName;
    QString m_currentHubName;
};
