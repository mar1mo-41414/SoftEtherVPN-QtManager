#include "HubEditDialog.h"

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

HubEditDialog::HubEditDialog(bool isNew, QWidget *parent)
    : QDialog(parent)
    , m_isNew(isNew)
{
    setWindowTitle(isNew ? tr("仮想 HUB の作成") : tr("仮想 HUB のプロパティ"));

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setEnabled(isNew);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordConfirmEdit = new QLineEdit(this);
    m_passwordConfirmEdit->setEchoMode(QLineEdit::Password);

    // R_NO_ENUM
    m_noEnumCheck = new QCheckBox(tr("匿名ユーザーに対してこの仮想 HUB を列挙しない"), this);

    // R_LIMIT_MAX_SESSION
    m_limitMaxSessionCheck = new QCheckBox(tr("最大同時接続セッション数を制限する"), this);
    m_maxSessionSpin = new QSpinBox(this);
    m_maxSessionSpin->setRange(1, 1000000);
    m_maxSessionSpin->setValue(100);
    m_maxSessionSpin->setEnabled(false);
    connect(m_limitMaxSessionCheck, &QCheckBox::toggled, m_maxSessionSpin, &QSpinBox::setEnabled);

    // R_ONLINE / R_OFFLINE
    m_onlineRadio = new QRadioButton(tr("オンライン"), this);
    m_offlineRadio = new QRadioButton(tr("オフライン"), this);
    m_onlineRadio->setChecked(true);

    auto *nameForm = new QFormLayout;
    nameForm->addRow(tr("仮想 HUB 名(&N):"), m_nameEdit);

    auto *securityForm = new QFormLayout;
    securityForm->addRow(tr("パスワード(&P):"), m_passwordEdit);
    securityForm->addRow(tr("確認入力(&C):"), m_passwordConfirmEdit);
    securityForm->addRow(QString(), m_noEnumCheck);
    auto *securityGroup = new QGroupBox(tr("この仮想 HUB の管理用パスワード"), this);
    securityGroup->setLayout(securityForm);

    auto *optionForm = new QFormLayout;
    optionForm->addRow(QString(), m_limitMaxSessionCheck);
    optionForm->addRow(tr("最大同時接続セッション数(&Z):"), m_maxSessionSpin);
    auto *optionGroup = new QGroupBox(tr("仮想 HUB オプション"), this);
    optionGroup->setLayout(optionForm);

    auto *stateLayout = new QVBoxLayout;
    stateLayout->addWidget(m_onlineRadio);
    stateLayout->addWidget(m_offlineRadio);
    auto *stateGroup = new QGroupBox(tr("仮想 HUB の状態"), this);
    stateGroup->setLayout(stateLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &HubEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(nameForm);
    layout->addWidget(securityGroup);
    layout->addWidget(optionGroup);
    layout->addWidget(stateGroup);
    layout->addWidget(buttonBox);

    resize(420, sizeHint().height());
}

void HubEditDialog::setValues(const QString &hubName, bool online, bool noEnum, quint32 maxSession)
{
    m_nameEdit->setText(hubName);
    m_onlineRadio->setChecked(online);
    m_offlineRadio->setChecked(!online);
    m_noEnumCheck->setChecked(noEnum);

    // MaxSession_u32 == 0 は「無制限」を意味する (公式Managerの挙動を踏襲)。
    if (maxSession > 0) {
        m_limitMaxSessionCheck->setChecked(true);
        m_maxSessionSpin->setValue(static_cast<int>(maxSession));
    } else {
        m_limitMaxSessionCheck->setChecked(false);
    }
}

QString HubEditDialog::hubName() const
{
    return m_nameEdit->text().trimmed();
}

QJsonObject HubEditDialog::toRpcParams() const
{
    QJsonObject params;
    params["HubName_str"] = m_nameEdit->text().trimmed();
    // 編集時に空欄のままなら「パスワードを変更しない」として扱われる (API仕様通り)。
    params["AdminPasswordPlainText_str"] = m_passwordEdit->text();
    params["Online_bool"] = m_onlineRadio->isChecked();
    params["MaxSession_u32"] = m_limitMaxSessionCheck->isChecked() ? m_maxSessionSpin->value() : 0;
    params["NoEnum_bool"] = m_noEnumCheck->isChecked();
    params["HubType_u32"] = 0; // スタンドアロン (クラスタリング対応は将来のフェーズ)
    return params;
}

void HubEditDialog::accept()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("仮想 HUB 名を入力してください。"));
        return;
    }
    if (m_passwordEdit->text() != m_passwordConfirmEdit->text()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("パスワードと確認入力が一致しません。"));
        return;
    }

    QDialog::accept();
}
