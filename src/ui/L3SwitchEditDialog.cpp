#include "L3SwitchEditDialog.h"
#include "L3IfAddDialog.h"
#include "L3TableAddDialog.h"

#include "util/RpcUiHelpers.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

QTableWidget *makeTable(QWidget *parent, const QStringList &headers)
{
    auto *table = new QTableWidget(parent);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    return table;
}

} // namespace

L3SwitchEditDialog::L3SwitchEditDialog(VpnServerRpc *rpc, QString switchName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_switchName(std::move(switchName))
{
    // D_SM_L3_SW CAPTION
    setWindowTitle(tr("仮想レイヤ 3 スイッチ \"%1\" の編集").arg(m_switchName));

    auto *introLabel = new QLabel(
        tr("1 つの仮想レイヤ 3 スイッチには、複数個の仮想インターフェイスとルーティングテーブルを定義することができます。\n"
           "仮想インターフェイスは仮想 HUB に関連付けられ、仮想 HUB が動作しているときに仮想 HUB 内で 1 台の IP ホストのように"
           "動作します。"),
        this);
    introLabel->setWordWrap(true);

    // SM_L3_SW_IF_COLUMN1〜3 / SM_L3_SW_TABLE_COLUMN1〜4
    m_ifTable = makeTable(this, {tr("IP アドレス"), tr("サブネットマスク"), tr("仮想 HUB 名")});
    m_routeTable = makeTable(this, {tr("ネットワークアドレス"), tr("サブネットマスク"), tr("ゲートウェイアドレス"), tr("メトリック")});

    // B_ADD_IF / B_DEL_IF / B_ADD_TABLE / B_DEL_TABLE / B_START / B_STOP / IDCANCEL
    auto *addIfButton = new QPushButton(tr("仮想インターフェイスの追加(&A)"), this);
    m_deleteIfButton = new QPushButton(tr("仮想インターフェイスの削除(&E)"), this);
    auto *addRouteButton = new QPushButton(tr("ルーティングテーブルエントリの追加(&D)"), this);
    m_deleteRouteButton = new QPushButton(tr("ルーティングテーブルエントリの削除(&L)"), this);
    auto *startButton = new QPushButton(tr("動作開始(&S)"), this);
    auto *stopButton = new QPushButton(tr("動作停止(&T)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&C)"), this);
    m_deleteIfButton->setEnabled(false);
    m_deleteRouteButton->setEnabled(false);
    connect(m_ifTable, &QTableWidget::itemSelectionChanged, this,
            [this]() { m_deleteIfButton->setEnabled(!m_ifTable->selectedItems().isEmpty()); });
    connect(m_routeTable, &QTableWidget::itemSelectionChanged, this,
            [this]() { m_deleteRouteButton->setEnabled(!m_routeTable->selectedItems().isEmpty()); });
    connect(addIfButton, &QPushButton::clicked, this, &L3SwitchEditDialog::onAddInterface);
    connect(m_deleteIfButton, &QPushButton::clicked, this, &L3SwitchEditDialog::onDeleteInterface);
    connect(addRouteButton, &QPushButton::clicked, this, &L3SwitchEditDialog::onAddTable);
    connect(m_deleteRouteButton, &QPushButton::clicked, this, &L3SwitchEditDialog::onDeleteTable);
    connect(startButton, &QPushButton::clicked, this, &L3SwitchEditDialog::onStart);
    connect(stopButton, &QPushButton::clicked, this, &L3SwitchEditDialog::onStop);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *ifButtons = new QHBoxLayout;
    ifButtons->addWidget(addIfButton);
    ifButtons->addWidget(m_deleteIfButton);
    ifButtons->addStretch();
    auto *routeButtons = new QHBoxLayout;
    routeButtons->addWidget(addRouteButton);
    routeButtons->addWidget(m_deleteRouteButton);
    routeButtons->addStretch();
    auto *bottomButtons = new QHBoxLayout;
    bottomButtons->addWidget(startButton);
    bottomButtons->addWidget(stopButton);
    bottomButtons->addStretch();
    bottomButtons->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addWidget(new QLabel(tr("仮想インターフェイス一覧:"), this));
    layout->addWidget(m_ifTable);
    layout->addLayout(ifButtons);
    layout->addWidget(new QLabel(tr("ルーティングテーブル:"), this));
    layout->addWidget(m_routeTable);
    layout->addLayout(routeButtons);
    layout->addLayout(bottomButtons);

    resize(680, 600);
    reload();
}

void L3SwitchEditDialog::reload()
{
    QJsonObject params;
    params["Name_str"] = m_switchName;

    m_rpc->call(
        QStringLiteral("EnumL3If"), params,
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            const QJsonArray list = result.value("L3IFList").toArray();
            m_ifTable->setRowCount(list.size());
            for (int row = 0; row < list.size(); ++row) {
                const QJsonObject item = list.at(row).toObject();
                m_ifTable->setItem(row, 0, new QTableWidgetItem(item.value("IpAddress_ip").toString()));
                m_ifTable->setItem(row, 1, new QTableWidgetItem(item.value("SubnetMask_ip").toString()));
                m_ifTable->setItem(row, 2, new QTableWidgetItem(item.value("HubName_str").toString()));
            }
            m_ifTable->resizeColumnsToContents();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想インターフェイス一覧の取得"), error); }));

    m_rpc->call(
        QStringLiteral("EnumL3Table"), params,
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            const QJsonArray list = result.value("L3Table").toArray();
            m_routeTable->setRowCount(list.size());
            for (int row = 0; row < list.size(); ++row) {
                const QJsonObject item = list.at(row).toObject();
                m_routeTable->setItem(row, 0, new QTableWidgetItem(item.value("NetworkAddress_ip").toString()));
                m_routeTable->setItem(row, 1, new QTableWidgetItem(item.value("SubnetMask_ip").toString()));
                m_routeTable->setItem(row, 2, new QTableWidgetItem(item.value("GatewayAddress_ip").toString()));
                m_routeTable->setItem(row, 3, new QTableWidgetItem(QString::number(item.value("Metric_u32").toInt())));
            }
            m_routeTable->resizeColumnsToContents();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("ルーティングテーブルの取得"), error); }));
}

void L3SwitchEditDialog::onAddInterface()
{
    L3IfAddDialog dialog(m_rpc, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QJsonObject params = dialog.toRpcParams();
    params["Name_str"] = m_switchName;
    m_rpc->call(
        QStringLiteral("AddL3If"), params, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想インターフェイスの追加"), error); }));
}

void L3SwitchEditDialog::onDeleteInterface()
{
    const QList<QTableWidgetItem *> selected = m_ifTable->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    QJsonObject params;
    params["Name_str"] = m_switchName;
    params["HubName_str"] = m_ifTable->item(selected.first()->row(), 2)->text();
    m_rpc->call(
        QStringLiteral("DelL3If"), params, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想インターフェイスの削除"), error); }));
}

void L3SwitchEditDialog::onAddTable()
{
    L3TableAddDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QJsonObject params = dialog.toRpcParams();
    params["Name_str"] = m_switchName;
    m_rpc->call(
        QStringLiteral("AddL3Table"), params, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("ルーティングテーブルエントリの追加"), error); }));
}

void L3SwitchEditDialog::onDeleteTable()
{
    const QList<QTableWidgetItem *> selected = m_routeTable->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const int row = selected.first()->row();
    QJsonObject params;
    params["Name_str"] = m_switchName;
    params["NetworkAddress_ip"] = m_routeTable->item(row, 0)->text();
    params["SubnetMask_ip"] = m_routeTable->item(row, 1)->text();
    params["GatewayAddress_ip"] = m_routeTable->item(row, 2)->text();
    params["Metric_u32"] = m_routeTable->item(row, 3)->text().toInt();
    m_rpc->call(
        QStringLiteral("DelL3Table"), params, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("ルーティングテーブルエントリの削除"), error); }));
}

void L3SwitchEditDialog::onStart()
{
    QJsonObject params;
    params["Name_str"] = m_switchName;
    m_rpc->call(
        QStringLiteral("StartL3Switch"), params, [](const QJsonObject &) {},
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想レイヤ 3 スイッチの動作開始"), error); }));
}

void L3SwitchEditDialog::onStop()
{
    QJsonObject params;
    params["Name_str"] = m_switchName;
    m_rpc->call(
        QStringLiteral("StopL3Switch"), params, [](const QJsonObject &) {},
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("仮想レイヤ 3 スイッチの動作停止"), error); }));
}
