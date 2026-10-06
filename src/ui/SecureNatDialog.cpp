#include "SecureNatDialog.h"
#include "DhcpTableDialog.h"
#include "NatTableDialog.h"
#include "InfoTableDialog.h"
#include "SecureNatOptionDialog.h"

#include "util/RpcUiHelpers.h"
#include "util/DialogSizing.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

SecureNatDialog::SecureNatDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_SNAT CAPTION
    setWindowTitle(tr("仮想 NAT および仮想 DHCP 機能 (SecureNAT) の設定"));

    auto note = [this](const QString &text) {
        auto *label = new QLabel(text, this);
        label->setWordWrap(true);
        return label;
    };

    // S_TITLE
    auto *title = note(tr("SecureNAT 機能を有効にすると、仮想 HUB \"%1\" 内の仮想ネットワーク内において NAT ルータ (IP マスカレード) や DHCP サーバー機能を仮想的に動作させることができるようになります。").arg(m_hubName));

    // STATIC1 / S_WARNING / S_WARNING2
    auto *warningGroup = new QGroupBox(tr("SecureNAT 機能に関する警告"), this);
    auto *warningLayout = new QVBoxLayout(warningGroup);
    auto *warningBold = note(tr("SecureNAT 機能はシステム管理者やネットワークに関して詳しい知識のある方向けの機能です。"));
    QFont boldFont = warningBold->font();
    boldFont.setBold(true);
    warningBold->setFont(boldFont);
    warningLayout->addWidget(warningBold);
    warningLayout->addWidget(note(tr("SecureNAT 機能を正しく使用すると、VPN を経由した安全なリモートアクセスが実現できます。しかし、誤った方法で使用すると、ネットワーク全体を危険な状態にする可能性もあります。ネットワークに関する十分な知識をお持ちでない場合や、ネットワーク管理者の許可を得ていない場合は、SecureNAT 機能を有効にしないでください。SecureNAT 機能に関する詳しい説明は、VPN Server のマニュアルやオンラインドキュメントを参照してください。")));

    // B_ENABLE / B_DISABLE / B_CONFIG
    m_enableButton = new QPushButton(tr("SecureNAT 機能を有効にする(&E)"), this);
    m_disableButton = new QPushButton(tr("SecureNAT 機能を無効にする(&D)"), this);
    m_configButton = new QPushButton(tr("SecureNAT の設定(&C)"), this);
    connect(m_enableButton, &QPushButton::clicked, this, &SecureNatDialog::onEnable);
    connect(m_disableButton, &QPushButton::clicked, this, &SecureNatDialog::onDisable);
    connect(m_configButton, &QPushButton::clicked, this, &SecureNatDialog::onConfig);

    auto *controlLayout = new QHBoxLayout;
    controlLayout->addWidget(m_enableButton);
    controlLayout->addWidget(m_disableButton);
    controlLayout->addWidget(m_configButton);
    auto *controlGroup = new QGroupBox(tr("SecureNAT 機能の有効 / 無効および設定の変更"), this);
    auto *controlGroupLayout = new QVBoxLayout(controlGroup);
    controlGroupLayout->addWidget(note(tr("この仮想 HUB 内で SecureNAT 機能を有効または無効にしたり、設定を変更できます。")));
    controlGroupLayout->addLayout(controlLayout);
    controlGroupLayout->addWidget(note(tr("※ 動作中の SecureNAT を無効にした場合、現在 SecureNAT を経由して接続中の TCP または UDP セッションはすべて切断されます。")));

    // B_NAT / B_DHCP / B_STATUS
    m_natButton = new QPushButton(tr("仮想 NAT ルータの状況(&N)"), this);
    m_dhcpButton = new QPushButton(tr("仮想 DHCP サーバーの状況(&H)"), this);
    m_statusButton = new QPushButton(tr("SecureNAT の動作状況の表示(&S)"), this);
    connect(m_natButton, &QPushButton::clicked, this, &SecureNatDialog::onShowNatTable);
    connect(m_dhcpButton, &QPushButton::clicked, this, &SecureNatDialog::onShowDhcpTable);
    connect(m_statusButton, &QPushButton::clicked, this, &SecureNatDialog::onShowStatus);

    auto *statusLayout = new QHBoxLayout;
    statusLayout->addWidget(m_natButton);
    statusLayout->addWidget(m_dhcpButton);
    statusLayout->addWidget(m_statusButton);
    auto *statusGroup = new QGroupBox(tr("現在の SecureNAT の状況の表示"), this);
    auto *statusGroupLayout = new QVBoxLayout(statusGroup);
    statusGroupLayout->addWidget(note(tr("現在の SecureNAT の動作状況を表示することができます。")));
    statusGroupLayout->addLayout(statusLayout);

    // IDCANCEL
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(warningGroup);
    layout->addWidget(controlGroup);
    layout->addWidget(statusGroup);
    layout->addLayout(bottomLayout);

    DialogSizing::fitToWidth(this, 640);
    refreshEnabledState();
}

void SecureNatDialog::refreshEnabledState()
{
    m_rpc->getHubStatus(
        m_hubName,
        RpcUi::guarded(this, [this](const QJsonObject &status) {
            const bool enabled = status.value("SecureNATEnabled_bool").toBool();

            // 公式Manager同様、有効時は「有効にする」を、無効時は「無効にする」と
            // 設定・状況表示系のボタンをグレーアウトする。
            m_enableButton->setEnabled(!enabled);
            m_disableButton->setEnabled(enabled);
            m_natButton->setEnabled(enabled);
            m_dhcpButton->setEnabled(enabled);
            m_statusButton->setEnabled(enabled);
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("SecureNAT の状態取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void SecureNatDialog::onEnable()
{
    m_rpc->enableSecureNAT(
        m_hubName,
        RpcUi::guarded(this, [this](const QJsonObject &) {
            refreshEnabledState();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("SecureNAT 機能の有効化に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void SecureNatDialog::onDisable()
{
    m_rpc->disableSecureNAT(
        m_hubName,
        RpcUi::guarded(this, [this](const QJsonObject &) {
            refreshEnabledState();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("SecureNAT 機能の無効化に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void SecureNatDialog::onConfig()
{
    m_rpc->getSecureNATOption(
        m_hubName,
        RpcUi::guarded(this, [this](const QJsonObject &option) {
            auto *dialog = new SecureNatOptionDialog(m_hubName, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setValues(option);
            connect(dialog, &QDialog::accepted, this, [this, dialog]() {
                QJsonObject params = dialog->toRpcParams();
                params["RpcHubName_str"] = m_hubName;
                m_rpc->setSecureNATOption(
                    params, [](const QJsonObject &) {},
                    RpcUi::guarded(this, [this](const RpcError &error) {
                        QMessageBox::warning(this, tr("エラー"),
                                              tr("SecureNAT の設定変更に失敗しました: %1 (code %2)")
                                                  .arg(error.message)
                                                  .arg(error.code));
                    }));
            });
            dialog->open();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("SecureNAT の設定取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void SecureNatDialog::onShowNatTable()
{
    NatTableDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}

void SecureNatDialog::onShowDhcpTable()
{
    DhcpTableDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}

void SecureNatDialog::onShowStatus()
{
    m_rpc->getSecureNATStatus(
        m_hubName,
        RpcUi::guarded(this, [this](const QJsonObject &status) {
            // NM_STATUS_* (公式の「SecureNAT の動作状況」と同じ項目/値の表)
            auto *dialog = new InfoTableDialog(
                tr("SecureNAT の動作状況"), tr("SecureNAT の動作状況"), /*refreshable=*/true,
                [rpc = m_rpc, hubName = m_hubName](const InfoTableDialog::Deliver &deliver, const InfoTableDialog::Fail &fail) {
                    rpc->getSecureNATStatus(
                        hubName,
                        [deliver, hubName](const QJsonObject &st) {
                            const auto sessions = [&](const char *key) { return tr("%1 セッション").arg(st.value(key).toInt()); };
                            const auto yesNo = [](bool v) { return v ? tr("はい") : tr("いいえ"); };
                            InfoTable::Rows rows;
                            rows << qMakePair(tr("仮想 HUB 名"), hubName);
                            rows << qMakePair(tr("NAT TCP/IP セッション数"), sessions("NumTcpSessions_u32"));
                            rows << qMakePair(tr("NAT UDP/IP セッション数"), sessions("NumUdpSessions_u32"));
                            rows << qMakePair(tr("NAT ICMP セッション数"), sessions("NumIcmpSessions_u32"));
                            rows << qMakePair(tr("NAT DNS セッション数"), sessions("NumDnsSessions_u32"));
                            rows << qMakePair(tr("割り当て済み DHCP クライアント数"),
                                              tr("%1 クライアント").arg(st.value("NumDhcpClients_u32").toInt()));
                            rows << qMakePair(tr("カーネルモード NAT で動作中"), yesNo(st.value("IsKernelMode_bool").toBool()));
                            rows << qMakePair(tr("Raw IP モード NAT で動作中"), yesNo(st.value("IsRawIpMode_bool").toBool()));
                            deliver(rows);
                        },
                        [fail](const RpcError &error) { fail(error); });
                },
                this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->open();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("SecureNAT の動作状況の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}
