#include "L3SwitchListDialog.h"
#include "L3SwitchEditDialog.h"

#include "util/RpcUiHelpers.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

L3SwitchListDialog::L3SwitchListDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_L3 CAPTION
    setWindowTitle(tr("仮想レイヤ 3 スイッチ設定"));

    auto *introLabel = new QLabel(
        tr("この VPN Server 内で動作している複数の仮想 HUB 間で仮想のレイヤ 3 スイッチを定義し、異なった IP ネットワーク間を"
           "ルーティングすることができます。\n\n"
           "仮想レイヤ 3 スイッチ機能は、ネットワークおよび IP ルーティングに関する詳しい知識をお持ちの方やネットワーク管理者の"
           "ための機能です。通常の VPN 機能を使用する場合は、仮想レイヤ 3 スイッチ機能を使用する必要はありません。"),
        this);
    introLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    // SM_L3_SW_COLUMN1〜4
    m_table->setHorizontalHeaderLabels({tr("レイヤ 3 スイッチ名"), tr("動作状況"), tr("インターフェイス数"), tr("ルーティングテーブル数")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &L3SwitchListDialog::onSelectionChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &L3SwitchListDialog::onEdit);

    // B_ADD / B_START / B_STOP / IDOK(編集) / B_DELETE / IDCANCEL
    auto *addButton = new QPushButton(tr("新規作成(&N)"), this);
    m_startButton = new QPushButton(tr("動作開始(&S)"), this);
    m_stopButton = new QPushButton(tr("動作停止(&T)"), this);
    m_editButton = new QPushButton(tr("編集(&E)"), this);
    m_deleteButton = new QPushButton(tr("削除(&D)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&C)"), this);
    connect(addButton, &QPushButton::clicked, this, &L3SwitchListDialog::onAdd);
    connect(m_startButton, &QPushButton::clicked, this, &L3SwitchListDialog::onStart);
    connect(m_stopButton, &QPushButton::clicked, this, &L3SwitchListDialog::onStop);
    connect(m_editButton, &QPushButton::clicked, this, &L3SwitchListDialog::onEdit);
    connect(m_deleteButton, &QPushButton::clicked, this, &L3SwitchListDialog::onDelete);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addWidget(new QLabel(tr("定義されている仮想レイヤ 3 スイッチの一覧(&L):"), this));
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(680, 460);
    onSelectionChanged();
    reload();
}

QString L3SwitchListDialog::selectedName() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    return selected.isEmpty() ? QString() : m_table->item(selected.first()->row(), 0)->text();
}

void L3SwitchListDialog::onSelectionChanged()
{
    const bool hasSelection = !selectedName().isEmpty();
    m_editButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
    m_startButton->setEnabled(hasSelection);
    m_stopButton->setEnabled(hasSelection);
}

void L3SwitchListDialog::reload()
{
    m_rpc->call(
        QStringLiteral("EnumL3Switch"), {},
        [this](const QJsonObject &result) {
            const QJsonArray list = result.value("L3SWList").toArray();
            m_table->setRowCount(list.size());
            for (int row = 0; row < list.size(); ++row) {
                const QJsonObject item = list.at(row).toObject();
                // SM_L3_SW_ST_F_F / SM_L3_SW_ST_T_F / SM_L3_SW_ST_T_T
                QString state = tr("停止");
                if (item.value("Online_bool").toBool()) {
                    state = item.value("Active_bool").toBool() ? tr("開始 (動作中)") : tr("開始 (エラー)");
                }
                m_table->setItem(row, 0, new QTableWidgetItem(item.value("Name_str").toString()));
                m_table->setItem(row, 1, new QTableWidgetItem(state));
                m_table->setItem(row, 2, new QTableWidgetItem(QString::number(item.value("NumInterfaces_u32").toInt())));
                m_table->setItem(row, 3, new QTableWidgetItem(QString::number(item.value("NumTables_u32").toInt())));
            }
            m_table->resizeColumnsToContents();
            onSelectionChanged();
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("仮想レイヤ 3 スイッチ一覧の取得"), error); });
}

void L3SwitchListDialog::onAdd()
{
    // D_SM_L3_ADD
    QDialog nameDialog(this);
    nameDialog.setWindowTitle(tr("新規仮想レイヤ 3 スイッチの作成"));
    auto *nameEdit = new QLineEdit(&nameDialog);
    auto *form = new QFormLayout;
    form->addRow(tr("名前(&N):"), nameEdit);
    auto *hint = new QLabel(
        tr("新しい仮想レイヤ 3 スイッチを作成します。名前を入力してください。\n"
           "仮想レイヤ 3 スイッチの名前は、既にこの VPN Server に存在する他の仮想レイヤ 3 スイッチと重複することはできません。"),
        &nameDialog);
    hint->setWordWrap(true);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &nameDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, &nameDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &nameDialog, &QDialog::reject);
    auto *layout = new QVBoxLayout(&nameDialog);
    layout->addWidget(hint);
    layout->addLayout(form);
    layout->addWidget(buttonBox);
    nameDialog.resize(460, nameDialog.sizeHint().height());

    if (nameDialog.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) {
        return;
    }

    QJsonObject params;
    params["Name_str"] = nameEdit->text().trimmed();
    m_rpc->call(
        QStringLiteral("AddL3Switch"), params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("仮想レイヤ 3 スイッチの作成"), error); });
}

void L3SwitchListDialog::onEdit()
{
    const QString name = selectedName();
    if (name.isEmpty()) {
        return;
    }
    L3SwitchEditDialog dialog(m_rpc, name, this);
    dialog.exec();
    reload();
}

void L3SwitchListDialog::onDelete()
{
    const QString name = selectedName();
    if (name.isEmpty()) {
        return;
    }
    // SM_L3_SW_DEL_MSG
    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"),
                            tr("仮想レイヤ 3 スイッチ \"%1\" を削除します。\nよろしいですか?").arg(name), QMessageBox::NoButton, this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }
    QJsonObject params;
    params["Name_str"] = name;
    m_rpc->call(
        QStringLiteral("DelL3Switch"), params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("仮想レイヤ 3 スイッチの削除"), error); });
}

void L3SwitchListDialog::onStart()
{
    const QString name = selectedName();
    if (name.isEmpty()) {
        return;
    }
    QJsonObject params;
    params["Name_str"] = name;
    m_rpc->call(
        QStringLiteral("StartL3Switch"), params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("仮想レイヤ 3 スイッチの動作開始"), error); });
}

void L3SwitchListDialog::onStop()
{
    const QString name = selectedName();
    if (name.isEmpty()) {
        return;
    }
    QJsonObject params;
    params["Name_str"] = name;
    m_rpc->call(
        QStringLiteral("StopL3Switch"), params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("仮想レイヤ 3 スイッチの動作停止"), error); });
}
