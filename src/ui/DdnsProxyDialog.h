#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QLineEdit;
class QRadioButton;
class QSpinBox;

// 公式Manager「プロキシサーバー経由の接続」(D_SM_PROXY) 相当。DDNSサーバーへの接続用。
// APIがSOCKS4/SOCKS5を区別しないため、SOCKSは1つの選択肢として扱う。
class DdnsProxyDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit DdnsProxyDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onOk();
    void onTypeChanged();

private:
    VpnServerRpc *m_rpc;
    QRadioButton *m_directRadio;
    QRadioButton *m_httpRadio;
    QRadioButton *m_socksRadio;
    QLineEdit *m_hostEdit;
    QSpinBox *m_portSpin;
    QLineEdit *m_userEdit;
    QLineEdit *m_passwordEdit;
};
