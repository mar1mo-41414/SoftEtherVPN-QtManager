#pragma once

#include <QDialog>
#include <QJsonObject>

class QCheckBox;
class QLineEdit;
class QRadioButton;
class QSpinBox;

// 公式Manager「カスケード接続の新規作成/編集」相当。
// このフェーズは匿名認証・パスワード認証のみ対応。証明書認証・プロキシ経由接続・
// セキュリティポリシー・サーバー証明書検証は後続フェーズで追加する。
class CascadeLinkEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CascadeLinkEditDialog(bool isNew, QWidget *parent = nullptr);

    void setValues(const QJsonObject &link);
    QJsonObject toRpcParams() const;
    QString accountName() const;

private slots:
    void accept() override;

private:
    QLineEdit *m_accountNameEdit;
    QLineEdit *m_hostEdit;
    QSpinBox *m_portSpin;
    QLineEdit *m_targetHubNameEdit;
    QRadioButton *m_anonymousRadio;
    QRadioButton *m_passwordRadio;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QCheckBox *m_encryptCheck;
    QCheckBox *m_compressCheck;
    QSpinBox *m_maxConnectionSpin;
};
