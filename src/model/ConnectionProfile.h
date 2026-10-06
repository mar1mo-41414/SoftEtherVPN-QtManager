#pragma once

#include <QString>

// 公式Manager「新しい接続の設定」(D_SM_EDIT_SETTING) 相当の1件分の設定。
struct ConnectionProfile
{
    QString name;
    QString host;
    quint16 port = 443;
    bool hubAdminMode = false;
    QString hubName;
    QString password;
    // R_NO_SAVE (管理パスワードを保存しない) がオンの場合、password は常に空で保持する。
    bool noSavePassword = false;

    // 経由するプロキシサーバー (D_SM_EDIT_SETTING の STATIC8〜)。0:直接 1:HTTP 2:SOCKS
    int proxyType = 0;
    QString proxyHost;
    quint16 proxyPort = 8080;
    QString proxyUser;
    QString proxyPassword;
};
