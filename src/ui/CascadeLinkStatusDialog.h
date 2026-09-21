#pragma once

#include <QDialog>
#include <QJsonObject>

// 公式Manager「カスケード接続の状態」(GetLinkStatus) 相当の読み取り専用ダイアログ。
class CascadeLinkStatusDialog : public QDialog
{
    Q_OBJECT

public:
    CascadeLinkStatusDialog(const QString &accountName, const QJsonObject &status, QWidget *parent = nullptr);
};
