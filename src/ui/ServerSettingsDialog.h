#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QComboBox;
class QLineEdit;
class QSpinBox;

// 公式Manager「暗号化と通信関係の設定」(D_SM_SSL) 相当。サーバー全体の設定。
// インターネット接続の維持機能・管理者パスワードの変更・VPN over ICMP/DNS設定・
// 更新通知設定は後続フェーズで追加する。
class ServerSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit ServerSettingsDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onImportCert();
    void onExportCert();
    void onViewCert();
    void onRegenerateCert();
    void onOk();

private:
    void reload();

    VpnServerRpc *m_rpc;
    QByteArray m_certDer;
    QByteArray m_keyDer;
    bool m_certChanged = false;

    QComboBox *m_cipherCombo;
    QComboBox *m_syslogCombo;
    QLineEdit *m_syslogHostEdit;
    QSpinBox *m_syslogPortSpin;
};
