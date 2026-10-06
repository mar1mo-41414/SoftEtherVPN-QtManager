#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QPushButton;

// 公式Manager「OpenVPN / MS-SSTP 設定」(D_SM_OPENVPN) 相当。サーバー全体の設定。
class OpenVpnSstpDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit OpenVpnSstpDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onGenerateConfig();
    void onIPsec();
    void onOk();

private:
    VpnServerRpc *m_rpc;
    QCheckBox *m_openVpnCheck;
    QLineEdit *m_portsEdit;
    QCheckBox *m_sstpCheck;
    QPushButton *m_configButton;
};
