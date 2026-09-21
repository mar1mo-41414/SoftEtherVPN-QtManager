#include "HubManagementPage.h"
#include "AccessListDialog.h"
#include "CascadeLinkListDialog.h"
#include "GroupListDialog.h"
#include "HubEditDialog.h"
#include "LogFileListDialog.h"
#include "SecureNatDialog.h"
#include "SessionListDialog.h"
#include "UserListDialog.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

HubManagementPage::HubManagementPage(QWidget *parent)
    : QWidget(parent)
{
    // D_SM_HUB S_TITLE
    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_titleLabel->setFont(titleFont);

    // STATIC1: セキュリティデータベースの管理
    auto *userButton = new QPushButton(tr("ユーザーの管理(&U)"), this);
    auto *groupButton = new QPushButton(tr("グループの管理(&G)"), this);
    auto *accessButton = new QPushButton(tr("アクセスリストの管理(&A)"), this);
    connect(userButton, &QPushButton::clicked, this, &HubManagementPage::onManageUsers);
    connect(groupButton, &QPushButton::clicked, this, &HubManagementPage::onManageGroups);
    connect(accessButton, &QPushButton::clicked, this, &HubManagementPage::onManageAccessList);

    auto *securityGroup = new QGroupBox(tr("セキュリティデータベースの管理"), this);
    auto *securityLayout = new QHBoxLayout(securityGroup);
    securityLayout->addWidget(userButton);
    securityLayout->addWidget(groupButton);
    securityLayout->addWidget(accessButton);

    // STATIC2: 仮想 HUB 設定
    auto *propertyButton = new QPushButton(tr("仮想 HUB のプロパティ(&P)"), this);
    auto *radiusButton = new QPushButton(tr("認証サーバーの設定(&E)"), this);
    radiusButton->setEnabled(false);
    radiusButton->setToolTip(tr("未実装 (今後のフェーズで対応予定)"));
    auto *linkButton = new QPushButton(tr("カスケード接続の管理(&C)"), this);
    connect(propertyButton, &QPushButton::clicked, this, &HubManagementPage::onEditProperty);
    connect(linkButton, &QPushButton::clicked, this, &HubManagementPage::onManageCascadeLinks);

    auto *settingGroup = new QGroupBox(tr("仮想 HUB 設定"), this);
    auto *settingLayout = new QHBoxLayout(settingGroup);
    settingLayout->addWidget(propertyButton);
    settingLayout->addWidget(radiusButton);
    settingLayout->addWidget(linkButton);

    // STATIC4: その他の管理
    auto *logButton = new QPushButton(tr("ログ保存設定(&L)"), this);
    auto *caButton = new QPushButton(tr("信頼する証明機関の証明書(&T)"), this);
    auto *crlButton = new QPushButton(tr("無効な証明書(&K)"), this);
    for (QPushButton *button : {logButton, caButton, crlButton}) {
        button->setEnabled(false);
        button->setToolTip(tr("未実装 (今後のフェーズで対応予定)"));
    }

    auto *logFileButton = new QPushButton(tr("ログファイル一覧(&Q)"), this);
    connect(logFileButton, &QPushButton::clicked, this, &HubManagementPage::onManageLogFiles);

    auto *snatButton = new QPushButton(tr("仮想 NAT および仮想 DHCP サーバー機能(&V)"), this);
    connect(snatButton, &QPushButton::clicked, this, &HubManagementPage::onManageSecureNAT);

    auto *sessionButton = new QPushButton(tr("セッションの管理(&S)"), this);
    connect(sessionButton, &QPushButton::clicked, this, &HubManagementPage::onManageSessions);

    auto *otherGroup = new QGroupBox(tr("その他の管理"), this);
    auto *otherLayout = new QGridLayout(otherGroup);
    otherLayout->addWidget(sessionButton, 0, 0);
    otherLayout->addWidget(snatButton, 0, 1);
    otherLayout->addWidget(logButton, 1, 0);
    otherLayout->addWidget(logFileButton, 1, 1);
    otherLayout->addWidget(caButton, 2, 0);
    otherLayout->addWidget(crlButton, 2, 1);

    // IDCANCEL
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(closeButton, &QPushButton::clicked, this, &HubManagementPage::backRequested);

    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_titleLabel);
    layout->addWidget(securityGroup);
    layout->addWidget(settingGroup);
    layout->addWidget(otherGroup);
    layout->addStretch();
    layout->addLayout(bottomLayout);
}

void HubManagementPage::setContext(VpnServerRpc *rpc, const QString &hubName)
{
    m_rpc = rpc;
    m_hubName = hubName;
    // S_TITLE
    m_titleLabel->setText(tr("%1 の管理").arg(hubName));
}

void HubManagementPage::onManageUsers()
{
    UserListDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}

void HubManagementPage::onManageGroups()
{
    GroupListDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}

void HubManagementPage::onManageSessions()
{
    SessionListDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}

void HubManagementPage::onManageAccessList()
{
    AccessListDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}

void HubManagementPage::onManageCascadeLinks()
{
    CascadeLinkListDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}

void HubManagementPage::onManageSecureNAT()
{
    SecureNatDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}

void HubManagementPage::onManageLogFiles()
{
    LogFileListDialog dialog(m_rpc, this);
    dialog.exec();
}

void HubManagementPage::onEditProperty()
{
    m_rpc->getHub(
        m_hubName,
        [this](const QJsonObject &hub) {
            auto *dialog = new HubEditDialog(/*isNew=*/false, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setValues(hub.value("HubName_str").toString(), hub.value("Online_bool").toBool(),
                               hub.value("NoEnum_bool").toBool(),
                               static_cast<quint32>(hub.value("MaxSession_u32").toDouble()));
            connect(dialog, &QDialog::accepted, this, [this, dialog]() {
                m_rpc->setHub(
                    dialog->toRpcParams(), [](const QJsonObject &) {},
                    [this](const RpcError &error) {
                        QMessageBox::warning(this, tr("エラー"),
                                              tr("仮想 HUB の設定変更に失敗しました: %1 (code %2)")
                                                  .arg(error.message)
                                                  .arg(error.code));
                    });
            });
            dialog->open();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("仮想 HUB の設定取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}
