#include "GroupListDialog.h"
#include "GroupEditDialog.h"
#include "UserListDialog.h"

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

GroupListDialog::GroupListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent, bool selectMode)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
    , m_selectMode(selectMode)
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
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::cellDoubleClicked, this,
            m_selectMode ? &GroupListDialog::onPick : &GroupListDialog::onEdit);

    // B_CREATE / IDOK(編集) / B_DELETE / B_REFRESH / B_USER / IDCANCEL
    auto *createButton = new QPushButton(tr("新規作成(&C)"), this);
    // 選択モードでは 編集 → 選択(SM_SELECT_GROUP)、閉じる → なし(SM_SELECT_NO_GROUP)
    m_editButton = new QPushButton(m_selectMode ? tr("選択(&S)") : tr("編集(&E)"), this);
    m_deleteButton = new QPushButton(tr("削除(&D)"), this);
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&R)"), this);
    m_memberButton = new QPushButton(tr("メンバ一覧(&M)"), this);
    auto *closeButton = new QPushButton(m_selectMode ? tr("なし(&N)") : tr("閉じる(&X)"), this);

    connect(createButton, &QPushButton::clicked, this, &GroupListDialog::onCreate);
    connect(m_editButton, &QPushButton::clicked, this, m_selectMode ? &GroupListDialog::onPick : &GroupListDialog::onEdit);
    connect(m_deleteButton, &QPushButton::clicked, this, &GroupListDialog::onDelete);
    connect(refreshButton, &QPushButton::clicked, this, &GroupListDialog::reload);
    connect(m_memberButton, &QPushButton::clicked, this, &GroupListDialog::onMembers);
    connect(closeButton, &QPushButton::clicked, this, m_selectMode ? &GroupListDialog::onPickNone : &QDialog::accept);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &GroupListDialog::updateButtons);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(m_memberButton);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(720, 440);
    updateButtons();
    reload();
}

void GroupListDialog::updateButtons()
{
    const bool selected = !m_table->selectedItems().isEmpty();
    m_editButton->setEnabled(selected);
    m_deleteButton->setEnabled(selected);
    m_memberButton->setEnabled(selected);
}

void GroupListDialog::onPick()
{
    const QString groupName = selectedGroupName();
    if (groupName.isEmpty()) {
        return;
    }
    m_pickedGroup = groupName;
    accept();
}

void GroupListDialog::onPickNone()
{
    m_pickedGroup.clear();
    accept();
}

void GroupListDialog::onMembers()
{
    const QString groupName = selectedGroupName();
    if (groupName.isEmpty()) {
        return;
    }
    UserListDialog dialog(m_rpc, m_hubName, this, groupName);
    dialog.exec();
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
            for (int column = 0; column < m_table->columnCount(); ++column) {
                m_table->setColumnWidth(column, qMax(m_table->columnWidth(column), 110));
            }
            updateButtons();
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
            dialog->setGroup(group);
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
