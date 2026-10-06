#pragma once

#include "model/ConnectionProfile.h"
#include "rpc/VpnServerRpc.h"

#include <QJsonObject>
#include <QList>
#include <QWidget>

class ListenerPanel;
class QLabel;
class QPushButton;
class QTableWidget;

// 公式Manager「VPN Server "%S" の管理」(D_SM_SERVER) 相当の、サーバー管理のトップ画面。
// 仮想HUB一覧と、リスナー・証明書・ブリッジ・L3・IPsec・OpenVPN・DDNS・Azure・クラスタ等の
// サーバー全体の設定への入口を、公式と同じ配置で並べる。
class HubListPage : public QWidget
{
    Q_OBJECT

public:
    explicit HubListPage(QWidget *parent = nullptr);

    // profile.hubAdminMode: 仮想HUB管理モードでの接続の場合、仮想HUBの作成/削除などは
    // サーバー管理権限が無いため操作対象外とし、ボタンを無効化する。
    void setConnection(VpnServerRpc *rpc, const QJsonObject &serverInfo, const ConnectionProfile &profile);

signals:
    void disconnectRequested();
    // rpcの所有権はHubListPageに残したまま (借用で使わせる)。
    void manageHubRequested(VpnServerRpc *rpc, const QString &hubName);

private slots:
    void onCreateHub();
    void onEditHub();
    void onDeleteHub();
    void onSetOnline();
    void onSetOffline();
    void onShowStatus();
    void onManageHub();
    void onSelectionChanged();
    void refreshAll();

private:
    void refreshHubList();
    void refreshFooter();
    void refreshFarmType();
    QString selectedHubName() const;
    bool selectedHubOnline() const;
    void updateHubButtons();
    QPushButton *addServerButton(QLayout *layout, const QString &text, std::function<void()> action);

    VpnServerRpc *m_rpc = nullptr;
    bool m_hubAdminMode = false;
    QString m_hostName;
    int m_farmType = 0;

    QLabel *m_titleLabel;
    QTableWidget *m_hubTable;
    QPushButton *m_manageButton;
    QPushButton *m_onlineButton;
    QPushButton *m_offlineButton;
    QPushButton *m_statusButton;
    QPushButton *m_createButton;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    ListenerPanel *m_listenerPanel;
    QPushButton *m_farmStatusButton = nullptr;
    QList<QPushButton *> m_serverAdminButtons;
    QPushButton *m_disconnectButton;
    QLabel *m_ddnsCaption;
    QLabel *m_ddnsValue;
    QLabel *m_azureCaption;
    QLabel *m_azureValue;
};
