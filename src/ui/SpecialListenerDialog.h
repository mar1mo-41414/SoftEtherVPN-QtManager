#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QCheckBox;

// 公式Manager「VPN over ICMP / DNS 機能の設定」(D_SM_SPECIALLISTENER) 相当。
class SpecialListenerDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit SpecialListenerDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onOk();

private:
    VpnServerRpc *m_rpc;
    QCheckBox *m_icmpCheck;
    QCheckBox *m_dnsCheck;
};
