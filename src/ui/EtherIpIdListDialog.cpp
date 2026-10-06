#include "EtherIpIdListDialog.h"
#include "EtherIpIdEditDialog.h"

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

EtherIpIdListDialog::EtherIpIdListDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_ETHERIP CAPTION
    setWindowTitle(tr("EtherIP / L2TPv3 サーバー機能の詳細設定"));

    auto *introLabel = new QLabel(
        tr("EtherIP / L2TPv3 による接続を受け付けるには、予め、クライアント側となる EtherIP / L2TPv3 対応ルータが"
           "この VPN Server に接続する際の IPsec Phase 1 ID 文字列と、接続先の仮想 HUB の情報の対応表を"
           "定義しておく必要があります。"),
        this);
    introLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    // SM_ETHERIP_COLUMN_0/1/2
    m_table->setHorizontalHeaderLabels({tr("ISAKMP Phase 1 ID"), tr("仮想 HUB 名"), tr("ユーザー名")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &EtherIpIdListDialog::onSelectionChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &EtherIpIdListDialog::onEdit);

    // B_ADD / IDOK(編集) / B_DELETE / IDCANCEL
    auto *addButton = new QPushButton(tr("追加(&A)"), this);
    m_editButton = new QPushButton(tr("編集(&E)"), this);
    m_deleteButton = new QPushButton(tr("削除(&D)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(addButton, &QPushButton::clicked, this, &EtherIpIdListDialog::onAdd);
    connect(m_editButton, &QPushButton::clicked, this, &EtherIpIdListDialog::onEdit);
    connect(m_deleteButton, &QPushButton::clicked, this, &EtherIpIdListDialog::onDelete);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(600, 400);
    onSelectionChanged();
    reload();
}

QString EtherIpIdListDialog::selectedId() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    return selected.isEmpty() ? QString() : m_table->item(selected.first()->row(), 0)->text();
}

void EtherIpIdListDialog::onSelectionChanged()
{
    const bool hasSelection = !selectedId().isEmpty();
    m_editButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
}

void EtherIpIdListDialog::reload()
{
    m_rpc->call(
        QStringLiteral("EnumEtherIpId"), {},
        [this](const QJsonObject &result) {
            const QJsonArray settings = result.value("Settings").toArray();
            m_table->setRowCount(settings.size());
            for (int row = 0; row < settings.size(); ++row) {
                const QJsonObject item = settings.at(row).toObject();
                m_table->setItem(row, 0, new QTableWidgetItem(item.value("Id_str").toString()));
                m_table->setItem(row, 1, new QTableWidgetItem(item.value("HubName_str").toString()));
                m_table->setItem(row, 2, new QTableWidgetItem(item.value("UserName_str").toString()));
            }
            m_table->resizeColumnsToContents();
            onSelectionChanged();
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("EtherIP / L2TPv3 定義一覧の取得"), error); });
}

void EtherIpIdListDialog::onAdd()
{
    EtherIpIdEditDialog dialog(m_rpc, /*isNew=*/true, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_rpc->call(
        QStringLiteral("AddEtherIpId"), dialog.toRpcParams(), [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("EtherIP / L2TPv3 定義の追加"), error); });
}

void EtherIpIdListDialog::onEdit()
{
    const QString id = selectedId();
    if (id.isEmpty()) {
        return;
    }

    QJsonObject idParams;
    idParams["Id_str"] = id;
    m_rpc->call(
        QStringLiteral("GetEtherIpId"), idParams,
        [this](const QJsonObject &current) {
            auto *dialog = new EtherIpIdEditDialog(m_rpc, /*isNew=*/false, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setValues(current);
            connect(dialog, &QDialog::accepted, this, [this, dialog, current]() {
                // 個別の更新APIが無いため、削除してから追加し直す。追加に失敗した場合は元の定義を復元する。
                const QJsonObject newParams = dialog->toRpcParams();
                QJsonObject deleteParams;
                deleteParams["Id_str"] = current.value("Id_str").toString();
                m_rpc->call(
                    QStringLiteral("DeleteEtherIpId"), deleteParams,
                    [this, newParams, current](const QJsonObject &) {
                        m_rpc->call(
                            QStringLiteral("AddEtherIpId"), newParams, [this](const QJsonObject &) { reload(); },
                            [this, current](const RpcError &error) {
                                RpcUi::showError(this, tr("EtherIP / L2TPv3 定義の更新"), error);
                                m_rpc->call(
                                    QStringLiteral("AddEtherIpId"), current, [this](const QJsonObject &) { reload(); },
                                    [](const RpcError &) {});
                            });
                    },
                    [this](const RpcError &error) { RpcUi::showError(this, tr("EtherIP / L2TPv3 定義の更新"), error); });
            });
            dialog->open();
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("EtherIP / L2TPv3 定義の取得"), error); });
}

void EtherIpIdListDialog::onDelete()
{
    const QString id = selectedId();
    if (id.isEmpty()) {
        return;
    }
    QJsonObject params;
    params["Id_str"] = id;
    m_rpc->call(
        QStringLiteral("DeleteEtherIpId"), params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("EtherIP / L2TPv3 定義の削除"), error); });
}
