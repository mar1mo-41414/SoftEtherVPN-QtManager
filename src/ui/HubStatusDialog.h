#pragma once

#include <QDialog>
#include <QJsonObject>

// 公式Manager「仮想HUB "%s" の状態」(D_SM_STATUS + GetHubStatus) 相当の読み取り専用ダイアログ。
class HubStatusDialog : public QDialog
{
    Q_OBJECT

public:
    HubStatusDialog(const QString &hubName, const QJsonObject &status, QWidget *parent = nullptr);
};
