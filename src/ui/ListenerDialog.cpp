#include "ListenerDialog.h"

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

ListenerDialog::ListenerDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    setWindowTitle(tr("リスナーの管理"));

    auto *titleLabel = new QLabel(tr("VPN Server がクライアントからの接続を待ち受ける TCP/IP ポートの一覧です。"), this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({tr("ポート番号"), tr("状態")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ListenerDialog::onSelectionChanged);

    // B_CREATE_LISTENER / B_DELETE_LISTENER / B_START / B_STOP
    auto *createButton = new QPushButton(tr("新規作成(&R)"), this);
    m_deleteButton = new QPushButton(tr("削除(&T)"), this);
    m_startButton = new QPushButton(tr("開始(&G)"), this);
    m_stopButton = new QPushButton(tr("停止(&P)"), this);
    connect(createButton, &QPushButton::clicked, this, &ListenerDialog::onCreate);
    connect(m_deleteButton, &QPushButton::clicked, this, &ListenerDialog::onDelete);
    connect(m_startButton, &QPushButton::clicked, this, &ListenerDialog::onStart);
    connect(m_stopButton, &QPushButton::clicked, this, &ListenerDialog::onStop);

    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(refreshButton, &QPushButton::clicked, this, &ListenerDialog::reload);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(480, 400);
    onSelectionChanged();
    reload();
}

int ListenerDialog::selectedPort(bool *ok) const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        *ok = false;
        return 0;
    }
    *ok = true;
    return m_table->item(selected.first()->row(), 0)->text().toInt();
}

void ListenerDialog::onSelectionChanged()
{
    bool ok = false;
    selectedPort(&ok);
    m_deleteButton->setEnabled(ok);
    m_startButton->setEnabled(ok);
    m_stopButton->setEnabled(ok);
}

void ListenerDialog::reload()
{
    m_rpc->enumListener(
        [this](const QJsonObject &result) {
            const QJsonArray list = result.value("ListenerList").toArray();
            m_table->setRowCount(list.size());
            for (int row = 0; row < list.size(); ++row) {
                const QJsonObject item = list.at(row).toObject();
                m_table->setItem(row, 0, new QTableWidgetItem(QString::number(item.value("Ports_u32").toInt())));

                QString state = item.value("Enables_bool").toBool() ? tr("動作中") : tr("停止中");
                if (item.value("Errors_bool").toBool()) {
                    state = tr("エラー");
                }
                m_table->setItem(row, 1, new QTableWidgetItem(state));
            }
            m_table->resizeColumnsToContents();
            onSelectionChanged();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("リスナー一覧の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void ListenerDialog::onCreate()
{
    // D_SM_CREATE_LISTENER
    QDialog createDialog(this);
    createDialog.setWindowTitle(tr("リスナーの新規作成"));
    auto *portSpin = new QSpinBox(&createDialog);
    portSpin->setRange(1, 65535);
    portSpin->setValue(443);
    auto *form = new QFormLayout;
    form->addRow(tr("ポート番号(&P):"), portSpin);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &createDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, &createDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &createDialog, &QDialog::reject);
    auto *layout = new QVBoxLayout(&createDialog);
    layout->addLayout(form);
    layout->addWidget(buttonBox);

    if (createDialog.exec() != QDialog::Accepted) {
        return;
    }

    m_rpc->createListener(
        static_cast<quint16>(portSpin->value()), true, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("リスナーの作成に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void ListenerDialog::onDelete()
{
    bool ok = false;
    const int port = selectedPort(&ok);
    if (!ok) {
        return;
    }
    m_rpc->deleteListener(
        static_cast<quint16>(port), [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("リスナーの削除に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void ListenerDialog::onStart()
{
    bool ok = false;
    const int port = selectedPort(&ok);
    if (!ok) {
        return;
    }
    m_rpc->enableListener(
        static_cast<quint16>(port), true, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("リスナーの開始に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void ListenerDialog::onStop()
{
    bool ok = false;
    const int port = selectedPort(&ok);
    if (!ok) {
        return;
    }
    m_rpc->enableListener(
        static_cast<quint16>(port), false, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("リスナーの停止に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}
