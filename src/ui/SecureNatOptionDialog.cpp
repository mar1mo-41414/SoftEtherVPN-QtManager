#include "SecureNatOptionDialog.h"

#include "util/SoftEtherLabels.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

SecureNatOptionDialog::SecureNatOptionDialog(const QString &hubName, QWidget *parent)
    : QDialog(parent)
{
    // D_NM_OPTION CAPTION
    setWindowTitle(tr("SecureNAT の設定"));

    auto *title = new QLabel(tr("SecureNAT 仮想ホストが仮想 HUB \"%1\" の仮想ネットワーク内でどのような動作を行うかを設定してください。").arg(hubName), this);
    title->setWordWrap(true);

    m_macEdit = new QLineEdit(this);
    m_ipEdit = new QLineEdit(this);
    m_maskEdit = new QLineEdit(this);
    auto *ifaceForm = new QFormLayout;
    ifaceForm->setLabelAlignment(Qt::AlignRight);
    ifaceForm->addRow(tr("MAC アドレス(M):"), m_macEdit);
    ifaceForm->addRow(tr("IP アドレス(P):"), m_ipEdit);
    ifaceForm->addRow(tr("サブネットマスク(S):"), m_maskEdit);
    auto *ifaceGroup = new QGroupBox(tr("仮想ホストのネットワークインターフェイスの設定:"), this);
    ifaceGroup->setLayout(ifaceForm);

    m_useNatCheck = new QCheckBox(tr("仮想 NAT 機能を使用する(&A)"), this);
    m_mtuSpin = new QSpinBox(this);
    m_mtuSpin->setRange(64, 1500);
    m_tcpTimeoutSpin = new QSpinBox(this);
    m_tcpTimeoutSpin->setRange(1, 86400);
    m_udpTimeoutSpin = new QSpinBox(this);
    m_udpTimeoutSpin->setRange(1, 86400);
    auto *natGrid = new QGridLayout;
    natGrid->addWidget(new QLabel(tr("MTU 値(T):"), this), 0, 0, Qt::AlignRight);
    natGrid->addWidget(m_mtuSpin, 0, 1);
    natGrid->addWidget(new QLabel(tr("バイト"), this), 0, 2);
    natGrid->addWidget(new QLabel(tr("TCP セッションのタイムアウト(C):"), this), 1, 0, Qt::AlignRight);
    natGrid->addWidget(m_tcpTimeoutSpin, 1, 1);
    natGrid->addWidget(new QLabel(tr("秒"), this), 1, 2);
    natGrid->addWidget(new QLabel(tr("UDP セッションのタイムアウト(U):"), this), 2, 0, Qt::AlignRight);
    natGrid->addWidget(m_udpTimeoutSpin, 2, 1);
    natGrid->addWidget(new QLabel(tr("秒"), this), 2, 2);
    natGrid->setColumnStretch(1, 1);
    auto *natGroup = new QGroupBox(tr("仮想 NAT の設定:"), this);
    auto *natLayout = new QVBoxLayout(natGroup);
    natLayout->addWidget(m_useNatCheck);
    natLayout->addLayout(natGrid);

    // 静的ルーティングテーブルのプッシュ (スプリットトンネリング)
    auto *pushButton = new QPushButton(tr("プッシュする静的ルーティングテーブルの編集"), this);
    auto *pushGroup = new QGroupBox(tr("静的ルーティングテーブルのプッシュ (スプリットトンネリング)"), this);
    auto *pushLayout = new QVBoxLayout(pushGroup);
    pushLayout->addWidget(new QLabel(tr("VPN クライアントに対して静的ルーティングテーブルをプッシュ送信することができます。"), this));
    pushLayout->addWidget(pushButton);

    m_saveLogCheck = new QCheckBox(tr("NAT および DHCP サーバーの動作をログファイルに保存する(&L)"), this);

    auto *leftColumn = new QVBoxLayout;
    leftColumn->addWidget(ifaceGroup);
    leftColumn->addWidget(natGroup);
    leftColumn->addWidget(pushGroup);
    leftColumn->addWidget(m_saveLogCheck);
    leftColumn->addStretch();

    // 仮想 DHCP サーバーの設定
    m_useDhcpCheck = new QCheckBox(tr("仮想 DHCP サーバー機能を使用する(&N)"), this);
    m_leaseStartEdit = new QLineEdit(this);
    m_leaseEndEdit = new QLineEdit(this);
    m_dhcpMaskEdit = new QLineEdit(this);
    m_leaseExpireSpin = new QSpinBox(this);
    m_leaseExpireSpin->setRange(1, 86400 * 365);
    m_gatewayEdit = new QLineEdit(this);
    m_dns1Edit = new QLineEdit(this);
    m_dns2Edit = new QLineEdit(this);
    m_domainEdit = new QLineEdit(this);
    auto *dhcpGrid = new QGridLayout;
    dhcpGrid->addWidget(new QLabel(tr("配布 IP アドレス帯(D):"), this), 0, 0, Qt::AlignRight);
    dhcpGrid->addWidget(m_leaseStartEdit, 0, 1);
    dhcpGrid->addWidget(new QLabel(tr("から"), this), 0, 2);
    dhcpGrid->addWidget(m_leaseEndEdit, 1, 1);
    dhcpGrid->addWidget(new QLabel(tr("まで"), this), 1, 2);
    dhcpGrid->addWidget(new QLabel(tr("サブネットマスク(B):"), this), 2, 0, Qt::AlignRight);
    dhcpGrid->addWidget(m_dhcpMaskEdit, 2, 1);
    dhcpGrid->addWidget(new QLabel(tr("リース期限(E):"), this), 3, 0, Qt::AlignRight);
    dhcpGrid->addWidget(m_leaseExpireSpin, 3, 1);
    dhcpGrid->addWidget(new QLabel(tr("秒"), this), 3, 2);
    dhcpGrid->setColumnStretch(1, 1);
    auto *optionCaption = new QLabel(tr("クライアントに割り当てるオプションの設定 (空欄でも可):"), this);
    auto *optionGrid = new QGridLayout;
    optionGrid->addWidget(new QLabel(tr("デフォルトゲートウェイの\nアドレス(F):"), this), 0, 0, Qt::AlignRight);
    optionGrid->addWidget(m_gatewayEdit, 0, 1);
    optionGrid->addWidget(new QLabel(tr("DNS サーバーのアドレス 1 (V):"), this), 1, 0, Qt::AlignRight);
    optionGrid->addWidget(m_dns1Edit, 1, 1);
    optionGrid->addWidget(new QLabel(tr("DNS サーバーのアドレス 2 (X):"), this), 2, 0, Qt::AlignRight);
    optionGrid->addWidget(m_dns2Edit, 2, 1);
    optionGrid->addWidget(new QLabel(tr("ドメイン名(W):"), this), 3, 0, Qt::AlignRight);
    optionGrid->addWidget(m_domainEdit, 3, 1);
    optionGrid->setColumnStretch(1, 1);
    auto *dhcpGroup = new QGroupBox(tr("仮想 DHCP サーバーの設定:"), this);
    auto *dhcpLayout = new QVBoxLayout(dhcpGroup);
    dhcpLayout->addWidget(m_useDhcpCheck);
    dhcpLayout->addLayout(dhcpGrid);
    dhcpLayout->addWidget(optionCaption);
    dhcpLayout->addLayout(optionGrid);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SecureNatOptionDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *rightColumn = new QVBoxLayout;
    rightColumn->addWidget(dhcpGroup);
    rightColumn->addStretch();
    rightColumn->addWidget(buttonBox);

    auto *columns = new QHBoxLayout;
    columns->addLayout(leftColumn, 1);
    columns->addLayout(rightColumn, 1);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addLayout(columns, 1);

    connect(m_useNatCheck, &QCheckBox::toggled, this, &SecureNatOptionDialog::updateState);
    connect(m_useDhcpCheck, &QCheckBox::toggled, this, &SecureNatOptionDialog::updateState);
    connect(pushButton, &QPushButton::clicked, this, &SecureNatOptionDialog::onEditPushRoutes);

    resize(860, 560);
    updateState();
}

void SecureNatOptionDialog::updateState()
{
    // 公式Managerと同様、無効にした機能の入力欄はグレーアウトする
    const bool nat = m_useNatCheck->isChecked();
    m_mtuSpin->setEnabled(nat);
    m_tcpTimeoutSpin->setEnabled(nat);
    m_udpTimeoutSpin->setEnabled(nat);
    const bool dhcp = m_useDhcpCheck->isChecked();
    for (QWidget *w : QList<QWidget *>{m_leaseStartEdit, m_leaseEndEdit, m_dhcpMaskEdit, m_leaseExpireSpin, m_gatewayEdit,
                                       m_dns1Edit, m_dns2Edit, m_domainEdit}) {
        w->setEnabled(dhcp);
    }
}

// D_NM_PUSH
void SecureNatOptionDialog::onEditPushRoutes()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("プッシュする静的ルーティングテーブルの編集"));
    auto note = [&](const QString &text) {
        auto *label = new QLabel(text, &dialog);
        label->setWordWrap(true);
        return label;
    };
    auto *edit = new QPlainTextEdit(m_pushRoutes, &dialog);
    auto *group = new QGroupBox(tr("プッシュする静的ルーティングテーブルの編集"), &dialog);
    auto *groupLayout = new QVBoxLayout(group);
    groupLayout->addWidget(note(tr("例: 192.168.5.0/255.255.255.0/192.168.4.254, 10.0.0.0/255.0.0.0/192.168.4.253\n\n複数のエントリ (最大 64 個) はカンマまたはスペースで区切ります。\n各エントリは、\"IP ネットワークアドレス/サブネットマスク/ゲートウェイ IP アドレス\" の書式で記述します。")));
    groupLayout->addWidget(edit);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    auto *footer = new QHBoxLayout;
    footer->addWidget(note(tr("クラスレス静的ルートについては、RFC 3442 をお読みください。")), 1);
    footer->addWidget(buttonBox);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(note(tr("VPN クライアントに対してこの仮想 DHCP サーバーから DHCP 応答を送信する際に、クラスレス静的ルート (RFC 3442) を併せて送信することができます。")));
    layout->addWidget(note(tr("VPN クライアントがクラスレス静的ルート (RFC 3442) を認識できるかどうかは、VPN クライアントソフトウェアによって異なります。SoftEther VPN Client および OpenVPN Client はクラスレス静的ルートに対応しています。L2TP/IPsec および MS-SSTP においては、利用の可否はクライアントソフトウェアに依存します。")));
    layout->addWidget(note(tr("仮想 DHCP サーバーのオプションでデフォルトゲートウェイを空欄に設定することで、スプリットトンネリングが実現できます。L2TP/IPsec および MS-SSTP クライアントを使用している場合は、IPv4 の設定画面でデフォルトゲートウェイを VPN サーバーに向けないようにする設定が必要です。")));
    layout->addWidget(note(tr("ローカルブリッジ経由で外部に DHCP サーバーがある場合は、その DHCP サーバーでクラスレス静的ルート (RFC 3442) をプッシュするよう設定することもできます。その場合は、SecureNAT の仮想 DHCP サーバー機能は無効にしてください。また、この画面での設定は必要ありません。")));
    layout->addWidget(group, 1);
    layout->addLayout(footer);
    dialog.resize(720, 560);

    if (dialog.exec() == QDialog::Accepted) {
        m_pushRoutes = edit->toPlainText().trimmed();
    }
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
    m_pushRoutes = option.value("DhcpPushRoutes_str").toString();

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
    updateState();
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
    params["ApplyDhcpPushRoutes_bool"] = !m_pushRoutes.isEmpty();
    params["DhcpPushRoutes_str"] = m_pushRoutes;

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
