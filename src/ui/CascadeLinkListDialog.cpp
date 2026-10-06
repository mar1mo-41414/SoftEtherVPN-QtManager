#include "CascadeLinkListDialog.h"
#include "CascadeLinkEditDialog.h"
#include "CascadeLinkStatusDialog.h"

#include "util/RpcUiHelpers.h"
#include "util/ErrorStrings.h"
#include "util/SoftEtherLabels.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPointer>
#include <QTimer>
#include <QVBoxLayout>

namespace {

// SM_LINK_STATUS_OFFLINE / _ERROR / _ONLINE / SM_LINK_CONNECTING
QString linkStatusText(const QJsonObject &link)
{
    if (!link.value("Online_bool").toBool()) {
        return CascadeLinkListDialog::tr("オフライン (停止中)");
    }
    if (link.value("Connected_bool").toBool()) {
        return CascadeLinkListDialog::tr("オンライン (接続済み)");
    }
    const unsigned error = static_cast<unsigned>(link.value("LastError_u32").toDouble());
    if (error != 0) {
        return CascadeLinkListDialog::tr("エラー%1:%2").arg(error).arg(ErrorStrings::message(error));
    }
    return CascadeLinkListDialog::tr("接続処理中");
}

} // namespace

CascadeLinkListDialog::CascadeLinkListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_LINK CAPTION
    setWindowTitle(tr("%1 上のカスケード接続").arg(m_hubName));

    auto *titleLabel =
        new QLabel(tr("カスケード接続を使用すると、この仮想 HUB を同一または別のコンピュータ上で動作している他の仮想 HUB にレイヤ 2 カスケード接続することができます。"),
                   this);
    titleLabel->setWordWrap(true);
    // STATIC2 / STATIC3
    auto *warningBox = new QGroupBox(tr("カスケード接続における警告"), this);
    auto *warningLayout = new QVBoxLayout(warningBox);
    auto *warningLabel = new QLabel(
        tr("カスケード接続を使用すると、複数の仮想 HUB 間でのレイヤ 2 ブリッジが可能ですが、接続方法を間違えると、ループ状のカスケード接続を作成してしまう場合があります。カスケード接続機能を使用する際には、慎重にネットワークトポロジを設計してください。"),
        this);
    warningLabel->setWordWrap(true);
    warningLayout->addWidget(warningLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    // SM_LINK_COLUMN_1〜5
    m_table->setHorizontalHeaderLabels(
        {tr("接続設定名"), tr("状態"), tr("接続完了時刻"), tr("接続先 VPN Server"), tr("接続先仮想 HUB")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &CascadeLinkListDialog::onSelectionChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &CascadeLinkListDialog::onEdit);

    auto *createButton = new QPushButton(tr("新規作成(&C)"), this);
    m_editButton = new QPushButton(tr("編集(&E)"), this);
    m_onlineButton = new QPushButton(tr("オンライン(&N)"), this);
    m_offlineButton = new QPushButton(tr("オフライン(&F)"), this);
    m_statusButton = new QPushButton(tr("状態(&S)"), this);
    m_deleteButton = new QPushButton(tr("削除(&D)"), this);
    m_renameButton = new QPushButton(tr("名前の変更(&A)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);

    connect(createButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onCreate);
    connect(m_editButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onEdit);
    connect(m_onlineButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onSetOnline);
    connect(m_offlineButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onSetOffline);
    connect(m_statusButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onShowStatus);
    connect(m_deleteButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onDelete);
    connect(m_renameButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onRename);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_onlineButton);
    buttonLayout->addWidget(m_offlineButton);
    buttonLayout->addWidget(m_statusButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addWidget(m_renameButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(warningBox);
    layout->addWidget(m_table, 1);
    layout->addLayout(buttonLayout);

    resize(780, 560);
    onSelectionChanged();
    reload();

    // 接続状態 (接続処理中→接続済み 等) は時間とともに変わるため、公式Managerと同様に定期更新する。
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() { reload(/*silent=*/true); });
    timer->start(2000);
}

QString CascadeLinkListDialog::selectedAccountName() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return QString();
    }
    return m_table->item(selected.first()->row(), 0)->text();
}

void CascadeLinkListDialog::onSelectionChanged()
{
    const bool hasSelection = !selectedAccountName().isEmpty();
    bool online = false;
    if (hasSelection) {
        online = m_table->item(m_table->selectedItems().first()->row(), 1)->data(Qt::UserRole).toBool();
    }
    m_editButton->setEnabled(hasSelection);
    m_onlineButton->setEnabled(hasSelection && !online);
    m_offlineButton->setEnabled(hasSelection && online);
    m_statusButton->setEnabled(hasSelection && online);
    m_deleteButton->setEnabled(hasSelection);
    m_renameButton->setEnabled(hasSelection);
}

void CascadeLinkListDialog::reload(bool silent)
{
    if (m_reloading) {
        return;
    }
    m_reloading = true;
    const QString previousSelection = selectedAccountName();
    QPointer<CascadeLinkListDialog> guard(this);
    m_rpc->enumLink(
        m_hubName,
        RpcUi::guarded(this, [this, previousSelection, guard](const QJsonObject &result) {
            if (!guard) {
                return;
            }
            m_reloading = false;
            const QJsonArray linkList = result.value("LinkList").toArray();
            m_table->setRowCount(linkList.size());
            for (int row = 0; row < linkList.size(); ++row) {
                const QJsonObject link = linkList.at(row).toObject();
                m_table->setItem(row, 0, new QTableWidgetItem(link.value("AccountName_utf").toString()));
                const bool connected = link.value("Connected_bool").toBool();
                auto *statusItem = new QTableWidgetItem(linkStatusText(link));
                statusItem->setData(Qt::UserRole, link.value("Online_bool").toBool());
                statusItem->setData(Qt::UserRole + 1, connected);
                m_table->setItem(row, 1, statusItem);
                m_table->setItem(row, 2, new QTableWidgetItem(connected ? SoftEtherLabels::dateTime(link.value("ConnectedTime_dt").toString())
                                                                         : QString()));
                m_table->setItem(row, 3, new QTableWidgetItem(link.value("Hostname_str").toString()));
                m_table->setItem(row, 4, new QTableWidgetItem(link.value("TargetHubName_str").toString()));
            }
            m_table->resizeColumnsToContents();
            for (int column = 0; column < m_table->columnCount(); ++column) {
                m_table->setColumnWidth(column, qBound(110, m_table->columnWidth(column), 260));
            }
            // 更新前に選択していた接続設定を選び直す
            for (int row = 0; row < m_table->rowCount(); ++row) {
                if (!previousSelection.isEmpty() && m_table->item(row, 0)->text() == previousSelection) {
                    m_table->selectRow(row);
                    break;
                }
            }
            onSelectionChanged();
        }),
        RpcUi::guarded(this, [this, silent, guard](const RpcError &error) {
            if (!guard) {
                return;
            }
            m_reloading = false;
            if (silent) {
                return; // 定期更新の失敗でダイアログを出し続けない
            }
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続一覧の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void CascadeLinkListDialog::onCreate()
{
    CascadeLinkEditDialog dialog(/*isNew=*/true, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QJsonObject params = dialog.toRpcParams();
    params["HubName_Ex_str"] = m_hubName;
    m_rpc->createLink(
        params, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続の作成に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void CascadeLinkListDialog::onEdit()
{
    const QString accountName = selectedAccountName();
    if (accountName.isEmpty()) {
        return;
    }

    m_rpc->getLink(
        m_hubName, accountName,
        RpcUi::guarded(this, [this](const QJsonObject &link) {
            auto *dialog = new CascadeLinkEditDialog(/*isNew=*/false, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setValues(link);
            connect(dialog, &QDialog::accepted, this, [this, dialog]() {
                QJsonObject params = dialog->toRpcParams();
                params["HubName_Ex_str"] = m_hubName;
                m_rpc->setLink(
                    params, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
                    RpcUi::guarded(this, [this](const RpcError &error) {
                        QMessageBox::warning(this, tr("エラー"),
                                              tr("カスケード接続の設定変更に失敗しました: %1 (code %2)")
                                                  .arg(error.message)
                                                  .arg(error.code));
                    }));
            });
            dialog->open();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続の設定取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void CascadeLinkListDialog::onDelete()
{
    const QString accountName = selectedAccountName();
    if (accountName.isEmpty()) {
        return;
    }

    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"),
                            tr("カスケード接続 \"%1\" を削除します。よろしいですか?").arg(accountName),
                            QMessageBox::NoButton, this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }

    m_rpc->deleteLink(
        m_hubName, accountName, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続の削除に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void CascadeLinkListDialog::onRename()
{
    const QString oldName = selectedAccountName();
    if (oldName.isEmpty()) {
        return;
    }

    QDialog renameDialog(this);
    renameDialog.setWindowTitle(tr("名前の変更"));
    auto *nameEdit = new QLineEdit(oldName, &renameDialog);
    auto *form = new QFormLayout;
    form->addRow(tr("新しい接続設定名(&N):"), nameEdit);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &renameDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &renameDialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &renameDialog, &QDialog::reject);
    auto *layout = new QVBoxLayout(&renameDialog);
    layout->addLayout(form);
    layout->addWidget(buttonBox);

    if (renameDialog.exec() != QDialog::Accepted) {
        return;
    }
    const QString newName = nameEdit->text().trimmed();
    if (newName.isEmpty() || newName == oldName) {
        return;
    }

    m_rpc->renameLink(
        m_hubName, oldName, newName, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続の名前の変更に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void CascadeLinkListDialog::onSetOnline()
{
    const QString accountName = selectedAccountName();
    if (accountName.isEmpty()) {
        return;
    }
    m_rpc->setLinkOnline(
        m_hubName, accountName, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続のオンライン化に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void CascadeLinkListDialog::onSetOffline()
{
    const QString accountName = selectedAccountName();
    if (accountName.isEmpty()) {
        return;
    }
    m_rpc->setLinkOffline(
        m_hubName, accountName, RpcUi::guarded(this, [this](const QJsonObject &) { reload(); }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続のオフライン化に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void CascadeLinkListDialog::onShowStatus()
{
    const QString accountName = selectedAccountName();
    if (accountName.isEmpty()) {
        return;
    }
    m_rpc->getLinkStatus(
        m_hubName, accountName,
        RpcUi::guarded(this, [this, accountName](const QJsonObject &status) {
            auto *dialog = new CascadeLinkStatusDialog(accountName, status, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->open();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続の状態取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}
