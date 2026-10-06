#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>
#include <QJsonObject>

class QComboBox;
class QLineEdit;

// 公式Manager「仮想インターフェイスの追加」(D_SM_L3_SW_IF) 相当。
class L3IfAddDialog : public QDialog
{
    Q_OBJECT

public:
    L3IfAddDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

    // Name_str (L3スイッチ名) は呼び出し側で追加する。
    QJsonObject toRpcParams() const;

private slots:
    void accept() override;

private:
    QComboBox *m_hubCombo;
    QLineEdit *m_ipEdit;
    QLineEdit *m_maskEdit;
};
