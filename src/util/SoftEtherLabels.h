#pragma once

#include <QCoreApplication>
#include <QDateTime>
#include <QString>

// 公式Manager (strtable_ja.stb) の表記に合わせた、複数画面で共有する変換関数。
namespace SoftEtherLabels {

inline QString onlineStatus(bool online)
{
    // SM_HUB_ONLINE / SM_HUB_OFFLINE
    return online ? QCoreApplication::translate("SoftEtherLabels", "オンライン")
                  : QCoreApplication::translate("SoftEtherLabels", "オフライン");
}

inline QString hubType(int hubType)
{
    // SM_HUB_STANDALONE / SM_HUB_STATIC / SM_HUB_DYNAMIC
    switch (hubType) {
    case 0:
        return QCoreApplication::translate("SoftEtherLabels", "スタンドアロン");
    case 1:
        return QCoreApplication::translate("SoftEtherLabels", "スタティック仮想 HUB");
    case 2:
        return QCoreApplication::translate("SoftEtherLabels", "ダイナミック仮想 HUB");
    default:
        return QCoreApplication::translate("SoftEtherLabels", "不明");
    }
}

inline QString authType(int authType)
{
    // SM_AUTHTYPE_0〜5
    switch (authType) {
    case 0:
        return QCoreApplication::translate("SoftEtherLabels", "匿名認証");
    case 1:
        return QCoreApplication::translate("SoftEtherLabels", "パスワード認証");
    case 2:
        return QCoreApplication::translate("SoftEtherLabels", "固有証明書認証");
    case 3:
        return QCoreApplication::translate("SoftEtherLabels", "署名済み証明書認証");
    case 4:
        return QCoreApplication::translate("SoftEtherLabels", "RADIUS 認証");
    case 5:
        return QCoreApplication::translate("SoftEtherLabels", "NT ドメイン認証");
    default:
        return QCoreApplication::translate("SoftEtherLabels", "不明");
    }
}

inline QString secureNatEnabled(bool enabled)
{
    // SM_HUB_SECURE_NAT_YES / SM_HUB_SECURE_NAT_NO
    return enabled ? QCoreApplication::translate("SoftEtherLabels", "有効")
                   : QCoreApplication::translate("SoftEtherLabels", "無効");
}

inline QString dateTime(const QString &isoString)
{
    const QDateTime dt = QDateTime::fromString(isoString, Qt::ISODateWithMs);
    if (!dt.isValid()) {
        return QStringLiteral("-");
    }
    return dt.toLocalTime().toString(QStringLiteral("yyyy/MM/dd HH:mm:ss"));
}

} // namespace SoftEtherLabels
