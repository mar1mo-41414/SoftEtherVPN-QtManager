#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;
class QRadioButton;
class QSpinBox;

// 公式Manager「クラスタリング構成」(D_SM_FARM) 相当。サーバー全体の設定。
// 変更するとVPN Serverが再起動し、管理用コネクションも切断される。
class FarmDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    FarmDialog(VpnServerRpc *rpc, QString serverName, QWidget *parent = nullptr);

private slots:
    void onModeChanged();
    void onOk();

private:
    VpnServerRpc *m_rpc;
    QRadioButton *m_standaloneRadio;
    QRadioButton *m_controllerRadio;
    QRadioButton *m_memberRadio;
    QLineEdit *m_publicIpEdit;
    QLineEdit *m_portsEdit;
    QLineEdit *m_controllerHostEdit;
    QSpinBox *m_controllerPortSpin;
    QLineEdit *m_passwordEdit;
    QSpinBox *m_weightSpin;
    QCheckBox *m_controllerOnlyCheck;
    QLabel *m_currentModeLabel;
};
