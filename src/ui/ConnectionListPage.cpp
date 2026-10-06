#include "ConnectionListPage.h"
#include "ConnectionEditDialog.h"
#include "PasswordPromptDialog.h"

#include "model/ConnectionProfileStore.h"

#include "util/RpcUiHelpers.h"

#include "Version.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QGridLayout>
#include <QGroupBox>
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

    // 公式Managerの接続一覧 (D_SM_MAIN): 上部バナー、「接続設定」グループ、その下に補助ボタン群。
    // バナー画像・アイコン類は再現せず、配置だけを合わせている。
    auto *banner = new QLabel(tr("SoftEther VPN Server Manager"), this);
    QFont bannerFont = banner->font();
    bannerFont.setBold(true);
    bannerFont.setPointSize(bannerFont.pointSize() + 8);
    banner->setFont(bannerFont);
    banner->setAlignment(Qt::AlignCenter);
    banner->setMinimumHeight(56);

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
    m_table->verticalHeader()->hide();
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &ConnectionListPage::onConnect);

    // B_NEW_SETTING / B_EDIT_SETTING / B_DELETE / IDOK
    m_newButton = new QPushButton(tr("新しい接続設定(&N)"), this);
    m_editButton = new QPushButton(tr("接続設定の編集(&E)"), this);
    m_deleteButton = new QPushButton(tr("接続設定の削除(&D)"), this);
    m_connectButton = new QPushButton(tr("接続(&C)"), this);
    m_connectButton->setDefault(true);
    QFont connectFont = m_connectButton->font();
    connectFont.setBold(true);
    m_connectButton->setFont(connectFont);

    connect(m_newButton, &QPushButton::clicked, this, &ConnectionListPage::onNewSetting);
    connect(m_editButton, &QPushButton::clicked, this, &ConnectionListPage::onEditSetting);
    connect(m_deleteButton, &QPushButton::clicked, this, &ConnectionListPage::onDeleteSetting);
    connect(m_connectButton, &QPushButton::clicked, this, &ConnectionListPage::onConnect);

    // 3列のボタングリッド。「接続」は3列目の2段目に置く。
    auto *buttonGrid = new QGridLayout;
    buttonGrid->addWidget(m_newButton, 0, 0);
    buttonGrid->addWidget(m_editButton, 0, 1);
    buttonGrid->addWidget(m_deleteButton, 0, 2);
    buttonGrid->addWidget(m_connectButton, 1, 2);

    auto *group = new QGroupBox(tr("SoftEther VPN Server への接続設定(&P):"), this);
    auto *groupLayout = new QVBoxLayout(group);
    groupLayout->addWidget(description);
    groupLayout->addWidget(m_table, 1);
    groupLayout->addLayout(buttonGrid);

    // B_CERT_TOOL / B_SECURE_MANAGER / B_SELECT_SECURE / B_ABOUT / IDCANCEL
    auto *certToolButton = new QPushButton(tr("証明書作成ツール(&R)"), this);
    auto *secureManagerButton = new QPushButton(tr("スマートカードマネージャ(&S)..."), this);
    auto *selectSecureButton = new QPushButton(tr("スマートカード選択(&M)..."), this);
    for (QPushButton *button : {certToolButton, secureManagerButton, selectSecureButton}) {
        button->setEnabled(false);
        button->setToolTip(tr("未対応"));
    }
    auto *aboutButton = new QPushButton(tr("バージョン情報(&A)"), this);
    auto *quitButton = new QPushButton(tr("管理マネージャの終了(&X)"), this);
    connect(aboutButton, &QPushButton::clicked, this, &ConnectionListPage::onAbout);
    connect(quitButton, &QPushButton::clicked, this, &ConnectionListPage::quitRequested);

    // 公式Managerは3列均等のグリッド。終了ボタンは2列分の幅を使う。
    auto *toolsGrid = new QGridLayout;
    for (int column = 0; column < 3; ++column) {
        toolsGrid->setColumnStretch(column, 1);
    }
    toolsGrid->addWidget(certToolButton, 0, 2);
    toolsGrid->addWidget(secureManagerButton, 1, 1);
    toolsGrid->addWidget(selectSecureButton, 1, 2);
    toolsGrid->addWidget(aboutButton, 2, 0);
    toolsGrid->addWidget(quitButton, 2, 1, 1, 2);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(banner);
    layout->addWidget(group, 1);
    layout->addLayout(toolsGrid);

    reloadTable();
}

void ConnectionListPage::reloadTable()
{
    m_table->setRowCount(m_profiles.size());
    for (int row = 0; row < m_profiles.size(); ++row) {
        const ConnectionProfile &profile = m_profiles.at(row);

        // 公式Managerは接続先列にホスト名のみを表示する。
        const QString destination = profile.host;
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
    rpc->setProxy(profile.proxyType, profile.proxyHost, profile.proxyPort, profile.proxyUser, profile.proxyPassword);
    rpc->connectToServer(profile.host, profile.port, profile.hubAdminMode ? profile.hubName : QString(), password);

    rpc->test(
        RpcUi::guarded(this, [this, rpc, profile](const QJsonObject &) {
            rpc->getServerInfo(
                RpcUi::guarded(this, [this, rpc, profile](const QJsonObject &info) {
                    emit connected(rpc, info, profile);
                }),
                RpcUi::guarded(this, [this, rpc](const RpcError &error) {
                    rpc->deleteLater();
                    QMessageBox::warning(this, tr("エラー"),
                                          tr("サーバー情報の取得に失敗しました: %1 (code %2)")
                                              .arg(error.message)
                                              .arg(error.code));
                }));
        }),
        RpcUi::guarded(this, [this, rpc](const RpcError &error) {
            rpc->deleteLater();
            QMessageBox::warning(this, tr("エラー"),
                                  tr("接続に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}

void ConnectionListPage::onAbout()
{
    // B_ABOUT
    QMessageBox::about(this, tr("バージョン情報"),
                        tr("<b>SoftEtherVPN-QtManager</b> v%1<br><br>"
                           "SoftEther VPN Server の JSON-RPC 管理 API を利用する、Qt (%2) 製のクロスプラットフォーム管理ツールです。<br>"
                           "公式の VPN Server Manager (Windows 専用) の画面構成を参考にしています。<br><br>"
                           "<a href=\"https://github.com/mar1mo-41414/SoftEtherVPN-QtManager\">"
                           "github.com/mar1mo-41414/SoftEtherVPN-QtManager</a><br><br>"
                           "SoftEther VPN は Apache License 2.0 で公開されています。")
                            .arg(QStringLiteral(SOFTETHERVPN_QTMANAGER_VERSION))
                            .arg(QString::fromLatin1(qVersion())));
}
