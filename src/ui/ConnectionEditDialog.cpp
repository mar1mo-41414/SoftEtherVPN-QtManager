#include "ConnectionEditDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

ConnectionEditDialog::ConnectionEditDialog(QStringList existingNames, QWidget *parent)
    : QDialog(parent)
    , m_existingNames(std::move(existingNames))
{
    // SM_EDIT_CAPTION_1
    setWindowTitle(tr("新しい接続設定の作成"));

    m_nameEdit = new QLineEdit(this);

    m_hostEdit = new QLineEdit(this);
    // R_LOCALHOST
    m_localhostCheck = new QCheckBox(tr("このコンピュータ (localhost) に接続"), this);
    connect(m_localhostCheck, &QCheckBox::toggled, this, &ConnectionEditDialog::onLocalhostToggled);

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(443);

    // R_SERVER_ADMIN / R_HUB_ADMIN
    m_serverAdminRadio = new QRadioButton(tr("サーバー管理モード"), this);
    m_hubAdminRadio = new QRadioButton(tr("仮想 HUB 管理モード"), this);
    m_serverAdminRadio->setChecked(true);
    connect(m_hubAdminRadio, &QRadioButton::toggled, this, &ConnectionEditDialog::onHubAdminToggled);

    m_hubNameEdit = new QLineEdit(this);
    m_hubNameEdit->setEnabled(false);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    // R_NO_SAVE
    m_noSaveCheck = new QCheckBox(tr("管理パスワードを保存しない"), this);

    auto *destForm = new QFormLayout;
    destForm->addRow(tr("ホスト名(&H):"), m_hostEdit);
    destForm->addRow(QString(), m_localhostCheck);
    destForm->addRow(tr("ポート番号(&P):"), m_portSpin);

    auto *destGroup = new QGroupBox(tr("接続先 VPN Server の指定"), this);
    destGroup->setLayout(destForm);

    auto *modeLayout = new QVBoxLayout;
    modeLayout->addWidget(m_serverAdminRadio);
    modeLayout->addWidget(m_hubAdminRadio);
    auto *modeForm = new QFormLayout;
    modeForm->addRow(tr("仮想 HUB 名(&V):"), m_hubNameEdit);
    modeForm->addRow(tr("管理パスワード(&A):"), m_passwordEdit);
    modeForm->addRow(QString(), m_noSaveCheck);

    auto *modeGroup = new QGroupBox(tr("管理モードの選択とパスワードの入力"), this);
    auto *modeGroupLayout = new QVBoxLayout;
    modeGroupLayout->addLayout(modeLayout);
    modeGroupLayout->addLayout(modeForm);
    modeGroup->setLayout(modeGroupLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    // QtバンドルのQtBase翻訳を読み込んでいないため、標準ボタンの文言を明示的に上書きする。
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConnectionEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    auto *nameForm = new QFormLayout;
    nameForm->addRow(tr("接続設定名(&N):"), m_nameEdit);
    layout->addLayout(nameForm);
    layout->addWidget(destGroup);
    layout->addWidget(modeGroup);
    layout->addWidget(buttonBox);

    resize(460, sizeHint().height());
}

void ConnectionEditDialog::onLocalhostToggled(bool checked)
{
    m_hostEdit->setEnabled(!checked);
    if (checked) {
        m_hostEdit->setText(QStringLiteral("localhost"));
    }
}

void ConnectionEditDialog::onHubAdminToggled(bool checked)
{
    m_hubNameEdit->setEnabled(checked);
}

void ConnectionEditDialog::setProfile(const ConnectionProfile &profile)
{
    m_originalName = profile.name;
    // SM_EDIT_CAPTION_2
    setWindowTitle(tr("%1 の編集").arg(profile.name));

    m_nameEdit->setText(profile.name);
    m_hostEdit->setText(profile.host);
    m_localhostCheck->setChecked(profile.host == QStringLiteral("localhost"));
    m_portSpin->setValue(profile.port);
    m_hubAdminRadio->setChecked(profile.hubAdminMode);
    m_serverAdminRadio->setChecked(!profile.hubAdminMode);
    m_hubNameEdit->setEnabled(profile.hubAdminMode);
    m_hubNameEdit->setText(profile.hubName);
    m_passwordEdit->setText(profile.password);
    m_noSaveCheck->setChecked(profile.noSavePassword);
}

ConnectionProfile ConnectionEditDialog::profile() const
{
    ConnectionProfile profile;
    profile.name = m_nameEdit->text().trimmed();
    profile.host = m_hostEdit->text().trimmed();
    profile.port = static_cast<quint16>(m_portSpin->value());
    profile.hubAdminMode = m_hubAdminRadio->isChecked();
    profile.hubName = m_hubNameEdit->text().trimmed();
    profile.noSavePassword = m_noSaveCheck->isChecked();
    profile.password = profile.noSavePassword ? QString() : m_passwordEdit->text();
    return profile;
}

void ConnectionEditDialog::accept()
{
    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("接続設定名を入力してください。"));
        return;
    }
    if (m_existingNames.contains(name)) {
        // SM_SETTING_EXISTS
        QMessageBox::warning(this, tr("入力エラー"),
                              tr("すでに同じ名前の接続設定 \"%1\" が登録されています。別の名前を指定してください。").arg(name));
        return;
    }
    if (m_hostEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("ホスト名を入力してください。"));
        return;
    }
    if (m_hubAdminRadio->isChecked() && m_hubNameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("仮想 HUB 名を入力してください。"));
        return;
    }

    QDialog::accept();
}
