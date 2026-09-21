#pragma once

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QCheckBox;
class QSpinBox;
class QRadioButton;

// 公式Manager「仮想HUBの作成/プロパティ」(D_SM_EDIT_HUB) 相当の編集ダイアログ。
// クラスタリング関連(スタティック/ダイナミック)、管理オプション、接続元IP制限リスト、
// 拡張オプション、メッセージ設定は後続フェーズで別ダイアログとして追加する。
class HubEditDialog : public QDialog
{
    Q_OBJECT

public:
    // isNew: 新規作成なら true。既存編集なら false (仮想HUB名を変更不可にする)。
    explicit HubEditDialog(bool isNew, QWidget *parent = nullptr);

    void setValues(const QString &hubName, bool online, bool noEnum, quint32 maxSession);

    // CreateHub/SetHub にそのまま渡せるパラメータを返す。
    // AdminPasswordPlainText_str は入力が空なら省略される(既存編集時に「変更しない」を表す)。
    QJsonObject toRpcParams() const;
    QString hubName() const;

private slots:
    void accept() override;

private:
    bool m_isNew;
    QLineEdit *m_nameEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_passwordConfirmEdit;
    QCheckBox *m_noEnumCheck;
    QCheckBox *m_limitMaxSessionCheck;
    QSpinBox *m_maxSessionSpin;
    QRadioButton *m_onlineRadio;
    QRadioButton *m_offlineRadio;
};
