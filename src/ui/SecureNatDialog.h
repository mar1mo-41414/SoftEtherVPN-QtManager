#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QLabel;
class QPushButton;

// 公式Manager「仮想 NAT および仮想 DHCP 機能 (SecureNAT) の設定」(D_SM_SNAT) 相当。
class SecureNatDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    SecureNatDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onEnable();
    void onDisable();
    void onConfig();
    void onShowNatTable();
    void onShowDhcpTable();
    void onShowStatus();

private:
    void refreshEnabledState();

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QPushButton *m_enableButton;
    QPushButton *m_disableButton;
    QPushButton *m_configButton;
    QPushButton *m_natButton;
    QPushButton *m_dhcpButton;
    QPushButton *m_statusButton;
};
