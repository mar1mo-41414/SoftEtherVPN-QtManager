#pragma once

#include <QByteArray>
#include <QDialog>

// 公式Manager「証明書」(D_CERT) 相当の読み取り専用ダイアログ。
// derBytesはX.509証明書のDERバイナリ。
class CertInfoDialog : public QDialog
{
    Q_OBJECT

public:
    CertInfoDialog(const QByteArray &derBytes, QWidget *parent = nullptr);
};
