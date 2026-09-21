#include "PasswordPromptDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

PasswordPromptDialog::PasswordPromptDialog(const QString &targetLabel, QWidget *parent)
    : QDialog(parent)
{
    // D_PASSWORD CAPTION: "%S へのログイン"
    setWindowTitle(tr("%1 へのログイン").arg(targetLabel));

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    auto *form = new QFormLayout;
    // S_PASSWORD
    form->addRow(tr("管理パスワード(&A):"), m_passwordEdit);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttonBox);

    resize(360, sizeHint().height());
}

QString PasswordPromptDialog::password() const
{
    return m_passwordEdit->text();
}
