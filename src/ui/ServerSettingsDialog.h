#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QCheckBox;
class QLabel;
class QComboBox;
class QLineEdit;
class QRadioButton;
class QSpinBox;

// 公式Manager「暗号化と通信関係の設定」(D_SM_SSL) 相当。サーバー全体の設定。
// 更新通知設定は未対応。
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
    void onChangePassword();
    void onSpecialListener();
    void onOk();

private:
    void reload();
    void updateCertInfo();

    VpnServerRpc *m_rpc;
    QByteArray m_certDer;
    QByteArray m_keyDer;
    bool m_certChanged = false;

    QLabel *m_certInfoLabel;
    QComboBox *m_cipherCombo;
    QComboBox *m_syslogCombo;
    QLineEdit *m_syslogHostEdit;
    QSpinBox *m_syslogPortSpin;

    QCheckBox *m_keepCheck;
    QLineEdit *m_keepHostEdit;
    QSpinBox *m_keepPortSpin;
    QSpinBox *m_keepIntervalSpin;
    QRadioButton *m_keepTcpRadio;
    QRadioButton *m_keepUdpRadio;
};
