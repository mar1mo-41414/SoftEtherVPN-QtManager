#include "AzureDialog.h"
#include "DdnsDialog.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QDesktopServices>
#include <QPointer>
#include <QTimer>
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

    auto note = [this](const QString &text) {
        auto *label = new QLabel(text, this);
        label->setWordWrap(true);
        return label;
    };

    // S_TITLE / S_1 / S_2 / S_3
    auto *titleLabel = new QLabel(tr("VPN Azure クラウド型 VPN サービス (無料)"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleLabel->setFont(titleFont);

    // B_WEB (S_3 の右)
    auto *webButton = new QPushButton(tr("VPN Azure の使い方\n(Web サイトを表示)"), this);
    connect(webButton, &QPushButton::clicked, this, &AzureDialog::onOpenWeb);
    auto *descriptionRow = new QHBoxLayout;
    descriptionRow->addWidget(note(tr("VPN Azure は、SoftEther VPN Server をお使いの方はどなたでも無料で利用できるクラウド VPN サービスです。ソフトイーサ株式会社によって運営されています。使い方を表示するには、右のボタンをクリックしてください。")), 1);
    descriptionRow->addWidget(webButton);

    // B_BOLD / R_ENABLE / R_DISABLE / SM_AZURE_STATUS_*
    m_enableRadio = new QRadioButton(tr("VPN Azure を有効にする(&E)"), this);
    m_disableRadio = new QRadioButton(tr("VPN Azure を無効にする(&D)"), this);
    m_statusLabel = new QLabel(this);
    auto *azureGroup = new QGroupBox(tr("VPN Azure 設定"), this);
    auto *azureLayout = new QVBoxLayout(azureGroup);
    azureLayout->addWidget(m_enableRadio);
    azureLayout->addWidget(m_statusLabel);
    azureLayout->addWidget(m_disableRadio);

    // S_HOSTNAME_BORDER / S_HOSTNAME_INFO / B_CHANGE
    m_hostNameLabel = new QLabel(this);
    QFont boldFont = m_hostNameLabel->font();
    boldFont.setBold(true);
    m_hostNameLabel->setFont(boldFont);
    m_hostNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *changeButton = new QPushButton(tr("ホスト名の変更(&H)"), this);
    connect(changeButton, &QPushButton::clicked, this, &AzureDialog::onChangeHostName);
    auto *hostGroup = new QGroupBox(tr("現在の VPN Azure ホスト名"), this);
    auto *hostLayout = new QVBoxLayout(hostGroup);
    hostLayout->addWidget(note(tr("VPN Azure ホスト名はダイナミック DNS サービスのホスト名のドメイン部分を \"vpnazure.net\" に変更したものが使用されます。")));
    auto *hostRow = new QHBoxLayout;
    hostRow->addWidget(m_hostNameLabel, 1);
    hostRow->addWidget(changeButton);
    hostLayout->addLayout(hostRow);

    // IDCANCEL (表示は "OK")。ラジオの切り替えは即時にサーバーへ反映する。
    auto *okButton = new QPushButton(tr("OK"), this);
    okButton->setDefault(true);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_enableRadio, &QRadioButton::clicked, this, &AzureDialog::onSetStatus);
    connect(m_disableRadio, &QRadioButton::clicked, this, &AzureDialog::onSetStatus);

    auto *bottomRow = new QHBoxLayout;
    bottomRow->addWidget(azureGroup, 1);
    bottomRow->addWidget(hostGroup, 1);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(okButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(note(tr("VPN Azure により、会社のパソコンに自宅や外出先のパソコンから非常に簡単に VPN 接続できるようになります。VPN 接続中は会社のパソコンを経由して、社内 LAN の他のサーバーにもアクセスできます。")));
    layout->addWidget(note(tr("会社のパソコン (VPN Server) にはグローバル IP アドレスは不要です。ファイアウォールや NAT の内側であっても動作し、ネットワーク管理者による設定は一切必要ありません。VPN クライアントとなる自宅のパソコンでは、Windows に標準付属の SSTP VPN クライアントを使用できます。")));
    layout->addLayout(descriptionRow);
    layout->addLayout(bottomRow);
    layout->addLayout(buttons);

    resize(760, 460);
    reload();

    // 公式Managerと同様に1秒ごとに状態を更新する
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &AzureDialog::reload);
    timer->start(1000);
}

void AzureDialog::reload()
{
    // ダイアログが閉じられた後にRPC応答が返っても触らないようにする (定期更新で毎秒呼ばれるため)
    QPointer<AzureDialog> guard(this);
    m_rpc->call(
        QStringLiteral("GetAzureStatus"), {},
        [this, guard](const QJsonObject &result) {
            if (!guard) {
                return;
            }
            const bool enabled = result.value("IsEnabled_bool").toBool();
            if (!m_settingStatus) {
                m_enableRadio->setChecked(enabled);
                m_disableRadio->setChecked(!enabled);
            }
            // SM_AZURE_STATUS_CONNECTED / SM_AZURE_STATUS_NOT_CONNECTED
            m_statusLabel->setText(!enabled ? QString()
                                            : (result.value("IsConnected_bool").toBool() ? tr("状態: クラウドに接続完了")
                                                                                          : tr("状態: クラウドに未接続")));
        },
        [](const RpcError &) {});

    m_rpc->call(
        QStringLiteral("GetDDnsClientStatus"), {},
        [this, guard](const QJsonObject &status) {
            if (!guard) {
                return;
            }
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

void AzureDialog::onSetStatus()
{
    QJsonObject params;
    params["IsEnabled_bool"] = m_enableRadio->isChecked();
    m_settingStatus = true;
    m_rpc->call(
        QStringLiteral("SetAzureStatus"), params,
        [this, guard = QPointer<AzureDialog>(this)](const QJsonObject &) {
            if (!guard) {
                return;
            }
            m_settingStatus = false;
            reload();
        },
        [this, guard = QPointer<AzureDialog>(this)](const RpcError &error) {
            if (!guard) {
                return;
            }
            m_settingStatus = false;
            RpcUi::showError(this, tr("VPN Azure 設定の変更"), error);
            reload();
        });
}
