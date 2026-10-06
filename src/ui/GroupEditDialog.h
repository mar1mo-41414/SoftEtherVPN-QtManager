#pragma once

#include <QDialog>
#include <QJsonObject>

class QCheckBox;
class QLineEdit;
class QPushButton;
class QTableWidget;

// 公式Manager「グループの新規作成/編集」(D_SM_EDIT_GROUP) 相当。
class GroupEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GroupEditDialog(bool isNew, QWidget *parent = nullptr);

    // GetGroup の結果をフォームに反映する (編集時)。
    void setGroup(const QJsonObject &group);

    // CreateGroup / SetGroup にそのまま渡せる JSON (HubName_str は含まない)。
    QJsonObject toRpcParams() const;

private slots:
    void accept() override;
    void updateState();
    void onPolicy();

private:
    QLineEdit *m_nameEdit;
    QLineEdit *m_realnameEdit;
    QLineEdit *m_noteEdit;
    QCheckBox *m_policyCheck;
    QPushButton *m_policyButton;
    QJsonObject m_policy;
    QTableWidget *m_statsTable;
    QPushButton *m_okButton;
};
