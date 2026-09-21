#pragma once

#include <QMainWindow>

class QStackedWidget;
class ConnectionListPage;
class HubListPage;
class VpnServerRpc;
class QJsonObject;

// アプリのトップレベルウィンドウ。接続設定一覧画面と、接続後の仮想HUB一覧画面を
// QStackedWidgetで切り替える (公式Managerが1つのウィンドウの中身を差し替えるのと同じ構成)。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnected(VpnServerRpc *rpc, const QJsonObject &serverInfo, bool hubAdminMode);
    void onDisconnectRequested();

private:
    QStackedWidget *m_stack;
    ConnectionListPage *m_connectionListPage;
    HubListPage *m_hubListPage;
};
