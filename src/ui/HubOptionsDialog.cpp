#include "HubOptionsDialog.h"

#include "util/HubOptionTexts.h"
#include "util/RpcUiHelpers.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>

HubOptionsDialog::HubOptionsDialog(VpnServerRpc *rpc, QString hubName, Kind kind, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
    , m_kind(kind)
{
    const bool admin = kind == Kind::Admin;
    // D_SM_ADMIN_OPTION CAPTION
    setWindowTitle(admin ? tr("仮想 HUB 管理オプション") : tr("仮想 HUB 拡張オプション"));

    auto *info = new QLabel(admin ? tr("現在、仮想 HUB \"%1\" には以下の管理オプションが設定されています。").arg(m_hubName)
                                  : tr("現在、仮想 HUB \"%1\" には以下の管理オプションが設定されています。").arg(m_hubName),
                            this);
    info->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    // SM_AO_COLUMN_1 / SM_AO_COLUMN_2
    m_table->setHorizontalHeaderLabels({tr("値の名前"), tr("設定値")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &HubOptionsDialog::onSelectionChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &HubOptionsDialog::onEdit);

    m_editButton = new QPushButton(tr("値の編集(&E)"), this);
    m_editButton->setEnabled(false);
    connect(m_editButton, &QPushButton::clicked, this, &HubOptionsDialog::onEdit);
    auto *sideLayout = new QVBoxLayout;
    sideLayout->addWidget(m_editButton);
    sideLayout->addStretch();

    auto *body = new QHBoxLayout;
    body->addWidget(m_table, 1);
    body->addLayout(sideLayout);

    // S_BOLD
    auto *descriptionCaption = new QLabel(tr("説明:"), this);
    QFont bold = descriptionCaption->font();
    bold.setBold(true);
    descriptionCaption->setFont(bold);
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setReadOnly(true);
    m_descriptionEdit->setFixedHeight(80);
    m_descriptionEdit->setPlainText(tr("項目名を 1 つ選択すると、その項目名に関する説明文が表示されます。"));

    // STATIC1 / STATIC2 (管理オプション) または 拡張オプションの説明
    auto *note = new QLabel(
        admin ? tr("仮想 HUB 管理オプションは、VPN Server の管理者が各仮想 HUB の管理者に仮想 HUB の管理を委任している場合に、設定範囲を制限するために使用します。\n"
                   "仮想 HUB の管理オプションを編集することができるのは、この VPN Server 全体の管理権限を持った管理者のみです。仮想 HUB の管理者は、管理オプションを表示できますが、変更することはできません。\n"
                   "ただし、allow_hub_admin_change_option が 1 に設定されている場合は、仮想 HUB の管理者でも管理オプションを編集することができます。")
              : tr("仮想 HUB 拡張オプションを使用すると、この仮想 HUB に関するより詳細な設定を行うことができるようになります。\n"
                   "標準では、VPN Server 全体の管理者および仮想 HUB 管理者の両方とも、仮想 HUB 拡張オプションを編集することができます。\n"
                   "ただし、仮想 HUB 管理オプションの deny_hub_admin_change_ext_option が 1 に設定されている場合は、仮想 HUB の管理者は、拡張オプションを編集することができません (設定内容を表示することはできます)。"),
        this);
    note->setWordWrap(true);

    auto *saveButton = new QPushButton(tr("保存(&S)"), this);
    auto *cancelButton = new QPushButton(tr("キャンセル"), this);
    connect(saveButton, &QPushButton::clicked, this, &HubOptionsDialog::onSave);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(saveButton);
    buttons->addWidget(cancelButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(info);
    layout->addLayout(body, 1);
    layout->addWidget(descriptionCaption);
    layout->addWidget(m_descriptionEdit);
    layout->addWidget(note);
    layout->addLayout(buttons);

    resize(620, 640);
    load();
}

void HubOptionsDialog::load()
{
    const auto onResult = RpcUi::guarded(this, [this](const QJsonObject &result) {
        m_items = result.value("AdminOptionList").toArray();
        m_table->setRowCount(m_items.size());
        for (int row = 0; row < m_items.size(); ++row) {
            const QJsonObject item = m_items.at(row).toObject();
            m_table->setItem(row, 0, new QTableWidgetItem(item.value("Name_str").toString()));
            m_table->setItem(row, 1, new QTableWidgetItem(QString::number(static_cast<qint64>(item.value("Value_u32").toDouble()))));
        }
        m_table->setColumnWidth(0, 340);
    });
    const auto onError = RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("オプションの取得"), error); });
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    m_rpc->call(m_kind == Kind::Admin ? QStringLiteral("GetHubAdminOptions") : QStringLiteral("GetHubExtOptions"), params,
                onResult, onError);
}

int HubOptionsDialog::selectedRow() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    return selected.isEmpty() ? -1 : selected.first()->row();
}

void HubOptionsDialog::onSelectionChanged()
{
    const int row = selectedRow();
    m_editButton->setEnabled(row >= 0);
    if (row < 0) {
        return;
    }
    const QJsonObject item = m_items.at(row).toObject();
    QString text = HubOptionTexts::description(item.value("Name_str").toString());
    if (text.isEmpty()) {
        text = item.value("Descrption_utf").toString();
    }
    m_descriptionEdit->setPlainText(text);
}

// D_SM_AO_VALUE
void HubOptionsDialog::onEdit()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    QJsonObject item = m_items.at(row).toObject();

    QDialog dialog(this);
    dialog.setWindowTitle(tr("名前と値"));
    auto *nameEdit = new QLineEdit(item.value("Name_str").toString(), &dialog);
    nameEdit->setEnabled(false);
    auto *valueSpin = new QSpinBox(&dialog);
    valueSpin->setRange(0, 2147483647);
    valueSpin->setValue(static_cast<int>(item.value("Value_u32").toDouble()));
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(tr("名前(N):"), &dialog));
    layout->addWidget(nameEdit);
    layout->addWidget(new QLabel(tr("値(V):"), &dialog));
    auto *valueRow = new QHBoxLayout;
    valueRow->addWidget(valueSpin);
    valueRow->addWidget(new QLabel(tr("(整数値)"), &dialog));
    valueRow->addStretch();
    layout->addLayout(valueRow);
    layout->addWidget(buttonBox);
    dialog.resize(360, 200);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    item["Value_u32"] = valueSpin->value();
    m_items[row] = item;
    m_table->item(row, 1)->setText(QString::number(valueSpin->value()));
}

void HubOptionsDialog::onSave()
{
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    params["AdminOptionList"] = m_items;
    m_rpc->call(
        m_kind == Kind::Admin ? QStringLiteral("SetHubAdminOptions") : QStringLiteral("SetHubExtOptions"), params,
        RpcUi::guarded(this, [this](const QJsonObject &) { accept(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("オプションの保存"), error); }));
}
