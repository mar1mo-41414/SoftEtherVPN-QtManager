#include "LocalBridgeDialog.h"

#include "util/RpcUiHelpers.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

LocalBridgeDialog::LocalBridgeDialog(VpnServerRpc *rpc, QStringList hubNames, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubNames(std::move(hubNames))
{
    // D_SM_BRIDGE CAPTION
    setWindowTitle(tr("ローカルブリッジ設定"));

    auto *titleLabel = new QLabel(
        tr("ローカルブリッジを使用すると、この VPN Server 上で動作する仮想 HUB と物理的な Ethernet デバイス "
           "(LAN カード) との間でレイヤ 2 ブリッジ接続を構成することができます。\n"
           "また、システムに tap デバイス (仮想のネットワークインターフェイス) を作成し、仮想 HUB との間でブリッジ接続することもできます。(Linux 版のみサポート)"),
        this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    // SM_BRIDGE_COLUMN_1〜4
    m_table->setHorizontalHeaderLabels({tr("番号"), tr("仮想 HUB 名"), tr("ブリッジ先 LAN カードまたは tap デバイス名"), tr("状態")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    // B_DELETE
    auto *deleteButton = new QPushButton(tr("ローカルブリッジの削除(&D)"), this);
    deleteButton->setEnabled(false);
    connect(deleteButton, &QPushButton::clicked, this, &LocalBridgeDialog::onDelete);
    connect(m_table, &QTableWidget::itemSelectionChanged, this,
            [this, deleteButton]() { deleteButton->setEnabled(!m_table->selectedItems().isEmpty()); });
    auto *tableButtonLayout = new QHBoxLayout;
    tableButtonLayout->addStretch();
    tableButtonLayout->addWidget(deleteButton);

    // STATIC2〜STATIC7: 新しいローカルブリッジの定義
    m_hubCombo = new QComboBox(this);
    m_hubCombo->setEditable(true);
    m_hubCombo->addItems(m_hubNames);

    m_physicalRadio = new QRadioButton(tr("物理的な既存の LAN カードとのブリッジ接続(&P)"), this);
    m_tapRadio = new QRadioButton(tr("新しい tap デバイスとのブリッジ接続(&T)"), this);
    m_physicalRadio->setChecked(true);
    connect(m_physicalRadio, &QRadioButton::toggled, this, &LocalBridgeDialog::onBridgeTypeToggled);

    m_ethernetCombo = new QComboBox(this);
    m_tapNameEdit = new QLineEdit(this);
    m_tapNameEdit->setMaxLength(11);
    m_tapNameEdit->setEnabled(false);

    auto *newForm = new QFormLayout;
    newForm->addRow(tr("仮想 HUB(&H):"), m_hubCombo);
    newForm->addRow(tr("作成する種類(&Y):"), m_physicalRadio);
    newForm->addRow(QString(), m_tapRadio);
    newForm->addRow(tr("LAN カード(&L):"), m_ethernetCombo);
    newForm->addRow(tr("新しい tap デバイス名(&V) (11 文字以内):"), m_tapNameEdit);

    // IDOK
    auto *addButton = new QPushButton(tr("ローカルブリッジを追加(&A)"), this);
    connect(addButton, &QPushButton::clicked, this, &LocalBridgeDialog::onAdd);
    auto *addButtonLayout = new QHBoxLayout;
    addButtonLayout->addStretch();
    addButtonLayout->addWidget(addButton);

    auto *newGroup = new QGroupBox(tr("新しいローカルブリッジの定義(N):"), this);
    auto *newGroupLayout = new QVBoxLayout(newGroup);
    newGroupLayout->addWidget(new QLabel(tr("ブリッジする仮想 HUB を選択するか、名前を入力してください。"), this));
    newGroupLayout->addLayout(newForm);
    auto *bridgeNote = new QLabel(tr("※ 稼動中の任意の LAN カード との間でブリッジするときは、高負荷環境においてはブリッジ専用に LAN カードを用意することをお勧めします。"), this);
    bridgeNote->setWordWrap(true);
    newGroupLayout->addWidget(bridgeNote);
    newGroupLayout->addLayout(addButtonLayout);

    // IDCANCEL
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(tableButtonLayout);
    layout->addWidget(newGroup);
    auto *footer = new QLabel(tr("最近システムに追加された LAN カードが表示されない場合は、コンピュータを再起動して再度この画面を開けば表示されます。"), this);
    footer->setWordWrap(true);
    layout->addWidget(footer);
    layout->addLayout(bottomLayout);

    resize(640, 620);
    loadEthernetList();
    reload();
}

void LocalBridgeDialog::onBridgeTypeToggled(bool usePhysical)
{
    m_ethernetCombo->setEnabled(usePhysical);
    m_tapNameEdit->setEnabled(!usePhysical);
}

void LocalBridgeDialog::loadEthernetList()
{
    m_rpc->enumEthernet(
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            const QJsonArray ethList = result.value("EthList").toArray();
            for (const QJsonValue &value : ethList) {
                const QJsonObject eth = value.toObject();
                const QString deviceName = eth.value("DeviceName_str").toString();
                const QString description = eth.value("NetworkConnectionName_utf").toString();
                m_ethernetCombo->addItem(description.isEmpty() ? deviceName : tr("%1 (%2)").arg(deviceName, description),
                                          deviceName);
            }
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("LAN カード一覧の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void LocalBridgeDialog::reload()
{
    m_rpc->enumLocalBridge(
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            const QJsonArray list = result.value("LocalBridgeList").toArray();
            m_table->setRowCount(list.size());
            for (int row = 0; row < list.size(); ++row) {
                const QJsonObject item = list.at(row).toObject();
                m_table->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
                m_table->setItem(row, 1, new QTableWidgetItem(item.value("HubNameLB_str").toString()));
                m_table->setItem(row, 2, new QTableWidgetItem(item.value("DeviceName_str").toString()));
                // SM_BRIDGE_ONLINE / SM_BRIDGE_ERROR / SM_BRIDGE_OFFLINE
                const bool active = item.value("Active_bool").toBool();
                const bool online = item.value("Online_bool").toBool();
                m_table->setItem(row, 3, new QTableWidgetItem(online ? (active ? tr("動作中") : tr("エラー発生")) : tr("オフライン")));
            }
            m_table->resizeColumnsToContents();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("ローカルブリッジ一覧の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void LocalBridgeDialog::onAdd()
{
    const QString hubName = m_hubCombo->currentText().trimmed();
    if (hubName.isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("仮想 HUB を指定してください。"));
        return;
    }

    const QString deviceName =
        m_physicalRadio->isChecked() ? m_ethernetCombo->currentData().toString() : m_tapNameEdit->text().trimmed();
    if (deviceName.isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("LAN カードまたは tap デバイス名を指定してください。"));
        return;
    }

    m_rpc->addLocalBridge(
        deviceName, hubName, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("ローカルブリッジの追加に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void LocalBridgeDialog::onDelete()
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const int row = selected.first()->row();
    const QString hubName = m_table->item(row, 1)->text();
    const QString deviceName = m_table->item(row, 2)->text();

    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"),
                            tr("仮想 HUB \"%1\" からデバイス \"%2\" までのローカルブリッジを削除しますか?").arg(hubName, deviceName),
                            QMessageBox::NoButton, this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }

    m_rpc->deleteLocalBridge(
        deviceName, hubName, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("ローカルブリッジの削除に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}
