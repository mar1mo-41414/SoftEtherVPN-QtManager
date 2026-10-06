#include "SpecialListenerDialog.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SpecialListenerDialog::SpecialListenerDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_SPECIALLISTENER CAPTION
    setWindowTitle(tr("VPN over ICMP / DNS 機能の設定"));

    auto *introLabel = new QLabel(
        tr("ファイアウォールやルータ等の故障や過負荷、設定ミス等により TCP/IP 通信が遮断されている環境のネットワークからでも、"
           "ICMP (Ping) または DNS パケットの通信が可能であれば、この VPN Server との間で VPN 通信を行うことができます。"
           "そのためには、予め以下の機能を有効にしておく必要があります。"),
        this);
    introLabel->setWordWrap(true);

    // R_OVER_ICMP / R_OVER_DNS
    m_icmpCheck = new QCheckBox(tr("VPN over ICMP サーバー機能を有効にする(&I)"), this);
    m_dnsCheck = new QCheckBox(tr("VPN over DNS サーバー機能を有効にする (UDP ポート 53 を使用します)(&D)"), this);

    auto *versionLabel = new QLabel(tr("接続元の VPN Client または VPN Bridge は内部バージョン 4.0 以降が必要です。"), this);
    versionLabel->setWordWrap(true);
    auto *warningLabel = new QLabel(
        tr("警告: これは、ファイウォールやルータ等が一時的に不調となっており ICMP または DNS のみ安定した通信が可能な環境で VPN "
           "通信を確立するための機能です。緊急時などには有益ですが、長期間の利用には適さない場合があります。"),
        this);
    warningLabel->setWordWrap(true);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SpecialListenerDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addWidget(m_icmpCheck);
    layout->addWidget(m_dnsCheck);
    layout->addWidget(versionLabel);
    layout->addWidget(warningLabel);
    layout->addWidget(buttonBox);

    DialogSizing::fitToWidth(this, 520);

    m_rpc->call(
        QStringLiteral("GetSpecialListener"), {},
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            m_icmpCheck->setChecked(result.value("VpnOverIcmpListener_bool").toBool());
            m_dnsCheck->setChecked(result.value("VpnOverDnsListener_bool").toBool());
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("VPN over ICMP / DNS 設定の取得"), error); }));
}

void SpecialListenerDialog::onOk()
{
    QJsonObject params;
    params["VpnOverIcmpListener_bool"] = m_icmpCheck->isChecked();
    params["VpnOverDnsListener_bool"] = m_dnsCheck->isChecked();
    m_rpc->call(
        QStringLiteral("SetSpecialListener"), params, RpcUi::guarded(this, [this](const QJsonObject &) { accept(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("VPN over ICMP / DNS 設定の変更"), error); }));
}
