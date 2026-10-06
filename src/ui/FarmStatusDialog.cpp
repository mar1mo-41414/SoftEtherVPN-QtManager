#include "FarmStatusDialog.h"

#include "util/RpcUiHelpers.h"
#include "util/SoftEtherLabels.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

FarmStatusDialog::FarmStatusDialog(VpnServerRpc *rpc, bool isController, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    if (isController) {
        buildControllerView();
    } else {
        buildMemberView();
    }
}

void FarmStatusDialog::buildControllerView()
{
    // D_SM_FARM_MEMBER CAPTION
    setWindowTitle(tr("クラスタメンバ一覧"));

    auto *titleLabel = new QLabel(tr("現在、このクラスタコントローラには以下のクラスタメンバサーバーが接続しています。"), this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(9);
    // SM_FM_COLUMN_1〜9
    m_table->setHorizontalHeaderLabels({tr("種類"), tr("接続時刻"), tr("ホスト名"), tr("ポイント"), tr("セッション数"),
                                         tr("TCP コネクション数"), tr("動作 HUB 数"), tr("消費クライアント接続ライセンス"),
                                         tr("消費ブリッジ接続ライセンス")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this,
            [this]() { m_infoButton->setEnabled(!m_table->selectedItems().isEmpty()); });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &FarmStatusDialog::onShowMemberInfo);

    // IDOK / B_REFRESH / IDCANCEL
    m_infoButton = new QPushButton(tr("クラスタメンバサーバーの情報を表示(&I)"), this);
    m_infoButton->setEnabled(false);
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(m_infoButton, &QPushButton::clicked, this, &FarmStatusDialog::onShowMemberInfo);
    connect(refreshButton, &QPushButton::clicked, this, &FarmStatusDialog::reloadMembers);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_infoButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(860, 420);
    reloadMembers();
}

void FarmStatusDialog::reloadMembers()
{
    m_rpc->call(
        QStringLiteral("EnumFarmMember"), {},
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            const QJsonArray members = result.value("FarmMemberList").toArray();
            m_table->setRowCount(members.size());
            for (int row = 0; row < members.size(); ++row) {
                const QJsonObject member = members.at(row).toObject();
                // SM_FM_CONTROLLER / SM_FM_MEMBER
                auto *typeItem = new QTableWidgetItem(member.value("Controller_bool").toBool() ? tr("コントローラ") : tr("メンバ"));
                typeItem->setData(Qt::UserRole, member.value("Id_u32").toDouble());
                m_table->setItem(row, 0, typeItem);
                m_table->setItem(row, 1, new QTableWidgetItem(SoftEtherLabels::dateTime(member.value("ConnectedTime_dt").toString())));
                m_table->setItem(row, 2, new QTableWidgetItem(member.value("Hostname_str").toString()));
                m_table->setItem(row, 3, new QTableWidgetItem(QString::number(member.value("Point_u32").toInt())));
                m_table->setItem(row, 4, new QTableWidgetItem(QString::number(member.value("NumSessions_u32").toInt())));
                m_table->setItem(row, 5, new QTableWidgetItem(QString::number(member.value("NumTcpConnections_u32").toInt())));
                m_table->setItem(row, 6, new QTableWidgetItem(QString::number(member.value("NumHubs_u32").toInt())));
                m_table->setItem(row, 7, new QTableWidgetItem(QString::number(member.value("AssignedClientLicense_u32").toInt())));
                m_table->setItem(row, 8, new QTableWidgetItem(QString::number(member.value("AssignedBridgeLicense_u32").toInt())));
            }
            m_table->resizeColumnsToContents();
            m_infoButton->setEnabled(!m_table->selectedItems().isEmpty());
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("クラスタメンバ一覧の取得"), error); }));
}

void FarmStatusDialog::onShowMemberInfo()
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const quint32 id = static_cast<quint32>(m_table->item(selected.first()->row(), 0)->data(Qt::UserRole).toUInt());

    QJsonObject params;
    params["Id_u32"] = static_cast<qint64>(id);
    m_rpc->call(
        QStringLiteral("GetFarmInfo"), params,
        RpcUi::guarded(this, [this](const QJsonObject &info) {
            auto *dialog = new QDialog(this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            // SM_FMINFO_CAPTION
            dialog->setWindowTitle(tr("クラスタメンバサーバーの状態"));

            auto *form = new QFormLayout;
            // SM_FMINFO_*
            form->addRow(tr("サーバーの種類:"),
                         new QLabel(info.value("Controller_bool").toBool() ? tr("コントローラ") : tr("メンバ"), dialog));
            form->addRow(tr("接続確立時刻:"), new QLabel(SoftEtherLabels::dateTime(info.value("ConnectedTime_dt").toString()), dialog));
            form->addRow(tr("IP アドレス:"), new QLabel(info.value("Ip_ip").toString(), dialog));
            form->addRow(tr("ホスト名:"), new QLabel(info.value("Hostname_str").toString(), dialog));
            form->addRow(tr("ポイント:"), new QLabel(QString::number(info.value("Point_u32").toInt()), dialog));
            form->addRow(tr("性能基準比:"), new QLabel(QString::number(info.value("Weight_u32").toInt()), dialog));
            QStringList ports;
            for (const QJsonValue &port : info.value("Ports_u32").toArray()) {
                ports << QString::number(port.toInt());
            }
            form->addRow(tr("公開ポート:"), new QLabel(ports.join(QStringLiteral(", ")), dialog));
            QStringList hubs;
            for (const QJsonValue &value : info.value("HubsList").toArray()) {
                const QJsonObject hub = value.toObject();
                hubs << (hub.value("DynamicHub_bool").toBool() ? tr("%1 (ダイナミック)") : tr("%1 (スタティック)"))
                                .arg(hub.value("HubName_str").toString());
            }
            form->addRow(tr("動作している仮想 HUB:"), new QLabel(hubs.join(QLatin1Char('\n')), dialog));
            form->addRow(tr("セッション数:"), new QLabel(QString::number(info.value("NumSessions_u32").toInt()), dialog));
            form->addRow(tr("TCP コネクション数:"), new QLabel(QString::number(info.value("NumTcpConnections_u32").toInt()), dialog));

            auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
            buttonBox->button(QDialogButtonBox::Close)->setText(tr("閉じる(&X)"));
            connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
            connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
            auto *layout = new QVBoxLayout(dialog);
            layout->addLayout(form);
            layout->addWidget(buttonBox);
            dialog->open();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("クラスタメンバ情報の取得"), error); }));
}

void FarmStatusDialog::buildMemberView()
{
    // SM_FC_STATUS_CAPTION
    setWindowTitle(tr("クラスタコントローラへの接続状態"));

    auto *form = new QFormLayout;
    auto *ipLabel = new QLabel(this);
    auto *portLabel = new QLabel(this);
    auto *statusLabel = new QLabel(this);
    auto *errorLabel = new QLabel(this);
    auto *startLabel = new QLabel(this);
    auto *firstLabel = new QLabel(this);
    auto *currentLabel = new QLabel(this);
    auto *tryLabel = new QLabel(this);
    auto *connectedLabel = new QLabel(this);
    auto *failedLabel = new QLabel(this);
    // SM_FC_*
    form->addRow(tr("コントローラの IP アドレス:"), ipLabel);
    form->addRow(tr("コントローラの TCP/IP ポート番号:"), portLabel);
    form->addRow(tr("接続状態:"), statusLabel);
    form->addRow(tr("最後に発生したエラー:"), errorLabel);
    form->addRow(tr("接続開始時刻:"), startLabel);
    form->addRow(tr("最初の接続確立成功時刻:"), firstLabel);
    form->addRow(tr("現在の接続確立成功時刻:"), currentLabel);
    form->addRow(tr("接続試行回数:"), tryLabel);
    form->addRow(tr("接続に成功した回数:"), connectedLabel);
    form->addRow(tr("接続に失敗した回数:"), failedLabel);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonBox->button(QDialogButtonBox::Close)->setText(tr("閉じる(&X)"));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttonBox);

    m_rpc->call(
        QStringLiteral("GetFarmConnectionStatus"), {},
        RpcUi::guarded(this, [this, ipLabel, portLabel, statusLabel, errorLabel, startLabel, firstLabel, currentLabel, tryLabel, connectedLabel,
         failedLabel](const QJsonObject &status) {
            ipLabel->setText(status.value("Ip_ip").toString());
            portLabel->setText(QString::number(status.value("Port_u32").toInt()));
            statusLabel->setText(status.value("Online_bool").toBool() ? tr("オンライン") : tr("オフライン"));
            const int lastError = status.value("LastError_u32").toInt();
            errorLabel->setText(lastError == 0 ? tr("(なし)") : tr("エラーコード: %1").arg(lastError));
            const bool connected = status.value("NumConnected_u32").toInt() > 0;
            startLabel->setText(SoftEtherLabels::dateTime(status.value("StartedTime_dt").toString()));
            firstLabel->setText(connected ? SoftEtherLabels::dateTime(status.value("FirstConnectedTime_dt").toString()) : tr("(未接続)"));
            currentLabel->setText(connected ? SoftEtherLabels::dateTime(status.value("CurrentConnectedTime_dt").toString()) : tr("(未接続)"));
            tryLabel->setText(QString::number(status.value("NumTry_u32").toInt()));
            connectedLabel->setText(QString::number(status.value("NumConnected_u32").toInt()));
            failedLabel->setText(QString::number(status.value("NumFailed_u32").toInt()));
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("クラスタコントローラへの接続状態の取得"), error); }));
}
