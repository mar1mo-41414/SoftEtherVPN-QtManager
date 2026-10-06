#include "HubManagementPage.h"
#include "AccessListDialog.h"
#include "CascadeLinkListDialog.h"
#include "GroupListDialog.h"
#include "HubCaDialog.h"
#include "HubCrlDialog.h"
#include "HubEditDialog.h"
#include "HubLogSettingsDialog.h"
#include "HubRadiusDialog.h"
#include "InfoTableDialog.h"
#include "LogFileListDialog.h"
#include "SecureNatDialog.h"
#include "ServerInfoDialogs.h"
#include "SessionListDialog.h"
#include "UserListDialog.h"

#include "util/RpcUiHelpers.h"

#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

// ボタン1つと、その下の説明文を1行分として並べる (公式D_SM_HUBの各行)。
QWidget *actionRow(QWidget *parent, const QList<QPushButton *> &buttons, const QString &description)
{
    auto *row = new QWidget(parent);
    auto *layout = new QVBoxLayout(row);
    layout->setContentsMargins(0, 4, 0, 4);
    auto *buttonLayout = new QHBoxLayout;
    for (QPushButton *button : buttons) {
        buttonLayout->addWidget(button);
    }
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    auto *label = new QLabel(description, row);
    label->setWordWrap(true);
    layout->addWidget(label);
    return row;
}

QFrame *separator(QWidget *parent)
{
    auto *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

} // namespace

HubManagementPage::HubManagementPage(QWidget *parent)
    : QWidget(parent)
{
    // D_SM_HUB S_TITLE
    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 7);
    m_titleLabel->setFont(titleFont);

    // 左列 1: セキュリティデータベースの管理(D)
    auto *userButton = new QPushButton(tr("ユーザーの管理(&U)"), this);
    auto *groupButton = new QPushButton(tr("グループの管理(&G)"), this);
    auto *accessButton = new QPushButton(tr("アクセスリストの管理(&A)"), this);
    connect(userButton, &QPushButton::clicked, this, &HubManagementPage::onManageUsers);
    connect(groupButton, &QPushButton::clicked, this, &HubManagementPage::onManageGroups);
    connect(accessButton, &QPushButton::clicked, this, &HubManagementPage::onManageAccessList);
    auto *securityGroup = new QGroupBox(tr("セキュリティデータベースの管理(&D):"), this);
    auto *securityLayout = new QVBoxLayout(securityGroup);
    securityLayout->addWidget(actionRow(this, {userButton}, tr("ユーザー アカウントを追加・削除・編集できます。")));
    securityLayout->addWidget(separator(this));
    securityLayout->addWidget(actionRow(this, {groupButton}, tr("グループを追加・削除・編集できます。")));
    securityLayout->addWidget(separator(this));
    securityLayout->addWidget(
        actionRow(this, {accessButton}, tr("アクセスリスト (パケットフィルタリングルール) を追加・削除できます。")));

    // 左列 2: 仮想 HUB 設定(N)
    auto *propertyButton = new QPushButton(tr("仮想 HUB のプロパティ(&P)"), this);
    auto *radiusButton = new QPushButton(tr("認証サーバーの設定(&E)"), this);
    connect(radiusButton, &QPushButton::clicked, this, &HubManagementPage::onRadius);
    auto *linkButton = new QPushButton(tr("カスケード接続の管理(&C)"), this);
    connect(propertyButton, &QPushButton::clicked, this, &HubManagementPage::onEditProperty);
    connect(linkButton, &QPushButton::clicked, this, &HubManagementPage::onManageCascadeLinks);
    auto *settingGroup = new QGroupBox(tr("仮想 HUB 設定(&N)"), this);
    auto *settingLayout = new QVBoxLayout(settingGroup);
    settingLayout->addWidget(actionRow(this, {propertyButton}, tr("この仮想 HUB の設定を変更できます。")));
    settingLayout->addWidget(separator(this));
    settingLayout->addWidget(
        actionRow(this, {radiusButton}, tr("ユーザー認証に使用する RADIUS 認証サーバーの設定を行うことができます。")));
    settingLayout->addWidget(separator(this));
    settingLayout->addWidget(actionRow(
        this, {linkButton}, tr("同一または別のサーバー上の複数の仮想 HUB 同士をカスケード接続することができます。")));

    auto *leftColumn = new QVBoxLayout;
    leftColumn->addWidget(securityGroup);
    leftColumn->addWidget(settingGroup);

    // 右列 1: この仮想 HUB の現在の状況(R)
    m_statusTable = InfoTable::makeTable(this);
    m_statusTable->setMinimumHeight(190);
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    connect(refreshButton, &QPushButton::clicked, this, &HubManagementPage::refreshStatus);
    auto *statusGroup = new QGroupBox(tr("この仮想 HUB の現在の状況(&R):"), this);
    auto *statusLayout = new QVBoxLayout(statusGroup);
    statusLayout->addWidget(m_statusTable, 1);
    auto *statusButtons = new QHBoxLayout;
    statusButtons->addStretch();
    statusButtons->addWidget(refreshButton);
    statusLayout->addLayout(statusButtons);

    // 右列 2: その他の管理(O)
    auto *logButton = new QPushButton(tr("ログ保存設定(&L)"), this);
    connect(logButton, &QPushButton::clicked, this, &HubManagementPage::onLogSettings);
    auto *logFileButton = new QPushButton(tr("ログファイル一覧(&Q)"), this);
    auto *caButton = new QPushButton(tr("信頼する証明機関の証明書(&T)"), this);
    connect(caButton, &QPushButton::clicked, this, &HubManagementPage::onTrustedCa);
    auto *crlButton = new QPushButton(tr("無効な証明書(&K)"), this);
    connect(crlButton, &QPushButton::clicked, this, &HubManagementPage::onCrl);
    auto *snatButton = new QPushButton(tr("仮想 NAT および仮想 DHCP サーバー機能(&V)"), this);
    connect(logFileButton, &QPushButton::clicked, this, &HubManagementPage::onManageLogFiles);
    connect(snatButton, &QPushButton::clicked, this, &HubManagementPage::onManageSecureNAT);
    auto *otherGroup = new QGroupBox(tr("その他の管理(&O)"), this);
    auto *otherLayout = new QVBoxLayout(otherGroup);
    otherLayout->addWidget(actionRow(this, {logButton, logFileButton}, tr("ログの保存に関する設定を行うことができます。")));
    otherLayout->addWidget(separator(this));
    otherLayout->addWidget(
        actionRow(this, {caButton, crlButton}, tr("この仮想 HUB が信頼する証明機関の証明書を管理します。")));
    otherLayout->addWidget(separator(this));
    otherLayout->addWidget(actionRow(
        this, {snatButton}, tr("この仮想 HUB 内で SecureNAT 機能を動作させます。仮想 NAT と仮想 DHCP を稼動できます。")));

    // 右列 3: セッションの管理(I) + 閉じる(X)
    auto *sessionButton = new QPushButton(tr("セッションの管理(&S)"), this);
    connect(sessionButton, &QPushButton::clicked, this, &HubManagementPage::onManageSessions);
    auto *sessionGroup = new QGroupBox(tr("セッションの管理(&I):"), this);
    auto *sessionLayout = new QHBoxLayout(sessionGroup);
    sessionLayout->addWidget(sessionButton);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(closeButton, &QPushButton::clicked, this, &HubManagementPage::backRequested);
    auto *sessionRow = new QHBoxLayout;
    sessionRow->addWidget(sessionGroup, 1);
    sessionRow->addWidget(closeButton, 0, Qt::AlignBottom);

    auto *rightColumn = new QVBoxLayout;
    rightColumn->addWidget(statusGroup, 1);
    rightColumn->addWidget(otherGroup);
    rightColumn->addLayout(sessionRow);

    auto *columns = new QHBoxLayout;
    columns->addLayout(leftColumn, 1);
    columns->addLayout(rightColumn, 1);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_titleLabel);
    layout->addLayout(columns, 1);
}

void HubManagementPage::setContext(VpnServerRpc *rpc, const QString &hubName)
{
    m_rpc = rpc;
    m_hubName = hubName;
    // S_TITLE
    m_titleLabel->setText(tr("%1 の管理").arg(hubName));
    refreshStatus();
}

void HubManagementPage::refreshStatus()
{
    if (!m_rpc) {
        return;
    }
    m_rpc->getHubStatus(
        m_hubName,
        RpcUi::guarded(this, [this](const QJsonObject &status) { InfoTable::setRows(m_statusTable, ServerInfoDialogs::hubStatusRows(status)); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想 HUB の状態の取得"), error); }));
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
        RpcUi::guarded(this, [this](const QJsonObject &hub) {
            auto *dialog = new HubEditDialog(m_rpc, /*isNew=*/false, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setHub(hub);
            connect(dialog, &QDialog::accepted, this, [this, dialog]() {
                m_rpc->setHub(
                    dialog->toRpcParams(), [](const QJsonObject &) {},
                    RpcUi::guarded(this, [this](const RpcError &error) {
                        QMessageBox::warning(this, tr("エラー"),
                                              tr("仮想 HUB の設定変更に失敗しました: %1 (code %2)")
                                                  .arg(error.message)
                                                  .arg(error.code));
                    }));
            });
            dialog->open();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("仮想 HUB の設定取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void HubManagementPage::onRadius()
{
    HubRadiusDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}

void HubManagementPage::onLogSettings()
{
    HubLogSettingsDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}

void HubManagementPage::onTrustedCa()
{
    HubCaDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}

void HubManagementPage::onCrl()
{
    HubCrlDialog dialog(m_rpc, m_hubName, this);
    dialog.exec();
}
