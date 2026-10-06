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

    // S_TITLE / S01 / S02
    auto note = [this](const QString &text) {
        auto *label = new QLabel(text, this);
        label->setWordWrap(true);
        return label;
    };
    auto *titleLabel = new QLabel(tr("EtherIP / L2TPv3 サーバー機能"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleLabel->setFont(titleFont);
    auto *tableCaption = new QLabel(tr("IPsec Phase 1 ID と接続先仮想 HUB との対応表(T):"), this);
    QFont boldFont = tableCaption->font();
    boldFont.setBold(true);
    tableCaption->setFont(boldFont);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    // SM_ETHERIP_COLUMN_0/1/2
    m_table->setHorizontalHeaderLabels({tr("ISAKMP Phase 1 ID"), tr("仮想 HUB 名"), tr("ユーザー名")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
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

    // 公式Managerは操作ボタンを表の右側に縦に並べる
    auto *buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    auto *body = new QHBoxLayout;
    body->addWidget(m_table, 1);
    body->addLayout(buttonLayout);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(note(tr("VPN Server に EtherIP / L2TPv3 over IPsec に対応した市販のルータ機器からレイヤ 2 Ethernet ブリッジ接続を行うことができます。\nCisco 社のルータや NEC 製の IX ルータ、IIJ 製の SEIL ルータ等がお勧めです。")));
    layout->addWidget(note(tr("EtherIP / L2TPv3 による接続を受け付けるには、予め、クライアント側となる EtherIP / L2TPv3 対応ルータがこの VPN Server に接続する際の IPsec Phase 1 ID 文字列と、接続先の仮想 HUB の情報の対応表を定義しておく必要があります。")));
    layout->addWidget(tableCaption);
    layout->addLayout(body, 1);

    resize(700, 520);
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
        RpcUi::guarded(this, [this](const QJsonObject &result) {
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
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("EtherIP / L2TPv3 定義一覧の取得"), error); }));
}

void EtherIpIdListDialog::onAdd()
{
    EtherIpIdEditDialog dialog(m_rpc, /*isNew=*/true, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_rpc->call(
        QStringLiteral("AddEtherIpId"), dialog.toRpcParams(),
        RpcUi::guarded(this, [this](const QJsonObject &) {
            reload();
            // SM_ETHERIP_ADD_OK
            QMessageBox::information(this, tr("EtherIP / L2TPv3 サーバー機能の詳細設定"),
                                      tr("新しい EtherIP / L2TPv3 クライアントの接続設定を追加しました。"));
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("EtherIP / L2TPv3 定義の追加"), error); }));
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
        RpcUi::guarded(this, [this](const QJsonObject &current) {
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
                    RpcUi::guarded(this, [this, newParams, current](const QJsonObject &) {
                        m_rpc->call(
                            QStringLiteral("AddEtherIpId"), newParams, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
                            RpcUi::guarded(this, [this, current](const RpcError &error) {
                                RpcUi::showError(this, tr("EtherIP / L2TPv3 定義の更新"), error);
                                m_rpc->call(
                                    QStringLiteral("AddEtherIpId"), current, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
                                    [](const RpcError &) {});
                            }));
                    }),
                    RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("EtherIP / L2TPv3 定義の更新"), error); }));
            });
            dialog->open();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("EtherIP / L2TPv3 定義の取得"), error); }));
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
        QStringLiteral("DeleteEtherIpId"), params, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("EtherIP / L2TPv3 定義の削除"), error); }));
}
