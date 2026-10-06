#include "UserEditDialog.h"

#include "util/DialogSizing.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

UserEditDialog::UserEditDialog(bool isNew, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(isNew ? tr("ユーザーの新規作成") : tr("ユーザーの編集"));

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setEnabled(isNew);
    m_groupNameEdit = new QLineEdit(this);
    m_realnameEdit = new QLineEdit(this);
    m_noteEdit = new QTextEdit(this);
    m_noteEdit->setFixedHeight(60);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordConfirmEdit = new QLineEdit(this);
    m_passwordConfirmEdit->setEchoMode(QLineEdit::Password);

    auto *form = new QFormLayout;
    // IDC_STATIC1/3/4/5
    form->addRow(tr("ユーザー名(&U):"), m_nameEdit);
    form->addRow(tr("本名(&R):"), m_realnameEdit);
    form->addRow(tr("説明(&N):"), m_noteEdit);
    form->addRow(tr("グループ名 (省略可能)(&J):"), m_groupNameEdit);

    // S_PASSWORD_1/2/3
    auto *passwordForm = new QFormLayout;
    passwordForm->addRow(tr("パスワード(&P):"), m_passwordEdit);
    passwordForm->addRow(tr("パスワードの確認入力(&C):"), m_passwordConfirmEdit);
    auto *passwordGroup = new QGroupBox(tr("パスワード認証"), this);
    passwordGroup->setLayout(passwordForm);
    if (!isNew) {
        passwordGroup->setTitle(tr("パスワード認証 (空欄のままにすると変更しません)"));
    }

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &UserEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(passwordGroup);
    layout->addWidget(buttonBox);

    DialogSizing::fitToWidth(this, 400);
}

void UserEditDialog::setValues(const QString &name, const QString &groupName, const QString &realname,
                                const QString &note)
{
    m_nameEdit->setText(name);
    m_groupNameEdit->setText(groupName);
    m_realnameEdit->setText(realname);
    m_noteEdit->setPlainText(note);
}

QJsonObject UserEditDialog::toRpcParams() const
{
    QJsonObject params;
    params["Name_str"] = m_nameEdit->text().trimmed();
    params["GroupName_str"] = m_groupNameEdit->text().trimmed();
    params["Realname_utf"] = m_realnameEdit->text();
    params["Note_utf"] = m_noteEdit->toPlainText();
    params["AuthType_u32"] = 1; // パスワード認証
    params["Auth_Password_str"] = m_passwordEdit->text();
    return params;
}

void UserEditDialog::accept()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("ユーザー名を入力してください。"));
        return;
    }
    if (m_passwordEdit->text() != m_passwordConfirmEdit->text()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("パスワードと確認入力が一致しません。"));
        return;
    }
    QDialog::accept();
}
