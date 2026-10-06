#pragma once

#include <QDialog>
#include <QJsonObject>

class QCheckBox;
class QLineEdit;
class QSpinBox;

// 公式Manager「SecureNAT の設定」(D_NM_OPTION) 相当。
// 左列: 仮想ホストのNIC・仮想NAT・静的ルートのプッシュ / 右列: 仮想DHCPサーバー。
class SecureNatOptionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SecureNatOptionDialog(const QString &hubName, QWidget *parent = nullptr);

    void setValues(const QJsonObject &option);
    QJsonObject toRpcParams() const;

private slots:
    void accept() override;
    void updateState();
    void onEditPushRoutes();

private:
    QLineEdit *m_macEdit;
    QLineEdit *m_ipEdit;
    QLineEdit *m_maskEdit;

    QCheckBox *m_useNatCheck;
    QSpinBox *m_mtuSpin;
    QSpinBox *m_tcpTimeoutSpin;
    QSpinBox *m_udpTimeoutSpin;
    QCheckBox *m_saveLogCheck;
    QString m_pushRoutes;

    QCheckBox *m_useDhcpCheck;
    QLineEdit *m_leaseStartEdit;
    QLineEdit *m_leaseEndEdit;
    QLineEdit *m_dhcpMaskEdit;
    QSpinBox *m_leaseExpireSpin;
    QLineEdit *m_gatewayEdit;
    QLineEdit *m_dns1Edit;
    QLineEdit *m_dns2Edit;
    QLineEdit *m_domainEdit;
};
