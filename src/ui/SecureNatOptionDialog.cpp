#include "SecureNatOptionDialog.h"

#include "util/SoftEtherLabels.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

SecureNatOptionDialog::SecureNatOptionDialog(QWidget *parent)
    : QDialog(parent)
{
    // D_NM_OPTION CAPTION
    setWindowTitle(tr("SecureNAT の設定"));

    m_macEdit = new QLineEdit(this);
    m_ipEdit = new QLineEdit(this);
    m_ipEdit->setText(QStringLiteral("192.168.30.1"));
    m_maskEdit = new QLineEdit(this);
    m_maskEdit->setText(QStringLiteral("255.255.255.0"));

    auto *ifaceForm = new QFormLayout;
    ifaceForm->addRow(tr("MAC アドレス(&M):"), m_macEdit);
    ifaceForm->addRow(tr("IP アドレス(&P):"), m_ipEdit);
    ifaceForm->addRow(tr("サブネットマスク(&S):"), m_maskEdit);
    auto *ifaceGroup = new QGroupBox(tr("仮想ホストのネットワークインターフェイスの設定"), this);
    ifaceGroup->setLayout(ifaceForm);

    m_useNatCheck = new QCheckBox(tr("仮想 NAT 機能を使用する(&A)"), this);
    m_useNatCheck->setChecked(true);
    m_mtuSpin = new QSpinBox(this);
    m_mtuSpin->setRange(576, 1500);
    m_mtuSpin->setValue(1500);
    m_tcpTimeoutSpin = new QSpinBox(this);
    m_tcpTimeoutSpin->setRange(30, 86400);
    m_tcpTimeoutSpin->setValue(1800);
    m_udpTimeoutSpin = new QSpinBox(this);
    m_udpTimeoutSpin->setRange(30, 86400);
    m_udpTimeoutSpin->setValue(60);
    m_saveLogCheck = new QCheckBox(tr("NAT および DHCP サーバーの動作をログファイルに保存する(&L)"), this);

    auto *natForm = new QFormLayout;
    natForm->addRow(QString(), m_useNatCheck);
    natForm->addRow(tr("MTU 値(&T) (バイト):"), m_mtuSpin);
    natForm->addRow(tr("TCP セッションのタイムアウト(&C) (秒):"), m_tcpTimeoutSpin);
    natForm->addRow(tr("UDP セッションのタイムアウト(&U) (秒):"), m_udpTimeoutSpin);
    natForm->addRow(QString(), m_saveLogCheck);
    auto *natGroup = new QGroupBox(tr("仮想 NAT の設定"), this);
    natGroup->setLayout(natForm);

    m_useDhcpCheck = new QCheckBox(tr("仮想 DHCP サーバー機能を使用する(&N)"), this);
    m_useDhcpCheck->setChecked(true);
    m_leaseStartEdit = new QLineEdit(this);
    m_leaseStartEdit->setText(QStringLiteral("192.168.30.10"));
    m_leaseEndEdit = new QLineEdit(this);
    m_leaseEndEdit->setText(QStringLiteral("192.168.30.200"));
    m_dhcpMaskEdit = new QLineEdit(this);
    m_dhcpMaskEdit->setText(QStringLiteral("255.255.255.0"));
    m_leaseExpireSpin = new QSpinBox(this);
    m_leaseExpireSpin->setRange(60, 86400);
    m_leaseExpireSpin->setValue(7200);
    m_gatewayEdit = new QLineEdit(this);
    m_dns1Edit = new QLineEdit(this);
    m_dns2Edit = new QLineEdit(this);
    m_domainEdit = new QLineEdit(this);

    auto *leaseRangeLayout = new QHBoxLayout;
    leaseRangeLayout->addWidget(m_leaseStartEdit);
    leaseRangeLayout->addWidget(new QLabel(tr("から"), this));
    leaseRangeLayout->addWidget(m_leaseEndEdit);
    leaseRangeLayout->addWidget(new QLabel(tr("まで"), this));

    auto *dhcpForm = new QFormLayout;
    dhcpForm->addRow(QString(), m_useDhcpCheck);
    dhcpForm->addRow(tr("配布 IP アドレス帯(&D):"), leaseRangeLayout);
    dhcpForm->addRow(tr("サブネットマスク(&B):"), m_dhcpMaskEdit);
    dhcpForm->addRow(tr("リース期限(&E) (秒):"), m_leaseExpireSpin);
    dhcpForm->addRow(tr("デフォルトゲートウェイのアドレス(&F):"), m_gatewayEdit);
    dhcpForm->addRow(tr("DNS サーバーのアドレス 1 (&V):"), m_dns1Edit);
    dhcpForm->addRow(tr("DNS サーバーのアドレス 2 (&X):"), m_dns2Edit);
    dhcpForm->addRow(tr("ドメイン名(&W):"), m_domainEdit);
    auto *dhcpGroup = new QGroupBox(tr("仮想 DHCP サーバーの設定"), this);
    dhcpGroup->setLayout(dhcpForm);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SecureNatOptionDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(ifaceGroup);
    layout->addWidget(natGroup);
    layout->addWidget(dhcpGroup);
    layout->addWidget(buttonBox);

    resize(460, 700);
}

void SecureNatOptionDialog::setValues(const QJsonObject &option)
{
    m_macEdit->setText(SoftEtherLabels::macAddress(option.value("MacAddress_bin").toString()));
    m_ipEdit->setText(option.value("Ip_ip").toString());
    m_maskEdit->setText(option.value("Mask_ip").toString());

    m_useNatCheck->setChecked(option.value("UseNat_bool").toBool());
    const int mtu = option.value("Mtu_u32").toInt();
    m_mtuSpin->setValue(mtu > 0 ? mtu : 1500);
    const int tcpTimeout = option.value("NatTcpTimeout_u32").toInt();
    m_tcpTimeoutSpin->setValue(tcpTimeout > 0 ? tcpTimeout : 1800);
    const int udpTimeout = option.value("NatUdpTimeout_u32").toInt();
    m_udpTimeoutSpin->setValue(udpTimeout > 0 ? udpTimeout : 60);
    m_saveLogCheck->setChecked(option.value("SaveLog_bool").toBool());

    m_useDhcpCheck->setChecked(option.value("UseDhcp_bool").toBool());
    m_leaseStartEdit->setText(option.value("DhcpLeaseIPStart_ip").toString());
    m_leaseEndEdit->setText(option.value("DhcpLeaseIPEnd_ip").toString());
    m_dhcpMaskEdit->setText(option.value("DhcpSubnetMask_ip").toString());
    const int expire = option.value("DhcpExpireTimeSpan_u32").toInt();
    m_leaseExpireSpin->setValue(expire > 0 ? expire : 7200);
    m_gatewayEdit->setText(option.value("DhcpGatewayAddress_ip").toString());
    m_dns1Edit->setText(option.value("DhcpDnsServerAddress_ip").toString());
    m_dns2Edit->setText(option.value("DhcpDnsServerAddress2_ip").toString());
    m_domainEdit->setText(option.value("DhcpDomainName_str").toString());
}

QJsonObject SecureNatOptionDialog::toRpcParams() const
{
    QJsonObject params;
    params["MacAddress_bin"] = SoftEtherLabels::macAddressToBase64(m_macEdit->text());
    params["Ip_ip"] = m_ipEdit->text().trimmed();
    params["Mask_ip"] = m_maskEdit->text().trimmed();

    params["UseNat_bool"] = m_useNatCheck->isChecked();
    params["Mtu_u32"] = m_mtuSpin->value();
    params["NatTcpTimeout_u32"] = m_tcpTimeoutSpin->value();
    params["NatUdpTimeout_u32"] = m_udpTimeoutSpin->value();
    params["SaveLog_bool"] = m_saveLogCheck->isChecked();

    params["UseDhcp_bool"] = m_useDhcpCheck->isChecked();
    params["DhcpLeaseIPStart_ip"] = m_leaseStartEdit->text().trimmed();
    params["DhcpLeaseIPEnd_ip"] = m_leaseEndEdit->text().trimmed();
    params["DhcpSubnetMask_ip"] = m_dhcpMaskEdit->text().trimmed();
    params["DhcpExpireTimeSpan_u32"] = m_leaseExpireSpin->value();
    params["DhcpGatewayAddress_ip"] = m_gatewayEdit->text().trimmed();
    params["DhcpDnsServerAddress_ip"] = m_dns1Edit->text().trimmed();
    params["DhcpDnsServerAddress2_ip"] = m_dns2Edit->text().trimmed();
    params["DhcpDomainName_str"] = m_domainEdit->text().trimmed();
    params["ApplyDhcpPushRoutes_bool"] = false;
    params["DhcpPushRoutes_str"] = QString();

    return params;
}

void SecureNatOptionDialog::accept()
{
    if (m_ipEdit->text().trimmed().isEmpty() || m_maskEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("IP アドレスとサブネットマスクを入力してください。"));
        return;
    }
    QDialog::accept();
}
