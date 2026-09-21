#include "HubListPage.h"
#include "HubEditDialog.h"
#include "HubStatusDialog.h"
#include "LocalBridgeDialog.h"

#include "util/SoftEtherLabels.h"

#include <QAbstractItemView>
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
    // B_BRIDGE / B_REFRESH / IDCANCEL
    m_localBridgeButton = new QPushButton(tr("ローカルブリッジ設定(&B)"), this);
    m_refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    m_disconnectButton = new QPushButton(tr("閉じる(&X)"), this);

    connect(m_manageButton, &QPushButton::clicked, this, &HubListPage::onManageHub);
    connect(m_createButton, &QPushButton::clicked, this, &HubListPage::onCreateHub);
    connect(m_editButton, &QPushButton::clicked, this, &HubListPage::onEditHub);
    connect(m_deleteButton, &QPushButton::clicked, this, &HubListPage::onDeleteHub);
    connect(m_onlineButton, &QPushButton::clicked, this, &HubListPage::onSetOnline);
    connect(m_offlineButton, &QPushButton::clicked, this, &HubListPage::onSetOffline);
    connect(m_statusButton, &QPushButton::clicked, this, &HubListPage::onShowStatus);
    connect(m_localBridgeButton, &QPushButton::clicked, this, &HubListPage::onManageLocalBridge);
    connect(m_refreshButton, &QPushButton::clicked, this, &HubListPage::refreshHubList);
    connect(m_disconnectButton, &QPushButton::clicked, this, &HubListPage::disconnectRequested);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_manageButton);
    buttonLayout->addWidget(m_createButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addWidget(m_onlineButton);
    buttonLayout->addWidget(m_offlineButton);
    buttonLayout->addWidget(m_statusButton);
    buttonLayout->addWidget(m_localBridgeButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_disconnectButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_serverInfoLabel);
    layout->addWidget(m_hubTable);
    layout->addLayout(buttonLayout);

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

    // 仮想HUB管理モードでは仮想HUBの作成/削除・ローカルブリッジ設定にサーバー管理権限が
    // 必要なため操作させない。
    m_createButton->setEnabled(!hubAdminMode);
    m_localBridgeButton->setEnabled(!hubAdminMode);
    m_deleteButton->setEnabled(false);

    applyServerInfo(serverInfo);
    refreshHubList();
}

void HubListPage::applyServerInfo(const QJsonObject &info)
{
    const QString productName = info.value("ServerProductName_str").toString();
    const QString version = info.value("ServerVersionString_str").toString();
    const QString hostName = info.value("ServerHostName_str").toString();
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
