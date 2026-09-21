#include "CertInfoDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSslCertificate>
#include <QVBoxLayout>

CertInfoDialog::CertInfoDialog(const QByteArray &derBytes, QWidget *parent)
    : QDialog(parent)
{
    // D_CERT CAPTION
    setWindowTitle(tr("証明書"));

    const QSslCertificate cert(derBytes, QSsl::Der);

    auto *titleLabel = new QLabel(tr("この証明書に関する情報は以下の通りです。"), this);
    titleLabel->setWordWrap(true);

    auto *form = new QFormLayout;
    // STATIC2/3/4
    form->addRow(tr("発行先:"), new QLabel(cert.subjectDisplayName(), this));
    form->addRow(tr("発行者:"), new QLabel(cert.issuerDisplayName(), this));
    form->addRow(tr("有効期限:"),
                 new QLabel(tr("%1 〜 %2")
                                .arg(cert.effectiveDate().toString(Qt::ISODate), cert.expiryDate().toString(Qt::ISODate)),
                            this));

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonBox->button(QDialogButtonBox::Close)->setText(tr("閉じる(&X)"));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addLayout(form);
    layout->addWidget(buttonBox);
}
