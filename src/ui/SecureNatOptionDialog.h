#pragma once

#include <QDialog>
#include <QJsonObject>

class QCheckBox;
class QLineEdit;
class QSpinBox;

// 公式Manager「SecureNAT の設定」(D_NM_OPTION) 相当。
// 静的ルーティングテーブルのプッシュ (スプリットトンネリング) は後続フェーズで追加する。
class SecureNatOptionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SecureNatOptionDialog(QWidget *parent = nullptr);

    void setValues(const QJsonObject &option);
    QJsonObject toRpcParams() const;

private slots:
    void accept() override;

private:
    QLineEdit *m_macEdit;
    QLineEdit *m_ipEdit;
    QLineEdit *m_maskEdit;

    QCheckBox *m_useNatCheck;
    QSpinBox *m_mtuSpin;
    QSpinBox *m_tcpTimeoutSpin;
    QSpinBox *m_udpTimeoutSpin;
    QCheckBox *m_saveLogCheck;

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
