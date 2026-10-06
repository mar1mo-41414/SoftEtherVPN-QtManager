#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>
#include <QJsonArray>

class QButtonGroup;
class QCheckBox;
class QComboBox;

// 公式Manager「ログ保存設定」(D_SM_LOG) 相当。セキュリティログ / パケットログの保存設定。
class HubLogSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    HubLogSettingsDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onOk();
    void updateState();

private:
    VpnServerRpc *m_rpc;
    QString m_hubName;
    QCheckBox *m_securityCheck;
    QComboBox *m_securitySwitch;
    QCheckBox *m_packetCheck;
    QComboBox *m_packetSwitch;
    QList<QButtonGroup *> m_packetGroups; // 8 種類 (TCP接続/TCP/DHCP/UDP/ICMP/IP/ARP/Ethernet)
    QJsonArray m_originalPacketConfig;
};
