#include "AzureDialog.h"
#include "DdnsDialog.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QUrl>
#include <QVBoxLayout>

AzureDialog::AzureDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_AZURE CAPTION
    setWindowTitle(tr("VPN Azure サービスの設定"));

    auto *introLabel = new QLabel(
        tr("VPN Azure クラウド型 VPN サービス (無料)\n\n"
           "VPN Azure により、会社のパソコンに自宅や外出先のパソコンから非常に簡単に VPN 接続できるようになります。"
           "VPN 接続中は会社のパソコンを経由して、社内 LAN の他のサーバーにもアクセスできます。\n\n"
           "会社のパソコン (VPN Server) にはグローバル IP アドレスは不要です。ファイアウォールや NAT の内側であっても動作し、"
           "ネットワーク管理者による設定は一切必要ありません。VPN クライアントとなる自宅のパソコンでは、Windows に標準付属の "
           "SSTP VPN クライアントを使用できます。"),
        this);
    introLabel->setWordWrap(true);

    auto *webButton = new QPushButton(tr("VPN Azure の使い方 (Web サイトを表示)"), this);
    connect(webButton, &QPushButton::clicked, this, &AzureDialog::onOpenWeb);

    // R_ENABLE / R_DISABLE / SM_AZURE_STATUS_*
    m_enableRadio = new QRadioButton(tr("VPN Azure を有効にする(&E)"), this);
    m_disableRadio = new QRadioButton(tr("VPN Azure を無効にする(&D)"), this);
    m_statusLabel = new QLabel(this);
    auto *azureGroup = new QGroupBox(tr("VPN Azure 設定"), this);
    auto *azureLayout = new QVBoxLayout(azureGroup);
    azureLayout->addWidget(m_enableRadio);
    azureLayout->addWidget(m_disableRadio);
    azureLayout->addWidget(m_statusLabel);

    // S_HOSTNAME_BORDER / S_HOSTNAME_INFO / B_CHANGE
    m_hostNameLabel = new QLabel(this);
    QFont boldFont = m_hostNameLabel->font();
    boldFont.setBold(true);
    m_hostNameLabel->setFont(boldFont);
    auto *changeButton = new QPushButton(tr("ホスト名の変更(&H)"), this);
    connect(changeButton, &QPushButton::clicked, this, &AzureDialog::onChangeHostName);
    auto *hostInfo = new QLabel(
        tr("VPN Azure ホスト名はダイナミック DNS サービスのホスト名のドメイン部分を \"vpnazure.net\" に変更したものが"
           "使用されます。"),
        this);
    hostInfo->setWordWrap(true);
    auto *hostGroup = new QGroupBox(tr("現在の VPN Azure ホスト名"), this);
    auto *hostLayout = new QVBoxLayout(hostGroup);
    hostLayout->addWidget(m_hostNameLabel);
    hostLayout->addWidget(hostInfo);
    hostLayout->addWidget(changeButton, 0, Qt::AlignRight);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AzureDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addWidget(webButton, 0, Qt::AlignRight);
    layout->addWidget(azureGroup);
    layout->addWidget(hostGroup);
    layout->addWidget(buttonBox);

    DialogSizing::fitToWidth(this, 560);
    reload();
}

void AzureDialog::reload()
{
    m_rpc->call(
        QStringLiteral("GetAzureStatus"), {},
        [this](const QJsonObject &result) {
            const bool enabled = result.value("IsEnabled_bool").toBool();
            m_enableRadio->setChecked(enabled);
            m_disableRadio->setChecked(!enabled);
            // SM_AZURE_STATUS_CONNECTED / SM_AZURE_STATUS_NOT_CONNECTED
            m_statusLabel->setText(!enabled ? QString()
                                            : (result.value("IsConnected_bool").toBool() ? tr("状態: クラウドに接続完了")
                                                                                          : tr("状態: クラウドに未接続")));
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("VPN Azure 状態の取得"), error); });

    m_rpc->call(
        QStringLiteral("GetDDnsClientStatus"), {},
        [this](const QJsonObject &status) {
            const QString hostName = status.value("CurrentHostName_str").toString();
            m_hostNameLabel->setText(hostName.isEmpty() ? tr("(なし)") : hostName + QStringLiteral(".vpnazure.net"));
        },
        [](const RpcError &) {});
}

void AzureDialog::onChangeHostName()
{
    DdnsDialog dialog(m_rpc, this);
    dialog.exec();
    reload();
}

void AzureDialog::onOpenWeb()
{
    // 文字列テーブル SE_VPNAZURE_URL
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://selinks.org/?vpnazure")));
}

void AzureDialog::onOk()
{
    QJsonObject params;
    params["IsEnabled_bool"] = m_enableRadio->isChecked();
    m_rpc->call(
        QStringLiteral("SetAzureStatus"), params, [this](const QJsonObject &) { accept(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("VPN Azure 設定の変更"), error); });
}
