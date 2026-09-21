#include "CascadeLinkListDialog.h"
#include "CascadeLinkEditDialog.h"
#include "CascadeLinkStatusDialog.h"

#include "util/SoftEtherLabels.h"

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

namespace {

QString linkStatusText(bool online, bool connected)
{
    if (!online) {
        return CascadeLinkListDialog::tr("オフライン");
    }
    return connected ? CascadeLinkListDialog::tr("オンライン (接続済み)") : CascadeLinkListDialog::tr("オンライン (未接続)");
}

} // namespace

CascadeLinkListDialog::CascadeLinkListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_LINK CAPTION
    setWindowTitle(tr("%1 上のカスケード接続").arg(m_hubName));

    auto *titleLabel = new QLabel(
        tr("カスケード接続を使用すると、この仮想 HUB を同一または別のコンピュータ上で動作している他の仮想 HUB に"
           "レイヤ 2 カスケード接続することができます。接続方法を間違えるとループが発生する可能性があるため、"
           "ネットワークトポロジには注意してください。"),
        this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(
        {tr("接続設定名"), tr("状態"), tr("接続先ホスト名"), tr("接続先仮想 HUB 名"), tr("接続完了時刻")});
    m_table->horizontalHeader()->setStretchLastSection(true);
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
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&R)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);

    connect(createButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onCreate);
    connect(m_editButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onEdit);
    connect(m_onlineButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onSetOnline);
    connect(m_offlineButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onSetOffline);
    connect(m_statusButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onShowStatus);
    connect(m_deleteButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onDelete);
    connect(m_renameButton, &QPushButton::clicked, this, &CascadeLinkListDialog::onRename);
    connect(refreshButton, &QPushButton::clicked, this, &CascadeLinkListDialog::reload);
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
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(780, 460);
    onSelectionChanged();
    reload();
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
    m_editButton->setEnabled(hasSelection);
    m_onlineButton->setEnabled(hasSelection);
    m_offlineButton->setEnabled(hasSelection);
    m_statusButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
    m_renameButton->setEnabled(hasSelection);
}

void CascadeLinkListDialog::reload()
{
    m_rpc->enumLink(
        m_hubName,
        [this](const QJsonObject &result) {
            const QJsonArray linkList = result.value("LinkList").toArray();
            m_table->setRowCount(linkList.size());
            for (int row = 0; row < linkList.size(); ++row) {
                const QJsonObject link = linkList.at(row).toObject();
                m_table->setItem(row, 0, new QTableWidgetItem(link.value("AccountName_utf").toString()));
                m_table->setItem(row, 1,
                                  new QTableWidgetItem(linkStatusText(link.value("Online_bool").toBool(),
                                                                       link.value("Connected_bool").toBool())));
                m_table->setItem(row, 2, new QTableWidgetItem(link.value("Hostname_str").toString()));
                m_table->setItem(row, 3, new QTableWidgetItem(link.value("TargetHubName_str").toString()));
                m_table->setItem(row, 4,
                                  new QTableWidgetItem(SoftEtherLabels::dateTime(link.value("ConnectedTime_dt").toString())));
            }
            m_table->resizeColumnsToContents();
            onSelectionChanged();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続一覧の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
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
        params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続の作成に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void CascadeLinkListDialog::onEdit()
{
    const QString accountName = selectedAccountName();
    if (accountName.isEmpty()) {
        return;
    }

    m_rpc->getLink(
        m_hubName, accountName,
        [this](const QJsonObject &link) {
            auto *dialog = new CascadeLinkEditDialog(/*isNew=*/false, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setValues(link);
            connect(dialog, &QDialog::accepted, this, [this, dialog]() {
                QJsonObject params = dialog->toRpcParams();
                params["HubName_Ex_str"] = m_hubName;
                m_rpc->setLink(
                    params, [this](const QJsonObject &) { reload(); },
                    [this](const RpcError &error) {
                        QMessageBox::warning(this, tr("エラー"),
                                              tr("カスケード接続の設定変更に失敗しました: %1 (code %2)")
                                                  .arg(error.message)
                                                  .arg(error.code));
                    });
            });
            dialog->open();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続の設定取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
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
        m_hubName, accountName, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続の削除に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
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
        m_hubName, oldName, newName, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続の名前の変更に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void CascadeLinkListDialog::onSetOnline()
{
    const QString accountName = selectedAccountName();
    if (accountName.isEmpty()) {
        return;
    }
    m_rpc->setLinkOnline(
        m_hubName, accountName, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続のオンライン化に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void CascadeLinkListDialog::onSetOffline()
{
    const QString accountName = selectedAccountName();
    if (accountName.isEmpty()) {
        return;
    }
    m_rpc->setLinkOffline(
        m_hubName, accountName, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続のオフライン化に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void CascadeLinkListDialog::onShowStatus()
{
    const QString accountName = selectedAccountName();
    if (accountName.isEmpty()) {
        return;
    }
    m_rpc->getLinkStatus(
        m_hubName, accountName,
        [this, accountName](const QJsonObject &status) {
            auto *dialog = new CascadeLinkStatusDialog(accountName, status, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->open();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("カスケード接続の状態取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}
