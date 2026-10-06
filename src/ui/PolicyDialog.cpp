#include "PolicyDialog.h"

#include "util/DialogSizing.h"
#include "util/PolicyTable.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

QString keyOf(const PolicyTable::Def &def)
{
    return QStringLiteral("policy:%1_%2")
        .arg(QString::fromLatin1(def.key), def.kind == PolicyTable::Kind::Bool ? "bool" : "u32");
}

QString unitSuffix(PolicyTable::Unit unit, unsigned value)
{
    // POL_INT_COUNT / POL_INT_SEC / POL_INT_BPS / POL_INT_VLAN
    switch (unit) {
    case PolicyTable::Unit::Count:
        return PolicyDialog::tr("%1 個").arg(value);
    case PolicyTable::Unit::Sec:
        return PolicyDialog::tr("%1 秒").arg(value);
    case PolicyTable::Unit::Bps:
        return PolicyDialog::tr("%1 bps").arg(value);
    default:
        return QString::number(value);
    }
}

QString policyName(const PolicyTable::Def &def)
{
    return QCoreApplication::translate("PolicyTable", def.name);
}

} // namespace

PolicyDialog::PolicyDialog(const QString &windowTitle, const QString &heading, const QJsonObject &policy,
                           QWidget *parent)
    : QDialog(parent)
    , m_policy(defaultPolicy())
{
    setWindowTitle(windowTitle);
    for (auto it = policy.begin(); it != policy.end(); ++it) {
        if (it.key().startsWith(QLatin1String("policy:"))) {
            m_policy[it.key()] = it.value();
        }
    }

    auto *headingLabel = new QLabel(heading, this);
    QFont headingFont = headingLabel->font();
    headingFont.setBold(true);
    headingFont.setPointSize(headingFont.pointSize() + 2);
    headingLabel->setFont(headingFont);

    // POL_TITLE_STR / POL_VALUE_STR
    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({tr("ポリシー名"), tr("ポリシー設定値")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    int count = 0;
    const PolicyTable::Def *defs = PolicyTable::defs(&count);
    m_table->setRowCount(count);
    for (int i = 0; i < count; ++i) {
        m_table->setItem(i, 0, new QTableWidgetItem(policyName(defs[i])));
        m_table->setItem(i, 1, new QTableWidgetItem(valueText(i)));
    }
    m_table->setColumnWidth(0, 300);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &PolicyDialog::onSelectionChanged);

    // 右ペイン (STATIC1 / S_BOLD / S_BOLD2)
    auto *selectedCaption = new QLabel(tr("選択されているポリシー(P):"), this);
    m_nameLabel = new QLabel(tr("左のリストからポリシー項目を選択してください。"), this);
    m_nameLabel->setWordWrap(true);
    QFont boldFont = m_nameLabel->font();
    boldFont.setBold(true);
    m_nameLabel->setFont(boldFont);

    auto *descriptionCaption = new QLabel(tr("このポリシーの説明(C):"), this);
    descriptionCaption->setFont(boldFont);
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setReadOnly(true);
    m_descriptionEdit->setFixedHeight(120);

    auto *valueCaption = new QLabel(tr("設定値(V):"), this);
    valueCaption->setFont(boldFont);
    m_onRadio = new QRadioButton(tr("このポリシーを有効にする(&E)"), this);
    m_offRadio = new QRadioButton(tr("このポリシーを無効にする(&D)"), this);
    m_spin = new QSpinBox(this);
    connect(m_onRadio, &QRadioButton::toggled, this, &PolicyDialog::onValueEdited);
    connect(m_spin, qOverload<int>(&QSpinBox::valueChanged), this, &PolicyDialog::onValueEdited);

    auto *line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    auto *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    auto *rightLayout = new QVBoxLayout;
    rightLayout->addWidget(selectedCaption);
    rightLayout->addWidget(m_nameLabel);
    rightLayout->addWidget(line1);
    rightLayout->addWidget(descriptionCaption);
    rightLayout->addWidget(m_descriptionEdit);
    rightLayout->addWidget(line2);
    rightLayout->addWidget(valueCaption);
    rightLayout->addWidget(m_onRadio);
    rightLayout->addWidget(m_spin);
    rightLayout->addWidget(m_offRadio);
    rightLayout->addStretch();

    auto *columns = new QHBoxLayout;
    columns->addWidget(m_table, 3);
    columns->addLayout(rightLayout, 2);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(headingLabel);
    layout->addLayout(columns, 1);
    layout->addWidget(buttonBox);

    resize(860, 560);
    m_onRadio->setEnabled(false);
    m_offRadio->setEnabled(false);
    m_spin->setEnabled(false);
    m_spin->hide();
    m_table->selectRow(0);
}

QJsonObject PolicyDialog::defaultPolicy()
{
    QJsonObject policy;
    int count = 0;
    const PolicyTable::Def *defs = PolicyTable::defs(&count);
    for (int i = 0; i < count; ++i) {
        const QString key = keyOf(defs[i]);
        if (defs[i].kind == PolicyTable::Kind::Bool) {
            policy[key] = false;
        } else {
            policy[key] = 0;
        }
    }
    // 公式Managerの既定値: アクセス許可・TCPコネクション数 32・タイムアウト 20 秒
    policy["policy:Access_bool"] = true;
    policy["policy:MaxConnection_u32"] = 32;
    policy["policy:TimeOut_u32"] = 20;
    policy["policy:Ver3_bool"] = true;
    return policy;
}

QJsonObject PolicyDialog::extract(const QJsonObject &obj)
{
    QJsonObject policy;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.key().startsWith(QLatin1String("policy:"))) {
            policy[it.key()] = it.value();
        }
    }
    return policy;
}

int PolicyDialog::currentIndex() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    return selected.isEmpty() ? -1 : selected.first()->row();
}

QString PolicyDialog::valueText(int index) const
{
    int count = 0;
    const PolicyTable::Def &def = PolicyTable::defs(&count)[index];
    const QJsonValue value = m_policy.value(keyOf(def));
    if (def.kind == PolicyTable::Kind::Bool) {
        // POL_BOOL_ENABLE / POL_BOOL_DISABLE
        return value.toBool() ? tr("有効") : tr("－");
    }
    const unsigned number = static_cast<unsigned>(value.toDouble());
    return number == 0 ? tr("－") : unitSuffix(def.unit, number);
}

void PolicyDialog::refreshRow(int index)
{
    m_table->item(index, 1)->setText(valueText(index));
}

void PolicyDialog::onSelectionChanged()
{
    const int index = currentIndex();
    if (index < 0) {
        return;
    }
    int count = 0;
    const PolicyTable::Def &def = PolicyTable::defs(&count)[index];

    m_loading = true;
    m_nameLabel->setText(policyName(def));
    m_descriptionEdit->setPlainText(QCoreApplication::translate("PolicyTable", def.description));
    const QJsonValue value = m_policy.value(keyOf(def));
    m_onRadio->setEnabled(true);
    m_offRadio->setEnabled(true);
    if (def.kind == PolicyTable::Kind::Bool) {
        m_onRadio->setText(tr("このポリシーを有効にする(&E)"));
        m_spin->hide();
        const bool on = value.toBool();
        m_onRadio->setChecked(on);
        m_offRadio->setChecked(!on);
    } else {
        // R_DEFINE / R_DISABLE
        m_onRadio->setText(tr("このポリシーの値を定義する(&F)"));
        m_spin->show();
        m_spin->setRange(static_cast<int>(def.min), static_cast<int>(def.max));
        const unsigned number = static_cast<unsigned>(value.toDouble());
        const bool defined = number != 0 || !def.canDisable;
        m_onRadio->setChecked(defined);
        m_offRadio->setChecked(!defined);
        m_offRadio->setEnabled(def.canDisable);
        m_spin->setValue(number == 0 ? static_cast<int>(def.min) : static_cast<int>(number));
        m_spin->setEnabled(defined);
        m_spin->setSuffix(def.unit == PolicyTable::Unit::Count   ? tr(" 個")
                          : def.unit == PolicyTable::Unit::Sec   ? tr(" 秒")
                          : def.unit == PolicyTable::Unit::Bps   ? tr(" bps")
                                                                  : QString());
    }
    m_loading = false;
}

void PolicyDialog::onValueEdited()
{
    if (m_loading) {
        return;
    }
    const int index = currentIndex();
    if (index < 0) {
        return;
    }
    int count = 0;
    const PolicyTable::Def &def = PolicyTable::defs(&count)[index];
    if (def.kind == PolicyTable::Kind::Bool) {
        m_policy[keyOf(def)] = m_onRadio->isChecked();
    } else {
        m_spin->setEnabled(m_onRadio->isChecked());
        m_policy[keyOf(def)] = m_onRadio->isChecked() ? m_spin->value() : 0;
    }
    refreshRow(index);
}
