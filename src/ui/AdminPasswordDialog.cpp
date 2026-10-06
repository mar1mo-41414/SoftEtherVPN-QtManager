#include "AdminPasswordDialog.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

AdminPasswordDialog::AdminPasswordDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_CHANGE_PASSWORD CAPTION
    setWindowTitle(tr("管理者パスワードの設定"));

    auto *titleLabel = new QLabel(
        tr("サーバーの管理者パスワードを設定します。新しいパスワードを入力してから [OK] をクリックしてください。"),
        this);
    titleLabel->setWordWrap(true);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_confirmEdit = new QLineEdit(this);
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    auto *form = new QFormLayout;
    form->addRow(tr("新しいパスワード(P):"), m_passwordEdit);
    form->addRow(tr("確認入力(C):"), m_confirmEdit);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AdminPasswordDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addLayout(form);
    layout->addWidget(buttonBox);

    DialogSizing::fitToWidth(this, 480);
}

void AdminPasswordDialog::onOk()
{
    // SM_CHANGE_PASSWORD_1
    if (m_passwordEdit->text() != m_confirmEdit->text()) {
        QMessageBox::warning(this, tr("入力エラー"),
                              tr("確認入力がパスワードと一致しません。[パスワード] と [確認入力] には同一のパスワードを入力してください。"));
        return;
    }
    // SM_CHANGE_PASSWORD_2
    if (m_passwordEdit->text().isEmpty()) {
        QMessageBox confirmBox(QMessageBox::Warning, tr("確認"), tr("パスワードが入力されていません。続行しますか?"),
                                QMessageBox::NoButton, this);
        QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
        confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
        confirmBox.exec();
        if (confirmBox.clickedButton() != yesButton) {
            return;
        }
    }

    const QString newPassword = m_passwordEdit->text();
    QJsonObject params;
    params["PlainTextPassword_str"] = newPassword;
    m_rpc->call(
        QStringLiteral("SetServerPassword"), params,
        RpcUi::guarded(this, [this, newPassword](const QJsonObject &) {
            m_rpc->updatePassword(newPassword);
            // SM_CHANGE_PASSWORD_3
            QMessageBox::information(this, tr("完了"),
                                      tr("パスワードを変更しました。\n\n接続設定に保存されているパスワードは自動では更新されません。"
                                         "次回接続時のために、接続設定の編集から新しいパスワードを設定し直してください。"));
            accept();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("管理者パスワードの変更"), error); }));
}
