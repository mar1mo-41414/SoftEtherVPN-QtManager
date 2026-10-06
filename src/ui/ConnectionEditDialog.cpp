#include "ConnectionEditDialog.h"
#include "ProxySettingsDialog.h"

#include "util/DialogSizing.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace {

QLabel *wrapLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    return label;
}

} // namespace

ConnectionEditDialog::ConnectionEditDialog(QStringList existingNames, QWidget *parent)
    : QDialog(parent)
    , m_existingNames(std::move(existingNames))
{
    // SM_EDIT_CAPTION_1
    setWindowTitle(tr("新しい接続設定の作成"));

    // 上段: 説明と接続設定名
    m_nameEdit = new QLineEdit(this);
    auto *nameRow = new QFormLayout;
    nameRow->addRow(tr("接続設定名(&N):"), m_nameEdit);

    // 左列: 接続先 VPN Server の指定(B)
    m_hostEdit = new QLineEdit(this);
    m_localhostCheck = new QCheckBox(tr("このコンピュータ (localhost) に接続(&L)"), this);
    m_portCombo = new QComboBox(this);
    m_portCombo->setEditable(true);
    // CM_PORT_1〜4
    m_portCombo->addItem(tr("8888 (PX-VPN ポート)"));
    m_portCombo->addItem(tr("443 (HTTPS ポート)"));
    m_portCombo->addItem(tr("992 (telnets ポート)"));
    m_portCombo->addItem(tr("5555 (SE-VPN ポート)"));
    m_portCombo->setCurrentIndex(1);

    auto *destForm = new QFormLayout;
    destForm->addRow(tr("ホスト名(&H):"), m_hostEdit);
    destForm->addRow(QString(), m_localhostCheck);
    auto *portRow = new QHBoxLayout;
    portRow->addWidget(m_portCombo, 1);
    portRow->addWidget(new QLabel(tr("(TCP ポート)"), this));
    destForm->addRow(tr("ポート番号(&P):"), portRow);

    auto *destGroup = new QGroupBox(tr("接続先 VPN Server の指定(&B):"), this);
    auto *destLayout = new QVBoxLayout(destGroup);
    destLayout->addWidget(wrapLabel(
        tr("管理したい VPN Server が動作しているコンピュータのホスト名または IP アドレスおよびポート番号を指定してください。"),
        this));
    destLayout->addLayout(destForm);

    // 左列: 経由するプロキシサーバーの設定(X)
    m_directRadio = new QRadioButton(tr("直接 TCP/IP 接続 (プロキシを使わない)(&D)"), this);
    m_httpRadio = new QRadioButton(tr("HTTP プロキシサーバー経由接続(&T)"), this);
    m_socksRadio = new QRadioButton(tr("SOCKS プロキシサーバー経由接続(&K)"), this);
    m_directRadio->setChecked(true);
    m_proxyConfigButton = new QPushButton(tr("プロキシサーバーの接続設定(&R)"), this);
    connect(m_proxyConfigButton, &QPushButton::clicked, this, &ConnectionEditDialog::onProxyConfig);

    auto *proxyGroup = new QGroupBox(tr("経由するプロキシサーバーの設定(&X):"), this);
    auto *proxyLayout = new QVBoxLayout(proxyGroup);
    proxyLayout->addWidget(wrapLabel(tr("プロキシサーバーを経由して VPN Server に接続することができます。"), this));
    proxyLayout->addWidget(new QLabel(tr("プロキシの種類:"), this));
    proxyLayout->addWidget(m_directRadio);
    proxyLayout->addWidget(m_httpRadio);
    proxyLayout->addWidget(m_socksRadio);
    proxyLayout->addWidget(m_proxyConfigButton, 0, Qt::AlignHCenter);

    auto *leftColumn = new QVBoxLayout;
    leftColumn->addWidget(destGroup);
    leftColumn->addWidget(proxyGroup);
    leftColumn->addStretch();

    // 右列: 管理モードの選択とパスワードの入力(M)
    m_serverAdminRadio = new QRadioButton(tr("サーバー管理モード(&S)"), this);
    m_hubAdminRadio = new QRadioButton(tr("仮想 HUB 管理モード(&U)"), this);
    m_serverAdminRadio->setChecked(true);
    auto *modeRow = new QHBoxLayout;
    modeRow->addWidget(m_serverAdminRadio);
    modeRow->addWidget(m_hubAdminRadio);
    modeRow->addStretch();

    m_hubNameCombo = new QComboBox(this);
    m_hubNameCombo->setEditable(true);
    m_hubNameCombo->setEnabled(false);
    auto *hubForm = new QFormLayout;
    hubForm->addRow(tr("仮想 HUB 名(&V):"), m_hubNameCombo);

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_noSaveCheck = new QCheckBox(tr("管理パスワードを保存しない(&W)"), this);
    auto *passwordForm = new QFormLayout;
    passwordForm->addRow(tr("管理パスワード(&A):"), m_passwordEdit);
    passwordForm->addRow(QString(), m_noSaveCheck);

    auto *modeGroup = new QGroupBox(tr("管理モードの選択とパスワードの入力(&M)"), this);
    auto *modeLayout = new QVBoxLayout(modeGroup);
    modeLayout->addWidget(wrapLabel(
        tr("VPN Server には、サーバー管理モードと仮想 HUB 管理モードのどちらかのモードで接続できます。\n\n"
           "サーバー管理モードで接続すると、VPN Server の設定とすべての仮想 HUB が管理できます。\n\n"
           "仮想 HUB 管理モードで接続すると、権限を持っている仮想 HUB の管理ができます。"),
        this));
    modeLayout->addLayout(modeRow);
    modeLayout->addLayout(hubForm);
    modeLayout->addWidget(separator);
    modeLayout->addWidget(wrapLabel(tr("管理モードで接続する際のパスワードを入力してください。"), this));
    modeLayout->addLayout(passwordForm);
    modeLayout->addStretch();

    auto *columns = new QHBoxLayout;
    columns->addLayout(leftColumn, 1);
    columns->addWidget(modeGroup, 1);

    // 下段: OK / キャンセル
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &ConnectionEditDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(wrapLabel(tr("管理する VPN Server の接続設定を行います。"), this));
    layout->addLayout(nameRow);
    layout->addLayout(columns);
    layout->addWidget(m_buttonBox);

    connect(m_localhostCheck, &QCheckBox::toggled, this, &ConnectionEditDialog::onLocalhostToggled);
    connect(m_serverAdminRadio, &QRadioButton::toggled, this, &ConnectionEditDialog::onModeChanged);
    connect(m_directRadio, &QRadioButton::toggled, this, &ConnectionEditDialog::onProxyTypeChanged);
    connect(m_httpRadio, &QRadioButton::toggled, this, &ConnectionEditDialog::onProxyTypeChanged);
    connect(m_socksRadio, &QRadioButton::toggled, this, &ConnectionEditDialog::onProxyTypeChanged);
    for (QLineEdit *edit : {m_nameEdit, m_hostEdit}) {
        connect(edit, &QLineEdit::textChanged, this, &ConnectionEditDialog::updateOkEnabled);
    }
    connect(m_hubNameCombo, &QComboBox::editTextChanged, this, &ConnectionEditDialog::updateOkEnabled);
    connect(m_portCombo, &QComboBox::editTextChanged, this, &ConnectionEditDialog::updateOkEnabled);

    onProxyTypeChanged();
    updateOkEnabled();
    DialogSizing::fitToWidth(this, 760);
}

int ConnectionEditDialog::portValue() const
{
    // "5555 (SE-VPN ポート)" のような表記から先頭の数値だけを取り出す。
    const QString text = m_portCombo->currentText().trimmed();
    int end = 0;
    while (end < text.size() && text.at(end).isDigit()) {
        ++end;
    }
    const int value = text.left(end).toInt();
    return value >= 1 && value <= 65535 ? value : 0;
}

void ConnectionEditDialog::onLocalhostToggled(bool checked)
{
    m_hostEdit->setEnabled(!checked);
    if (checked) {
        m_hostEdit->setText(QStringLiteral("localhost"));
    }
}

void ConnectionEditDialog::onModeChanged()
{
    m_hubNameCombo->setEnabled(m_hubAdminRadio->isChecked());
    updateOkEnabled();
}

void ConnectionEditDialog::onProxyTypeChanged()
{
    m_proxyConfigButton->setEnabled(!m_directRadio->isChecked());
    updateOkEnabled();
}

void ConnectionEditDialog::onProxyConfig()
{
    ProxySettingsDialog dialog(this);
    dialog.setValues(m_proxyHost, m_proxyPort, m_proxyUser, m_proxyPassword);
    if (dialog.exec() == QDialog::Accepted) {
        m_proxyHost = dialog.host();
        m_proxyPort = dialog.port();
        m_proxyUser = dialog.user();
        m_proxyPassword = dialog.password();
        updateOkEnabled();
    }
}

void ConnectionEditDialog::updateOkEnabled()
{
    // 公式同様、必須項目が揃うまでOKを押せない。
    bool valid = !m_nameEdit->text().trimmed().isEmpty() && !m_hostEdit->text().trimmed().isEmpty() && portValue() > 0;
    if (m_hubAdminRadio->isChecked()) {
        valid = valid && !m_hubNameCombo->currentText().trimmed().isEmpty();
    }
    if (!m_directRadio->isChecked()) {
        valid = valid && !m_proxyHost.isEmpty();
    }
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

void ConnectionEditDialog::setProfile(const ConnectionProfile &profile)
{
    // SM_EDIT_CAPTION_2
    setWindowTitle(tr("%1 の編集").arg(profile.name));

    m_nameEdit->setText(profile.name);
    m_hostEdit->setText(profile.host);
    m_localhostCheck->setChecked(profile.host == QStringLiteral("localhost"));
    m_portCombo->setEditText(QString::number(profile.port));
    m_hubAdminRadio->setChecked(profile.hubAdminMode);
    m_serverAdminRadio->setChecked(!profile.hubAdminMode);
    m_hubNameCombo->setEditText(profile.hubName);
    m_passwordEdit->setText(profile.password);
    m_noSaveCheck->setChecked(profile.noSavePassword);

    m_proxyHost = profile.proxyHost;
    m_proxyPort = profile.proxyPort;
    m_proxyUser = profile.proxyUser;
    m_proxyPassword = profile.proxyPassword;
    m_httpRadio->setChecked(profile.proxyType == 1);
    m_socksRadio->setChecked(profile.proxyType == 2);
    m_directRadio->setChecked(profile.proxyType != 1 && profile.proxyType != 2);
    updateOkEnabled();
}

ConnectionProfile ConnectionEditDialog::profile() const
{
    ConnectionProfile profile;
    profile.name = m_nameEdit->text().trimmed();
    profile.host = m_hostEdit->text().trimmed();
    profile.port = static_cast<quint16>(portValue());
    profile.hubAdminMode = m_hubAdminRadio->isChecked();
    profile.hubName = m_hubNameCombo->currentText().trimmed();
    profile.noSavePassword = m_noSaveCheck->isChecked();
    profile.password = profile.noSavePassword ? QString() : m_passwordEdit->text();
    profile.proxyType = m_httpRadio->isChecked() ? 1 : (m_socksRadio->isChecked() ? 2 : 0);
    profile.proxyHost = m_proxyHost;
    profile.proxyPort = m_proxyPort;
    profile.proxyUser = m_proxyUser;
    profile.proxyPassword = m_proxyPassword;
    return profile;
}

void ConnectionEditDialog::accept()
{
    const QString name = m_nameEdit->text().trimmed();
    if (m_existingNames.contains(name)) {
        // SM_SETTING_EXISTS
        QMessageBox::warning(this, tr("入力エラー"),
                              tr("すでに同じ名前の接続設定 \"%1\" が登録されています。別の名前を指定してください。").arg(name));
        return;
    }
    QDialog::accept();
}
