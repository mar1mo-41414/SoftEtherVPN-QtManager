#include "AccessListDialog.h"
#include "AccessEditDialog.h"

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

AccessListDialog::AccessListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_ACCESS_LIST CAPTION
    setWindowTitle(tr("アクセスリスト"));

    auto *titleLabel =
        new QLabel(tr("仮想 HUB \"%1\" には、現在以下のアクセスリスト (パケットフィルタリングルール) が登録されています。\n"
                       "優先順位はリストの上のものほど高くなります。どの項目にも一致しなかったパケットは無条件で通過します。")
                       .arg(m_hubName),
                   this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels(
        {tr("有効"), tr("優先順位"), tr("動作"), tr("説明"), tr("送信元"), tr("宛先"), tr("プロトコル/ポート")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &AccessListDialog::onEdit);

    // B_ADD(IPv4) / IDOK(編集) / B_DELETE / IDCANCEL
    auto *addButton = new QPushButton(tr("追加 (IPv&4)"), this);
    auto *editButton = new QPushButton(tr("編集(&E)"), this);
    auto *deleteButton = new QPushButton(tr("削除(&D)"), this);
    auto *closeButton = new QPushButton(tr("キャンセル(&C)"), this);
    connect(addButton, &QPushButton::clicked, this, &AccessListDialog::onAdd);
    connect(editButton, &QPushButton::clicked, this, &AccessListDialog::onEdit);
    connect(deleteButton, &QPushButton::clicked, this, &AccessListDialog::onDelete);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(760, 420);
    reload();
}

void AccessListDialog::reload()
{
    m_rpc->enumAccess(
        m_hubName,
        [this](const QJsonObject &result) {
            const QJsonArray accessList = result.value("AccessList").toArray();
            m_table->setRowCount(accessList.size());

            for (int row = 0; row < accessList.size(); ++row) {
                const QJsonObject access = accessList.at(row).toObject();

                auto *enabledItem = new QTableWidgetItem(access.value("Active_bool").toBool() ? tr("有効") : tr("無効"));
                enabledItem->setData(Qt::UserRole, access.value("Id_u32").toDouble());
                m_table->setItem(row, 0, enabledItem);
                m_table->setItem(row, 1, new QTableWidgetItem(QString::number(access.value("Priority_u32").toInt())));
                m_table->setItem(row, 2,
                                  new QTableWidgetItem(access.value("Discard_bool").toBool() ? tr("破棄") : tr("通過")));
                m_table->setItem(row, 3, new QTableWidgetItem(access.value("Note_utf").toString()));

                const QString srcIp = access.value("SrcIpAddress_ip").toString();
                const QString src = (srcIp.isEmpty() || srcIp == QStringLiteral("0.0.0.0"))
                                         ? tr("すべて")
                                         : QStringLiteral("%1 / %2").arg(srcIp, access.value("SrcSubnetMask_ip").toString());
                m_table->setItem(row, 4, new QTableWidgetItem(src));

                const QString dstIp = access.value("DestIpAddress_ip").toString();
                const QString dst =
                    (dstIp.isEmpty() || dstIp == QStringLiteral("0.0.0.0"))
                        ? tr("すべて")
                        : QStringLiteral("%1 / %2").arg(dstIp, access.value("DestSubnetMask_ip").toString());
                m_table->setItem(row, 5, new QTableWidgetItem(dst));

                m_table->setItem(row, 6, new QTableWidgetItem(SoftEtherLabels::protocolName(access.value("Protocol_u32").toInt())));
            }

            m_table->resizeColumnsToContents();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("アクセスリストの取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void AccessListDialog::onAdd()
{
    AccessEditDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_rpc->addAccess(
        m_hubName, dialog.toRpcParams(), [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("アクセスリスト項目の追加に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void AccessListDialog::onEdit()
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const int row = selected.first()->row();

    // このフェーズはEnumAccessの結果をそのまま編集ダイアログに渡す簡易実装のため、
    // 選択行のデータを再度EnumAccessで取り直す (件数が多くない前提)。
    m_rpc->enumAccess(
        m_hubName,
        [this, row](const QJsonObject &result) {
            const QJsonArray accessList = result.value("AccessList").toArray();
            if (row >= accessList.size()) {
                return;
            }
            const QJsonObject access = accessList.at(row).toObject();

            auto *dialog = new AccessEditDialog(this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setValues(access);
            const quint32 id = static_cast<quint32>(access.value("Id_u32").toDouble());
            connect(dialog, &QDialog::accepted, this, [this, dialog, id]() {
                // AddAccess/DeleteAccessのみが提供されているため、削除してから新しい内容で
                // 追加し直すことで編集を実現する。
                QJsonObject params = dialog->toRpcParams();
                m_rpc->deleteAccess(
                    m_hubName, id,
                    [this, params](const QJsonObject &) {
                        m_rpc->addAccess(
                            m_hubName, params, [this](const QJsonObject &) { reload(); },
                            [this](const RpcError &error) {
                                QMessageBox::warning(this, tr("エラー"),
                                                      tr("アクセスリスト項目の更新に失敗しました: %1 (code %2)")
                                                          .arg(error.message)
                                                          .arg(error.code));
                            });
                    },
                    [this](const RpcError &error) {
                        QMessageBox::warning(this, tr("エラー"),
                                              tr("アクセスリスト項目の更新に失敗しました: %1 (code %2)")
                                                  .arg(error.message)
                                                  .arg(error.code));
                    });
            });
            dialog->open();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("アクセスリストの取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void AccessListDialog::onDelete()
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const quint32 id = static_cast<quint32>(m_table->item(selected.first()->row(), 0)->data(Qt::UserRole).toUInt());

    m_rpc->deleteAccess(
        m_hubName, id, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("アクセスリスト項目の削除に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}
