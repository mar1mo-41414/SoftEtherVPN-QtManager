#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;

// 公式Manager「IPsec / L2TP / EtherIP / L2TPv3 設定」(D_SM_IPSEC) 相当。サーバー全体の設定。
class IPsecSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit IPsecSettingsDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onDetail();
    void onOk();

private:
    VpnServerRpc *m_rpc;
    QCheckBox *m_l2tpIpsecCheck;
    QCheckBox *m_l2tpRawCheck;
    QCheckBox *m_etherIpCheck;
    QComboBox *m_defaultHubCombo;
    QLineEdit *m_pskEdit;
};
