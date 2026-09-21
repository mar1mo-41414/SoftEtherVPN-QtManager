#include "AccessEditDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

AccessEditDialog::AccessEditDialog(QWidget *parent)
    : QDialog(parent)
{
    // D_SM_EDIT_ACCESS CAPTION
    setWindowTitle(tr("アクセスリスト項目の編集"));

    m_noteEdit = new QLineEdit(this);
    m_passRadio = new QRadioButton(tr("通過(&P)"), this);
    m_discardRadio = new QRadioButton(tr("破棄(&D)"), this);
    m_passRadio->setChecked(true);
    m_prioritySpin = new QSpinBox(this);
    m_prioritySpin->setRange(1, 999999);
    m_prioritySpin->setValue(1000);
    m_activeCheck = new QCheckBox(tr("有効にする"), this);
    m_activeCheck->setChecked(true);

    auto *actionLayout = new QHBoxLayout;
    actionLayout->addWidget(m_passRadio);
    actionLayout->addWidget(m_discardRadio);

    auto *basicForm = new QFormLayout;
    basicForm->addRow(tr("アクセスリストの説明(&N):"), m_noteEdit);
    basicForm->addRow(tr("動作(&A):"), actionLayout);
    basicForm->addRow(tr("優先順位(&R):"), m_prioritySpin);
    basicForm->addRow(QString(), m_activeCheck);
    auto *basicGroup = new QGroupBox(tr("基本設定"), this);
    basicGroup->setLayout(basicForm);

    m_srcAllCheck = new QCheckBox(tr("すべての送信元に対して適用する"), this);
    m_srcIpEdit = new QLineEdit(this);
    m_srcMaskEdit = new QLineEdit(this);
    m_srcMaskEdit->setText(QStringLiteral("255.255.255.255"));
    connect(m_srcAllCheck, &QCheckBox::toggled, m_srcIpEdit, &QLineEdit::setDisabled);
    connect(m_srcAllCheck, &QCheckBox::toggled, m_srcMaskEdit, &QLineEdit::setDisabled);

    m_dstAllCheck = new QCheckBox(tr("すべての宛先に対して適用する"), this);
    m_dstIpEdit = new QLineEdit(this);
    m_dstMaskEdit = new QLineEdit(this);
    m_dstMaskEdit->setText(QStringLiteral("255.255.255.255"));
    connect(m_dstAllCheck, &QCheckBox::toggled, m_dstIpEdit, &QLineEdit::setDisabled);
    connect(m_dstAllCheck, &QCheckBox::toggled, m_dstMaskEdit, &QLineEdit::setDisabled);

    m_protocolCombo = new QComboBox(this);
    m_protocolCombo->addItem(tr("すべてのプロトコル"), 0);
    m_protocolCombo->addItem(tr("TCP"), 6);
    m_protocolCombo->addItem(tr("UDP"), 17);
    m_protocolCombo->addItem(tr("ICMP"), 1);

    auto *ipForm = new QFormLayout;
    ipForm->addRow(tr("送信元 IP アドレス:"), m_srcAllCheck);
    ipForm->addRow(tr("IPv4 アドレス:"), m_srcIpEdit);
    ipForm->addRow(tr("マスク:"), m_srcMaskEdit);
    ipForm->addRow(tr("宛先 IP アドレス:"), m_dstAllCheck);
    ipForm->addRow(tr("IPv4 アドレス:"), m_dstIpEdit);
    ipForm->addRow(tr("マスク:"), m_dstMaskEdit);
    ipForm->addRow(tr("プロトコルの種類:"), m_protocolCombo);
    auto *ipGroup = new QGroupBox(tr("IP ヘッダに関するフィルタリングオプション"), this);
    ipGroup->setLayout(ipForm);

    auto *intValidator = new QIntValidator(0, 65535, this);
    m_srcPortStartEdit = new QLineEdit(this);
    m_srcPortStartEdit->setValidator(intValidator);
    m_srcPortEndEdit = new QLineEdit(this);
    m_srcPortEndEdit->setValidator(intValidator);
    m_dstPortStartEdit = new QLineEdit(this);
    m_dstPortStartEdit->setValidator(intValidator);
    m_dstPortEndEdit = new QLineEdit(this);
    m_dstPortEndEdit->setValidator(intValidator);

    auto *srcPortLayout = new QHBoxLayout;
    srcPortLayout->addWidget(new QLabel(tr("最小値"), this));
    srcPortLayout->addWidget(m_srcPortStartEdit);
    srcPortLayout->addWidget(new QLabel(tr("最大値"), this));
    srcPortLayout->addWidget(m_srcPortEndEdit);
    auto *dstPortLayout = new QHBoxLayout;
    dstPortLayout->addWidget(new QLabel(tr("最小値"), this));
    dstPortLayout->addWidget(m_dstPortStartEdit);
    dstPortLayout->addWidget(new QLabel(tr("最大値"), this));
    dstPortLayout->addWidget(m_dstPortEndEdit);

    auto *portForm = new QFormLayout;
    portForm->addRow(tr("送信元ポート番号:"), srcPortLayout);
    portForm->addRow(tr("宛先ポート番号:"), dstPortLayout);
    auto *portHint = new QLabel(
        tr("ポート番号が空欄の場合はすべてのポートに対して適用されます。TCP/UDPの場合のみ有効です。"), this);
    portHint->setWordWrap(true);
    auto *portGroup = new QGroupBox(tr("TCP ヘッダまたは UDP ヘッダに関するフィルタリングオプション"), this);
    auto *portGroupLayout = new QVBoxLayout(portGroup);
    portGroupLayout->addLayout(portForm);
    portGroupLayout->addWidget(portHint);

    m_srcUsernameEdit = new QLineEdit(this);
    m_dstUsernameEdit = new QLineEdit(this);
    auto *userForm = new QFormLayout;
    userForm->addRow(tr("送信元の名前:"), m_srcUsernameEdit);
    userForm->addRow(tr("宛先の名前:"), m_dstUsernameEdit);
    auto *userHint = new QLabel(tr("それぞれユーザー名またはグループ名を指定してください。指定しない場合は空欄にしてください。"), this);
    userHint->setWordWrap(true);
    auto *userGroup = new QGroupBox(tr("ユーザーまたはグループに関するフィルタリングオプション"), this);
    auto *userGroupLayout = new QVBoxLayout(userGroup);
    userGroupLayout->addLayout(userForm);
    userGroupLayout->addWidget(userHint);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AccessEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(basicGroup);
    layout->addWidget(ipGroup);
    layout->addWidget(portGroup);
    layout->addWidget(userGroup);
    layout->addWidget(buttonBox);

    resize(460, 640);
}

void AccessEditDialog::setValues(const QJsonObject &access)
{
    m_id = static_cast<quint32>(access.value("Id_u32").toDouble());
    m_noteEdit->setText(access.value("Note_utf").toString());
    m_discardRadio->setChecked(access.value("Discard_bool").toBool());
    m_passRadio->setChecked(!access.value("Discard_bool").toBool());
    m_prioritySpin->setValue(access.value("Priority_u32").toInt());
    m_activeCheck->setChecked(access.value("Active_bool").toBool());

    const QString srcIp = access.value("SrcIpAddress_ip").toString();
    m_srcAllCheck->setChecked(srcIp.isEmpty() || srcIp == QStringLiteral("0.0.0.0"));
    m_srcIpEdit->setText(srcIp);
    m_srcMaskEdit->setText(access.value("SrcSubnetMask_ip").toString());

    const QString dstIp = access.value("DestIpAddress_ip").toString();
    m_dstAllCheck->setChecked(dstIp.isEmpty() || dstIp == QStringLiteral("0.0.0.0"));
    m_dstIpEdit->setText(dstIp);
    m_dstMaskEdit->setText(access.value("DestSubnetMask_ip").toString());

    const int protocolIndex = m_protocolCombo->findData(access.value("Protocol_u32").toInt());
    m_protocolCombo->setCurrentIndex(protocolIndex >= 0 ? protocolIndex : 0);

    const int srcPortStart = access.value("SrcPortStart_u32").toInt();
    if (srcPortStart > 0) {
        m_srcPortStartEdit->setText(QString::number(srcPortStart));
        m_srcPortEndEdit->setText(QString::number(access.value("SrcPortEnd_u32").toInt()));
    }
    const int dstPortStart = access.value("DestPortStart_u32").toInt();
    if (dstPortStart > 0) {
        m_dstPortStartEdit->setText(QString::number(dstPortStart));
        m_dstPortEndEdit->setText(QString::number(access.value("DestPortEnd_u32").toInt()));
    }

    m_srcUsernameEdit->setText(access.value("SrcUsername_str").toString());
    m_dstUsernameEdit->setText(access.value("DestUsername_str").toString());
}

QJsonObject AccessEditDialog::toRpcParams() const
{
    QJsonObject params;
    params["Id_u32"] = static_cast<qint64>(m_id);
    params["Note_utf"] = m_noteEdit->text();
    params["Active_bool"] = m_activeCheck->isChecked();
    params["Priority_u32"] = m_prioritySpin->value();
    params["Discard_bool"] = m_discardRadio->isChecked();
    params["IsIPv6_bool"] = false;

    params["SrcIpAddress_ip"] = m_srcAllCheck->isChecked() ? QStringLiteral("0.0.0.0") : m_srcIpEdit->text();
    params["SrcSubnetMask_ip"] = m_srcAllCheck->isChecked() ? QStringLiteral("0.0.0.0") : m_srcMaskEdit->text();
    params["DestIpAddress_ip"] = m_dstAllCheck->isChecked() ? QStringLiteral("0.0.0.0") : m_dstIpEdit->text();
    params["DestSubnetMask_ip"] = m_dstAllCheck->isChecked() ? QStringLiteral("0.0.0.0") : m_dstMaskEdit->text();

    params["Protocol_u32"] = m_protocolCombo->currentData().toInt();
    const int srcPortStart = m_srcPortStartEdit->text().isEmpty() ? 0 : m_srcPortStartEdit->text().toInt();
    const int dstPortStart = m_dstPortStartEdit->text().isEmpty() ? 0 : m_dstPortStartEdit->text().toInt();
    params["SrcPortStart_u32"] = srcPortStart;
    params["SrcPortEnd_u32"] = m_srcPortEndEdit->text().isEmpty() ? srcPortStart : m_srcPortEndEdit->text().toInt();
    params["DestPortStart_u32"] = dstPortStart;
    params["DestPortEnd_u32"] = m_dstPortEndEdit->text().isEmpty() ? dstPortStart : m_dstPortEndEdit->text().toInt();

    params["SrcUsername_str"] = m_srcUsernameEdit->text().trimmed();
    params["DestUsername_str"] = m_dstUsernameEdit->text().trimmed();

    return params;
}

void AccessEditDialog::accept()
{
    if (!m_srcAllCheck->isChecked() && m_srcIpEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("送信元 IPv4 アドレスを入力するか、「すべての送信元に対して適用する」を選んでください。"));
        return;
    }
    if (!m_dstAllCheck->isChecked() && m_dstIpEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("宛先 IPv4 アドレスを入力するか、「すべての宛先に対して適用する」を選んでください。"));
        return;
    }
    QDialog::accept();
}
