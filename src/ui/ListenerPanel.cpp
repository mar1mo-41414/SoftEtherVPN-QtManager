#include "ListenerPanel.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

ListenerPanel::ListenerPanel(QWidget *parent)
    : QGroupBox(tr("リスナーの管理(&J)"), parent)
{
    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    // CM_LISTENER_COLUMN_1/2
    m_table->setHorizontalHeaderLabels({tr("ポート番号"), tr("状態")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ListenerPanel::updateButtons);

    // B_CREATE_LISTENER / B_DELETE_LISTENER / B_START / B_STOP
    m_createButton = new QPushButton(tr("新規作成(&R)"), this);
    m_deleteButton = new QPushButton(tr("削除(&T)"), this);
    m_startButton = new QPushButton(tr("開始(&G)"), this);
    m_stopButton = new QPushButton(tr("停止(&P)"), this);
    connect(m_createButton, &QPushButton::clicked, this, &ListenerPanel::onCreate);
    connect(m_deleteButton, &QPushButton::clicked, this, &ListenerPanel::onDelete);
    connect(m_startButton, &QPushButton::clicked, this, &ListenerPanel::onStart);
    connect(m_stopButton, &QPushButton::clicked, this, &ListenerPanel::onStop);

    auto *buttons = new QVBoxLayout;
    buttons->addWidget(m_createButton);
    buttons->addWidget(m_deleteButton);
    buttons->addWidget(m_startButton);
    buttons->addWidget(m_stopButton);
    buttons->addStretch();

    auto *row = new QHBoxLayout;
    row->addWidget(m_table, 1);
    row->addLayout(buttons);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("リスナー一覧 (TCP/IP ポート):"), this));
    layout->addLayout(row, 1);

    updateButtons();
}

void ListenerPanel::setRpc(VpnServerRpc *rpc)
{
    m_rpc = rpc;
    if (!m_rpc) {
        m_table->setRowCount(0);
    }
    updateButtons();
}

void ListenerPanel::setOperable(bool operable)
{
    m_operable = operable;
    updateButtons();
}

int ListenerPanel::selectedPort(bool *running) const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return 0;
    }
    const int row = selected.first()->row();
    if (running) {
        *running = m_table->item(row, 0)->data(Qt::UserRole).toBool();
    }
    return m_table->item(row, 0)->data(Qt::UserRole + 1).toInt();
}

void ListenerPanel::updateButtons()
{
    bool running = false;
    const bool hasSelection = selectedPort(&running) > 0;
    m_createButton->setEnabled(m_operable && m_rpc);
    m_deleteButton->setEnabled(m_operable && hasSelection);
    // 公式同様、動作中なら「停止」、停止中なら「開始」だけを押せる。
    m_startButton->setEnabled(m_operable && hasSelection && !running);
    m_stopButton->setEnabled(m_operable && hasSelection && running);
}

void ListenerPanel::reload()
{
    if (!m_rpc) {
        return;
    }
    m_rpc->enumListener(
        [this](const QJsonObject &result) {
            const QJsonArray list = result.value("ListenerList").toArray();
            m_table->setRowCount(list.size());
            for (int row = 0; row < list.size(); ++row) {
                const QJsonObject item = list.at(row).toObject();
                const int port = item.value("Ports_u32").toInt();
                const bool enabled = item.value("Enables_bool").toBool();

                // CM_LISTENER_TCP_PORT / CM_LISTENER_ONLINE / CM_LISTENER_OFFLINE / CM_LISTENER_ERROR
                QString state = enabled ? tr("動作中") : tr("停止中");
                if (item.value("Errors_bool").toBool()) {
                    state = tr("エラー発生");
                }
                auto *portItem = new QTableWidgetItem(tr("TCP %1").arg(port));
                portItem->setData(Qt::UserRole, enabled);
                portItem->setData(Qt::UserRole + 1, port);
                m_table->setItem(row, 0, portItem);
                m_table->setItem(row, 1, new QTableWidgetItem(state));
            }
            m_table->resizeColumnToContents(0);
            updateButtons();
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("リスナー一覧の取得"), error); });
}

void ListenerPanel::onCreate()
{
    // D_SM_CREATE_LISTENER
    QDialog createDialog(this);
    createDialog.setWindowTitle(tr("リスナーの新規作成"));
    auto *hint = new QLabel(
        tr("VPN Server がクライアントからの接続を待ち受ける TCP/IP ポート番号を追加することができます。\n\n"
           "新しく追加するポート番号を指定してください。\n\n"
           "ポート番号がすでに別のサーバー プログラムによって使用されている場合など、ポートの確保に失敗した場合は、"
           "リスナーの状態がエラー状態となります。その場合は、同じポート番号を開いている別のプログラムを停止してください。"),
        &createDialog);
    hint->setWordWrap(true);
    auto *portSpin = new QSpinBox(&createDialog);
    portSpin->setRange(1, 65535);
    portSpin->setValue(443);
    auto *portRow = new QHBoxLayout;
    portRow->addWidget(portSpin);
    portRow->addWidget(new QLabel(tr("(TCP/IP ポート)"), &createDialog));
    portRow->addStretch();
    auto *form = new QFormLayout;
    form->addRow(tr("ポート番号(&P):"), portRow);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &createDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, &createDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &createDialog, &QDialog::reject);
    auto *layout = new QVBoxLayout(&createDialog);
    layout->addWidget(hint);
    layout->addLayout(form);
    layout->addWidget(buttonBox);
    DialogSizing::fitToWidth(&createDialog, 480);

    if (createDialog.exec() != QDialog::Accepted) {
        return;
    }
    m_rpc->createListener(
        static_cast<quint16>(portSpin->value()), true, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("リスナーの作成"), error); });
}

void ListenerPanel::onDelete()
{
    const int port = selectedPort(nullptr);
    if (port <= 0) {
        return;
    }
    m_rpc->deleteListener(
        static_cast<quint16>(port), [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("リスナーの削除"), error); });
}

void ListenerPanel::onStart()
{
    const int port = selectedPort(nullptr);
    if (port <= 0) {
        return;
    }
    m_rpc->enableListener(
        static_cast<quint16>(port), true, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("リスナーの開始"), error); });
}

void ListenerPanel::onStop()
{
    const int port = selectedPort(nullptr);
    if (port <= 0) {
        return;
    }
    m_rpc->enableListener(
        static_cast<quint16>(port), false, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("リスナーの停止"), error); });
}
