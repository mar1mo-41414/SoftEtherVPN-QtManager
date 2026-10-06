#include "AccessListDialog.h"
#include "AccessEditDialog.h"

#include "util/RpcUiHelpers.h"

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

#include <algorithm>

AccessListDialog::AccessListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_ACCESS_LIST CAPTION
    setWindowTitle(tr("アクセスリスト"));

    auto *titleLabel =
        new QLabel(tr("仮想 HUB \"%1\" には、現在以下のアクセスリスト (パケットフィルタリングルール) が登録されています。")
                       .arg(m_hubName),
                   this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    // SM_ACCESS_COLUMN_0〜5
    m_table->setHorizontalHeaderLabels({tr("ID"), tr("動作"), tr("状態"), tr("優先順位"), tr("説明"), tr("内容")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &AccessListDialog::onEdit);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &AccessListDialog::updateButtons);

    // B_ADD / B_ADD_V6 / IDOK(編集) / B_DELETE / B_CLONE / B_ENABLE / B_DISABLE / B_SAVE / IDCANCEL
    auto *addButton = new QPushButton(tr("追加 (IPv4)"), this);
    auto *addV6Button = new QPushButton(tr("追加 (IPv6)"), this);
    m_editButton = new QPushButton(tr("編集(&E)"), this);
    m_deleteButton = new QPushButton(tr("削除(&D)"), this);
    m_cloneButton = new QPushButton(tr("クローン(&O)"), this);
    m_enableButton = new QPushButton(tr("有効にする(&N)"), this);
    m_disableButton = new QPushButton(tr("無効にする(&I)"), this);
    auto *saveButton = new QPushButton(tr("保存(&S)"), this);
    auto *cancelButton = new QPushButton(tr("キャンセル(&C)"), this);
    connect(addButton, &QPushButton::clicked, this, &AccessListDialog::onAddIPv4);
    connect(addV6Button, &QPushButton::clicked, this, &AccessListDialog::onAddIPv6);
    connect(m_editButton, &QPushButton::clicked, this, &AccessListDialog::onEdit);
    connect(m_deleteButton, &QPushButton::clicked, this, &AccessListDialog::onDelete);
    connect(m_cloneButton, &QPushButton::clicked, this, &AccessListDialog::onClone);
    connect(m_enableButton, &QPushButton::clicked, this, [this]() { onSetActive(true); });
    connect(m_disableButton, &QPushButton::clicked, this, [this]() { onSetActive(false); });
    connect(saveButton, &QPushButton::clicked, this, &AccessListDialog::onSave);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    auto *hint = new QLabel(tr("優先順位はリストの上のものほど高くなります。"), this);
    hint->setWordWrap(true);

    auto *sideLayout = new QVBoxLayout;
    sideLayout->addWidget(addButton);
    sideLayout->addWidget(addV6Button);
    sideLayout->addWidget(m_editButton);
    sideLayout->addWidget(m_deleteButton);
    sideLayout->addSpacing(16);
    sideLayout->addWidget(m_cloneButton);
    sideLayout->addSpacing(16);
    sideLayout->addWidget(m_enableButton);
    sideLayout->addWidget(m_disableButton);
    sideLayout->addSpacing(8);
    sideLayout->addWidget(hint);
    sideLayout->addStretch();
    sideLayout->addWidget(saveButton);
    sideLayout->addWidget(cancelButton);

    auto *bodyLayout = new QHBoxLayout;
    bodyLayout->addWidget(m_table, 1);
    bodyLayout->addLayout(sideLayout);

    // STATIC2
    auto *footer = new QLabel(tr("VPN Server では、どのアクセスリスト項目にも一致しなかった IP パケットは、無条件で仮想 HUB を通過できます。"), this);
    footer->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addLayout(bodyLayout, 1);
    layout->addWidget(footer);

    resize(900, 520);
    updateButtons();
    load();
}

void AccessListDialog::load()
{
    m_rpc->enumAccess(
        m_hubName,
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            m_items.clear();
            const QJsonArray accessList = result.value("AccessList").toArray();
            for (const QJsonValue &value : accessList) {
                m_items.append(value.toObject());
            }
            refreshTable();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("アクセスリストの取得"), error); }));
}

void AccessListDialog::refreshTable()
{
    // 優先順位の昇順 (小さいほど高い)。同順位は元の並びを保つ。
    std::stable_sort(m_items.begin(), m_items.end(), [](const QJsonObject &a, const QJsonObject &b) {
        return a.value("Priority_u32").toDouble() < b.value("Priority_u32").toDouble();
    });

    m_table->setRowCount(m_items.size());
    for (int row = 0; row < m_items.size(); ++row) {
        const QJsonObject &access = m_items.at(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(static_cast<qint64>(access.value("Id_u32").toDouble()))));
        // SM_ACCESS_PASS / SM_ACCESS_DISCARD / SM_ACCESS_ENABLE / SM_ACCESS_DISABLE
        m_table->setItem(row, 1, new QTableWidgetItem(access.value("Discard_bool").toBool() ? tr("破棄") : tr("通過")));
        m_table->setItem(row, 2, new QTableWidgetItem(access.value("Active_bool").toBool() ? tr("有効") : tr("無効")));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(access.value("Priority_u32").toInt())));
        m_table->setItem(row, 4, new QTableWidgetItem(access.value("Note_utf").toString()));
        m_table->setItem(row, 5, new QTableWidgetItem(AccessEditDialog::describe(access)));
    }
    m_table->resizeColumnsToContents();
    for (int column = 0; column < 5; ++column) {
        m_table->setColumnWidth(column, qMax(m_table->columnWidth(column), column == 0 ? 40 : 70));
    }
    updateButtons();
}

int AccessListDialog::selectedRow() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    return selected.isEmpty() ? -1 : selected.first()->row();
}

int AccessListDialog::nextPriority() const
{
    int maxPriority = 0;
    for (const QJsonObject &item : m_items) {
        maxPriority = qMax(maxPriority, item.value("Priority_u32").toInt());
    }
    return m_items.isEmpty() ? 1000 : maxPriority + 100;
}

quint32 AccessListDialog::nextId() const
{
    quint32 maxId = 0;
    for (const QJsonObject &item : m_items) {
        maxId = qMax(maxId, static_cast<quint32>(item.value("Id_u32").toDouble()));
    }
    return maxId + 1;
}

void AccessListDialog::updateButtons()
{
    const int row = selectedRow();
    const bool selected = row >= 0;
    m_editButton->setEnabled(selected);
    m_deleteButton->setEnabled(selected);
    m_cloneButton->setEnabled(selected);
    m_enableButton->setEnabled(selected && !m_items.at(row).value("Active_bool").toBool());
    m_disableButton->setEnabled(selected && m_items.at(row).value("Active_bool").toBool());
}

void AccessListDialog::addItem(bool ipv6, const QJsonObject &base)
{
    AccessEditDialog dialog(m_rpc, m_hubName, ipv6, this);
    if (!base.isEmpty()) {
        dialog.setValues(base);
    }
    dialog.setPriority(base.isEmpty() ? nextPriority() : base.value("Priority_u32").toInt() + 1);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QJsonObject item = dialog.toRpcParams();
    item["Id_u32"] = static_cast<qint64>(nextId());
    item["Active_bool"] = base.isEmpty() ? true : base.value("Active_bool").toBool(true);
    m_items.append(item);
    refreshTable();
}

void AccessListDialog::onAddIPv4()
{
    addItem(false, QJsonObject());
}

void AccessListDialog::onAddIPv6()
{
    addItem(true, QJsonObject());
}

void AccessListDialog::onClone()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    // 選択項目の内容を引き継いだ新規項目として編集ダイアログを開く
    const QJsonObject base = m_items.at(row);
    addItem(base.value("IsIPv6_bool").toBool(), base);
}

void AccessListDialog::onEdit()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    const QJsonObject current = m_items.at(row);
    AccessEditDialog dialog(m_rpc, m_hubName, current.value("IsIPv6_bool").toBool(), this);
    dialog.setValues(current);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QJsonObject item = dialog.toRpcParams();
    item["Id_u32"] = current.value("Id_u32");
    item["Active_bool"] = current.value("Active_bool");
    m_items[row] = item;
    refreshTable();
}

void AccessListDialog::onDelete()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    // SM_ACCESS_DELETE_MSG 相当の確認
    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"), tr("選択したアクセスリスト項目を削除します。よろしいですか?"),
                            QMessageBox::NoButton, this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }
    m_items.removeAt(row);
    refreshTable();
}

void AccessListDialog::onSetActive(bool active)
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    m_items[row]["Active_bool"] = active;
    refreshTable();
    m_table->selectRow(row);
}

void AccessListDialog::onSave()
{
    QJsonArray list;
    for (const QJsonObject &item : m_items) {
        list.append(item);
    }
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    params["AccessList"] = list;
    m_rpc->call(
        QStringLiteral("SetAccessList"), params, RpcUi::guarded(this, [this](const QJsonObject &) { accept(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("アクセスリストの保存"), error); }));
}
