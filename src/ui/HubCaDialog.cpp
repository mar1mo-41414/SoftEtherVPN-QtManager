#include "HubCaDialog.h"
#include "CertInfoDialog.h"

#include "util/RpcUiHelpers.h"
#include "util/SoftEtherLabels.h"

#include <QAbstractItemView>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSslCertificate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

HubCaDialog::HubCaDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_CA CAPTION
    setWindowTitle(tr("信頼する証明機関の証明書の管理"));

    auto *title = new QLabel(
        tr("この仮想 HUB が信頼する証明機関の証明書一覧を管理します。\n\nここで登録された証明機関の証明書一覧は、VPN Client が署名済み証明書認証モードで接続してきた際の証明書の検証に使用されます。"),
        this);
    title->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("発行先"), tr("発行者"), tr("有効期限")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &HubCaDialog::updateButtons);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &HubCaDialog::onView);

    auto *addButton = new QPushButton(tr("追加(&A)"), this);
    m_deleteButton = new QPushButton(tr("削除(&D)"), this);
    m_viewButton = new QPushButton(tr("証明書の表示(&V)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&C)"), this);
    connect(addButton, &QPushButton::clicked, this, &HubCaDialog::onAdd);
    connect(m_deleteButton, &QPushButton::clicked, this, &HubCaDialog::onDelete);
    connect(m_viewButton, &QPushButton::clicked, this, &HubCaDialog::onView);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(addButton);
    buttons->addWidget(m_deleteButton);
    buttons->addWidget(m_viewButton);
    buttons->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(m_table, 1);
    layout->addLayout(buttons);
    resize(780, 460);
    updateButtons();
    reload();
}

int HubCaDialog::selectedRow() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    return selected.isEmpty() ? -1 : selected.first()->row();
}

void HubCaDialog::updateButtons()
{
    const bool selected = selectedRow() >= 0;
    m_deleteButton->setEnabled(selected);
    m_viewButton->setEnabled(selected);
}

void HubCaDialog::reload()
{
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    m_rpc->call(
        QStringLiteral("EnumCa"), params,
        [this](const QJsonObject &result) {
            const QJsonArray list = result.value("CAList").toArray();
            m_table->setRowCount(list.size());
            for (int row = 0; row < list.size(); ++row) {
                const QJsonObject ca = list.at(row).toObject();
                auto *subject = new QTableWidgetItem(ca.value("SubjectName_utf").toString());
                subject->setData(Qt::UserRole, ca.value("Key_u32").toDouble());
                m_table->setItem(row, 0, subject);
                m_table->setItem(row, 1, new QTableWidgetItem(ca.value("IssuerName_utf").toString()));
                m_table->setItem(row, 2, new QTableWidgetItem(SoftEtherLabels::dateTime(ca.value("Expires_dt").toString())));
            }
            m_table->resizeColumnsToContents();
            for (int column = 0; column < 2; ++column) {
                m_table->setColumnWidth(column, qMax(m_table->columnWidth(column), 220));
            }
            updateButtons();
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("証明機関の一覧の取得"), error); });
}

void HubCaDialog::onAdd()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("信頼する証明機関の証明書の追加"), QString(),
        tr("X.509 証明書ファイル (*.cer *.crt *.pem *.der);;すべてのファイル (*)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("エラー"), tr("ファイルを開けませんでした: %1").arg(file.errorString()));
        return;
    }
    const QByteArray data = file.readAll();
    QSslCertificate cert(data, QSsl::Pem);
    if (cert.isNull()) {
        cert = QSslCertificate(data, QSsl::Der);
    }
    if (cert.isNull()) {
        QMessageBox::warning(this, tr("エラー"), tr("X.509 証明書として読み込めませんでした。"));
        return;
    }
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    params["Cert_bin"] = QString::fromLatin1(cert.toDer().toBase64());
    m_rpc->call(
        QStringLiteral("AddCa"), params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("証明機関の追加"), error); });
}

void HubCaDialog::onDelete()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    // SM_CRL_DELETE_MSG と同じ文言の確認
    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"), tr("選択した項目を削除します。よろしいですか?"), QMessageBox::NoButton,
                            this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    params["Key_u32"] = m_table->item(row, 0)->data(Qt::UserRole).toDouble();
    m_rpc->call(
        QStringLiteral("DeleteCa"), params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("証明機関の削除"), error); });
}

void HubCaDialog::onView()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    params["Key_u32"] = m_table->item(row, 0)->data(Qt::UserRole).toDouble();
    m_rpc->call(
        QStringLiteral("GetCa"), params,
        [this](const QJsonObject &result) {
            const QByteArray der = QByteArray::fromBase64(result.value("Cert_bin").toString().toLatin1());
            auto *dialog = new CertInfoDialog(der, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->open();
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("証明書の取得"), error); });
}
