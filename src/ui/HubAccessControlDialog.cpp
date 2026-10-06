#include "HubAccessControlDialog.h"

#include "util/RpcUiHelpers.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

namespace {

QString ruleContent(const QJsonObject &rule)
{
    const QString address = rule.value("IpAddress_ip").toString();
    if (rule.value("Masked_bool").toBool()) {
        return QStringLiteral("%1 / %2").arg(address, rule.value("SubnetMask_ip").toString());
    }
    return address;
}

} // namespace

HubAccessControlDialog::HubAccessControlDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_AC_LIST CAPTION
    setWindowTitle(tr("接続元 IP 制限リスト"));

    auto *title = new QLabel(
        tr("クライアントコンピュータの IP アドレスによって、この VPN Server の仮想 HUB \"%1\" への VPN 接続を許可または拒否することができます。下記に接続を許可または拒否するルールを設定できます。")
            .arg(m_hubName),
        this);
    title->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    // SM_AC_COLUMN_1〜4
    m_table->setHorizontalHeaderLabels({tr("ID"), tr("優先順位"), tr("動作"), tr("内容")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &HubAccessControlDialog::updateButtons);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &HubAccessControlDialog::onEdit);

    auto *addButton = new QPushButton(tr("ルールの追加(&A)"), this);
    m_editButton = new QPushButton(tr("ルールの編集(&E)"), this);
    m_deleteButton = new QPushButton(tr("ルールの削除(&D)"), this);
    auto *saveButton = new QPushButton(tr("保存(&S)"), this);
    auto *cancelButton = new QPushButton(tr("キャンセル(&C)"), this);
    connect(addButton, &QPushButton::clicked, this, &HubAccessControlDialog::onAdd);
    connect(m_editButton, &QPushButton::clicked, this, &HubAccessControlDialog::onEdit);
    connect(m_deleteButton, &QPushButton::clicked, this, &HubAccessControlDialog::onDelete);
    connect(saveButton, &QPushButton::clicked, this, &HubAccessControlDialog::onSave);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    auto *hint = new QLabel(tr("優先順位はリストの上のものほど高くなります。"), this);
    hint->setWordWrap(true);
    auto *side = new QVBoxLayout;
    side->addWidget(addButton);
    side->addWidget(m_editButton);
    side->addWidget(m_deleteButton);
    side->addWidget(hint);
    side->addStretch();
    side->addWidget(saveButton);
    side->addWidget(cancelButton);

    auto *body = new QHBoxLayout;
    body->addWidget(m_table, 1);
    body->addLayout(side);

    auto *footer = new QLabel(tr("クライアントの IP アドレスがリスト内のどの項目にも一致しなかった場合は、この仮想 HUB への VPN 接続を許可されます。"), this);
    footer->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addLayout(body, 1);
    layout->addWidget(footer);

    resize(640, 460);
    updateButtons();
    load();
}

void HubAccessControlDialog::load()
{
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    m_rpc->call(
        QStringLiteral("GetAcList"), params,
        [this](const QJsonObject &result) {
            m_rules.clear();
            for (const QJsonValue &value : result.value("ACList").toArray()) {
                m_rules.append(value.toObject());
            }
            refreshTable();
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("接続元 IP 制限リストの取得"), error); });
}

void HubAccessControlDialog::refreshTable()
{
    std::stable_sort(m_rules.begin(), m_rules.end(), [](const QJsonObject &a, const QJsonObject &b) {
        return a.value("Priority_u32").toDouble() < b.value("Priority_u32").toDouble();
    });
    m_table->setRowCount(m_rules.size());
    for (int row = 0; row < m_rules.size(); ++row) {
        const QJsonObject &rule = m_rules.at(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(rule.value("Priority_u32").toInt())));
        // SM_AC_PASS / SM_AC_DENY
        m_table->setItem(row, 2, new QTableWidgetItem(rule.value("Deny_bool").toBool() ? tr("拒否") : tr("許可")));
        m_table->setItem(row, 3, new QTableWidgetItem(ruleContent(rule)));
    }
    for (int column = 0; column < 3; ++column) {
        m_table->setColumnWidth(column, 80);
    }
    updateButtons();
}

int HubAccessControlDialog::selectedRow() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    return selected.isEmpty() ? -1 : selected.first()->row();
}

void HubAccessControlDialog::updateButtons()
{
    const bool selected = selectedRow() >= 0;
    m_editButton->setEnabled(selected);
    m_deleteButton->setEnabled(selected);
}

// D_SM_AC (ルール項目の編集)
bool HubAccessControlDialog::editRule(QJsonObject *rule)
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("接続元 IP 制限リストのルール項目の編集"));

    auto *v4Radio = new QRadioButton(tr("IPv4"), &dialog);
    auto *v6Radio = new QRadioButton(tr("IPv6"), &dialog);
    auto *singleRadio = new QRadioButton(tr("単一の IP アドレス(&S)"), &dialog);
    auto *maskedRadio = new QRadioButton(tr("複数の IP アドレス (IP ネットワークアドレスとネットマスクで指定) (&M) :"), &dialog);
    auto *addressEdit = new QLineEdit(&dialog);
    auto *maskEdit = new QLineEdit(&dialog);
    auto *passRadio = new QRadioButton(tr("接続を許可する(&P)"), &dialog);
    auto *denyRadio = new QRadioButton(tr("接続を拒否する(&D)"), &dialog);
    auto *prioritySpin = new QSpinBox(&dialog);
    prioritySpin->setRange(1, 999999999);

    const QString address = rule->value("IpAddress_ip").toString();
    const bool masked = rule->value("Masked_bool").toBool();
    v6Radio->setChecked(address.contains(QLatin1Char(':')));
    v4Radio->setChecked(!v6Radio->isChecked());
    maskedRadio->setChecked(masked);
    singleRadio->setChecked(!masked);
    addressEdit->setText(address);
    maskEdit->setText(rule->value("SubnetMask_ip").toString());
    (rule->value("Deny_bool").toBool() ? denyRadio : passRadio)->setChecked(true);
    prioritySpin->setValue(qMax(1, rule->value("Priority_u32").toInt(100)));
    const auto sync = [=]() { maskEdit->setEnabled(maskedRadio->isChecked()); };
    connect(maskedRadio, &QRadioButton::toggled, &dialog, sync);
    sync();

    auto *versionRow = new QHBoxLayout;
    versionRow->addWidget(new QLabel(tr("IP プロトコル バージョン:"), &dialog));
    versionRow->addWidget(v4Radio);
    versionRow->addWidget(v6Radio);
    versionRow->addStretch();
    auto *contentGroup = new QGroupBox(tr("ルール項目の内容"), &dialog);
    auto *contentLayout = new QGridLayout(contentGroup);
    contentLayout->addWidget(new QLabel(tr("クライアントの IP アドレスが以下のときにルールを適用する:"), &dialog), 0, 0, 1, 2);
    contentLayout->addLayout(versionRow, 1, 0, 1, 2);
    contentLayout->addWidget(singleRadio, 2, 0, 1, 2);
    contentLayout->addWidget(maskedRadio, 3, 0, 1, 2);
    contentLayout->addWidget(new QLabel(tr("アドレス(A):"), &dialog), 4, 0, Qt::AlignRight);
    contentLayout->addWidget(addressEdit, 4, 1);
    contentLayout->addWidget(new QLabel(tr("ネットマスク(K):"), &dialog), 5, 0, Qt::AlignRight);
    contentLayout->addWidget(maskEdit, 5, 1);
    contentLayout->setColumnStretch(1, 1);

    auto *actionGroup = new QGroupBox(tr("動作"), &dialog);
    auto *actionLayout = new QHBoxLayout(actionGroup);
    actionLayout->addWidget(passRadio);
    actionLayout->addWidget(denyRadio);
    actionLayout->addStretch();

    auto *otherGroup = new QGroupBox(tr("その他"), &dialog);
    auto *otherLayout = new QHBoxLayout(otherGroup);
    otherLayout->addWidget(new QLabel(tr("優先順位(R):"), &dialog));
    otherLayout->addWidget(prioritySpin);
    otherLayout->addWidget(new QLabel(tr("(整数値: 小さいほど優先順位が高くなります)"), &dialog));
    otherLayout->addStretch();

    auto *header = new QLabel(
        tr("IP アクセス制限リストのルール項目を設定してください。ここで設定した項目は、VPN Client が仮想 HUB に接続しようとした際にそのクライアントからの接続を許可するか拒否するかを決定するために使用されます。"),
        &dialog);
    header->setWordWrap(true);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, [&]() {
        QHostAddress host;
        const bool v6 = v6Radio->isChecked();
        const auto expected = v6 ? QAbstractSocket::IPv6Protocol : QAbstractSocket::IPv4Protocol;
        if (!host.setAddress(addressEdit->text().trimmed()) || host.protocol() != expected) {
            QMessageBox::warning(&dialog, tr("入力エラー"), tr("IP アドレスが正しくありません。"));
            return;
        }
        if (maskedRadio->isChecked()) {
            QHostAddress mask;
            if (!mask.setAddress(maskEdit->text().trimmed()) || mask.protocol() != expected) {
                QMessageBox::warning(&dialog, tr("入力エラー"), tr("ネットマスクが正しくありません。"));
                return;
            }
        }
        dialog.accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(header);
    layout->addWidget(contentGroup);
    layout->addWidget(actionGroup);
    layout->addWidget(otherGroup);
    layout->addWidget(buttonBox);
    dialog.resize(560, 480);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }
    (*rule)["IpAddress_ip"] = addressEdit->text().trimmed();
    (*rule)["Masked_bool"] = maskedRadio->isChecked();
    (*rule)["SubnetMask_ip"] = maskedRadio->isChecked() ? maskEdit->text().trimmed() : QString();
    (*rule)["Deny_bool"] = denyRadio->isChecked();
    (*rule)["Priority_u32"] = prioritySpin->value();
    return true;
}

void HubAccessControlDialog::onAdd()
{
    int maxPriority = 0;
    for (const QJsonObject &rule : m_rules) {
        maxPriority = qMax(maxPriority, rule.value("Priority_u32").toInt());
    }
    QJsonObject rule;
    rule["Priority_u32"] = m_rules.isEmpty() ? 100 : maxPriority + 100;
    if (editRule(&rule)) {
        m_rules.append(rule);
        refreshTable();
    }
}

void HubAccessControlDialog::onEdit()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    QJsonObject rule = m_rules.at(row);
    if (editRule(&rule)) {
        m_rules[row] = rule;
        refreshTable();
    }
}

void HubAccessControlDialog::onDelete()
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
    m_rules.removeAt(row);
    refreshTable();
}

void HubAccessControlDialog::onSave()
{
    QJsonArray list;
    int id = 1;
    for (QJsonObject rule : m_rules) {
        rule["Id_u32"] = id++;
        list.append(rule);
    }
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    params["ACList"] = list;
    m_rpc->call(
        QStringLiteral("SetAcList"), params, [this](const QJsonObject &) { accept(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("接続元 IP 制限リストの保存"), error); });
}
