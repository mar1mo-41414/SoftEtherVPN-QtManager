#include "GroupEditDialog.h"

#include "util/DialogSizing.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

GroupEditDialog::GroupEditDialog(bool isNew, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(isNew ? tr("グループの新規作成") : tr("グループの編集"));

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setEnabled(isNew);
    m_realnameEdit = new QLineEdit(this);
    m_noteEdit = new QTextEdit(this);
    m_noteEdit->setFixedHeight(60);

    auto *form = new QFormLayout;
    form->addRow(tr("グループ名(&G):"), m_nameEdit);
    form->addRow(tr("本名(&R):"), m_realnameEdit);
    form->addRow(tr("説明(&N):"), m_noteEdit);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &GroupEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttonBox);

    DialogSizing::fitToWidth(this, 380);
}

void GroupEditDialog::setValues(const QString &name, const QString &realname, const QString &note)
{
    m_nameEdit->setText(name);
    m_realnameEdit->setText(realname);
    m_noteEdit->setPlainText(note);
}

QJsonObject GroupEditDialog::toRpcParams() const
{
    QJsonObject params;
    params["Name_str"] = m_nameEdit->text().trimmed();
    params["Realname_utf"] = m_realnameEdit->text();
    params["Note_utf"] = m_noteEdit->toPlainText();
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
