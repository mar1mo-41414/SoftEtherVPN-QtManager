#include "GroupListDialog.h"
#include "GroupEditDialog.h"

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

GroupListDialog::GroupListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_GROUP CAPTION
    setWindowTitle(tr("グループの管理"));

    // S_TITLE
    auto *titleLabel = new QLabel(tr("仮想 HUB \"%1\" に登録されているグループは以下の通りです。").arg(m_hubName), this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    // SM_GROUPLIST_NAME/REALNAME/NOTE/NUMUSERS
    m_table->setHorizontalHeaderLabels({tr("グループ名"), tr("本名"), tr("説明"), tr("ユーザー数")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &GroupListDialog::onEdit);

    // B_CREATE / IDOK(編集) / B_DELETE / B_REFRESH / IDCANCEL
    auto *createButton = new QPushButton(tr("新規作成(&C)"), this);
    auto *editButton = new QPushButton(tr("編集(&E)"), this);
    auto *deleteButton = new QPushButton(tr("削除(&D)"), this);
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&R)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);

    connect(createButton, &QPushButton::clicked, this, &GroupListDialog::onCreate);
    connect(editButton, &QPushButton::clicked, this, &GroupListDialog::onEdit);
    connect(deleteButton, &QPushButton::clicked, this, &GroupListDialog::onDelete);
    connect(refreshButton, &QPushButton::clicked, this, &GroupListDialog::reload);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(560, 400);
    reload();
}

QString GroupListDialog::selectedGroupName() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return QString();
    }
    return m_table->item(selected.first()->row(), 0)->text();
}

void GroupListDialog::reload()
{
    m_rpc->enumGroup(
        m_hubName,
        [this](const QJsonObject &result) {
            const QJsonArray groupList = result.value("GroupList").toArray();
            m_table->setRowCount(groupList.size());
            for (int row = 0; row < groupList.size(); ++row) {
                const QJsonObject group = groupList.at(row).toObject();
                m_table->setItem(row, 0, new QTableWidgetItem(group.value("Name_str").toString()));
                m_table->setItem(row, 1, new QTableWidgetItem(group.value("Realname_utf").toString()));
                m_table->setItem(row, 2, new QTableWidgetItem(group.value("Note_utf").toString()));
                m_table->setItem(row, 3, new QTableWidgetItem(QString::number(group.value("NumUsers_u32").toInt())));
            }
            m_table->resizeColumnsToContents();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("グループ一覧の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void GroupListDialog::onCreate()
{
    GroupEditDialog dialog(/*isNew=*/true, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QJsonObject params = dialog.toRpcParams();
    params["HubName_str"] = m_hubName;
    m_rpc->createGroup(
        params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("グループの作成に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void GroupListDialog::onEdit()
{
    const QString groupName = selectedGroupName();
    if (groupName.isEmpty()) {
        return;
    }

    m_rpc->getGroup(
        m_hubName, groupName,
        [this](const QJsonObject &group) {
            auto *dialog = new GroupEditDialog(/*isNew=*/false, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setValues(group.value("Name_str").toString(), group.value("Realname_utf").toString(),
                               group.value("Note_utf").toString());
            connect(dialog, &QDialog::accepted, this, [this, dialog]() {
                QJsonObject params = dialog->toRpcParams();
                params["HubName_str"] = m_hubName;
                m_rpc->setGroup(
                    params, [this](const QJsonObject &) { reload(); },
                    [this](const RpcError &error) {
                        QMessageBox::warning(this, tr("エラー"),
                                              tr("グループの設定変更に失敗しました: %1 (code %2)")
                                                  .arg(error.message)
                                                  .arg(error.code));
                    });
            });
            dialog->open();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("グループの設定取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void GroupListDialog::onDelete()
{
    const QString groupName = selectedGroupName();
    if (groupName.isEmpty()) {
        return;
    }

    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"),
                            tr("グループ \"%1\" を削除します。よろしいですか?").arg(groupName), QMessageBox::NoButton,
                            this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }

    m_rpc->deleteGroup(
        m_hubName, groupName, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("グループの削除に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}
