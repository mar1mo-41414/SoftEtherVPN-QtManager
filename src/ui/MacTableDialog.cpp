#include "MacTableDialog.h"

#include "util/RpcUiHelpers.h"
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

MacTableDialog::MacTableDialog(VpnServerRpc *rpc, QString hubName, QString filterSessionName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
    , m_filterSessionName(std::move(filterSessionName))
{
    // D_SM_MAC CAPTION
    setWindowTitle(tr("MAC アドレステーブル"));

    // S_TITLE (+ SM_SESSION_FILTER)
    QString titleText = tr("仮想 HUB \"%1\" 上の MAC アドレステーブルデータベースは以下の通りです。").arg(m_hubName);
    if (!m_filterSessionName.isEmpty()) {
        titleText += tr(" (セッション %1 のエントリのみ表示)").arg(m_filterSessionName);
    }
    auto *titleLabel = new QLabel(titleText, this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    // SM_MAC_COLUMN_1/2/3/4/5/1A
    m_table->setHorizontalHeaderLabels(
        {tr("セッション名"), tr("VLAN ID"), tr("MAC アドレス"), tr("作成時刻"), tr("更新時刻"), tr("場所")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    // B_DELETE / B_REFRESH / IDCANCEL
    auto *deleteButton = new QPushButton(tr("選択したエントリを削除(&D)"), this);
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(deleteButton, &QPushButton::clicked, this, &MacTableDialog::onDelete);
    connect(refreshButton, &QPushButton::clicked, this, &MacTableDialog::reload);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(680, 420);
    reload();
}

void MacTableDialog::reload()
{
    m_rpc->enumMacTable(
        m_hubName,
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            const QJsonArray macTable = result.value("MacTable").toArray();

            m_table->setRowCount(0);
            int row = 0;
            for (const QJsonValue &value : macTable) {
                const QJsonObject entry = value.toObject();
                const QString sessionName = entry.value("SessionName_str").toString();
                if (!m_filterSessionName.isEmpty() && sessionName != m_filterSessionName) {
                    continue;
                }

                m_table->insertRow(row);
                auto *sessionItem = new QTableWidgetItem(sessionName);
                sessionItem->setData(Qt::UserRole, entry.value("Key_u32").toDouble());
                m_table->setItem(row, 0, sessionItem);
                m_table->setItem(row, 1, new QTableWidgetItem(QString::number(entry.value("VlanId_u32").toInt())));
                m_table->setItem(row, 2, new QTableWidgetItem(SoftEtherLabels::macAddress(
                                              entry.value("MacAddress_bin").toString())));
                m_table->setItem(row, 3,
                                  new QTableWidgetItem(SoftEtherLabels::dateTime(entry.value("CreatedTime_dt").toString())));
                m_table->setItem(row, 4,
                                  new QTableWidgetItem(SoftEtherLabels::dateTime(entry.value("UpdatedTime_dt").toString())));
                m_table->setItem(row, 5,
                                  new QTableWidgetItem(SoftEtherLabels::macIpLocation(
                                      entry.value("RemoteItem_bool").toBool(), entry.value("RemoteHostname_str").toString())));
                ++row;
            }

            m_table->resizeColumnsToContents();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("MAC アドレステーブルの取得に失敗しました: %1 (code %2)")
                                      .arg(error.message)
                                      .arg(error.code));
        }));
}

void MacTableDialog::onDelete()
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const quint32 key = static_cast<quint32>(m_table->item(selected.first()->row(), 0)->data(Qt::UserRole).toUInt());

    m_rpc->deleteMacTable(
        m_hubName, key, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("MAC アドレステーブルエントリの削除に失敗しました: %1 (code %2)")
                                      .arg(error.message)
                                      .arg(error.code));
        }));
}
