#include "UserListDialog.h"
#include "UserEditDialog.h"

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

UserListDialog::UserListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_USER CAPTION
    setWindowTitle(tr("ユーザーの管理"));

    // S_TITLE
    auto *titleLabel = new QLabel(tr("仮想 HUB \"%1\" に登録されているユーザーは以下の通りです。").arg(m_hubName), this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels(
        {tr("ユーザー名"), tr("グループ名"), tr("本名"), tr("説明"), tr("認証方法"), tr("ログイン回数")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &UserListDialog::onEdit);

    // IDOK(編集) / B_CREATE / B_DELETE / B_REFRESH / IDCANCEL
    auto *editButton = new QPushButton(tr("編集(&E)"), this);
    auto *createButton = new QPushButton(tr("新規作成(&C)"), this);
    auto *deleteButton = new QPushButton(tr("削除(&D)"), this);
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&R)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);

    connect(createButton, &QPushButton::clicked, this, &UserListDialog::onCreate);
    connect(editButton, &QPushButton::clicked, this, &UserListDialog::onEdit);
    connect(deleteButton, &QPushButton::clicked, this, &UserListDialog::onDelete);
    connect(refreshButton, &QPushButton::clicked, this, &UserListDialog::reload);
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

    resize(680, 420);
    reload();
}

QString UserListDialog::selectedUserName() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return QString();
    }
    return m_table->item(selected.first()->row(), 0)->text();
}

void UserListDialog::reload()
{
    m_rpc->enumUser(
        m_hubName,
        [this](const QJsonObject &result) {
            const QJsonArray userList = result.value("UserList").toArray();
            m_table->setRowCount(userList.size());
            for (int row = 0; row < userList.size(); ++row) {
                const QJsonObject user = userList.at(row).toObject();
                m_table->setItem(row, 0, new QTableWidgetItem(user.value("Name_str").toString()));
                m_table->setItem(row, 1, new QTableWidgetItem(user.value("GroupName_str").toString()));
                m_table->setItem(row, 2, new QTableWidgetItem(user.value("Realname_utf").toString()));
                m_table->setItem(row, 3, new QTableWidgetItem(user.value("Note_utf").toString()));
                m_table->setItem(row, 4,
                                  new QTableWidgetItem(SoftEtherLabels::authType(user.value("AuthType_u32").toInt())));
                m_table->setItem(row, 5, new QTableWidgetItem(QString::number(user.value("NumLogin_u32").toInt())));
            }
            m_table->resizeColumnsToContents();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("ユーザー一覧の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void UserListDialog::onCreate()
{
    UserEditDialog dialog(/*isNew=*/true, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QJsonObject params = dialog.toRpcParams();
    params["HubName_str"] = m_hubName;
    m_rpc->createUser(
        params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("ユーザーの作成に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void UserListDialog::onEdit()
{
    const QString userName = selectedUserName();
    if (userName.isEmpty()) {
        return;
    }

    m_rpc->getUser(
        m_hubName, userName,
        [this](const QJsonObject &user) {
            auto *dialog = new UserEditDialog(/*isNew=*/false, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setValues(user.value("Name_str").toString(), user.value("GroupName_str").toString(),
                               user.value("Realname_utf").toString(), user.value("Note_utf").toString());
            connect(dialog, &QDialog::accepted, this, [this, dialog]() {
                QJsonObject params = dialog->toRpcParams();
                params["HubName_str"] = m_hubName;
                m_rpc->setUser(
                    params, [this](const QJsonObject &) { reload(); },
                    [this](const RpcError &error) {
                        QMessageBox::warning(this, tr("エラー"),
                                              tr("ユーザーの設定変更に失敗しました: %1 (code %2)")
                                                  .arg(error.message)
                                                  .arg(error.code));
                    });
            });
            dialog->open();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("ユーザーの設定取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void UserListDialog::onDelete()
{
    const QString userName = selectedUserName();
    if (userName.isEmpty()) {
        return;
    }

    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"),
                            tr("ユーザー \"%1\" を削除します。よろしいですか?").arg(userName), QMessageBox::NoButton,
                            this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }

    m_rpc->deleteUser(
        m_hubName, userName, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("ユーザーの削除に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}
