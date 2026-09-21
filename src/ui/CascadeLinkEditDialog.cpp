#include "CascadeLinkEditDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

CascadeLinkEditDialog::CascadeLinkEditDialog(bool isNew, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(isNew ? tr("カスケード接続の新規作成") : tr("カスケード接続の編集"));

    m_accountNameEdit = new QLineEdit(this);
    m_accountNameEdit->setEnabled(isNew);
    m_hostEdit = new QLineEdit(this);
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(443);
    m_targetHubNameEdit = new QLineEdit(this);

    auto *destForm = new QFormLayout;
    destForm->addRow(tr("接続設定名(&N):"), m_accountNameEdit);
    destForm->addRow(tr("ホスト名(&H):"), m_hostEdit);
    destForm->addRow(tr("ポート番号(&P):"), m_portSpin);
    destForm->addRow(tr("接続先仮想 HUB 名(&B):"), m_targetHubNameEdit);
    auto *destGroup = new QGroupBox(tr("接続先 VPN Server の指定"), this);
    destGroup->setLayout(destForm);

    m_anonymousRadio = new QRadioButton(tr("匿名認証"), this);
    m_passwordRadio = new QRadioButton(tr("パスワード認証"), this);
    m_passwordRadio->setChecked(true);
    m_usernameEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    auto *authRadioLayout = new QVBoxLayout;
    authRadioLayout->addWidget(m_anonymousRadio);
    authRadioLayout->addWidget(m_passwordRadio);
    auto *authForm = new QFormLayout;
    authForm->addRow(tr("ユーザー名(&U):"), m_usernameEdit);
    authForm->addRow(tr("パスワード(&W):"), m_passwordEdit);
    auto *authGroup = new QGroupBox(tr("認証方法"), this);
    auto *authGroupLayout = new QVBoxLayout(authGroup);
    authGroupLayout->addLayout(authRadioLayout);
    authGroupLayout->addLayout(authForm);

    m_encryptCheck = new QCheckBox(tr("暗号化を使用する"), this);
    m_encryptCheck->setChecked(true);
    m_compressCheck = new QCheckBox(tr("データ圧縮を使用する"), this);
    m_maxConnectionSpin = new QSpinBox(this);
    m_maxConnectionSpin->setRange(1, 32);
    m_maxConnectionSpin->setValue(1);

    auto *optionForm = new QFormLayout;
    optionForm->addRow(QString(), m_encryptCheck);
    optionForm->addRow(QString(), m_compressCheck);
    optionForm->addRow(tr("使用する TCP コネクション数(&M):"), m_maxConnectionSpin);
    auto *optionGroup = new QGroupBox(tr("接続オプション"), this);
    optionGroup->setLayout(optionForm);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CascadeLinkEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(destGroup);
    layout->addWidget(authGroup);
    layout->addWidget(optionGroup);
    layout->addWidget(buttonBox);

    resize(440, sizeHint().height());
}

void CascadeLinkEditDialog::setValues(const QJsonObject &link)
{
    m_accountNameEdit->setText(link.value("AccountName_utf").toString());
    m_hostEdit->setText(link.value("Hostname_str").toString());
    m_portSpin->setValue(link.value("Port_u32").toInt());
    m_targetHubNameEdit->setText(link.value("HubName_str").toString());

    const int authType = link.value("AuthType_u32").toInt();
    m_anonymousRadio->setChecked(authType == 0);
    m_passwordRadio->setChecked(authType != 0);
    m_usernameEdit->setText(link.value("Username_str").toString());
    // パスワードは取得APIから返らないため空欄のままにする (変更する場合のみ入力)。

    m_encryptCheck->setChecked(link.value("UseEncrypt_bool").toBool());
    m_compressCheck->setChecked(link.value("UseCompress_bool").toBool());
    const int maxConnection = link.value("MaxConnection_u32").toInt();
    m_maxConnectionSpin->setValue(maxConnection > 0 ? maxConnection : 1);
}

QJsonObject CascadeLinkEditDialog::toRpcParams() const
{
    QJsonObject params;
    params["AccountName_utf"] = m_accountNameEdit->text().trimmed();
    params["Hostname_str"] = m_hostEdit->text().trimmed();
    params["Port_u32"] = m_portSpin->value();
    params["HubName_str"] = m_targetHubNameEdit->text().trimmed();
    params["ProxyType_u32"] = 0; // 直接TCP/IP接続 (プロキシ経由は後続フェーズ)
    params["AuthType_u32"] = m_anonymousRadio->isChecked() ? 0 : 2; // 0:匿名 2:平文パスワード
    params["Username_str"] = m_usernameEdit->text().trimmed();
    params["PlainPassword_str"] = m_passwordEdit->text();
    params["UseEncrypt_bool"] = m_encryptCheck->isChecked();
    params["UseCompress_bool"] = m_compressCheck->isChecked();
    params["MaxConnection_u32"] = m_maxConnectionSpin->value();
    params["HalfConnection_bool"] = false;
    return params;
}

QString CascadeLinkEditDialog::accountName() const
{
    return m_accountNameEdit->text().trimmed();
}

void CascadeLinkEditDialog::accept()
{
    if (m_accountNameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("接続設定名を入力してください。"));
        return;
    }
    if (m_hostEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("ホスト名を入力してください。"));
        return;
    }
    if (m_targetHubNameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("接続先仮想 HUB 名を入力してください。"));
        return;
    }
    QDialog::accept();
}
