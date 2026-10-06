#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

// 公式Manager「認証サーバーの設定」(D_SM_RADIUS) 相当。ユーザー認証に使う RADIUS サーバー。
class HubRadiusDialog : public QDialog
{
    Q_OBJECT

public:
    HubRadiusDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onOk();
    void updateState();

private:
    VpnServerRpc *m_rpc;
    QString m_hubName;
    QCheckBox *m_useCheck;
    QLineEdit *m_hostEdit;
    QSpinBox *m_portSpin;
    QLineEdit *m_secretEdit;
    QLineEdit *m_secretConfirmEdit;
    QSpinBox *m_retrySpin;
    QPushButton *m_okButton;
};
