#include "HubListPage.h"
#include "AzureDialog.h"
#include "DdnsDialog.h"
#include "FarmDialog.h"
#include "FarmStatusDialog.h"
#include "HubEditDialog.h"
#include "HubStatusDialog.h"
#include "IPsecSettingsDialog.h"
#include "L3SwitchListDialog.h"
#include "ListenerDialog.h"
#include "LocalBridgeDialog.h"
#include "OpenVpnSstpDialog.h"
#include "ServerSettingsDialog.h"

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
    // 公式Managerの列構成 (SM_HUB_COLUMN_1〜9) を踏襲。
    m_hubTable = new QTableWidget(this);
    m_hubTable->setColumnCount(9);
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
    });
    m_hubTable->horizontalHeader()->setStretchLastSection(true);
    m_hubTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_hubTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_hubTable->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_hubTable, &QTableWidget::itemSelectionChanged, this, &HubListPage::onSelectionChanged);
    connect(m_hubTable, &QTableWidget::cellDoubleClicked, this, &HubListPage::onManageHub);

    m_serverInfoLabel = new QLabel(this);

    // D_SM_SERVER: IDOK(仮想HUBの管理) / B_CREATE / B_EDIT / B_DELETE / B_ONLINE / B_OFFLINE / B_HUB_STATUS
    m_manageButton = new QPushButton(tr("仮想 HUB の管理(&A)"), this);
    m_manageButton->setDefault(true);
    m_createButton = new QPushButton(tr("仮想 HUB の作成(&C)"), this);
    m_editButton = new QPushButton(tr("プロパティ(&E)"), this);
    m_deleteButton = new QPushButton(tr("削除(&D)"), this);
    m_onlineButton = new QPushButton(tr("オンライン(&O)"), this);
    m_offlineButton = new QPushButton(tr("オフライン(&F)"), this);
    m_statusButton = new QPushButton(tr("状態の表示(&S)"), this);
    m_refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    m_disconnectButton = new QPushButton(tr("閉じる(&X)"), this);

    connect(m_manageButton, &QPushButton::clicked, this, &HubListPage::onManageHub);
    connect(m_createButton, &QPushButton::clicked, this, &HubListPage::onCreateHub);
    connect(m_editButton, &QPushButton::clicked, this, &HubListPage::onEditHub);
    connect(m_deleteButton, &QPushButton::clicked, this, &HubListPage::onDeleteHub);
    connect(m_onlineButton, &QPushButton::clicked, this, &HubListPage::onSetOnline);
    connect(m_offlineButton, &QPushButton::clicked, this, &HubListPage::onSetOffline);
    connect(m_statusButton, &QPushButton::clicked, this, &HubListPage::onShowStatus);
    connect(m_refreshButton, &QPushButton::clicked, this, &HubListPage::refreshHubList);
    connect(m_disconnectButton, &QPushButton::clicked, this, &HubListPage::disconnectRequested);

    auto *hubButtonLayout = new QHBoxLayout;
    hubButtonLayout->addWidget(m_manageButton);
    hubButtonLayout->addWidget(m_createButton);
    hubButtonLayout->addWidget(m_editButton);
    hubButtonLayout->addWidget(m_deleteButton);
    hubButtonLayout->addWidget(m_onlineButton);
    hubButtonLayout->addWidget(m_offlineButton);
    hubButtonLayout->addWidget(m_statusButton);

    // 「サーバー情報の参照および設定」(D_SM_SERVER STATIC3) のボタン群。
    // いずれもサーバー全体の管理権限が必要なため、仮想HUB管理モードでは無効化する。
    auto *serverGrid = new QGridLayout;
    auto addServerButton = [this, serverGrid](int row, int column, const QString &text, std::function<void()> action) {
        auto *button = new QPushButton(text, this);
        connect(button, &QPushButton::clicked, this, [action = std::move(action)]() { action(); });
        serverGrid->addWidget(button, row, column);
        m_serverAdminButtons << button;
    };
    addServerButton(0, 0, tr("暗号化と通信関係の設定(&W)"), [this]() { onManageServerSettings(); });
    addServerButton(0, 1, tr("リスナーの管理(&J)"), [this]() { onManageListeners(); });
    addServerButton(0, 2, tr("ローカルブリッジ設定(&B)"), [this]() { onManageLocalBridge(); });
    addServerButton(0, 3, tr("レイヤ 3 スイッチ設定(&3)"), [this]() { L3SwitchListDialog(m_rpc, this).exec(); });
    addServerButton(1, 0, tr("IPsec / L&2TP 設定"), [this]() { IPsecSettingsDialog(m_rpc, this).exec(); });
    addServerButton(1, 1, tr("OpenVPN / MS-SSTP 設定"), [this]() { OpenVpnSstpDialog(m_rpc, this).exec(); });
    addServerButton(1, 2, tr("ダイナミック DNS 設定"), [this]() { DdnsDialog(m_rpc, this).exec(); });
    addServerButton(1, 3, tr("VPN Azure 設定"), [this]() { AzureDialog(m_rpc, this).exec(); });
    addServerButton(2, 0, tr("クラスタリング構成(&M)"), [this]() { onManageFarm(); });
    addServerButton(2, 1, tr("クラスタリング状態(&Z)"), [this]() { onShowFarmStatus(); });

    auto *serverGroup = new QGroupBox(tr("サーバー情報の参照および設定(&N)"), this);
    serverGroup->setLayout(serverGrid);

    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_refreshButton);
    bottomLayout->addWidget(m_disconnectButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_serverInfoLabel);
    layout->addWidget(m_hubTable);
    layout->addLayout(hubButtonLayout);
    layout->addWidget(serverGroup);
    layout->addLayout(bottomLayout);

    setHubActionButtonsEnabled(false);
}

void HubListPage::setConnection(VpnServerRpc *rpc, const QJsonObject &serverInfo, bool hubAdminMode)
{
    if (m_rpc) {
        m_rpc->deleteLater();
    }
    m_rpc = rpc;
    m_rpc->setParent(this);
    m_hubAdminMode = hubAdminMode;

    // 仮想HUB管理モードでは仮想HUBの作成/削除とサーバー全体の設定にサーバー管理権限が
    // 必要なため操作させない。
    m_createButton->setEnabled(!hubAdminMode);
    for (QPushButton *button : std::as_const(m_serverAdminButtons)) {
        button->setEnabled(!hubAdminMode);
    }
    m_deleteButton->setEnabled(false);

    applyServerInfo(serverInfo);
    refreshHubList();
}

void HubListPage::applyServerInfo(const QJsonObject &info)
{
    const QString productName = info.value("ServerProductName_str").toString();
    const QString version = info.value("ServerVersionString_str").toString();
    const QString hostName = info.value("ServerHostName_str").toString();
    m_serverName = hostName;
    m_serverInfoLabel->setText(tr("接続先: %1 (%2 %3)").arg(hostName, productName, version));
}

QString HubListPage::selectedHubName() const
{
    const QList<QTableWidgetItem *> selected = m_hubTable->selectedItems();
    if (selected.isEmpty()) {
        return QString();
    }
    return m_hubTable->item(selected.first()->row(), 0)->text();
}

void HubListPage::setHubActionButtonsEnabled(bool enabled)
{
    m_manageButton->setEnabled(enabled);
    m_editButton->setEnabled(enabled);
    m_deleteButton->setEnabled(enabled && !m_hubAdminMode);
    m_onlineButton->setEnabled(enabled);
    m_offlineButton->setEnabled(enabled);
    m_statusButton->setEnabled(enabled);
}

void HubListPage::onSelectionChanged()
{
    setHubActionButtonsEnabled(!selectedHubName().isEmpty());
}

void HubListPage::refreshHubList()
{
    if (!m_rpc) {
        return;
    }

    m_rpc->enumHub(
        [this](const QJsonObject &result) {
            const QJsonArray hubList = result.value("HubList").toArray();
            m_hubTable->setRowCount(hubList.size());

            for (int row = 0; row < hubList.size(); ++row) {
                const QJsonObject hub = hubList.at(row).toObject();

                m_hubTable->setItem(row, 0, new QTableWidgetItem(hub.value("HubName_str").toString()));
                m_hubTable->setItem(row, 1,
                                     new QTableWidgetItem(SoftEtherLabels::onlineStatus(hub.value("Online_bool").toBool())));
                m_hubTable->setItem(row, 2,
                                     new QTableWidgetItem(SoftEtherLabels::hubType(hub.value("HubType_u32").toInt())));
                m_hubTable->setItem(row, 3, new QTableWidgetItem(QString::number(hub.value("NumUsers_u32").toInt())));
                m_hubTable->setItem(row, 4, new QTableWidgetItem(QString::number(hub.value("NumGroups_u32").toInt())));
                m_hubTable->setItem(row, 5, new QTableWidgetItem(QString::number(hub.value("NumSessions_u32").toInt())));
                m_hubTable->setItem(row, 6, new QTableWidgetItem(QString::number(hub.value("NumMacTables_u32").toInt())));
                m_hubTable->setItem(row, 7, new QTableWidgetItem(QString::number(hub.value("NumIpTables_u32").toInt())));
                m_hubTable->setItem(row, 8, new QTableWidgetItem(QString::number(hub.value("NumLogin_u32").toInt())));
            }

            m_hubTable->resizeColumnsToContents();
            setHubActionButtonsEnabled(!selectedHubName().isEmpty());
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("仮想 HUB 一覧の取得に失敗しました: %1 (code %2)")
                                      .arg(error.message)
                                      .arg(error.code));
        });
}

void HubListPage::onCreateHub()
{
    HubEditDialog dialog(/*isNew=*/true, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_rpc->createHub(
        dialog.toRpcParams(), [this](const QJsonObject &) { refreshHubList(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("仮想 HUB の作成に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void HubListPage::onEditHub()
{
    const QString hubName = selectedHubName();
    if (hubName.isEmpty()) {
        return;
    }

    m_rpc->getHub(
        hubName,
        [this](const QJsonObject &hub) {
            auto *dialog = new HubEditDialog(/*isNew=*/false, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setValues(hub.value("HubName_str").toString(), hub.value("Online_bool").toBool(),
                               hub.value("NoEnum_bool").toBool(),
                               static_cast<quint32>(hub.value("MaxSession_u32").toDouble()));

            connect(dialog, &QDialog::accepted, this, [this, dialog]() {
                m_rpc->setHub(
                    dialog->toRpcParams(), [this](const QJsonObject &) { refreshHubList(); },
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
        hubName, [this](const QJsonObject &) { refreshHubList(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("仮想 HUB の削除に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void HubListPage::onSetOnline()
{
    const QString hubName = selectedHubName();
    if (hubName.isEmpty()) {
        return;
    }
    m_rpc->setHubOnline(
        hubName, true, [this](const QJsonObject &) { refreshHubList(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("仮想 HUB のオンライン化に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void HubListPage::onSetOffline()
{
    const QString hubName = selectedHubName();
    if (hubName.isEmpty()) {
        return;
    }
    m_rpc->setHubOnline(
        hubName, false, [this](const QJsonObject &) { refreshHubList(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("仮想 HUB のオフライン化に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void HubListPage::onManageHub()
{
    const QString hubName = selectedHubName();
    if (hubName.isEmpty()) {
        return;
    }
    emit manageHubRequested(m_rpc, hubName);
}

void HubListPage::onManageLocalBridge()
{
    QStringList hubNames;
    for (int row = 0; row < m_hubTable->rowCount(); ++row) {
        hubNames << m_hubTable->item(row, 0)->text();
    }
    LocalBridgeDialog dialog(m_rpc, hubNames, this);
    dialog.exec();
}

void HubListPage::onManageListeners()
{
    ListenerDialog dialog(m_rpc, this);
    dialog.exec();
}

void HubListPage::onManageServerSettings()
{
    ServerSettingsDialog dialog(m_rpc, this);
    dialog.exec();
}

void HubListPage::onManageFarm()
{
    FarmDialog dialog(m_rpc, m_serverName, this);
    dialog.exec();
}

void HubListPage::onShowFarmStatus()
{
    // 現在の動作モード(スタンドアロン/コントローラ/メンバ)によって表示内容が変わるため、先に取得する。
    m_rpc->call(
        QStringLiteral("GetFarmSetting"), {},
        [this](const QJsonObject &setting) {
            const int type = setting.value("ServerType_u32").toInt();
            if (type == 0) {
                QMessageBox::information(this, tr("クラスタリング状態"),
                                          tr("このサーバーはスタンドアロンサーバーとして動作しています。"
                                             "クラスタリング構成は行われていません。"));
                return;
            }
            FarmStatusDialog dialog(m_rpc, /*isController=*/type == 1, this);
            dialog.exec();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("クラスタリング構成の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void HubListPage::onShowStatus()
{
    const QString hubName = selectedHubName();
    if (hubName.isEmpty()) {
        return;
    }

    m_rpc->getHubStatus(
        hubName,
        [this, hubName](const QJsonObject &status) {
            auto *dialog = new HubStatusDialog(hubName, status, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->open();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("仮想 HUB の状態取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}
