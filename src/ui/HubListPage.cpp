#include "HubListPage.h"

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

namespace {

// SM_HUB_ONLINE / SM_HUB_OFFLINE (strtable_ja.stb)
QString onlineStatusText(bool online)
{
    return online ? HubListPage::tr("オンライン") : HubListPage::tr("オフライン");
}

// SM_HUB_STANDALONE / SM_HUB_STATIC / SM_HUB_DYNAMIC (strtable_ja.stb)
QString hubTypeText(int hubType)
{
    switch (hubType) {
    case 0:
        return HubListPage::tr("スタンドアロン");
    case 1:
        return HubListPage::tr("スタティック仮想 HUB");
    case 2:
        return HubListPage::tr("ダイナミック仮想 HUB");
    default:
        return HubListPage::tr("不明");
    }
}

} // namespace

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

    m_serverInfoLabel = new QLabel(this);

    // B_REFRESH / IDCANCEL (D_SM_SERVER)
    m_refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    m_disconnectButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(m_refreshButton, &QPushButton::clicked, this, &HubListPage::refreshHubList);
    connect(m_disconnectButton, &QPushButton::clicked, this, &HubListPage::disconnectRequested);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_disconnectButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_serverInfoLabel);
    layout->addWidget(m_hubTable);
    layout->addLayout(buttonLayout);
}

void HubListPage::setConnection(VpnServerRpc *rpc, const QJsonObject &serverInfo)
{
    if (m_rpc) {
        m_rpc->deleteLater();
    }
    m_rpc = rpc;
    m_rpc->setParent(this);

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
                m_hubTable->setItem(row, 1, new QTableWidgetItem(onlineStatusText(hub.value("Online_bool").toBool())));
                m_hubTable->setItem(row, 2, new QTableWidgetItem(hubTypeText(hub.value("HubType_u32").toInt())));
                m_hubTable->setItem(row, 3, new QTableWidgetItem(QString::number(hub.value("NumUsers_u32").toInt())));
                m_hubTable->setItem(row, 4, new QTableWidgetItem(QString::number(hub.value("NumGroups_u32").toInt())));
                m_hubTable->setItem(row, 5, new QTableWidgetItem(QString::number(hub.value("NumSessions_u32").toInt())));
                m_hubTable->setItem(row, 6, new QTableWidgetItem(QString::number(hub.value("NumMacTables_u32").toInt())));
                m_hubTable->setItem(row, 7, new QTableWidgetItem(QString::number(hub.value("NumIpTables_u32").toInt())));
                m_hubTable->setItem(row, 8, new QTableWidgetItem(QString::number(hub.value("NumLogin_u32").toInt())));
            }

            m_hubTable->resizeColumnsToContents();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("仮想 HUB 一覧の取得に失敗しました: %1 (code %2)")
                                      .arg(error.message)
                                      .arg(error.code));
        });
}
