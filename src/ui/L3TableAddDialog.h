#pragma once

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QSpinBox;

// 公式Manager「ルーティングテーブルエントリの追加」(D_SM_L3_SW_TABLE) 相当。
class L3TableAddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit L3TableAddDialog(QWidget *parent = nullptr);

    // Name_str (L3スイッチ名) は呼び出し側で追加する。
    QJsonObject toRpcParams() const;

private slots:
    void accept() override;

private:
    QLineEdit *m_networkEdit;
    QLineEdit *m_maskEdit;
    QLineEdit *m_gatewayEdit;
    QSpinBox *m_metricSpin;
};
