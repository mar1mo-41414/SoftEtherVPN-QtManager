#include "AccessEditDialog.h"
#include "UserListDialog.h"

#include "util/SoftEtherLabels.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

QFrame *separator(QWidget *parent)
{
    auto *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

QLabel *note(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    return label;
}

// ---- IP アドレス / マスク ----

QByteArray ipv6Bytes(const QHostAddress &address)
{
    const Q_IPV6ADDR v6 = address.toIPv6Address();
    return QByteArray(reinterpret_cast<const char *>(v6.c), 16);
}

QString ipv6Text(const QByteArray &bytes)
{
    if (bytes.size() != 16) {
        return QString();
    }
    Q_IPV6ADDR v6;
    for (int i = 0; i < 16; ++i) {
        v6.c[i] = static_cast<quint8>(bytes.at(i));
    }
    return QHostAddress(v6).toString();
}

QByteArray ipv4ToBytes(quint32 v4)
{
    QByteArray bytes(4, 0);
    for (int i = 0; i < 4; ++i) {
        bytes[i] = static_cast<char>((v4 >> (24 - 8 * i)) & 0xff);
    }
    return bytes;
}

// 連続した上位ビットのみが1のマスクなら、そのビット数を返す (それ以外は -1)。
int prefixLength(const QByteArray &mask)
{
    int bits = 0;
    bool zeroSeen = false;
    for (char c : mask) {
        const quint8 byte = static_cast<quint8>(c);
        for (int bit = 7; bit >= 0; --bit) {
            const bool one = (byte >> bit) & 1;
            if (one && zeroSeen) {
                return -1;
            }
            if (one) {
                ++bits;
            } else {
                zeroSeen = true;
            }
        }
    }
    return bits;
}

QByteArray prefixMask(int bits, int bytes)
{
    QByteArray mask(bytes, 0);
    for (int i = 0; i < bits && i < bytes * 8; ++i) {
        mask[i / 8] = static_cast<char>(static_cast<quint8>(mask[i / 8]) | (0x80 >> (i % 8)));
    }
    return mask;
}

// "/24" 形式なら prefix、そうでなければ通常のアドレス表記としてマスクを解釈する。
bool parseMask(const QString &text, bool ipv6, QByteArray *out)
{
    const QString trimmed = text.trimmed();
    const int bytes = ipv6 ? 16 : 4;
    if (trimmed.startsWith(QLatin1Char('/'))) {
        bool ok = false;
        const int bits = trimmed.mid(1).toInt(&ok);
        if (!ok || bits < 0 || bits > bytes * 8) {
            return false;
        }
        *out = prefixMask(bits, bytes);
        return true;
    }
    QHostAddress address;
    if (!address.setAddress(trimmed)) {
        return false;
    }
    if (ipv6) {
        if (address.protocol() != QAbstractSocket::IPv6Protocol) {
            return false;
        }
        *out = ipv6Bytes(address);
    } else {
        if (address.protocol() != QAbstractSocket::IPv4Protocol) {
            return false;
        }
        const quint32 v4 = address.toIPv4Address();
        *out = ipv4ToBytes(v4);
    }
    return true;
}

QString ipv4MaskText(const QByteArray &mask)
{
    if (mask.size() != 4) {
        return QString();
    }
    return QStringLiteral("%1.%2.%3.%4")
        .arg(static_cast<quint8>(mask[0]))
        .arg(static_cast<quint8>(mask[1]))
        .arg(static_cast<quint8>(mask[2]))
        .arg(static_cast<quint8>(mask[3]));
}

QByteArray ipv4Bytes(const QString &text)
{
    return ipv4ToBytes(QHostAddress(text).toIPv4Address());
}

QString maskForDisplay(const QByteArray &mask, bool ipv6)
{
    const int bits = prefixLength(mask);
    if (bits >= 0) {
        return QStringLiteral("/%1").arg(bits);
    }
    return ipv6 ? ipv6Text(mask) : ipv4MaskText(mask);
}

// ---- MAC アドレス ----

bool parseMac(const QString &text, QByteArray *out)
{
    QString hex = text;
    hex.remove(QRegularExpression(QStringLiteral("[-:\\s]")));
    static const QRegularExpression re(QStringLiteral("^[0-9A-Fa-f]{12}$"));
    if (!re.match(hex).hasMatch()) {
        return false;
    }
    *out = QByteArray::fromHex(hex.toLatin1());
    return true;
}

QString macText(const QByteArray &bytes)
{
    QStringList parts;
    for (char c : bytes) {
        parts << QStringLiteral("%1").arg(static_cast<quint8>(c), 2, 16, QLatin1Char('0')).toUpper();
    }
    return parts.join(QLatin1Char('-'));
}

QByteArray decodeBin(const QJsonValue &value)
{
    return QByteArray::fromBase64(value.toString().toLatin1());
}

QString encodeBin(const QByteArray &bytes)
{
    return QString::fromLatin1(bytes.toBase64());
}

// ---- 一覧用の「内容」文字列 ----

QString protocolDescription(int protocol)
{
    switch (protocol) {
    case 1:
        return QStringLiteral("ICMPv4");
    case 6:
        return QStringLiteral("TCP");
    case 17:
        return QStringLiteral("UDP");
    case 58:
        return QStringLiteral("ICMPv6");
    default:
        return QString::number(protocol);
    }
}

QString portDescription(int start, int end)
{
    return start == end || end == 0 ? QString::number(start) : QStringLiteral("%1-%2").arg(start).arg(end);
}

} // namespace

AccessEditDialog::AccessEditDialog(VpnServerRpc *rpc, const QString &hubName, bool ipv6, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(hubName)
    , m_ipv6(ipv6)
{
    // D_SM_EDIT_ACCESS CAPTION + SM_ACCESS_IPV4/IPV6 相当
    setWindowTitle(ipv6 ? tr("アクセスリスト項目の編集 (IPv6)") : tr("アクセスリスト項目の編集 (IPv4)"));

    auto *heading = note(
        tr("アクセスリスト項目を設定してください。ここで設定したアクセスリストは、仮想 HUB 内を通過するすべての IP パケットに対して適用されます。"),
        this);

    // ---------------- 左列 ----------------
    // 基本設定
    m_noteEdit = new QLineEdit(this);
    m_passRadio = new QRadioButton(tr("通過(P)"), this);
    m_discardRadio = new QRadioButton(tr("破棄(D)"), this);
    m_passRadio->setChecked(true);
    m_prioritySpin = new QSpinBox(this);
    m_prioritySpin->setRange(1, 999999999);
    m_prioritySpin->setValue(1000);
    auto *actionRow = new QHBoxLayout;
    actionRow->addWidget(m_passRadio);
    actionRow->addWidget(m_discardRadio);
    actionRow->addStretch();
    auto *priorityRow = new QHBoxLayout;
    priorityRow->addWidget(m_prioritySpin);
    priorityRow->addWidget(new QLabel(tr("(整数値: 小さいほど\n優先順位が高くなります)"), this));
    priorityRow->addStretch();
    auto *basicForm = new QFormLayout;
    basicForm->setLabelAlignment(Qt::AlignRight);
    basicForm->addRow(tr("アクセスリストの説明(N):"), m_noteEdit);
    basicForm->addRow(tr("動作(A):"), actionRow);
    basicForm->addRow(tr("優先順位(R):"), priorityRow);
    auto *basicGroup = new QGroupBox(tr("基本設定"), this);
    basicGroup->setLayout(basicForm);

    // ユーザーまたはグループ
    m_srcUserEdit = new QLineEdit(this);
    m_dstUserEdit = new QLineEdit(this);
    auto *srcUserButton = new QPushButton(tr("参照..."), this);
    auto *dstUserButton = new QPushButton(tr("参照..."), this);
    auto *userGrid = new QGridLayout;
    userGrid->addWidget(new QLabel(tr("送信元の名前:"), this), 0, 0, Qt::AlignRight);
    userGrid->addWidget(m_srcUserEdit, 0, 1);
    userGrid->addWidget(srcUserButton, 0, 2);
    userGrid->addWidget(new QLabel(tr("宛先の名前:"), this), 1, 0, Qt::AlignRight);
    userGrid->addWidget(m_dstUserEdit, 1, 1);
    userGrid->addWidget(dstUserButton, 1, 2);
    userGrid->setColumnStretch(1, 1);
    auto *userGroup = new QGroupBox(tr("ユーザーまたはグループに関するフィルタリングオプション"), this);
    auto *userLayout = new QVBoxLayout(userGroup);
    userLayout->addWidget(note(
        tr("このアクセスリスト項目を特定のユーザーまたはグループが送信したパケット、または特定のユーザーまたはグループによって受信されるパケットに対してのみ適用することができます。"),
        this));
    userLayout->addLayout(userGrid);
    userLayout->addWidget(note(tr("それぞれユーザー名またはグループ名のどちらかを指定してください。指定しない場合は空欄にしてください。"), this));

    // MAC ヘッダ
    m_srcMacAllCheck = new QCheckBox(tr("すべての送信元に対して適用する"), this);
    m_srcMacAllCheck->setChecked(true);
    m_srcMacEdit = new QLineEdit(this);
    m_srcMacMaskEdit = new QLineEdit(this);
    m_dstMacAllCheck = new QCheckBox(tr("すべての宛先に対して適用する"), this);
    m_dstMacAllCheck->setChecked(true);
    m_dstMacEdit = new QLineEdit(this);
    m_dstMacMaskEdit = new QLineEdit(this);
    auto *macGrid = new QGridLayout;
    macGrid->addWidget(new QLabel(tr("送信元 MAC アドレス:"), this), 0, 0, Qt::AlignRight);
    macGrid->addWidget(m_srcMacAllCheck, 0, 1);
    macGrid->addWidget(new QLabel(tr("MAC アドレス:"), this), 1, 0, Qt::AlignRight);
    macGrid->addWidget(m_srcMacEdit, 1, 1);
    macGrid->addWidget(new QLabel(tr("マスク:"), this), 2, 0, Qt::AlignRight);
    macGrid->addWidget(m_srcMacMaskEdit, 2, 1);
    macGrid->addWidget(separator(this), 3, 0, 1, 2);
    macGrid->addWidget(new QLabel(tr("宛先 MAC アドレス:"), this), 4, 0, Qt::AlignRight);
    macGrid->addWidget(m_dstMacAllCheck, 4, 1);
    macGrid->addWidget(new QLabel(tr("MAC アドレス:"), this), 5, 0, Qt::AlignRight);
    macGrid->addWidget(m_dstMacEdit, 5, 1);
    macGrid->addWidget(new QLabel(tr("マスク:"), this), 6, 0, Qt::AlignRight);
    macGrid->addWidget(m_dstMacMaskEdit, 6, 1);
    macGrid->setColumnStretch(1, 1);
    auto *macGroup = new QGroupBox(tr("MAC ヘッダに関するフィルタリングオプション"), this);
    auto *macLayout = new QVBoxLayout(macGroup);
    macLayout->addLayout(macGrid);
    macLayout->addWidget(note(
        tr("MAC アドレスとマスクに 16 進数と \"-\" または \":\" の文字が使えますが、省略することも可能です。(FF-FF-FF-FF-FF-FF: 単一ホスト) "),
        this));

    auto *leftColumn = new QVBoxLayout;
    leftColumn->addWidget(basicGroup);
    leftColumn->addWidget(userGroup);
    leftColumn->addWidget(macGroup);
    leftColumn->addStretch();

    // ---------------- 右列 ----------------
    // IP ヘッダ
    m_srcAllCheck = new QCheckBox(tr("すべての送信元に対して適用する"), this);
    m_srcAllCheck->setChecked(true);
    m_srcIpEdit = new QLineEdit(this);
    m_srcMaskEdit = new QLineEdit(this);
    m_dstAllCheck = new QCheckBox(tr("すべての宛先に対して適用する"), this);
    m_dstAllCheck->setChecked(true);
    m_dstIpEdit = new QLineEdit(this);
    m_dstMaskEdit = new QLineEdit(this);
    const QString addressLabel = ipv6 ? tr("IPv6 アドレス:") : tr("IPv4 アドレス:");
    const QString maskHint =
        ipv6 ? tr("(マスク表記例: \"ffff:ff00::\" または \"/24\"。単一ホストの場合は \"/128\")") : tr("(255.255.255.255: 単一ホスト)");
    m_protocolCombo = new QComboBox(this);
    // SM_ACCESS_PROTO_1〜6
    m_protocolCombo->addItem(tr("すべての IPv4 / IPv6 プロトコル"), 0);
    m_protocolCombo->addItem(tr("6 (TCP/IP プロトコル)"), 6);
    m_protocolCombo->addItem(tr("17 (UDP/IP プロトコル)"), 17);
    m_protocolCombo->addItem(tr("1 (ICMPv4 プロトコル)"), 1);
    m_protocolCombo->addItem(tr("58 (ICMPv6 プロトコル)"), 58);
    m_protocolCombo->addItem(tr("IP プロトコル番号を指定"), -1);
    m_protocolNumberEdit = new QLineEdit(this);
    m_protocolNumberEdit->setValidator(new QIntValidator(0, 255, this));

    auto *ipGrid = new QGridLayout;
    ipGrid->addWidget(new QLabel(tr("送信元 IP アドレス:"), this), 0, 0, Qt::AlignRight);
    ipGrid->addWidget(m_srcAllCheck, 0, 1);
    ipGrid->addWidget(new QLabel(addressLabel, this), 1, 0, Qt::AlignRight);
    ipGrid->addWidget(m_srcIpEdit, 1, 1);
    ipGrid->addWidget(new QLabel(tr("マスク:"), this), 2, 0, Qt::AlignRight);
    ipGrid->addWidget(m_srcMaskEdit, 2, 1);
    ipGrid->addWidget(note(maskHint, this), 3, 1);
    ipGrid->addWidget(separator(this), 4, 0, 1, 2);
    ipGrid->addWidget(new QLabel(tr("宛先 IP アドレス:"), this), 5, 0, Qt::AlignRight);
    ipGrid->addWidget(m_dstAllCheck, 5, 1);
    ipGrid->addWidget(new QLabel(addressLabel, this), 6, 0, Qt::AlignRight);
    ipGrid->addWidget(m_dstIpEdit, 6, 1);
    ipGrid->addWidget(new QLabel(tr("マスク:"), this), 7, 0, Qt::AlignRight);
    ipGrid->addWidget(m_dstMaskEdit, 7, 1);
    ipGrid->addWidget(note(maskHint, this), 8, 1);
    ipGrid->addWidget(separator(this), 9, 0, 1, 2);
    ipGrid->addWidget(new QLabel(tr("プロトコルの種類:"), this), 10, 0, Qt::AlignRight);
    ipGrid->addWidget(m_protocolCombo, 10, 1);
    ipGrid->addWidget(new QLabel(tr("IP プロトコル番号の指定:"), this), 11, 0, Qt::AlignRight);
    ipGrid->addWidget(m_protocolNumberEdit, 11, 1);
    ipGrid->setColumnStretch(1, 1);
    auto *ipGroup = new QGroupBox(tr("IP ヘッダに関するフィルタリングオプション"), this);
    ipGroup->setLayout(ipGrid);

    // TCP / UDP ヘッダ
    m_srcPortMinEdit = new QLineEdit(this);
    m_srcPortMaxEdit = new QLineEdit(this);
    m_dstPortMinEdit = new QLineEdit(this);
    m_dstPortMaxEdit = new QLineEdit(this);
    auto *portValidator = new QIntValidator(0, 65535, this);
    for (QLineEdit *edit : {m_srcPortMinEdit, m_srcPortMaxEdit, m_dstPortMinEdit, m_dstPortMaxEdit}) {
        edit->setValidator(portValidator);
    }
    m_tcpStateCheck = new QCheckBox(tr("TCP コネクションの状態を検査 (TCP パケットのみ)"), this);
    m_establishedRadio = new QRadioButton(tr("確立済みパケット"), this);
    m_unestablishedRadio = new QRadioButton(tr("未確立パケット"), this);
    m_establishedRadio->setChecked(true);
    auto *portGrid = new QGridLayout;
    portGrid->addWidget(new QLabel(tr("最小値"), this), 0, 1, Qt::AlignHCenter);
    portGrid->addWidget(new QLabel(tr("最大値"), this), 0, 3, Qt::AlignHCenter);
    portGrid->addWidget(new QLabel(tr("送信元ポート番号:"), this), 1, 0, Qt::AlignRight);
    portGrid->addWidget(m_srcPortMinEdit, 1, 1);
    portGrid->addWidget(new QLabel(QStringLiteral("-"), this), 1, 2, Qt::AlignHCenter);
    portGrid->addWidget(m_srcPortMaxEdit, 1, 3);
    portGrid->addWidget(new QLabel(tr("宛先ポート番号:"), this), 2, 0, Qt::AlignRight);
    portGrid->addWidget(m_dstPortMinEdit, 2, 1);
    portGrid->addWidget(new QLabel(QStringLiteral("-"), this), 2, 2, Qt::AlignHCenter);
    portGrid->addWidget(m_dstPortMaxEdit, 2, 3);
    auto *stateRow = new QHBoxLayout;
    stateRow->addSpacing(24);
    stateRow->addWidget(m_establishedRadio);
    stateRow->addWidget(m_unestablishedRadio);
    stateRow->addStretch();
    auto *portGroup = new QGroupBox(tr("TCP ヘッダまたは UDP ヘッダに関するフィルタリングオプション"), this);
    auto *portLayout = new QVBoxLayout(portGroup);
    portLayout->addLayout(portGrid);
    portLayout->addWidget(note(
        tr("ポート番号が空欄の場合はすべてのポートに対して適用されます。\n最小値が指定されていて最大値が指定されていない場合は最小値と一致するパケットのみ適用されます。"),
        this));
    portLayout->addWidget(separator(this));
    portLayout->addWidget(m_tcpStateCheck);
    portLayout->addLayout(stateRow);

    auto *rightColumn = new QVBoxLayout;
    rightColumn->addWidget(ipGroup);
    rightColumn->addWidget(portGroup);
    rightColumn->addStretch();

    auto *columns = new QHBoxLayout;
    columns->addLayout(leftColumn, 1);
    columns->addLayout(rightColumn, 1);

    // ---------------- 下段 ----------------
    m_redirectCheck = new QCheckBox(tr("HTTP アクセスを強制的に指定 URL へリダイレクト"), this);
    m_redirectButton = new QPushButton(tr("リダイレクト先 URL..."), this);
    auto *simulationButton = new QPushButton(tr("遅延・パケットロス生成機能(&L)..."), this);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AccessEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto *bottom = new QHBoxLayout;
    bottom->addWidget(m_redirectCheck);
    bottom->addWidget(m_redirectButton);
    bottom->addWidget(simulationButton);
    bottom->addStretch();
    bottom->addWidget(buttonBox);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(heading);
    layout->addLayout(columns, 1);
    layout->addLayout(bottom);

    for (QCheckBox *check : {m_srcAllCheck, m_dstAllCheck, m_srcMacAllCheck, m_dstMacAllCheck, m_tcpStateCheck, m_redirectCheck}) {
        connect(check, &QCheckBox::toggled, this, &AccessEditDialog::updateState);
    }
    connect(m_protocolCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &AccessEditDialog::updateState);
    connect(srcUserButton, &QPushButton::clicked, this, &AccessEditDialog::onPickSrcUser);
    connect(dstUserButton, &QPushButton::clicked, this, &AccessEditDialog::onPickDstUser);
    connect(m_redirectButton, &QPushButton::clicked, this, &AccessEditDialog::onRedirect);
    connect(simulationButton, &QPushButton::clicked, this, &AccessEditDialog::onSimulation);

    resize(880, 660);
    updateState();
}

void AccessEditDialog::updateState()
{
    m_srcIpEdit->setEnabled(!m_srcAllCheck->isChecked());
    m_srcMaskEdit->setEnabled(!m_srcAllCheck->isChecked());
    m_dstIpEdit->setEnabled(!m_dstAllCheck->isChecked());
    m_dstMaskEdit->setEnabled(!m_dstAllCheck->isChecked());
    m_srcMacEdit->setEnabled(!m_srcMacAllCheck->isChecked());
    m_srcMacMaskEdit->setEnabled(!m_srcMacAllCheck->isChecked());
    m_dstMacEdit->setEnabled(!m_dstMacAllCheck->isChecked());
    m_dstMacMaskEdit->setEnabled(!m_dstMacAllCheck->isChecked());

    const int protocol = m_protocolCombo->currentData().toInt();
    m_protocolNumberEdit->setEnabled(protocol == -1);
    const bool portsEnabled = protocol == 6 || protocol == 17;
    for (QLineEdit *edit : {m_srcPortMinEdit, m_srcPortMaxEdit, m_dstPortMinEdit, m_dstPortMaxEdit}) {
        edit->setEnabled(portsEnabled);
    }
    m_tcpStateCheck->setEnabled(protocol == 6);
    m_establishedRadio->setEnabled(protocol == 6 && m_tcpStateCheck->isChecked());
    m_unestablishedRadio->setEnabled(protocol == 6 && m_tcpStateCheck->isChecked());
    m_redirectButton->setEnabled(m_redirectCheck->isChecked());
}

int AccessEditDialog::protocolNumber() const
{
    const int data = m_protocolCombo->currentData().toInt();
    return data == -1 ? m_protocolNumberEdit->text().toInt() : data;
}

void AccessEditDialog::setPriority(int priority)
{
    m_prioritySpin->setValue(priority);
}

void AccessEditDialog::setValues(const QJsonObject &a)
{
    m_id = static_cast<quint32>(a.value("Id_u32").toDouble());
    m_active = a.value("Active_bool").toBool(true);
    m_noteEdit->setText(a.value("Note_utf").toString());
    (a.value("Discard_bool").toBool() ? m_discardRadio : m_passRadio)->setChecked(true);
    m_prioritySpin->setValue(qMax(1, a.value("Priority_u32").toInt(1000)));

    m_srcUserEdit->setText(a.value("SrcUsername_str").toString());
    m_dstUserEdit->setText(a.value("DestUsername_str").toString());

    // MAC
    const bool checkSrcMac = a.value("CheckSrcMac_bool").toBool();
    m_srcMacAllCheck->setChecked(!checkSrcMac);
    if (checkSrcMac) {
        m_srcMacEdit->setText(macText(decodeBin(a.value("SrcMacAddress_bin"))));
        m_srcMacMaskEdit->setText(macText(decodeBin(a.value("SrcMacMask_bin"))));
    }
    const bool checkDstMac = a.value("CheckDstMac_bool").toBool();
    m_dstMacAllCheck->setChecked(!checkDstMac);
    if (checkDstMac) {
        m_dstMacEdit->setText(macText(decodeBin(a.value("DstMacAddress_bin"))));
        m_dstMacMaskEdit->setText(macText(decodeBin(a.value("DstMacMask_bin"))));
    }

    // IP
    if (m_ipv6) {
        const QByteArray srcMask = decodeBin(a.value("SrcSubnetMask6_bin"));
        const QByteArray dstMask = decodeBin(a.value("DestSubnetMask6_bin"));
        const bool srcAll = srcMask.size() != 16 || srcMask == QByteArray(16, 0);
        const bool dstAll = dstMask.size() != 16 || dstMask == QByteArray(16, 0);
        m_srcAllCheck->setChecked(srcAll);
        m_dstAllCheck->setChecked(dstAll);
        if (!srcAll) {
            m_srcIpEdit->setText(ipv6Text(decodeBin(a.value("SrcIpAddress6_bin"))));
            m_srcMaskEdit->setText(maskForDisplay(srcMask, true));
        }
        if (!dstAll) {
            m_dstIpEdit->setText(ipv6Text(decodeBin(a.value("DestIpAddress6_bin"))));
            m_dstMaskEdit->setText(maskForDisplay(dstMask, true));
        }
    } else {
        const QString srcMask = a.value("SrcSubnetMask_ip").toString();
        const QString dstMask = a.value("DestSubnetMask_ip").toString();
        const bool srcAll = srcMask.isEmpty() || srcMask == QStringLiteral("0.0.0.0");
        const bool dstAll = dstMask.isEmpty() || dstMask == QStringLiteral("0.0.0.0");
        m_srcAllCheck->setChecked(srcAll);
        m_dstAllCheck->setChecked(dstAll);
        if (!srcAll) {
            m_srcIpEdit->setText(a.value("SrcIpAddress_ip").toString());
            m_srcMaskEdit->setText(srcMask);
        }
        if (!dstAll) {
            m_dstIpEdit->setText(a.value("DestIpAddress_ip").toString());
            m_dstMaskEdit->setText(dstMask);
        }
    }

    const int protocol = a.value("Protocol_u32").toInt();
    const int index = m_protocolCombo->findData(protocol);
    if (index >= 0) {
        m_protocolCombo->setCurrentIndex(index);
    } else {
        m_protocolCombo->setCurrentIndex(m_protocolCombo->findData(-1));
        m_protocolNumberEdit->setText(QString::number(protocol));
    }

    const int srcStart = a.value("SrcPortStart_u32").toInt();
    const int srcEnd = a.value("SrcPortEnd_u32").toInt();
    if (srcStart != 0 || srcEnd != 0) {
        m_srcPortMinEdit->setText(QString::number(srcStart));
        if (srcEnd != srcStart) {
            m_srcPortMaxEdit->setText(QString::number(srcEnd));
        }
    }
    const int dstStart = a.value("DestPortStart_u32").toInt();
    const int dstEnd = a.value("DestPortEnd_u32").toInt();
    if (dstStart != 0 || dstEnd != 0) {
        m_dstPortMinEdit->setText(QString::number(dstStart));
        if (dstEnd != dstStart) {
            m_dstPortMaxEdit->setText(QString::number(dstEnd));
        }
    }

    m_tcpStateCheck->setChecked(a.value("CheckTcpState_bool").toBool());
    (a.value("Established_bool").toBool() ? m_establishedRadio : m_unestablishedRadio)->setChecked(true);

    m_redirectUrl = a.value("RedirectUrl_str").toString();
    m_redirectCheck->setChecked(!m_redirectUrl.isEmpty());
    m_delay = a.value("Delay_u32").toInt();
    m_jitter = a.value("Jitter_u32").toInt();
    m_loss = a.value("Loss_u32").toInt();
    updateState();
}

QJsonObject AccessEditDialog::toRpcParams() const
{
    QJsonObject p;
    p["Id_u32"] = static_cast<qint64>(m_id);
    p["Note_utf"] = m_noteEdit->text();
    p["Active_bool"] = m_active;
    p["Priority_u32"] = m_prioritySpin->value();
    p["Discard_bool"] = m_discardRadio->isChecked();
    p["IsIPv6_bool"] = m_ipv6;

    if (m_ipv6) {
        QByteArray mask;
        QHostAddress address;
        const bool srcAll = m_srcAllCheck->isChecked();
        if (!srcAll && parseMask(m_srcMaskEdit->text(), true, &mask) && address.setAddress(m_srcIpEdit->text().trimmed())) {
            p["SrcIpAddress6_bin"] = encodeBin(ipv6Bytes(address));
            p["SrcSubnetMask6_bin"] = encodeBin(mask);
        } else {
            p["SrcIpAddress6_bin"] = encodeBin(QByteArray(16, 0));
            p["SrcSubnetMask6_bin"] = encodeBin(QByteArray(16, 0));
        }
        const bool dstAll = m_dstAllCheck->isChecked();
        if (!dstAll && parseMask(m_dstMaskEdit->text(), true, &mask) && address.setAddress(m_dstIpEdit->text().trimmed())) {
            p["DestIpAddress6_bin"] = encodeBin(ipv6Bytes(address));
            p["DestSubnetMask6_bin"] = encodeBin(mask);
        } else {
            p["DestIpAddress6_bin"] = encodeBin(QByteArray(16, 0));
            p["DestSubnetMask6_bin"] = encodeBin(QByteArray(16, 0));
        }
    } else {
        QByteArray mask;
        if (!m_srcAllCheck->isChecked() && parseMask(m_srcMaskEdit->text(), false, &mask)) {
            p["SrcIpAddress_ip"] = m_srcIpEdit->text().trimmed();
            p["SrcSubnetMask_ip"] = ipv4MaskText(mask);
        } else {
            p["SrcIpAddress_ip"] = QStringLiteral("0.0.0.0");
            p["SrcSubnetMask_ip"] = QStringLiteral("0.0.0.0");
        }
        if (!m_dstAllCheck->isChecked() && parseMask(m_dstMaskEdit->text(), false, &mask)) {
            p["DestIpAddress_ip"] = m_dstIpEdit->text().trimmed();
            p["DestSubnetMask_ip"] = ipv4MaskText(mask);
        } else {
            p["DestIpAddress_ip"] = QStringLiteral("0.0.0.0");
            p["DestSubnetMask_ip"] = QStringLiteral("0.0.0.0");
        }
    }

    const int protocol = protocolNumber();
    p["Protocol_u32"] = protocol;
    const bool ports = protocol == 6 || protocol == 17;
    const auto portValue = [](const QLineEdit *edit) { return edit->text().isEmpty() ? -1 : edit->text().toInt(); };
    int srcStart = ports ? portValue(m_srcPortMinEdit) : -1;
    int srcEnd = ports ? portValue(m_srcPortMaxEdit) : -1;
    int dstStart = ports ? portValue(m_dstPortMinEdit) : -1;
    int dstEnd = ports ? portValue(m_dstPortMaxEdit) : -1;
    if (srcStart < 0) {
        srcStart = srcEnd = 0;
    } else if (srcEnd < 0) {
        srcEnd = srcStart;
    }
    if (dstStart < 0) {
        dstStart = dstEnd = 0;
    } else if (dstEnd < 0) {
        dstEnd = dstStart;
    }
    p["SrcPortStart_u32"] = srcStart;
    p["SrcPortEnd_u32"] = srcEnd;
    p["DestPortStart_u32"] = dstStart;
    p["DestPortEnd_u32"] = dstEnd;

    p["SrcUsername_str"] = m_srcUserEdit->text().trimmed();
    p["DestUsername_str"] = m_dstUserEdit->text().trimmed();

    QByteArray mac;
    QByteArray macMask;
    const bool checkSrcMac = !m_srcMacAllCheck->isChecked() && parseMac(m_srcMacEdit->text(), &mac) &&
                             parseMac(m_srcMacMaskEdit->text(), &macMask);
    p["CheckSrcMac_bool"] = checkSrcMac;
    p["SrcMacAddress_bin"] = encodeBin(checkSrcMac ? mac : QByteArray(6, 0));
    p["SrcMacMask_bin"] = encodeBin(checkSrcMac ? macMask : QByteArray(6, 0));
    const bool checkDstMac = !m_dstMacAllCheck->isChecked() && parseMac(m_dstMacEdit->text(), &mac) &&
                             parseMac(m_dstMacMaskEdit->text(), &macMask);
    p["CheckDstMac_bool"] = checkDstMac;
    p["DstMacAddress_bin"] = encodeBin(checkDstMac ? mac : QByteArray(6, 0));
    p["DstMacMask_bin"] = encodeBin(checkDstMac ? macMask : QByteArray(6, 0));

    const bool checkTcp = protocol == 6 && m_tcpStateCheck->isChecked();
    p["CheckTcpState_bool"] = checkTcp;
    p["Established_bool"] = checkTcp && m_establishedRadio->isChecked();

    p["Delay_u32"] = m_delay;
    p["Jitter_u32"] = m_jitter;
    p["Loss_u32"] = m_loss;
    p["RedirectUrl_str"] = m_redirectCheck->isChecked() ? m_redirectUrl : QString();
    return p;
}

void AccessEditDialog::accept()
{
    const auto fail = [this](const QString &message) {
        QMessageBox::warning(this, tr("入力エラー"), message);
    };

    // IP アドレス / マスク
    const struct {
        QCheckBox *all;
        QLineEdit *ip;
        QLineEdit *mask;
        const char *label;
    } ipRows[] = {{m_srcAllCheck, m_srcIpEdit, m_srcMaskEdit, "送信元"}, {m_dstAllCheck, m_dstIpEdit, m_dstMaskEdit, "宛先"}};
    for (const auto &row : ipRows) {
        if (row.all->isChecked()) {
            continue;
        }
        QHostAddress address;
        QByteArray mask;
        const auto expected = m_ipv6 ? QAbstractSocket::IPv6Protocol : QAbstractSocket::IPv4Protocol;
        if (!address.setAddress(row.ip->text().trimmed()) || address.protocol() != expected) {
            fail(tr("%1の IP アドレスが正しくありません。").arg(tr(row.label)));
            return;
        }
        if (!parseMask(row.mask->text(), m_ipv6, &mask)) {
            fail(tr("%1のマスクが正しくありません。").arg(tr(row.label)));
            return;
        }
    }

    // MAC アドレス
    const struct {
        QCheckBox *all;
        QLineEdit *mac;
        QLineEdit *mask;
        const char *label;
    } macRows[] = {{m_srcMacAllCheck, m_srcMacEdit, m_srcMacMaskEdit, "送信元"},
                   {m_dstMacAllCheck, m_dstMacEdit, m_dstMacMaskEdit, "宛先"}};
    for (const auto &row : macRows) {
        QByteArray bytes;
        if (!row.all->isChecked() && (!parseMac(row.mac->text(), &bytes) || !parseMac(row.mask->text(), &bytes))) {
            fail(tr("%1の MAC アドレスまたはマスクが正しくありません。").arg(tr(row.label)));
            return;
        }
    }

    // プロトコル番号 / ポート
    if (m_protocolCombo->currentData().toInt() == -1 && m_protocolNumberEdit->text().isEmpty()) {
        fail(tr("IP プロトコル番号を入力してください。"));
        return;
    }
    const struct {
        QLineEdit *min;
        QLineEdit *max;
    } portRows[] = {{m_srcPortMinEdit, m_srcPortMaxEdit}, {m_dstPortMinEdit, m_dstPortMaxEdit}};
    for (const auto &row : portRows) {
        if (!row.min->isEnabled()) {
            continue;
        }
        if (row.min->text().isEmpty() && !row.max->text().isEmpty()) {
            fail(tr("ポート番号の最小値を入力してください。"));
            return;
        }
        if (!row.max->text().isEmpty() && row.max->text().toInt() < row.min->text().toInt()) {
            fail(tr("ポート番号の最大値は最小値以上の値を指定してください。"));
            return;
        }
    }

    if (m_redirectCheck->isChecked() && m_redirectUrl.trimmed().isEmpty()) {
        fail(tr("リダイレクト先 URL を指定してください。"));
        return;
    }
    QDialog::accept();
}

void AccessEditDialog::pickUser(QLineEdit *target)
{
    UserListDialog dialog(m_rpc, m_hubName, this, QString(), /*selectMode=*/true);
    if (dialog.exec() == QDialog::Accepted) {
        target->setText(dialog.pickedName());
    }
}

void AccessEditDialog::onPickSrcUser()
{
    pickUser(m_srcUserEdit);
}

void AccessEditDialog::onPickDstUser()
{
    pickUser(m_dstUserEdit);
}

// D_SM_REDIRECT
void AccessEditDialog::onRedirect()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("HTTP URL リダイレクション設定"));
    auto *urlEdit = new QLineEdit(m_redirectUrl, &dialog);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(note(
        tr("仮想 HUB を経由する TCP コネクションがこのアクセスリストの条件に一致した場合、その TCP コネクションを用いてクライアントが何らかの通信を行おうとすると、強制的に以下に設定された URL 文字列をクライアントに対して応答します。\n\nこれにより、VPN クライアント上で起動している Web ブラウザが特定の IP アドレスにアクセスした場合などに任意の Web ページをその Web ブラウザ上に表示させることができます。"),
        &dialog));
    layout->addWidget(new QLabel(tr("リダイレクト先 URL の入力:"), &dialog));
    layout->addWidget(new QLabel(tr("リダイレクト先 URL(U):"), &dialog));
    layout->addWidget(urlEdit);
    layout->addWidget(new QLabel(tr("入力例: http://www.example.com/"), &dialog));
    layout->addWidget(separator(&dialog));
    auto *caution = new QLabel(tr("ご注意"), &dialog);
    QFont bold = caution->font();
    bold.setBold(true);
    caution->setFont(bold);
    layout->addWidget(caution);
    layout->addWidget(note(tr("この機能は TCP/IP に詳しいネットワーク管理者向けの機能です。以下の注意事項をよく読み、慎重に設定してください。"), &dialog));
    layout->addWidget(note(tr("・アクセスリストの条件に宛先セッションのユーザー名またはグループ名が指定されている場合で当該アクセスリストがパケットに一致した場合はこの機能は無視されます。"), &dialog));
    layout->addWidget(note(tr("・TCP 以外のパケットがアクセスリストに一致した場合はこの機能は無視されます。"), &dialog));
    layout->addWidget(note(tr("・すべての TCP パケットに対して HTTP リダイレクト応答を返します (ポート 80 に限定されません)。たとえばポート 80 に限定するためには、アクセスリストの条件で宛先ポートを TCP 80 に限定してください。"), &dialog));
    layout->addWidget(buttonBox);
    dialog.resize(560, 480);

    if (dialog.exec() == QDialog::Accepted) {
        m_redirectUrl = urlEdit->text().trimmed();
    }
}

// D_SM_SIMULATION
void AccessEditDialog::onSimulation()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("遅延・パケットロス生成機能"));

    auto *delayCheck = new QCheckBox(tr("遅延を発生させる(&D)"), &dialog);
    auto *delaySpin = new QSpinBox(&dialog);
    delaySpin->setRange(1, 10000);
    delaySpin->setSuffix(tr(" ミリ秒 (msecs)"));
    auto *jitterCheck = new QCheckBox(tr("遅延にジッタ (揺らぎ) を発生させる(&J)"), &dialog);
    auto *jitterSpin = new QSpinBox(&dialog);
    jitterSpin->setRange(1, 100);
    jitterSpin->setSuffix(tr(" パーセント (%)"));
    auto *lossCheck = new QCheckBox(tr("パケットロスを発生させる(&L)"), &dialog);
    auto *lossSpin = new QSpinBox(&dialog);
    lossSpin->setRange(1, 100);
    lossSpin->setSuffix(tr(" パーセント (%)"));

    delayCheck->setChecked(m_delay > 0);
    delaySpin->setValue(m_delay > 0 ? m_delay : 100);
    jitterCheck->setChecked(m_jitter > 0);
    jitterSpin->setValue(m_jitter > 0 ? m_jitter : 10);
    lossCheck->setChecked(m_loss > 0);
    lossSpin->setValue(m_loss > 0 ? m_loss : 10);
    const auto sync = [=]() {
        delaySpin->setEnabled(delayCheck->isChecked());
        // ジッタは遅延を発生させる場合のみ意味がある
        jitterCheck->setEnabled(delayCheck->isChecked());
        jitterSpin->setEnabled(delayCheck->isChecked() && jitterCheck->isChecked());
        lossSpin->setEnabled(lossCheck->isChecked());
    };
    connect(delayCheck, &QCheckBox::toggled, &dialog, sync);
    connect(jitterCheck, &QCheckBox::toggled, &dialog, sync);
    connect(lossCheck, &QCheckBox::toggled, &dialog, sync);
    sync();

    auto *contentGroup = new QGroupBox(tr("発生させる遅延・ジッタ・パケットロスの内容:"), &dialog);
    auto *grid = new QGridLayout(contentGroup);
    grid->addWidget(delayCheck, 0, 0, 1, 2);
    grid->addWidget(new QLabel(tr("発生させる遅延の量 (0 - 10000) :"), &dialog), 1, 0, Qt::AlignRight);
    grid->addWidget(delaySpin, 1, 1);
    grid->addWidget(jitterCheck, 2, 0, 1, 2);
    grid->addWidget(new QLabel(tr("発生させる遅延の揺らぎ (0 - 100) :"), &dialog), 3, 0, Qt::AlignRight);
    grid->addWidget(jitterSpin, 3, 1);
    grid->addWidget(lossCheck, 4, 0, 1, 2);
    grid->addWidget(new QLabel(tr("発生させるパケットロス率 (0 - 100) :"), &dialog), 5, 0, Qt::AlignRight);
    grid->addWidget(lossSpin, 5, 1);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(note(
        tr("このアクセスリストの条件に一致するパケットが仮想 HUB を通過する場合に、そのパケットに遅延・ジッタおよびパケットロスを発生させることができます。\n\nこの機能により、低速で品質の低いインターネット回線や WAN 回線、ワイヤレス回線などを利用した場合の動作を、LAN 内の机上で実験することができます。たとえば、IP 電話 (VoIP) 等の動作試験に便利です。"),
        &dialog));
    layout->addWidget(contentGroup);
    layout->addWidget(buttonBox);
    dialog.resize(520, 440);

    if (dialog.exec() == QDialog::Accepted) {
        m_delay = delayCheck->isChecked() ? delaySpin->value() : 0;
        m_jitter = (delayCheck->isChecked() && jitterCheck->isChecked()) ? jitterSpin->value() : 0;
        m_loss = lossCheck->isChecked() ? lossSpin->value() : 0;
    }
}

QString AccessEditDialog::describe(const QJsonObject &a)
{
    const bool ipv6 = a.value("IsIPv6_bool").toBool();
    QStringList parts;

    if (ipv6) {
        const QByteArray srcMask = decodeBin(a.value("SrcSubnetMask6_bin"));
        if (srcMask.size() == 16 && srcMask != QByteArray(16, 0)) {
            parts << QStringLiteral("SrcIPv6=%1%2").arg(ipv6Text(decodeBin(a.value("SrcIpAddress6_bin"))), maskForDisplay(srcMask, true));
        }
        const QByteArray dstMask = decodeBin(a.value("DestSubnetMask6_bin"));
        if (dstMask.size() == 16 && dstMask != QByteArray(16, 0)) {
            parts << QStringLiteral("DstIPv6=%1%2").arg(ipv6Text(decodeBin(a.value("DestIpAddress6_bin"))), maskForDisplay(dstMask, true));
        }
    } else {
        const QString srcMask = a.value("SrcSubnetMask_ip").toString();
        if (!srcMask.isEmpty() && srcMask != QStringLiteral("0.0.0.0")) {
            QByteArray bytes;
            parseMask(srcMask, false, &bytes);
            parts << QStringLiteral("SrcIPv4=%1%2").arg(a.value("SrcIpAddress_ip").toString(), maskForDisplay(bytes, false));
        }
        const QString dstMask = a.value("DestSubnetMask_ip").toString();
        if (!dstMask.isEmpty() && dstMask != QStringLiteral("0.0.0.0")) {
            QByteArray bytes;
            parseMask(dstMask, false, &bytes);
            parts << QStringLiteral("DstIPv4=%1%2").arg(a.value("DestIpAddress_ip").toString(), maskForDisplay(bytes, false));
        }
    }

    const int protocol = a.value("Protocol_u32").toInt();
    if (protocol != 0) {
        parts << QStringLiteral("Protocol=%1").arg(protocolDescription(protocol));
    }
    const int srcStart = a.value("SrcPortStart_u32").toInt();
    const int srcEnd = a.value("SrcPortEnd_u32").toInt();
    if (srcStart != 0 || srcEnd != 0) {
        parts << QStringLiteral("SrcPort=%1").arg(portDescription(srcStart, srcEnd));
    }
    const int dstStart = a.value("DestPortStart_u32").toInt();
    const int dstEnd = a.value("DestPortEnd_u32").toInt();
    if (dstStart != 0 || dstEnd != 0) {
        parts << QStringLiteral("DstPort=%1").arg(portDescription(dstStart, dstEnd));
    }
    if (!a.value("SrcUsername_str").toString().isEmpty()) {
        parts << QStringLiteral("SrcUsername=%1").arg(a.value("SrcUsername_str").toString());
    }
    if (!a.value("DestUsername_str").toString().isEmpty()) {
        parts << QStringLiteral("DstUsername=%1").arg(a.value("DestUsername_str").toString());
    }
    if (a.value("CheckSrcMac_bool").toBool()) {
        parts << QStringLiteral("SrcMac=%1/%2")
                     .arg(macText(decodeBin(a.value("SrcMacAddress_bin"))), macText(decodeBin(a.value("SrcMacMask_bin"))));
    }
    if (a.value("CheckDstMac_bool").toBool()) {
        parts << QStringLiteral("DstMac=%1/%2")
                     .arg(macText(decodeBin(a.value("DstMacAddress_bin"))), macText(decodeBin(a.value("DstMacMask_bin"))));
    }
    if (a.value("CheckTcpState_bool").toBool()) {
        parts << QStringLiteral("TCPState=%1").arg(a.value("Established_bool").toBool() ? "Established" : "Unestablished");
    }
    if (a.value("Delay_u32").toInt() > 0) {
        parts << QStringLiteral("Delay=%1").arg(a.value("Delay_u32").toInt());
    }
    if (a.value("Jitter_u32").toInt() > 0) {
        parts << QStringLiteral("Jitter=%1").arg(a.value("Jitter_u32").toInt());
    }
    if (a.value("Loss_u32").toInt() > 0) {
        parts << QStringLiteral("Loss=%1").arg(a.value("Loss_u32").toInt());
    }
    if (!a.value("RedirectUrl_str").toString().isEmpty()) {
        parts << QStringLiteral("RedirectUrl=%1").arg(a.value("RedirectUrl_str").toString());
    }
    return QStringLiteral("(%1) %2").arg(ipv6 ? "ipv6" : "ipv4", parts.join(QStringLiteral(", ")));
}
