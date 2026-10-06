#include "HubCrlDialog.h"

#include "util/RpcUiHelpers.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSslCertificate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

HubCrlDialog::HubCrlDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_CRL CAPTION
    setWindowTitle(tr("無効な証明書の一覧"));

    auto *title = new QLabel(
        tr("この仮想 HUB 内で無効な証明書の一覧を管理します。\n\n無効な証明書の一覧に証明書を登録すると、その証明書を提示したクライアントは、この仮想 HUB に証明書認証モードで接続できなくなります。"),
        this);
    title->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(1);
    m_table->setHorizontalHeaderLabels({tr("証明書の概要")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &HubCrlDialog::updateButtons);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &HubCrlDialog::onEdit);

    auto *addButton = new QPushButton(tr("追加(&A)"), this);
    m_deleteButton = new QPushButton(tr("削除(&D)"), this);
    m_editButton = new QPushButton(tr("編集(&E)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&C)"), this);
    connect(addButton, &QPushButton::clicked, this, &HubCrlDialog::onAdd);
    connect(m_deleteButton, &QPushButton::clicked, this, &HubCrlDialog::onDelete);
    connect(m_editButton, &QPushButton::clicked, this, &HubCrlDialog::onEdit);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(addButton);
    buttons->addWidget(m_deleteButton);
    buttons->addWidget(m_editButton);
    buttons->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(m_table, 1);
    layout->addLayout(buttons);
    resize(640, 440);
    updateButtons();
    reload();
}

int HubCrlDialog::selectedRow() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    return selected.isEmpty() ? -1 : selected.first()->row();
}

void HubCrlDialog::updateButtons()
{
    const bool selected = selectedRow() >= 0;
    m_editButton->setEnabled(selected);
    m_deleteButton->setEnabled(selected);
}

void HubCrlDialog::reload()
{
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    m_rpc->call(
        QStringLiteral("EnumCrl"), params,
        [this](const QJsonObject &result) {
            const QJsonArray list = result.value("CRLList").toArray();
            m_table->setRowCount(list.size());
            for (int row = 0; row < list.size(); ++row) {
                const QJsonObject crl = list.at(row).toObject();
                auto *item = new QTableWidgetItem(crl.value("CrlInfo_utf").toString());
                item->setData(Qt::UserRole, crl.value("Key_u32").toDouble());
                m_table->setItem(row, 0, item);
            }
            updateButtons();
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("無効な証明書の一覧の取得"), error); });
}

bool HubCrlDialog::editEntry(QJsonObject *entry)
{
    QDialog dialog(this);
    // D_SM_EDIT_CRL CAPTION
    dialog.setWindowTitle(tr("無効な証明書"));

    struct Field {
        const char *label;
        const char *key;
        QCheckBox *check = nullptr;
        QLineEdit *edit = nullptr;
    };
    QList<Field> fields = {{"名前 (CN):", "CommonName_utf"},   {"所属機関 (O):", "Organization_utf"},
                           {"組織単位 (OU):", "Unit_utf"},      {"国 (C):", "Country_utf"},
                           {"都道府県 (ST):", "State_utf"},    {"ローカル (L):", "Local_utf"}};
    struct HexField {
        const char *label;
        const char *key;
        int digits;
        QCheckBox *check = nullptr;
        QLineEdit *edit = nullptr;
    };
    QList<HexField> hexFields = {{"シリアル番号 (16進数):", "Serial_bin", 0},
                                 {"MD5 ダイジェスト値 (16進数, 128 bit):", "DigestMD5_bin", 32},
                                 {"SHA-1 ダイジェスト値 (16進数, 160 bit):", "DigestSHA1_bin", 40}};

    auto *contentGroup = new QGroupBox(tr("証明書の内容"), &dialog);
    auto *contentGrid = new QGridLayout(contentGroup);
    int row = 0;
    for (Field &field : fields) {
        field.check = new QCheckBox(QCoreApplication::translate("HubCrlDialog", field.label), &dialog);
        field.edit = new QLineEdit(&dialog);
        const QString value = entry->value(QLatin1String(field.key)).toString();
        field.check->setChecked(!value.isEmpty());
        field.edit->setText(value);
        field.edit->setEnabled(!value.isEmpty());
        QObject::connect(field.check, &QCheckBox::toggled, field.edit, &QLineEdit::setEnabled);
        contentGrid->addWidget(field.check, row, 0);
        contentGrid->addWidget(field.edit, row, 1);
        ++row;
    }
    contentGrid->setColumnStretch(1, 1);

    auto *attrGroup = new QGroupBox(tr("証明書の属性値:"), &dialog);
    auto *attrGrid = new QGridLayout(attrGroup);
    row = 0;
    for (HexField &field : hexFields) {
        field.check = new QCheckBox(QCoreApplication::translate("HubCrlDialog", field.label), &dialog);
        field.edit = new QLineEdit(&dialog);
        const QByteArray bytes = QByteArray::fromBase64(entry->value(QLatin1String(field.key)).toString().toLatin1());
        field.check->setChecked(!bytes.isEmpty());
        field.edit->setText(QString::fromLatin1(bytes.toHex().toUpper()));
        field.edit->setEnabled(!bytes.isEmpty());
        QObject::connect(field.check, &QCheckBox::toggled, field.edit, &QLineEdit::setEnabled);
        attrGrid->addWidget(field.check, row, 0);
        attrGrid->addWidget(field.edit, row, 1);
        ++row;
    }
    attrGrid->setColumnStretch(1, 1);
    auto *attrNote = new QLabel(
        tr("ダイジェスト値 (ハッシュ値) の指定は、証明書を事実上一意に指定することになります。通常、MD5 または SHA-1 のダイジェスト値を入力する場合は、その他の項目を入力する必要はありません。"),
        &dialog);
    attrNote->setWordWrap(true);

    // 既存の証明書ファイルからの指定
    auto *loadButton = new QPushButton(tr("証明書の読み込み(&L)..."), &dialog);
    QObject::connect(loadButton, &QPushButton::clicked, &dialog, [&]() {
        const QString path = QFileDialog::getOpenFileName(
            &dialog, tr("証明書の読み込み"), QString(), tr("X.509 証明書ファイル (*.cer *.crt *.pem *.der);;すべてのファイル (*)"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return;
        }
        const QByteArray data = file.readAll();
        QSslCertificate cert(data, QSsl::Pem);
        if (cert.isNull()) {
            cert = QSslCertificate(data, QSsl::Der);
        }
        if (cert.isNull()) {
            QMessageBox::warning(&dialog, tr("エラー"), tr("X.509 証明書として読み込めませんでした。"));
            return;
        }
        const auto setField = [](Field &field, const QString &value) {
            field.check->setChecked(!value.isEmpty());
            field.edit->setText(value);
        };
        const auto info = [&](QSslCertificate::SubjectInfo what) { return cert.subjectInfo(what).value(0); };
        setField(fields[0], info(QSslCertificate::CommonName));
        setField(fields[1], info(QSslCertificate::Organization));
        setField(fields[2], info(QSslCertificate::OrganizationalUnitName));
        setField(fields[3], info(QSslCertificate::CountryName));
        setField(fields[4], info(QSslCertificate::StateOrProvinceName));
        setField(fields[5], info(QSslCertificate::LocalityName));
        const QByteArray der = cert.toDer();
        const QString serial = QString::fromLatin1(cert.serialNumber()).remove(QLatin1Char(':')).toUpper();
        hexFields[0].check->setChecked(true);
        hexFields[0].edit->setText(serial);
        hexFields[1].check->setChecked(true);
        hexFields[1].edit->setText(QString::fromLatin1(QCryptographicHash::hash(der, QCryptographicHash::Md5).toHex().toUpper()));
        hexFields[2].check->setChecked(true);
        hexFields[2].edit->setText(QString::fromLatin1(QCryptographicHash::hash(der, QCryptographicHash::Sha1).toHex().toUpper()));
    });
    auto *fileGroup = new QGroupBox(tr("既存の証明書ファイルからの指定"), &dialog);
    auto *fileLayout = new QHBoxLayout(fileGroup);
    auto *fileNote = new QLabel(
        tr("無効にしたい証明書のファイルがある場合は、そのファイルを指定することにより証明書を正確に指定して無効リストに追加することができます。[証明書の読み込み] をクリックして指定した証明書ファイルの内容が自動的に入力されます。"),
        &dialog);
    fileNote->setWordWrap(true);
    fileLayout->addWidget(fileNote, 1);
    fileLayout->addWidget(loadButton);

    auto *header = new QLabel(
        tr("無効な証明書の一覧に登録する内容を設定します。\n\n仮想 HUB にユーザーが証明書認証モードで接続してきたとき、その証明書が無効な証明書の一覧に登録されている 1 つ以上の内容に一致する場合に、そのユーザーの接続を拒否します。"),
        &dialog);
    header->setWordWrap(true);
    auto *bold = new QLabel(tr("下記の定義された項目すべての内容に一致する証明書を無効とします。"), &dialog);
    QFont boldFont = bold->font();
    boldFont.setBold(true);
    bold->setFont(boldFont);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, [&]() {
        static const QRegularExpression hex(QStringLiteral("^[0-9A-Fa-f]*$"));
        for (const HexField &field : hexFields) {
            if (!field.check->isChecked()) {
                continue;
            }
            const QString text = field.edit->text().remove(QLatin1Char(' '));
            const bool lengthOk = field.digits == 0 ? (text.size() % 2 == 0 && !text.isEmpty()) : text.size() == field.digits;
            if (!hex.match(text).hasMatch() || !lengthOk) {
                QMessageBox::warning(&dialog, tr("入力エラー"), tr("16 進数の値が正しくありません。"));
                return;
            }
        }
        dialog.accept();
    });
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(header);
    layout->addWidget(bold);
    layout->addWidget(contentGroup);
    layout->addWidget(attrGroup);
    layout->addWidget(attrNote);
    layout->addWidget(fileGroup);
    layout->addWidget(buttonBox);
    dialog.resize(640, 620);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }
    bool anyChecked = false;
    for (const Field &field : fields) {
        (*entry)[QLatin1String(field.key)] = field.check->isChecked() ? field.edit->text() : QString();
        anyChecked = anyChecked || field.check->isChecked();
    }
    for (const HexField &field : hexFields) {
        const bool on = field.check->isChecked();
        (*entry)[QLatin1String(field.key)] =
            on ? QString::fromLatin1(QByteArray::fromHex(field.edit->text().remove(QLatin1Char(' ')).toLatin1()).toBase64()) : QString();
        anyChecked = anyChecked || on;
    }
    if (!anyChecked) {
        // SM_CRL_EMPTY_MSG
        QMessageBox confirmBox(QMessageBox::Warning, tr("確認"),
                                tr("項目が 1 つも選択されていません。\nこの無効な証明書エントリが追加されると、すべての証明書が無効として判断され、証明書認証モードで接続しようとするすべてのクライアントの接続が拒否されます。\n\nよろしいですか?"),
                                QMessageBox::NoButton, this);
        QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
        confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
        confirmBox.exec();
        return confirmBox.clickedButton() == yesButton;
    }
    return true;
}

void HubCrlDialog::onAdd()
{
    QJsonObject entry;
    if (!editEntry(&entry)) {
        return;
    }
    entry["HubName_str"] = m_hubName;
    m_rpc->call(
        QStringLiteral("AddCrl"), entry, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("無効な証明書の追加"), error); });
}

void HubCrlDialog::onEdit()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    const double key = m_table->item(row, 0)->data(Qt::UserRole).toDouble();
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    params["Key_u32"] = key;
    m_rpc->call(
        QStringLiteral("GetCrl"), params,
        [this, key](const QJsonObject &crl) {
            QJsonObject entry = crl;
            if (!editEntry(&entry)) {
                return;
            }
            entry["HubName_str"] = m_hubName;
            entry["Key_u32"] = key;
            m_rpc->call(
                QStringLiteral("SetCrl"), entry, [this](const QJsonObject &) { reload(); },
                [this](const RpcError &error) { RpcUi::showError(this, tr("無効な証明書の更新"), error); });
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("無効な証明書の取得"), error); });
}

void HubCrlDialog::onDelete()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }
    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"), tr("選択した項目を削除します。よろしいですか?"), QMessageBox::NoButton,
                            this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    params["Key_u32"] = m_table->item(row, 0)->data(Qt::UserRole).toDouble();
    m_rpc->call(
        QStringLiteral("DelCrl"), params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("無効な証明書の削除"), error); });
}
