#include "SecureNatDialog.h"
#include "DhcpTableDialog.h"
#include "NatTableDialog.h"
#include "SecureNatOptionDialog.h"

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

    auto *warningLabel = new QLabel(
        tr("SecureNAT 機能はシステム管理者やネットワークに関して詳しい知識のある方向けの機能です。\n"
           "誤った方法で使用すると、ネットワーク全体を危険な状態にする可能性があります。\n"
           "十分な知識をお持ちでない場合や、ネットワーク管理者の許可を得ていない場合は有効にしないでください。"),
        this);
    warningLabel->setWordWrap(true);

    m_stateLabel = new QLabel(this);

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
    controlGroupLayout->addWidget(m_stateLabel);
    controlGroupLayout->addLayout(controlLayout);

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
    statusGroup->setLayout(statusLayout);

    // IDCANCEL
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(warningLabel);
    layout->addWidget(controlGroup);
    layout->addWidget(statusGroup);
    layout->addLayout(bottomLayout);

    DialogSizing::fitToWidth(this, 560);
    refreshEnabledState();
}

void SecureNatDialog::refreshEnabledState()
{
    m_rpc->getHubStatus(
        m_hubName,
        [this](const QJsonObject &status) {
            const bool enabled = status.value("SecureNATEnabled_bool").toBool();
            m_stateLabel->setText(enabled ? tr("状態: 有効") : tr("状態: 無効"));

            // 公式Manager同様、有効時は「有効にする」を、無効時は「無効にする」および
            // 設定・状況表示系のボタンをグレーアウトする。
            m_enableButton->setEnabled(!enabled);
            m_disableButton->setEnabled(enabled);
            m_configButton->setEnabled(enabled);
            m_natButton->setEnabled(enabled);
            m_dhcpButton->setEnabled(enabled);
            m_statusButton->setEnabled(enabled);
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("SecureNAT の状態取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void SecureNatDialog::onEnable()
{
    m_rpc->enableSecureNAT(
        m_hubName,
        [this](const QJsonObject &) {
            refreshEnabledState();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("SecureNAT 機能の有効化に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void SecureNatDialog::onDisable()
{
    m_rpc->disableSecureNAT(
        m_hubName,
        [this](const QJsonObject &) {
            refreshEnabledState();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("SecureNAT 機能の無効化に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void SecureNatDialog::onConfig()
{
    m_rpc->getSecureNATOption(
        m_hubName,
        [this](const QJsonObject &option) {
            auto *dialog = new SecureNatOptionDialog(this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setValues(option);
            connect(dialog, &QDialog::accepted, this, [this, dialog]() {
                QJsonObject params = dialog->toRpcParams();
                params["RpcHubName_str"] = m_hubName;
                m_rpc->setSecureNATOption(
                    params, [](const QJsonObject &) {},
                    [this](const RpcError &error) {
                        QMessageBox::warning(this, tr("エラー"),
                                              tr("SecureNAT の設定変更に失敗しました: %1 (code %2)")
                                                  .arg(error.message)
                                                  .arg(error.code));
                    });
            });
            dialog->open();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("SecureNAT の設定取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
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
        [this](const QJsonObject &status) {
            auto *dialog = new QDialog(this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setWindowTitle(tr("SecureNAT の動作状況"));

            auto *form = new QFormLayout;
            form->addRow(tr("TCP セッション数:"), new QLabel(QString::number(status.value("NumTcpSessions_u32").toInt()), dialog));
            form->addRow(tr("UDP セッション数:"), new QLabel(QString::number(status.value("NumUdpSessions_u32").toInt()), dialog));
            form->addRow(tr("ICMP セッション数:"), new QLabel(QString::number(status.value("NumIcmpSessions_u32").toInt()), dialog));
            form->addRow(tr("DNS セッション数:"), new QLabel(QString::number(status.value("NumDnsSessions_u32").toInt()), dialog));
            form->addRow(tr("DHCP クライアント数:"), new QLabel(QString::number(status.value("NumDhcpClients_u32").toInt()), dialog));
            form->addRow(tr("カーネルモード:"), new QLabel(status.value("IsKernelMode_bool").toBool() ? tr("はい") : tr("いいえ"), dialog));
            form->addRow(tr("Raw IP モード:"), new QLabel(status.value("IsRawIpMode_bool").toBool() ? tr("はい") : tr("いいえ"), dialog));

            auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
            buttonBox->button(QDialogButtonBox::Close)->setText(tr("閉じる(&X)"));
            connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
            connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);

            auto *dialogLayout = new QVBoxLayout(dialog);
            dialogLayout->addLayout(form);
            dialogLayout->addWidget(buttonBox);

            dialog->open();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("SecureNAT の動作状況の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}
