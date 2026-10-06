#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>
#include <QJsonObject>

class QComboBox;
class QLineEdit;

// 公式Manager「EtherIP / L2TPv3 over IPsec クライアント定義」(D_SM_ETHERIP_ID) 相当。
class EtherIpIdEditDialog : public QDialog
{
    Q_OBJECT

public:
    // isNew=false の場合は ISAKMP Phase 1 ID を変更不可にする (編集は削除+再追加で行う)。
    EtherIpIdEditDialog(VpnServerRpc *rpc, bool isNew, QWidget *parent = nullptr);

    void setValues(const QJsonObject &setting);
    QJsonObject toRpcParams() const;

private slots:
    void accept() override;

private:
    QLineEdit *m_idEdit;
    QComboBox *m_hubCombo;
    QLineEdit *m_userEdit;
    QLineEdit *m_passwordEdit;
};
