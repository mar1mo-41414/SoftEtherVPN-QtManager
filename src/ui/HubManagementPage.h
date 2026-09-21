#pragma once

#include "rpc/VpnServerRpc.h"

#include <QWidget>

class QLabel;
class QPushButton;

// 公式Manager「仮想HUBの管理」(D_SM_HUB) 相当。ユーザー/グループ管理への入口。
// アクセスリスト・認証サーバー・カスケード接続・ログ・証明書・SecureNAT・セッション管理は
// 後続フェーズで有効化する (ボタンは配置済みだが無効状態)。
class HubManagementPage : public QWidget
{
    Q_OBJECT

public:
    explicit HubManagementPage(QWidget *parent = nullptr);

    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    void setContext(VpnServerRpc *rpc, const QString &hubName);

signals:
    void backRequested();

private slots:
    void onManageUsers();
    void onManageGroups();
    void onEditProperty();

private:
    VpnServerRpc *m_rpc = nullptr;
    QString m_hubName;
    QLabel *m_titleLabel;
};
