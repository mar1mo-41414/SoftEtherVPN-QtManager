#pragma once

#include <QByteArray>
#include <QDialog>
#include <QJsonObject>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QGroupBox;
class QLabel;

// 公式Manager「カスケード接続の新規作成/編集」(D_CM_ACCOUNT 相当) 。
// 接続先・プロキシ・サーバー証明書の検証・セキュリティポリシー・ユーザー認証
// (匿名/標準パスワード/RADIUS・NT/クライアント証明書)・高度な通信設定を扱う。
class CascadeLinkEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CascadeLinkEditDialog(bool isNew, QWidget *parent = nullptr);

    // GetLink の結果をフォームに反映する (編集時)。
    void setValues(const QJsonObject &link);
    QJsonObject toRpcParams() const;
    QString accountName() const;

private slots:
    void accept() override;
    void updateState();
    void onProxySettings();
    void onLoadServerCert();
    void onViewServerCert();
    void onPolicy();
    void onLoadClientCert();
    void onAdvanced();

private:
    int authType() const; // 0:匿名 1:標準パスワード 2:RADIUS/NT 3:クライアント証明書
    int proxyType() const;

    QLineEdit *m_accountNameEdit;
    QLineEdit *m_hostEdit;
    QComboBox *m_portCombo;
    QComboBox *m_hubCombo;

    QRadioButton *m_directRadio;
    QRadioButton *m_httpRadio;
    QRadioButton *m_socksRadio;
    QPushButton *m_proxyButton;
    QString m_proxyHost;
    int m_proxyPort = 8080;
    QString m_proxyUser;
    QString m_proxyPassword;

    QCheckBox *m_checkCertCheck;
    QPushButton *m_viewServerCertButton;
    QByteArray m_serverCert;

    QJsonObject m_policy;

    QComboBox *m_authCombo;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLabel *m_passwordLabel;
    QLabel *m_certInfoLabel;
    QPushButton *m_clientCertButton;
    QByteArray m_clientCert;
    QByteArray m_clientKey;
    QByteArray m_originalHashed;
    bool m_isNew;

    // 高度な通信設定
    int m_maxConnection = 8;
    int m_interval = 1;
    int m_disconnectSpan = 0;
    bool m_halfConnection = false;
    bool m_disableQoS = false;
    bool m_useEncrypt = true;
    bool m_useCompress = false;
    bool m_noUdp = false;

    QPushButton *m_okButton;
};
