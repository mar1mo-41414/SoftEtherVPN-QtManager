#include "GroupEditDialog.h"
#include "InfoTableDialog.h"
#include "PolicyDialog.h"

#include "util/DialogSizing.h"
#include "util/SoftEtherLabels.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

GroupEditDialog::GroupEditDialog(bool isNew, QWidget *parent)
    : QDialog(parent)
    , m_policy(PolicyDialog::defaultPolicy())
{
    // SM_EDIT_GROUP_CAPTION_1 / _2 (編集時は setGroup で更新)
    setWindowTitle(isNew ? tr("グループの新規作成") : tr("グループのプロパティ"));

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setEnabled(isNew);
    m_realnameEdit = new QLineEdit(this);
    m_noteEdit = new QLineEdit(this);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->addRow(tr("グループ名(G):"), m_nameEdit);
    form->addRow(tr("本名(R):"), m_realnameEdit);
    form->addRow(tr("説明(N):"), m_noteEdit);

    // S_POLICY_1 / R_POLICY / B_POLICY
    auto *policyGroup = new QGroupBox(tr("セキュリティポリシー"), this);
    m_policyCheck = new QCheckBox(tr("このグループのユーザーのセキュリティポリシーを設定する(&Y)"), this);
    m_policyButton = new QPushButton(tr("セキュリティポリシー(&M)"), this);
    auto *policyLayout = new QHBoxLayout(policyGroup);
    policyLayout->addWidget(m_policyCheck, 1);
    policyLayout->addWidget(m_policyButton);

    // S_POLICY_2
    auto *statsGroup = new QGroupBox(tr("このグループの統計情報"), this);
    m_statsTable = InfoTable::makeTable(this);
    m_statsTable->setMinimumHeight(180);
    m_statsTable->setEnabled(!isNew);
    auto *statsLayout = new QVBoxLayout(statsGroup);
    statsLayout->addWidget(m_statsTable);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &GroupEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(policyGroup);
    layout->addWidget(statsGroup, 1);
    layout->addWidget(buttonBox);

    connect(m_nameEdit, &QLineEdit::textChanged, this, &GroupEditDialog::updateState);
    connect(m_policyCheck, &QCheckBox::toggled, this, &GroupEditDialog::updateState);
    connect(m_policyButton, &QPushButton::clicked, this, &GroupEditDialog::onPolicy);

    DialogSizing::fitToWidth(this, 460);
    updateState();
}

void GroupEditDialog::updateState()
{
    m_policyButton->setEnabled(m_policyCheck->isChecked());
    m_okButton->setEnabled(!m_nameEdit->text().trimmed().isEmpty());
}

void GroupEditDialog::setGroup(const QJsonObject &group)
{
    using SoftEtherLabels::bytes;
    using SoftEtherLabels::packets;

    const QString name = group.value("Name_str").toString();
    // SM_EDIT_GROUP_CAPTION_2
    setWindowTitle(tr("グループ %1 のプロパティ").arg(name));
    m_nameEdit->setText(name);
    m_realnameEdit->setText(group.value("Realname_utf").toString());
    m_noteEdit->setText(group.value("Note_utf").toString());

    m_policy = PolicyDialog::defaultPolicy();
    const QJsonObject policy = PolicyDialog::extract(group);
    for (auto it = policy.begin(); it != policy.end(); ++it) {
        m_policy[it.key()] = it.value();
    }
    m_policyCheck->setChecked(group.value("UsePolicy_bool").toBool());

    InfoTable::Rows rows;
    rows << qMakePair(tr("送信ユニキャストパケット数"), packets(group.value("Send.UnicastCount_u64").toDouble()));
    rows << qMakePair(tr("送信ユニキャスト合計サイズ"), bytes(group.value("Send.UnicastBytes_u64").toDouble()));
    rows << qMakePair(tr("送信ブロードキャストパケット数"), packets(group.value("Send.BroadcastCount_u64").toDouble()));
    rows << qMakePair(tr("送信ブロードキャスト合計サイズ"), bytes(group.value("Send.BroadcastBytes_u64").toDouble()));
    rows << qMakePair(tr("受信ユニキャストパケット数"), packets(group.value("Recv.UnicastCount_u64").toDouble()));
    rows << qMakePair(tr("受信ユニキャスト合計サイズ"), bytes(group.value("Recv.UnicastBytes_u64").toDouble()));
    rows << qMakePair(tr("受信ブロードキャストパケット数"), packets(group.value("Recv.BroadcastCount_u64").toDouble()));
    rows << qMakePair(tr("受信ブロードキャスト合計サイズ"), bytes(group.value("Recv.BroadcastBytes_u64").toDouble()));
    InfoTable::setRows(m_statsTable, rows);
    updateState();
}

QJsonObject GroupEditDialog::toRpcParams() const
{
    QJsonObject params;
    params["Name_str"] = m_nameEdit->text().trimmed();
    params["Realname_utf"] = m_realnameEdit->text();
    params["Note_utf"] = m_noteEdit->text();
    params["UsePolicy_bool"] = m_policyCheck->isChecked();
    if (m_policyCheck->isChecked()) {
        for (auto it = m_policy.begin(); it != m_policy.end(); ++it) {
            params[it.key()] = it.value();
        }
    }
    return params;
}

void GroupEditDialog::accept()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("グループ名を入力してください。"));
        return;
    }
    QDialog::accept();
}

void GroupEditDialog::onPolicy()
{
    // SM_GROUP_POLICY_CAPTION
    const QString title = tr("グループ %1 のセキュリティポリシー").arg(m_nameEdit->text().trimmed());
    PolicyDialog dialog(title, title, m_policy, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_policy = dialog.policy();
    }
}
