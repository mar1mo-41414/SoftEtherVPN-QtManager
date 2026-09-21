#pragma once

#include "model/ConnectionProfile.h"
#include "rpc/VpnServerRpc.h"

#include <QJsonObject>
#include <QList>
#include <QWidget>

class QTableWidget;
class QPushButton;

// 公式Manager起動直後の接続設定一覧画面 (D_SM_MAIN) 相当。
class ConnectionListPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionListPage(QWidget *parent = nullptr);

signals:
    // 接続に成功したら発行される。rpcの所有権は受け取り側(MainWindow)に移る。
    void connected(VpnServerRpc *rpc, const QJsonObject &serverInfo);

private slots:
    void onNewSetting();
    void onEditSetting();
    void onDeleteSetting();
    void onConnect();

private:
    void reloadTable();
    void persist();
    int selectedRow() const;
    void connectToProfile(const ConnectionProfile &profile);

    QList<ConnectionProfile> m_profiles;
    QTableWidget *m_table;
    QPushButton *m_newButton;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QPushButton *m_connectButton;
};
