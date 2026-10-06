#include "UserListDialog.h"
#include "GroupListDialog.h"
#include "InfoTableDialog.h"
#include "UserEditDialog.h"

#include "util/SoftEtherLabels.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

UserListDialog::UserListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent, QString groupFilter,
                               bool selectMode)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
    , m_groupFilter(std::move(groupFilter))
    , m_selectMode(selectMode)
{
    // D_SM_USER CAPTION
    setWindowTitle(m_groupFilter.isEmpty()
                       ? tr("ユーザーの管理")
                       // SM_GROUP_MEMBER_STR
                       : tr("ユーザーの管理 (グループ %1 に所属しているユーザーのみ表示)").arg(m_groupFilter));

    // S_TITLE
    auto *titleLabel = new QLabel(
        m_selectMode ? tr("ユーザーを選択してください。")
                     : tr("仮想 HUB \"%1\" に登録されているユーザーは以下の通りです。").arg(m_hubName),
        this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    // SM_USER_COLUMN_1〜7
    m_table->setHorizontalHeaderLabels({tr("ユーザー名"), tr("本名"), tr("所属グループ"), tr("説明"), tr("認証方法"),
                                         tr("ログイン回数"), tr("最終ログイン日時")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::cellDoubleClicked, this,
            m_selectMode ? &UserListDialog::onPick : &UserListDialog::onEdit);

    // B_CREATE / IDOK(編集) / B_STATUS / B_DELETE / B_REFRESH / IDCANCEL
    auto *createButton = new QPushButton(tr("新規作成(&C)"), this);
    // 選択モードでは 編集 → 選択、削除 → グループを選択、閉じる → 選択しない
    m_editButton = new QPushButton(m_selectMode ? tr("選択(&S)") : tr("編集(&E)"), this);
    m_statusButton = new QPushButton(tr("ユーザー情報表示(&V)"), this);
    m_deleteButton = new QPushButton(m_selectMode ? tr("グループを選択(&G)...") : tr("削除(&D)"), this);
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&R)"), this);
    auto *closeButton = new QPushButton(m_selectMode ? tr("選択しない(&N)") : tr("閉じる(&X)"), this);

    connect(createButton, &QPushButton::clicked, this, &UserListDialog::onCreate);
    connect(m_editButton, &QPushButton::clicked, this, m_selectMode ? &UserListDialog::onPick : &UserListDialog::onEdit);
    connect(m_statusButton, &QPushButton::clicked, this, &UserListDialog::onStatus);
    connect(m_deleteButton, &QPushButton::clicked, this,
            m_selectMode ? &UserListDialog::onPickGroup : &UserListDialog::onDelete);
    connect(refreshButton, &QPushButton::clicked, this, &UserListDialog::reload);
    connect(closeButton, &QPushButton::clicked, this, m_selectMode ? &UserListDialog::onPickNone : &QDialog::accept);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &UserListDialog::updateButtons);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_statusButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(820, 480);
    updateButtons();
    reload();
}

void UserListDialog::updateButtons()
{
    const bool selected = !m_table->selectedItems().isEmpty();
    m_editButton->setEnabled(selected);
    m_statusButton->setEnabled(selected);
    // 選択モードの「グループを選択」は行の選択に関係なく使える。
    m_deleteButton->setEnabled(m_selectMode || selected);
}

void UserListDialog::onPick()
{
    const QString userName = selectedUserName();
    if (userName.isEmpty()) {
        return;
    }
    m_pickedName = userName;
    m_pickedIsGroup = false;
    accept();
}

void UserListDialog::onPickNone()
{
    m_pickedName.clear();
    m_pickedIsGroup = false;
    accept();
}

void UserListDialog::onPickGroup()
{
    GroupListDialog dialog(m_rpc, m_hubName, this, /*selectMode=*/true);
    if (dialog.exec() != QDialog::Accepted || dialog.selectedGroup().isEmpty()) {
        return;
    }
    m_pickedName = dialog.selectedGroup();
    m_pickedIsGroup = true;
    accept();
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
            QJsonArray userList = result.value("UserList").toArray();
            if (!m_groupFilter.isEmpty()) {
                QJsonArray filtered;
                for (const QJsonValue &value : userList) {
                    if (value.toObject().value("GroupName_str").toString() == m_groupFilter) {
                        filtered.append(value);
                    }
                }
                userList = filtered;
            }
            m_table->setRowCount(userList.size());
            for (int row = 0; row < userList.size(); ++row) {
                const QJsonObject user = userList.at(row).toObject();
                const QString group = user.value("GroupName_str").toString();
                m_table->setItem(row, 0, new QTableWidgetItem(user.value("Name_str").toString()));
                m_table->setItem(row, 1, new QTableWidgetItem(user.value("Realname_utf").toString()));
                // SM_NO_GROUP
                m_table->setItem(row, 2, new QTableWidgetItem(group.isEmpty() ? tr("－") : group));
                m_table->setItem(row, 3, new QTableWidgetItem(user.value("Note_utf").toString()));
                m_table->setItem(row, 4,
                                  new QTableWidgetItem(SoftEtherLabels::authType(user.value("AuthType_u32").toInt())));
                m_table->setItem(row, 5, new QTableWidgetItem(QString::number(user.value("NumLogin_u32").toInt())));
                m_table->setItem(row, 6,
                                  new QTableWidgetItem(SoftEtherLabels::dateTime(user.value("LastLoginTime_dt").toString())));
            }
            m_table->resizeColumnsToContents();
            for (int column = 0; column < m_table->columnCount(); ++column) {
                m_table->setColumnWidth(column, qMax(m_table->columnWidth(column), 110));
            }
            updateButtons();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("ユーザー一覧の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void UserListDialog::onCreate()
{
    UserEditDialog dialog(m_rpc, m_hubName, /*isNew=*/true, this);
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
            auto *dialog = new UserEditDialog(m_rpc, m_hubName, /*isNew=*/false, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setUser(user);
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

void UserListDialog::onStatus()
{
    const QString userName = selectedUserName();
    if (userName.isEmpty()) {
        return;
    }
    using SoftEtherLabels::bytes;
    using SoftEtherLabels::packets;

    VpnServerRpc *rpc = m_rpc;
    const QString hubName = m_hubName;
    // SM_USERINFO_CAPTION
    auto *dialog = new InfoTableDialog(
        tr("ユーザー \"%1\" の情報").arg(userName), tr("ユーザー \"%1\" の情報").arg(userName), /*refreshable=*/true,
        [rpc, hubName, userName](const InfoTableDialog::Deliver &deliver, const InfoTableDialog::Fail &fail) {
            rpc->getUser(
                hubName, userName,
                [deliver](const QJsonObject &u) {
                    InfoTable::Rows rows;
                    rows << qMakePair(tr("ユーザー名"), u.value("Name_str").toString());
                    if (!u.value("Realname_utf").toString().isEmpty()) {
                        rows << qMakePair(tr("本名"), u.value("Realname_utf").toString());
                    }
                    if (!u.value("Note_utf").toString().isEmpty()) {
                        rows << qMakePair(tr("説明"), u.value("Note_utf").toString());
                    }
                    if (!u.value("GroupName_str").toString().isEmpty()) {
                        rows << qMakePair(tr("グループ名"), u.value("GroupName_str").toString());
                    }
                    rows << qMakePair(tr("作成日時"), SoftEtherLabels::dateTime(u.value("CreatedTime_dt").toString()));
                    rows << qMakePair(tr("更新日時"), SoftEtherLabels::dateTime(u.value("UpdatedTime_dt").toString()));
                    const QDateTime expire = QDateTime::fromString(u.value("ExpireTime_dt").toString(), Qt::ISODateWithMs);
                    if (expire.isValid() && expire.date().year() > 1971) {
                        rows << qMakePair(tr("有効期限"), SoftEtherLabels::dateTime(u.value("ExpireTime_dt").toString()));
                    }
                    rows << qMakePair(tr("送信ユニキャストパケット数"), packets(u.value("Send.UnicastCount_u64").toDouble()));
                    rows << qMakePair(tr("送信ユニキャスト合計サイズ"), bytes(u.value("Send.UnicastBytes_u64").toDouble()));
                    rows << qMakePair(tr("送信ブロードキャストパケット数"),
                                      packets(u.value("Send.BroadcastCount_u64").toDouble()));
                    rows << qMakePair(tr("送信ブロードキャスト合計サイズ"),
                                      bytes(u.value("Send.BroadcastBytes_u64").toDouble()));
                    rows << qMakePair(tr("受信ユニキャストパケット数"), packets(u.value("Recv.UnicastCount_u64").toDouble()));
                    rows << qMakePair(tr("受信ユニキャスト合計サイズ"), bytes(u.value("Recv.UnicastBytes_u64").toDouble()));
                    rows << qMakePair(tr("受信ブロードキャストパケット数"),
                                      packets(u.value("Recv.BroadcastCount_u64").toDouble()));
                    rows << qMakePair(tr("受信ブロードキャスト合計サイズ"),
                                      bytes(u.value("Recv.BroadcastBytes_u64").toDouble()));
                    rows << qMakePair(tr("ログイン回数"), QString::number(u.value("NumLogin_u32").toInt()));
                    deliver(rows);
                },
                [fail](const RpcError &error) { fail(error); });
        },
        this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}
