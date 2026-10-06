#include "ProxySettingsDialog.h"

#include "util/DialogSizing.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

ProxySettingsDialog::ProxySettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("プロキシサーバーの接続設定"));

    auto *introLabel = new QLabel(
        tr("経由するプロキシサーバー (HTTP プロキシまたは SOCKS プロキシ) のホスト名または IP アドレス、ポート番号、および必要な場合は"
           "ユーザー名とパスワードを入力してください。"),
        this);
    introLabel->setWordWrap(true);

    m_hostEdit = new QLineEdit(this);
    m_portCombo = new QComboBox(this);
    m_portCombo->setEditable(true);
    m_portCombo->addItems({QStringLiteral("8080"), QStringLiteral("3128"), QStringLiteral("1080"), QStringLiteral("80")});
    m_userEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    // (オプション) の注記は入力欄の右に置く。
    auto withNote = [this](QWidget *edit, const QString &note) {
        auto *row = new QHBoxLayout;
        row->addWidget(edit, 1);
        row->addWidget(new QLabel(note, this));
        return row;
    };
    auto *form = new QFormLayout;
    form->addRow(tr("ホスト名(&H):"), m_hostEdit);
    form->addRow(tr("ポート番号(&A):"), m_portCombo);
    form->addRow(tr("ユーザー名(&U):"), withNote(m_userEdit, tr("(オプション)")));
    form->addRow(tr("パスワード(&P):"), withNote(m_passwordEdit, tr("(オプション)")));

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_hostEdit, &QLineEdit::textChanged, this, &ProxySettingsDialog::updateOkEnabled);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addLayout(form);
    layout->addWidget(m_buttonBox);

    DialogSizing::fitToWidth(this, 500);
    updateOkEnabled();
}

void ProxySettingsDialog::updateOkEnabled()
{
    // 公式同様、ホスト名が空の間はOKを押せない。
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!m_hostEdit->text().trimmed().isEmpty());
}

void ProxySettingsDialog::setValues(const QString &host, quint16 port, const QString &user, const QString &password)
{
    m_hostEdit->setText(host);
    m_portCombo->setCurrentText(QString::number(port > 0 ? port : 8080));
    m_userEdit->setText(user);
    m_passwordEdit->setText(password);
}

QString ProxySettingsDialog::host() const
{
    return m_hostEdit->text().trimmed();
}

quint16 ProxySettingsDialog::port() const
{
    const int value = m_portCombo->currentText().trimmed().toInt();
    return static_cast<quint16>(value > 0 && value <= 65535 ? value : 8080);
}

QString ProxySettingsDialog::user() const
{
    return m_userEdit->text().trimmed();
}

QString ProxySettingsDialog::password() const
{
    return m_passwordEdit->text();
}
