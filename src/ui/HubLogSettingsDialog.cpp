#include "HubLogSettingsDialog.h"

#include "util/RpcUiHelpers.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

HubLogSettingsDialog::HubLogSettingsDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_LOG CAPTION
    setWindowTitle(tr("ログ保存設定"));

    auto *title = new QLabel(
        tr("仮想 HUB \"%1\" に関するセキュリティログ (ユーザーのログインなどの記録) および仮想 HUB を通過するすべてのパケットに関するパケットログを保存することができます。")
            .arg(m_hubName),
        this);
    title->setWordWrap(true);

    // SM_LOG_SWITCH_0〜5
    const QStringList switchItems = {tr("切り替えを行わない"), tr("1 秒単位で切り替える"), tr("1 分単位で切り替える"),
                                     tr("1 時間単位で切り替える"), tr("1 日単位で切り替える"), tr("1 ヶ月単位で切り替える")};
    m_securityCheck = new QCheckBox(tr("セキュリティログを保存する(&E)"), this);
    m_securitySwitch = new QComboBox(this);
    m_securitySwitch->addItems(switchItems);
    auto *securityGroup = new QGroupBox(tr("セキュリティログ(S):"), this);
    auto *securityLayout = new QGridLayout(securityGroup);
    securityLayout->addWidget(m_securityCheck, 0, 0, 1, 2);
    securityLayout->addWidget(new QLabel(tr("ログファイルの切り替え周期(W):"), this), 1, 0, Qt::AlignRight);
    securityLayout->addWidget(m_securitySwitch, 1, 1);
    securityLayout->setColumnStretch(1, 1);

    m_packetCheck = new QCheckBox(tr("パケットログを保存する(&F)"), this);
    m_packetSwitch = new QComboBox(this);
    m_packetSwitch->addItems(switchItems);
    auto *packetGroup = new QGroupBox(tr("パケットログ(P):"), this);
    auto *packetLayout = new QGridLayout(packetGroup);
    packetLayout->addWidget(m_packetCheck, 0, 0, 1, 4);
    packetLayout->addWidget(new QLabel(tr("ログファイルの切り替え周期(X):"), this), 1, 0, Qt::AlignRight);
    packetLayout->addWidget(m_packetSwitch, 1, 1, 1, 3);
    const QStringList kinds = {tr("TCP コネクションログ:"), tr("TCP パケットログ:"), tr("DHCP パケットログ:"),
                               tr("UDP パケットログ:"),      tr("ICMP パケットログ:"), tr("IP パケットログ:"),
                               tr("ARP パケットログ:"),      tr("Ethernet\nパケットログ:")};
    for (int i = 0; i < kinds.size(); ++i) {
        auto *group = new QButtonGroup(this);
        const QStringList labels = {tr("保存無し"), tr("ヘッダ情報のみ"), tr("パケット内容すべて")};
        packetLayout->addWidget(new QLabel(kinds.at(i), this), 2 + i, 0, Qt::AlignRight);
        for (int level = 0; level < 3; ++level) {
            auto *radio = new QRadioButton(labels.at(level), this);
            group->addButton(radio, level);
            packetLayout->addWidget(radio, 2 + i, 1 + level);
        }
        group->button(0)->setChecked(true);
        m_packetGroups.append(group);
    }
    packetLayout->setColumnStretch(1, 1);

    auto *warning = new QLabel(tr("大量のパケットログを保存しようとすると、CPU およびハードディスクに大きな負担がかかり、仮想 HUB や VPN Server 全体のパフォーマンス低下の原因になる場合があります。必要なパケットログ情報のみ保存するように設定してください。"), this);
    warning->setWordWrap(true);

    auto *okButton = new QPushButton(tr("OK"), this);
    auto *cancelButton = new QPushButton(tr("キャンセル"), this);
    connect(okButton, &QPushButton::clicked, this, &HubLogSettingsDialog::onOk);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(okButton);
    buttons->addWidget(cancelButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(securityGroup);
    layout->addWidget(packetGroup);
    layout->addWidget(warning);
    layout->addLayout(buttons);

    connect(m_securityCheck, &QCheckBox::toggled, this, &HubLogSettingsDialog::updateState);
    connect(m_packetCheck, &QCheckBox::toggled, this, &HubLogSettingsDialog::updateState);
    resize(560, 680);
    updateState();

    QJsonObject params;
    params["HubName_str"] = m_hubName;
    m_rpc->call(
        QStringLiteral("GetHubLog"), params,
        RpcUi::guarded(this, [this](const QJsonObject &log) {
            m_securityCheck->setChecked(log.value("SaveSecurityLog_bool").toBool());
            m_securitySwitch->setCurrentIndex(qBound(0, log.value("SecurityLogSwitchType_u32").toInt(), 5));
            m_packetCheck->setChecked(log.value("SavePacketLog_bool").toBool());
            m_packetSwitch->setCurrentIndex(qBound(0, log.value("PacketLogSwitchType_u32").toInt(), 5));
            m_originalPacketConfig = log.value("PacketLogConfig_u32").toArray();
            for (int i = 0; i < m_packetGroups.size(); ++i) {
                const int level = qBound(0, m_originalPacketConfig.at(i).toInt(), 2);
                m_packetGroups.at(i)->button(level)->setChecked(true);
            }
            updateState();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("ログ保存設定の取得"), error); }));
}

void HubLogSettingsDialog::updateState()
{
    m_securitySwitch->setEnabled(m_securityCheck->isChecked());
    m_packetSwitch->setEnabled(m_packetCheck->isChecked());
    for (QButtonGroup *group : m_packetGroups) {
        for (QAbstractButton *button : group->buttons()) {
            button->setEnabled(m_packetCheck->isChecked());
        }
    }
}

void HubLogSettingsDialog::onOk()
{
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    params["SaveSecurityLog_bool"] = m_securityCheck->isChecked();
    params["SecurityLogSwitchType_u32"] = m_securitySwitch->currentIndex();
    params["SavePacketLog_bool"] = m_packetCheck->isChecked();
    params["PacketLogSwitchType_u32"] = m_packetSwitch->currentIndex();
    QJsonArray config = m_originalPacketConfig;
    while (config.size() < m_packetGroups.size()) {
        config.append(0);
    }
    for (int i = 0; i < m_packetGroups.size(); ++i) {
        config[i] = m_packetGroups.at(i)->checkedId();
    }
    params["PacketLogConfig_u32"] = config;
    m_rpc->call(
        QStringLiteral("SetHubLog"), params, RpcUi::guarded(this, [this](const QJsonObject &) { accept(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("ログ保存設定の保存"), error); }));
}
