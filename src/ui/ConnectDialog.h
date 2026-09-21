#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QLabel;

// 公式 VPN Server Manager の「新しい接続の設定」に相当する接続ダイアログ。
// Phase 2で接続設定の保存・一覧に対応するまでは、その場でホスト/ポート/管理モードを
// 指定して直接ログインするだけの簡易版。
class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectDialog(QWidget *parent = nullptr);

    // 接続に成功した場合、呼び出し側が所有権を引き継ぐ (parentをnullptrのまま返す)。
    VpnServerRpc *takeConnectedRpc();
    QJsonObject serverInfo() const { return m_serverInfo; }

private slots:
    void onConnectClicked();

private:
    void setBusy(bool busy);

    QLineEdit *m_hostEdit;
    QSpinBox *m_portSpin;
    QCheckBox *m_hubModeCheck;
    QLineEdit *m_hubNameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_connectButton;
    QPushButton *m_cancelButton;
    QLabel *m_statusLabel;

    VpnServerRpc *m_rpc = nullptr;
    QJsonObject m_serverInfo;
};
