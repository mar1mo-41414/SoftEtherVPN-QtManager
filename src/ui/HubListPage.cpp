#include "HubListPage.h"
#include "AzureDialog.h"
#include "ConfigEditDialog.h"
#include "DdnsDialog.h"
#include "FarmDialog.h"
#include "FarmStatusDialog.h"
#include "HubEditDialog.h"
#include "IPsecSettingsDialog.h"
#include "L3SwitchListDialog.h"
#include "ListenerPanel.h"
#include "LocalBridgeDialog.h"
#include "OpenVpnSstpDialog.h"
#include "ServerInfoDialogs.h"
#include "ServerSettingsDialog.h"
#include "TcpConnectionListDialog.h"

#include "util/RpcUiHelpers.h"
#include "util/SoftEtherLabels.h"

#include <QAbstractItemView>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

HubListPage::HubListPage(QWidget *parent)
    : QWidget(parent)
{
    // D_SM_SERVER S_TITLE
    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 7);
    m_titleLabel->setFont(titleFont);

    // 公式Managerの列構成 (SM_HUB_COLUMN_1〜11)。右に長いので横スクロールさせる。
    m_hubTable = new QTableWidget(this);
    m_hubTable->setColumnCount(11);
    m_hubTable->setHorizontalHeaderLabels({
        tr("仮想 HUB 名"),
        tr("状態"),
        tr("種類"),
        tr("ユーザー"),
        tr("グループ"),
        tr("セッション"),
        tr("MAC テーブル"),
        tr("IP テーブル"),
        tr("ログイン回数"),
        tr("最終ログイン日時"),
        tr("最終通信日時"),
    });
    m_hubTable->verticalHeader()->hide();
    m_hubTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_hubTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_hubTable->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_hubTable, &QTableWidget::itemSelectionChanged, this, &HubListPage::onSelectionChanged);
    connect(m_hubTable, &QTableWidget::cellDoubleClicked, this, &HubListPage::onManageHub);

    // 仮想HUB操作のボタン列: IDOK(管理) / B_ONLINE / B_OFFLINE / B_HUB_STATUS / B_CREATE / B_EDIT / B_DELETE
    m_manageButton = new QPushButton(tr("仮想 HUB の管理(&A)"), this);
    m_manageButton->setDefault(true);
    QFont boldFont = m_manageButton->font();
    boldFont.setBold(true);
    m_manageButton->setFont(boldFont);
    m_onlineButton = new QPushButton(tr("オンライン(&O)"), this);
    m_offlineButton = new QPushButton(tr("オフライン(&F)"), this);
    m_statusButton = new QPushButton(tr("状態の表示(&S)"), this);
    m_createButton = new QPushButton(tr("仮想 HUB の作成(&C)"), this);
    m_editButton = new QPushButton(tr("プロパティ(&E)"), this);
    m_deleteButton = new QPushButton(tr("削除(&D)"), this);
    connect(m_manageButton, &QPushButton::clicked, this, &HubListPage::onManageHub);
    connect(m_onlineButton, &QPushButton::clicked, this, &HubListPage::onSetOnline);
    connect(m_offlineButton, &QPushButton::clicked, this, &HubListPage::onSetOffline);
    connect(m_statusButton, &QPushButton::clicked, this, &HubListPage::onShowStatus);
    connect(m_createButton, &QPushButton::clicked, this, &HubListPage::onCreateHub);
    connect(m_editButton, &QPushButton::clicked, this, &HubListPage::onEditHub);
    connect(m_deleteButton, &QPushButton::clicked, this, &HubListPage::onDeleteHub);

    auto *hubButtons = new QHBoxLayout;
    for (QPushButton *button : {m_manageButton, m_onlineButton, m_offlineButton, m_statusButton, m_createButton,
                                 m_editButton, m_deleteButton}) {
        hubButtons->addWidget(button);
    }

    // 左: リスナーの管理(J)
    m_listenerPanel = new ListenerPanel(this);

    // 右: サーバー情報の参照および設定(N)  (左列 / 右列の2列)
    auto *serverGroup = new QGroupBox(tr("サーバー情報の参照および設定(&N)"), this);
    auto *serverGrid = new QGridLayout(serverGroup);
    auto addGrid = [this, serverGrid](int row, int column, const QString &text, std::function<void()> action) {
        auto *button = new QPushButton(text, this);
        connect(button, &QPushButton::clicked, this, [action = std::move(action)]() { action(); });
        serverGrid->addWidget(button, row, column);
        m_serverAdminButtons << button;
        return button;
    };
    // B_SSL / B_STATUS / B_INFO
    addGrid(0, 0, tr("暗号化と通信関係の設定(&W)"), [this]() { ServerSettingsDialog(m_rpc, this).exec(); });
    addGrid(1, 0, tr("サーバー状態の表示(&V)"), [this]() {
        auto *dialog = ServerInfoDialogs::createServerStatusDialog(m_rpc, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->open();
    });
    addGrid(2, 0, tr("SoftEther VPN Server\nに関する情報(&Q)"), [this]() {
        auto *dialog = ServerInfoDialogs::createServerInfoDialog(m_rpc, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->open();
    });
    // B_FARM / B_FARM_STATUS / B_CONNECTION / B_CONFIG
    addGrid(0, 1, tr("クラスタリング構成(&M)"), [this]() { FarmDialog(m_rpc, m_hostName, this).exec(); });
    m_farmStatusButton = addGrid(1, 1, tr("クラスタリング状態(&Z)"), [this]() {
        FarmStatusDialog dialog(m_rpc, /*isController=*/m_farmType == 1, this);
        dialog.exec();
    });
    addGrid(2, 1, tr("TCP/IP コネクション\n一覧の表示(&Y)"), [this]() { TcpConnectionListDialog(m_rpc, m_hostName, this).exec(); });
    addGrid(3, 1, tr("Config 編集(&K)"), [this]() { ConfigEditDialog(m_rpc, m_hostName, this).exec(); });
    for (int column = 0; column < 2; ++column) {
        serverGrid->setColumnStretch(column, 1);
    }

    auto *middleRow = new QHBoxLayout;
    middleRow->addWidget(m_listenerPanel, 4);
    middleRow->addWidget(serverGroup, 5);

    // 下段1: B_BRIDGE / レイヤ3 / IPsec / OpenVPN
    auto *featureRow = new QHBoxLayout;
    addServerButton(featureRow, tr("ローカルブリッジ設定(&B)"), [this]() {
        QStringList hubNames;
        for (int row = 0; row < m_hubTable->rowCount(); ++row) {
            hubNames << m_hubTable->item(row, 0)->text();
        }
        LocalBridgeDialog(m_rpc, hubNames, this).exec();
    });
    addServerButton(featureRow, tr("レイヤ 3 スイッチ設定(&3)"), [this]() { L3SwitchListDialog(m_rpc, this).exec(); });
    addServerButton(featureRow, tr("IPsec / L&2TP 設定"), [this]() { IPsecSettingsDialog(m_rpc, this).exec(); });
    addServerButton(featureRow, tr("OpenVPN / MS-SSTP 設定"), [this]() { OpenVpnSstpDialog(m_rpc, this).exec(); });

    // 下段2: DDNS / Azure ... 更新 / 閉じる
    auto *serviceRow = new QHBoxLayout;
    addServerButton(serviceRow, tr("ダイナミック DNS 設定"), [this]() {
        DdnsDialog(m_rpc, this).exec();
        refreshFooter();
    });
    addServerButton(serviceRow, tr("VPN Azure 設定"), [this]() {
        AzureDialog(m_rpc, this).exec();
        refreshFooter();
    });
    serviceRow->addStretch();
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    m_disconnectButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(refreshButton, &QPushButton::clicked, this, &HubListPage::refreshAll);
    connect(m_disconnectButton, &QPushButton::clicked, this, &HubListPage::disconnectRequested);
    serviceRow->addWidget(refreshButton);
    serviceRow->addWidget(m_disconnectButton);

    // フッター: 現在の DDNS ホスト名 / VPN Azure ホスト名 (有効な場合のみ表示)
    m_ddnsCaption = new QLabel(tr("現在の DDNS ホスト名:"), this);
    m_ddnsValue = new QLabel(this);
    m_azureCaption = new QLabel(tr("VPN Azure ホスト名:"), this);
    m_azureValue = new QLabel(this);
    QFont valueFont = m_ddnsValue->font();
    valueFont.setPointSize(valueFont.pointSize() + 2);
    m_ddnsValue->setFont(valueFont);
    m_azureValue->setFont(valueFont);
    auto *footer = new QHBoxLayout;
    footer->addWidget(m_ddnsCaption);
    footer->addWidget(m_ddnsValue);
    footer->addStretch();
    footer->addWidget(m_azureCaption);
    footer->addWidget(m_azureValue);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_hubTable, 1);
    layout->addLayout(hubButtons);
    layout->addLayout(middleRow);
    layout->addLayout(featureRow);
    layout->addLayout(serviceRow);
    layout->addLayout(footer);

    updateHubButtons();
}

QPushButton *HubListPage::addServerButton(QLayout *layout, const QString &text, std::function<void()> action)
{
    auto *button = new QPushButton(text, this);
    connect(button, &QPushButton::clicked, this, [action = std::move(action)]() { action(); });
    layout->addWidget(button);
    m_serverAdminButtons << button;
    return button;
}

void HubListPage::setConnection(VpnServerRpc *rpc, const QJsonObject &serverInfo, const ConnectionProfile &profile)
{
    Q_UNUSED(serverInfo)
    if (m_rpc) {
        m_rpc->deleteLater();
    }
    m_rpc = rpc;
    m_rpc->setParent(this);
    m_hubAdminMode = profile.hubAdminMode;
    m_hostName = profile.host;
    m_titleLabel->setText(tr("VPN Server \"%1\" の管理").arg(m_hostName));

    // 仮想HUB管理モードでは仮想HUBの作成/削除とサーバー全体の設定にサーバー管理権限が
    // 必要なため操作させない。
    for (QPushButton *button : std::as_const(m_serverAdminButtons)) {
        button->setEnabled(!m_hubAdminMode);
    }
    m_farmStatusButton->setEnabled(false);
    m_listenerPanel->setRpc(m_rpc);
    m_listenerPanel->setOperable(!m_hubAdminMode);

    updateHubButtons();
    refreshAll();
}

void HubListPage::refreshAll()
{
    refreshHubList();
    m_listenerPanel->reload();
    refreshFooter();
    refreshFarmType();
}

void HubListPage::refreshFarmType()
{
    if (!m_rpc || m_hubAdminMode) {
        return;
    }
    m_rpc->call(
        QStringLiteral("GetFarmSetting"), {},
        RpcUi::guarded(this, [this](const QJsonObject &setting) {
            m_farmType = setting.value("ServerType_u32").toInt();
            // 公式同様、クラスタを構成していない(スタンドアロン)間は「クラスタリング状態」を押せない。
            m_farmStatusButton->setEnabled(m_farmType != 0);
        }),
        [](const RpcError &) {});
}

void HubListPage::refreshFooter()
{
    m_ddnsCaption->hide();
    m_ddnsValue->hide();
    m_azureCaption->hide();
    m_azureValue->hide();
    if (!m_rpc || m_hubAdminMode) {
        return;
    }

    // 公式同様: DDNSホスト名があるときだけ表示し、VPN Azureが有効なときだけAzureホスト名を併記する。
    m_rpc->call(
        QStringLiteral("GetDDnsClientStatus"), {},
        RpcUi::guarded(this, [this](const QJsonObject &ddns) {
            const QString fqdn = ddns.value("CurrentFqdn_str").toString();
            if (fqdn.isEmpty()) {
                return;
            }
            m_ddnsValue->setText(fqdn);
            m_ddnsCaption->show();
            m_ddnsValue->show();

            const QString hostName = ddns.value("CurrentHostName_str").toString();
            m_rpc->call(
                QStringLiteral("GetAzureStatus"), {},
                RpcUi::guarded(this, [this, hostName](const QJsonObject &azure) {
                    if (azure.value("IsEnabled_bool").toBool()) {
                        m_azureValue->setText(hostName + QStringLiteral(".vpnazure.net"));
                        m_azureCaption->show();
                        m_azureValue->show();
                    }
                }),
                [](const RpcError &) {});
        }),
        [](const RpcError &) {});
}

QString HubListPage::selectedHubName() const
{
    const QList<QTableWidgetItem *> selected = m_hubTable->selectedItems();
    return selected.isEmpty() ? QString() : m_hubTable->item(selected.first()->row(), 0)->text();
}

bool HubListPage::selectedHubOnline() const
{
    const QList<QTableWidgetItem *> selected = m_hubTable->selectedItems();
    return !selected.isEmpty() && m_hubTable->item(selected.first()->row(), 0)->data(Qt::UserRole).toBool();
}

void HubListPage::updateHubButtons()
{
    const bool hasSelection = !selectedHubName().isEmpty();
    const bool online = selectedHubOnline();
    m_manageButton->setEnabled(hasSelection);
    // 公式同様、オンラインのHUBでは「オフライン」を、オフラインのHUBでは「オンライン」だけを押せる。
    m_onlineButton->setEnabled(hasSelection && !online);
    m_offlineButton->setEnabled(hasSelection && online);
    m_statusButton->setEnabled(hasSelection);
    m_createButton->setEnabled(!m_hubAdminMode);
    m_editButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection && !m_hubAdminMode);
}

void HubListPage::onSelectionChanged()
{
    updateHubButtons();
}

void HubListPage::refreshHubList()
{
    if (!m_rpc) {
        return;
    }

    const QString previouslySelected = selectedHubName();
    m_rpc->enumHub(
        RpcUi::guarded(this, [this, previouslySelected](const QJsonObject &result) {
            const QJsonArray hubList = result.value("HubList").toArray();
            m_hubTable->setRowCount(hubList.size());

            for (int row = 0; row < hubList.size(); ++row) {
                const QJsonObject hub = hubList.at(row).toObject();
                const bool online = hub.value("Online_bool").toBool();

                auto *nameItem = new QTableWidgetItem(hub.value("HubName_str").toString());
                nameItem->setData(Qt::UserRole, online);
                m_hubTable->setItem(row, 0, nameItem);
                m_hubTable->setItem(row, 1, new QTableWidgetItem(SoftEtherLabels::onlineStatus(online)));
                m_hubTable->setItem(row, 2, new QTableWidgetItem(SoftEtherLabels::hubType(hub.value("HubType_u32").toInt())));
                m_hubTable->setItem(row, 3, new QTableWidgetItem(QString::number(hub.value("NumUsers_u32").toInt())));
                m_hubTable->setItem(row, 4, new QTableWidgetItem(QString::number(hub.value("NumGroups_u32").toInt())));
                m_hubTable->setItem(row, 5, new QTableWidgetItem(QString::number(hub.value("NumSessions_u32").toInt())));
                m_hubTable->setItem(row, 6, new QTableWidgetItem(QString::number(hub.value("NumMacTables_u32").toInt())));
                m_hubTable->setItem(row, 7, new QTableWidgetItem(QString::number(hub.value("NumIpTables_u32").toInt())));
                m_hubTable->setItem(row, 8, new QTableWidgetItem(QString::number(hub.value("NumLogin_u32").toInt())));
                m_hubTable->setItem(row, 9, new QTableWidgetItem(SoftEtherLabels::dateTime(hub.value("LastLoginTime_dt").toString())));
                m_hubTable->setItem(row, 10, new QTableWidgetItem(SoftEtherLabels::dateTime(hub.value("LastCommTime_dt").toString())));

                if (nameItem->text() == previouslySelected) {
                    m_hubTable->selectRow(row);
                }
            }

            m_hubTable->resizeColumnsToContents();
            updateHubButtons();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想 HUB 一覧の取得"), error); }));
}

void HubListPage::onCreateHub()
{
    HubEditDialog dialog(m_rpc, /*isNew=*/true, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_rpc->createHub(
        dialog.toRpcParams(), RpcUi::guarded(this, [this](const QJsonObject &) { refreshHubList(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想 HUB の作成"), error); }));
}

void HubListPage::onEditHub()
{
    const QString hubName = selectedHubName();
    if (hubName.isEmpty()) {
        return;
    }

    m_rpc->getHub(
        hubName,
        RpcUi::guarded(this, [this](const QJsonObject &hub) {
            auto *dialog = new HubEditDialog(m_rpc, /*isNew=*/false, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setHub(hub);
            connect(dialog, &QDialog::accepted, this, [this, dialog]() {
                m_rpc->setHub(
                    dialog->toRpcParams(), RpcUi::guarded(this, [this](const QJsonObject &) { refreshHubList(); }),
                    RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想 HUB の設定変更"), error); }));
            });
            dialog->open();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想 HUB の設定取得"), error); }));
}

void HubListPage::onDeleteHub()
{
    const QString hubName = selectedHubName();
    if (hubName.isEmpty()) {
        return;
    }

    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"),
                            tr("仮想 HUB \"%1\" を削除します。\n"
                               "この仮想 HUB に属するユーザー・グループ・証明書・カスケード接続もすべて削除され、"
                               "元に戻すことはできません。よろしいですか?")
                                .arg(hubName),
                            QMessageBox::NoButton, this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }

    m_rpc->deleteHub(
        hubName, RpcUi::guarded(this, [this](const QJsonObject &) { refreshHubList(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想 HUB の削除"), error); }));
}

void HubListPage::onSetOnline()
{
    const QString hubName = selectedHubName();
    if (hubName.isEmpty()) {
        return;
    }
    m_rpc->setHubOnline(
        hubName, true, RpcUi::guarded(this, [this](const QJsonObject &) { refreshHubList(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想 HUB のオンライン化"), error); }));
}

void HubListPage::onSetOffline()
{
    const QString hubName = selectedHubName();
    if (hubName.isEmpty()) {
        return;
    }
    m_rpc->setHubOnline(
        hubName, false, RpcUi::guarded(this, [this](const QJsonObject &) { refreshHubList(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想 HUB のオフライン化"), error); }));
}

void HubListPage::onManageHub()
{
    const QString hubName = selectedHubName();
    if (hubName.isEmpty()) {
        return;
    }
    emit manageHubRequested(m_rpc, hubName);
}

void HubListPage::onShowStatus()
{
    const QString hubName = selectedHubName();
    if (hubName.isEmpty()) {
        return;
    }
    auto *dialog = ServerInfoDialogs::createHubStatusDialog(m_rpc, hubName, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}
