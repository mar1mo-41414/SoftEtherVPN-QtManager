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
    // cascadeMode: カスケード接続用 (公式Managerと同じく、ユーザー専用の項目を除いた一覧を表示)。
    PolicyDialog(const QString &windowTitle, const QString &heading, const QJsonObject &policy,
                 QWidget *parent = nullptr, bool cascadeMode = false);

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
    QString valueText(int defIndex) const;
    void refreshRow(int row);
    // 現在選択中の行に対応する PolicyTable の添字 (未選択なら -1)。
    int currentIndex() const;

    QJsonObject m_policy;
    bool m_loading = false;
    QList<int> m_defIndex; // 表示行 → PolicyTable の添字

    QTableWidget *m_table;
    QLabel *m_nameLabel;
    QTextEdit *m_descriptionEdit;
    QRadioButton *m_onRadio;
    QRadioButton *m_offRadio;
    QSpinBox *m_spin;
};
