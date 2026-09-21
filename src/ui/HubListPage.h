#pragma once

#include "rpc/VpnServerRpc.h"

#include <QJsonObject>
#include <QWidget>

class QTableWidget;
class QLabel;
class QPushButton;

// D_SM_SERVER (仮想HUB一覧・基本操作) 相当の画面。
// リスナー管理・証明書・クラスタリング等のサーバー全体設定は後続フェーズで追加する。
class HubListPage : public QWidget
{
    Q_OBJECT

public:
    explicit HubListPage(QWidget *parent = nullptr);

    // hubAdminMode: 仮想HUB管理モードでの接続の場合、仮想HUBの作成/削除は
    // サーバー管理権限が無いため操作対象外とし、ボタンを無効化する。
    void setConnection(VpnServerRpc *rpc, const QJsonObject &serverInfo, bool hubAdminMode);

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

private:
    void refreshHubList();
    void applyServerInfo(const QJsonObject &info);
    QString selectedHubName() const;
    void setHubActionButtonsEnabled(bool enabled);

    VpnServerRpc *m_rpc = nullptr;
    bool m_hubAdminMode = false;

    QTableWidget *m_hubTable;
    QLabel *m_serverInfoLabel;
    QPushButton *m_manageButton;
    QPushButton *m_createButton;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QPushButton *m_onlineButton;
    QPushButton *m_offlineButton;
    QPushButton *m_statusButton;
    QPushButton *m_refreshButton;
    QPushButton *m_disconnectButton;
};
