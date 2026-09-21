#pragma once

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QTextEdit;

// 公式Manager「グループの新規作成/編集」(D_SM_EDIT_GROUP) 相当。
// セキュリティポリシーの設定は後続フェーズで追加する。
class GroupEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GroupEditDialog(bool isNew, QWidget *parent = nullptr);

    void setValues(const QString &name, const QString &realname, const QString &note);
    QJsonObject toRpcParams() const;

private slots:
    void accept() override;

private:
    QLineEdit *m_nameEdit;
    QLineEdit *m_realnameEdit;
    QTextEdit *m_noteEdit;
};
