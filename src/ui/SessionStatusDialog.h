#pragma once

#include <QDialog>
#include <QJsonObject>

// 公式Manager「VPN セッション "%S" の状況」(GetSessionStatus) 相当の読み取り専用ダイアログ。
class SessionStatusDialog : public QDialog
{
    Q_OBJECT

public:
    SessionStatusDialog(const QString &sessionName, const QJsonObject &status, QWidget *parent = nullptr);
};
