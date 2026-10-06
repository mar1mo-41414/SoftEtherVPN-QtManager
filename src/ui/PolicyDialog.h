#pragma once

#include <QDialog>
#include <QJsonObject>

class QLabel;
class QRadioButton;
class QSpinBox;
class QTableWidget;
class QTextEdit;

// 公式Manager「セキュリティポリシー」(D_SM_POLICY) 相当。
// "policy:*_bool" / "policy:*_u32" の形式で保持された JSON をそのまま編集する。
// ユーザー・グループ・カスケード接続などで共用する。
class PolicyDialog : public QDialog
{
    Q_OBJECT

public:
    PolicyDialog(const QString &windowTitle, const QString &heading, const QJsonObject &policy,
                 QWidget *parent = nullptr);

    // 編集後の "policy:*" キーのみを持つ JSON。
    QJsonObject policy() const { return m_policy; }

    // 新規作成時などに使う初期値 (公式Managerの既定値)。
    static QJsonObject defaultPolicy();
    // obj から "policy:" で始まるキーのみを取り出す。
    static QJsonObject extract(const QJsonObject &obj);

private slots:
    void onSelectionChanged();
    void onValueEdited();

private:
    QString valueText(int index) const;
    void refreshRow(int index);
    int currentIndex() const;

    QJsonObject m_policy;
    bool m_loading = false;

    QTableWidget *m_table;
    QLabel *m_nameLabel;
    QTextEdit *m_descriptionEdit;
    QRadioButton *m_onRadio;
    QRadioButton *m_offRadio;
    QSpinBox *m_spin;
};
