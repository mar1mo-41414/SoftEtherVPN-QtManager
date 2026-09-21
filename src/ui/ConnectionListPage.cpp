#include "ConnectionListPage.h"
#include "ConnectionEditDialog.h"
#include "PasswordPromptDialog.h"

#include "model/ConnectionProfileStore.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

ConnectionListPage::ConnectionListPage(QWidget *parent)
    : QWidget(parent)
{
    m_profiles = ConnectionProfileStore::loadAll();

    // D_SM_MAIN STATIC2
    auto *description = new QLabel(
        tr("以下の VPN Server または VPN Bridge への接続設定が登録されています。名前をダブルクリックすると、"
           "サーバーに接続できます。\n新しい接続を追加するには [新しい接続設定] をクリックしてください。"),
        this);
    description->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    // SM_MAIN_COLUMN_1/2/3
    m_table->setHorizontalHeaderLabels({tr("接続設定名"), tr("接続先 VPN Server"), tr("管理対象")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &ConnectionListPage::onConnect);

    // B_NEW_SETTING / B_EDIT_SETTING / B_DELETE / IDOK
    m_newButton = new QPushButton(tr("新しい接続設定(&N)"), this);
    m_editButton = new QPushButton(tr("接続設定の編集(&E)"), this);
    m_deleteButton = new QPushButton(tr("接続設定の削除(&D)"), this);
    m_connectButton = new QPushButton(tr("接続(&C)"), this);
    m_connectButton->setDefault(true);

    connect(m_newButton, &QPushButton::clicked, this, &ConnectionListPage::onNewSetting);
    connect(m_editButton, &QPushButton::clicked, this, &ConnectionListPage::onEditSetting);
    connect(m_deleteButton, &QPushButton::clicked, this, &ConnectionListPage::onDeleteSetting);
    connect(m_connectButton, &QPushButton::clicked, this, &ConnectionListPage::onConnect);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_newButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_connectButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(description);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    reloadTable();
}

void ConnectionListPage::reloadTable()
{
    m_table->setRowCount(m_profiles.size());
    for (int row = 0; row < m_profiles.size(); ++row) {
        const ConnectionProfile &profile = m_profiles.at(row);

        // SM_HOSTNAME_AND_PORT: "%S:%u"
        const QString destination = QStringLiteral("%1:%2").arg(profile.host).arg(profile.port);
        // SM_MODE_SERVER / SM_MODE_HUB
        const QString target = profile.hubAdminMode ? profile.hubName : tr("サーバー全体");

        m_table->setItem(row, 0, new QTableWidgetItem(profile.name));
        m_table->setItem(row, 1, new QTableWidgetItem(destination));
        m_table->setItem(row, 2, new QTableWidgetItem(target));
    }
    m_table->resizeColumnsToContents();
}

void ConnectionListPage::persist()
{
    ConnectionProfileStore::saveAll(m_profiles);
}

int ConnectionListPage::selectedRow() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return -1;
    }
    return selected.first()->row();
}

void ConnectionListPage::onNewSetting()
{
    QStringList existingNames;
    for (const ConnectionProfile &profile : std::as_const(m_profiles)) {
        existingNames << profile.name;
    }

    ConnectionEditDialog dialog(existingNames, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_profiles.append(dialog.profile());
        persist();
        reloadTable();
    }
}

void ConnectionListPage::onEditSetting()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }

    QStringList existingNames;
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (i != row) {
            existingNames << m_profiles.at(i).name;
        }
    }

    ConnectionEditDialog dialog(existingNames, this);
    dialog.setProfile(m_profiles.at(row));
    if (dialog.exec() == QDialog::Accepted) {
        m_profiles[row] = dialog.profile();
        persist();
        reloadTable();
    }
}

void ConnectionListPage::onDeleteSetting()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }

    const ConnectionProfile &profile = m_profiles.at(row);

    // SM_SETTING_DELETE_MSG (QMessageBoxの標準ボタンはQt自体の翻訳リソースが無いと
    // 英語表記になるため、はい/いいえを明示的なボタンとして用意する)
    QMessageBox confirmBox(QMessageBox::Question, tr("確認"),
                            tr("接続設定 \"%1\" を削除します。よろしいですか?").arg(profile.name),
                            QMessageBox::NoButton, this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }

    m_profiles.removeAt(row);
    persist();
    reloadTable();
}

void ConnectionListPage::onConnect()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    connectToProfile(m_profiles.at(row));
}

void ConnectionListPage::connectToProfile(const ConnectionProfile &profile)
{
    QString password = profile.password;
    if (profile.noSavePassword || password.isEmpty()) {
        PasswordPromptDialog dialog(profile.host, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        password = dialog.password();
    }

    auto *rpc = new VpnServerRpc(this);
    rpc->connectToServer(profile.host, profile.port, profile.hubAdminMode ? profile.hubName : QString(), password);

    const bool hubAdminMode = profile.hubAdminMode;
    const QString hubName = profile.hubName;
    rpc->test(
        [this, rpc, hubAdminMode, hubName](const QJsonObject &) {
            rpc->getServerInfo(
                [this, rpc, hubAdminMode, hubName](const QJsonObject &info) {
                    emit connected(rpc, info, hubAdminMode, hubName);
                },
                [this, rpc](const RpcError &error) {
                    rpc->deleteLater();
                    QMessageBox::warning(this, tr("エラー"),
                                          tr("サーバー情報の取得に失敗しました: %1 (code %2)")
                                              .arg(error.message)
                                              .arg(error.code));
                });
        },
        [this, rpc](const RpcError &error) {
            rpc->deleteLater();
            QMessageBox::warning(this, tr("エラー"),
                                  tr("接続に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}
