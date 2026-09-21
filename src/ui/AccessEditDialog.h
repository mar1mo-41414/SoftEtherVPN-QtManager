#pragma once

#include <QDialog>
#include <QJsonObject>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QRadioButton;
class QSpinBox;

// 公式Manager「アクセスリスト項目の編集」(D_SM_EDIT_ACCESS) 相当。
// このフェーズはIPv4のみ対応。IPv6・MACヘッダフィルタ・TCP状態検査・
// リダイレクト・遅延/パケットロスシミュレーションは後続フェーズで追加する。
class AccessEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccessEditDialog(QWidget *parent = nullptr);

    void setValues(const QJsonObject &access);
    QJsonObject toRpcParams() const;

private slots:
    void accept() override;

private:
    quint32 m_id = 0;

    QLineEdit *m_noteEdit;
    QRadioButton *m_passRadio;
    QRadioButton *m_discardRadio;
    QSpinBox *m_prioritySpin;
    QCheckBox *m_activeCheck;

    QCheckBox *m_srcAllCheck;
    QLineEdit *m_srcIpEdit;
    QLineEdit *m_srcMaskEdit;
    QCheckBox *m_dstAllCheck;
    QLineEdit *m_dstIpEdit;
    QLineEdit *m_dstMaskEdit;

    QComboBox *m_protocolCombo;
    QLineEdit *m_srcPortStartEdit;
    QLineEdit *m_srcPortEndEdit;
    QLineEdit *m_dstPortStartEdit;
    QLineEdit *m_dstPortEndEdit;

    QLineEdit *m_srcUsernameEdit;
    QLineEdit *m_dstUsernameEdit;
};
