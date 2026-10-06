#include "EtherIpIdEditDialog.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

EtherIpIdEditDialog::EtherIpIdEditDialog(VpnServerRpc *rpc, bool isNew, QWidget *parent)
    : QDialog(parent)
{
    // D_SM_ETHERIP_ID CAPTION
    setWindowTitle(tr("EtherIP / L2TPv3 over IPsec クライアント定義"));

    auto *introLabel = new QLabel(
        tr("EtherIP / L2TPv3 over IPsec クライアントがこの VPN Server に接続しようとした際の ISAKMP (IKE) Phase 1 の"
           "イニシエータ ID 文字列が以下に一致する場合に、次の仮想 HUB への接続設定を適用します。"),
        this);
    introLabel->setWordWrap(true);

    m_idEdit = new QLineEdit(this);
    m_idEdit->setEnabled(isNew);
    m_hubCombo = new QComboBox(this);
    m_hubCombo->setEditable(true);
    m_userEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    RpcUi::populateHubCombo(rpc, m_hubCombo);

    // S02〜S05 / S07 / S06
    auto *idForm = new QFormLayout;
    idForm->addRow(tr("ISAKMP Phase 1 ID:"), m_idEdit);
    auto *form = new QFormLayout;
    form->addRow(tr("接続先の仮想 HUB:"), m_hubCombo);
    form->addRow(tr("ユーザー名(U):"), m_userEdit);
    form->addRow(tr("パスワード(P):"), m_passwordEdit);

    auto *idHint = new QLabel(
        tr("(ID はクライアント側のルータの接続設定で設定するものと同一の文字列を指定してください。文字列のほか、ID の種類が IP アドレスの場合は IP アドレスも指定できます。)\n\n"
           "なお、'*' (アスタリスク) を指定するとワイルドカード指定となり、他の明示的なルールに一致しないすべての接続元クライアントが対象となります。"),
        this);
    idHint->setWordWrap(true);
    auto *userHint = new QLabel(
        tr("ユーザー名とパスワードは、仮想 HUB に登録されている必要があります。EtherIP / L2TPv3 クライアントは、"
           "上記で入力された情報で識別されるユーザーの権限で仮想 HUB に接続したものとみなされます。"),
        this);
    userHint->setWordWrap(true);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EtherIpIdEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addLayout(idForm);
    layout->addWidget(idHint);
    layout->addLayout(form);
    layout->addWidget(userHint);
    layout->addWidget(buttonBox);

    DialogSizing::fitToWidth(this, 520);
}

void EtherIpIdEditDialog::setValues(const QJsonObject &setting)
{
    m_idEdit->setText(setting.value("Id_str").toString());
    m_hubCombo->setEditText(setting.value("HubName_str").toString());
    m_userEdit->setText(setting.value("UserName_str").toString());
    m_passwordEdit->setText(setting.value("Password_str").toString());
}

QJsonObject EtherIpIdEditDialog::toRpcParams() const
{
    QJsonObject params;
    params["Id_str"] = m_idEdit->text().trimmed();
    params["HubName_str"] = m_hubCombo->currentText().trimmed();
    params["UserName_str"] = m_userEdit->text().trimmed();
    params["Password_str"] = m_passwordEdit->text();
    return params;
}

void EtherIpIdEditDialog::accept()
{
    if (m_idEdit->text().trimmed().isEmpty() || m_hubCombo->currentText().trimmed().isEmpty() ||
        m_userEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"),
                              tr("ISAKMP Phase 1 ID・接続先の仮想 HUB・ユーザー名を入力してください。"));
        return;
    }
    QDialog::accept();
}
